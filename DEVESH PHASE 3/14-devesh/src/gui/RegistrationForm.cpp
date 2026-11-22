// RegistrationForm.cpp
#include "RegistrationForm.hpp"

#include "EsiDelegate.hpp"
#include "InsuranceFields.hpp"
#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"

#include <QButtonGroup>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>
#include <QSpinBox>
#include <QVBoxLayout>

#include <optional>

using Text::qs;
using Text::ss;

RegistrationForm::RegistrationForm(SessionHub& hub, RoleWindow& host, Kind kind, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host), kind_(kind)
{
    const bool emergency = kind_ == Kind::Emergency;
    const QString prefix = emergency ? QStringLiteral("er") : QStringLiteral("opd");

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(12);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    name_ = new QLineEdit(this);
    name_->setObjectName(prefix + QStringLiteral("Name"));
    form->addRow(QStringLiteral("Name"), name_);

    age_ = new QSpinBox(this);
    age_->setObjectName(prefix + QStringLiteral("Age"));
    age_->setRange(-1, 130);              // -1 = not entered yet (shown blank)
    age_->setSpecialValueText(QStringLiteral(" "));
    age_->setValue(-1);
    age_->setMaximumWidth(age_->sizeHint().width() * 2);
    form->addRow(QStringLiteral("Age"), age_);

    phone_ = new QLineEdit(this);
    phone_->setObjectName(prefix + QStringLiteral("Phone"));
    phone_->setPlaceholderText(QStringLiteral("Optional"));
    form->addRow(QStringLiteral("Phone"), phone_);

    complaint_ = new QLineEdit(this);
    complaint_->setObjectName(prefix + QStringLiteral("Complaint"));
    form->addRow(emergency ? QStringLiteral("Presenting complaint") : QStringLiteral("Reason for visit"),
                 complaint_);
    outer->addLayout(form);

    if (emergency) {
        auto* box = new QGroupBox(QStringLiteral("Severity (Emergency Severity Index)"), this);
        auto* grid = new QGridLayout(box);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(6);
        grid->setColumnStretch(2, 1);
        esi_ = new QButtonGroup(this);
        for (int level = 1; level <= 5; ++level) {
            auto* badge = new EsiBadge(level, box);
            auto* radio = new QRadioButton(Theme::esiName(level), box);
            radio->setObjectName(QStringLiteral("esi%1").arg(level));
            radio->setToolTip(Theme::esiDescription(level));
            auto* desc = new QLabel(Theme::esiDescription(level), box);
            desc->setProperty("textRole", QStringLiteral("hint"));
            desc->setWordWrap(true);
            desc->setBuddy(radio);
            esi_->addButton(radio, level);
            grid->addWidget(badge, level - 1, 0);
            grid->addWidget(radio, level - 1, 1);
            grid->addWidget(desc, level - 1, 2);
        }
        auto* rule = new QLabel(QStringLiteral(
            "ESI 1-2 are admitted to an ICU bed, ESI 3 to a ward bed; ESI 4-5 are treated in the "
            "emergency room and released."), box);
        rule->setProperty("textRole", QStringLiteral("hint"));
        rule->setWordWrap(true);
        grid->addWidget(rule, 5, 0, 1, 3);
        outer->addWidget(box);
    } else {
        auto* deptForm = new QFormLayout;
        deptForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        deptForm->setHorizontalSpacing(12);
        department_ = new QComboBox(this);
        department_->setObjectName(QStringLiteral("opdDepartment"));
        deptForm->addRow(QStringLiteral("Department"), department_);
        outer->addLayout(deptForm);
        refreshDepartments();
        connect(&hub_, &SessionHub::staffChanged, this, &RegistrationForm::refreshDepartments);
        connect(&hub_, &SessionHub::reloaded, this, &RegistrationForm::refreshDepartments);
    }

    auto* insuranceBox = new QGroupBox(QStringLiteral("Insurance"), this);
    auto* insuranceLayout = new QVBoxLayout(insuranceBox);
    insurance_ = new InsuranceFields(true, insuranceBox);
    insurance_->setObjectName(prefix + QStringLiteral("Insurance"));
    insuranceLayout->addWidget(insurance_);
    outer->addWidget(insuranceBox);

    auto* buttons = new QHBoxLayout;
    submit_ = new QPushButton(QStringLiteral("Register patient"), this);
    submit_->setObjectName(prefix + QStringLiteral("Submit"));
    submit_->setProperty("primary", true);
    connect(submit_, &QPushButton::clicked, this, [this] { submit(); });
    buttons->addWidget(submit_);
    buttons->addStretch(1);
    outer->addLayout(buttons);
    outer->addStretch(1);

    // Enter anywhere in the form registers the patient.
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), this);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(sc, &QShortcut::activated, this, [this] { submit(); });
    }
}

void RegistrationForm::refreshDepartments()
{
    if (!department_ || !hub_.isStarted())
        return;
    const QString current = department_->currentText();
    department_->clear();
    const std::vector<std::string> depts = hub_.session().opdDepartments();
    if (depts.empty())
        department_->addItem(QStringLiteral("General Medicine"));   // console fallback
    for (const std::string& d : depts)
        department_->addItem(qs(d));
    const int keep = department_->findText(current);
    if (keep >= 0)
        department_->setCurrentIndex(keep);
}

bool RegistrationForm::submit()
{
    host_.showMessage(QString());
    const QString name = name_->text().trimmed();
    const QString complaint = complaint_->text().trimmed();
    if (name.isEmpty()) {
        host_.showError(QStringLiteral("Enter the patient's name."));
        name_->setFocus();
        return false;
    }
    if (age_->value() < 0) {
        host_.showError(QStringLiteral("Enter the patient's age (0 to 130)."));
        age_->setFocus();
        return false;
    }
    if (complaint.isEmpty()) {
        host_.showError(kind_ == Kind::Emergency ? QStringLiteral("Enter the presenting complaint.")
                                                 : QStringLiteral("Enter the reason for the visit."));
        complaint_->setFocus();
        return false;
    }
    if (kind_ == Kind::Emergency && esi_->checkedId() < 1) {
        host_.showError(QStringLiteral("Choose an ESI level, 1 (most critical) to 5."));
        esi_->button(1)->setFocus();
        return false;
    }
    QString insuranceError;
    const std::optional<Insurance> cover = insurance_->value(&insuranceError);
    if (!insuranceError.isEmpty()) {
        host_.showError(insuranceError);
        return false;
    }

    QString id;
    QString message;
    const bool ok = host_.attempt([&] {
        const HospitalSession& s = hub_.session();
        if (kind_ == Kind::Emergency) {
            const Patient& p = hub_.registerEmergency(ss(name), age_->value(), ss(phone_->text()),
                                                      ss(complaint), esi_->checkedId(), cover);
            id = qs(p.id());
            // Same sentence as the console's option 1.
            message = QStringLiteral("Registered %1 (ESI %2). Position in the emergency queue: %3 of %4.")
                          .arg(id).arg(p.severity())
                          .arg(s.emergencyPosition(p.id()))
                          .arg(s.emergencyOrder().size());
        } else {
            const QString dept = department_->currentText();
            const Patient& p = hub_.registerOpd(ss(name), age_->value(), ss(phone_->text()),
                                                ss(complaint), ss(dept), cover);
            id = qs(p.id());
            // Same sentence as the console's option 2.
            message = QStringLiteral("Registered %1 for %2. Position in the OPD line: %3.")
                          .arg(id, dept).arg(s.triage().opdCount());
        }
    });
    if (!ok)
        return false;
    if (cover)
        message += QStringLiteral(" Insurance recorded; it is deducted once, when the bill is settled.");
    host_.showMessage(message);
    clear();
    Q_EMIT registered(id);
    return true;
}

void RegistrationForm::clear()
{
    name_->clear();
    age_->setValue(-1);
    phone_->clear();
    complaint_->clear();
    if (esi_) {
        esi_->setExclusive(false);
        for (QAbstractButton* b : esi_->buttons())
            b->setChecked(false);
        esi_->setExclusive(true);
    }
    insurance_->clear();
    name_->setFocus();
}
