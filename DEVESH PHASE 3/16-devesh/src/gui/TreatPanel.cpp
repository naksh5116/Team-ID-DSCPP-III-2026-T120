// TreatPanel.cpp
#include "TreatPanel.hpp"

#include "EsiDelegate.hpp"
#include "QueueModels.hpp"
#include "RoleWindow.hpp"
#include "RoutePanel.hpp"
#include "RouteStepsView.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

#include <optional>

using Text::qs;
using Text::ss;

namespace {

QLabel* valueLabel(QWidget* parent, const char* name, bool mono = false)
{
    auto* l = new QLabel(parent);
    l->setObjectName(QLatin1String(name));
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->setWordWrap(true);
    if (mono)
        l->setFont(Theme::monoFont());
    return l;
}

} // namespace

TreatPanel::TreatPanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Horizontal, this);
    split->setChildrenCollapsible(false);
    layout->addWidget(split);

    // ---------------------------------------------------- left column
    auto* left = new QWidget;
    auto* lv = new QVBoxLayout(left);
    lv->setContentsMargins(8, 8, 8, 8);
    lv->setSpacing(12);

    auto* nextBox = new QGroupBox(QStringLiteral("Next patient"), left);
    auto* nextForm = new QFormLayout(nextBox);
    nextForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nextForm->setHorizontalSpacing(12);
    nextForm->setVerticalSpacing(6);
    nextId_ = valueLabel(nextBox, "nextId", true);
    nextName_ = valueLabel(nextBox, "nextName");
    nextBadge_ = new EsiBadge(0, nextBox);
    nextKind_ = valueLabel(nextBox, "nextKind");
    auto* kindRow = new QHBoxLayout;
    kindRow->setSpacing(6);
    kindRow->addWidget(nextBadge_);
    kindRow->addWidget(nextKind_, 1);
    nextComplaint_ = valueLabel(nextBox, "nextComplaint");
    nextNeeds_ = valueLabel(nextBox, "nextNeeds");
    nextForm->addRow(QStringLiteral("Patient ID"), nextId_);
    nextForm->addRow(QStringLiteral("Name"), nextName_);
    nextForm->addRow(QStringLiteral("Triage"), kindRow);
    nextForm->addRow(QStringLiteral("Complaint"), nextComplaint_);
    nextForm->addRow(QStringLiteral("Care"), nextNeeds_);
    lv->addWidget(nextBox);

    // Outpatient notes, asked before the consultation is recorded.
    opdBox_ = new QGroupBox(QStringLiteral("Outpatient consultation"), left);
    auto* opdForm = new QFormLayout(opdBox_);
    opdForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    opdForm->setHorizontalSpacing(12);
    opdForm->setVerticalSpacing(8);
    diagnosis_ = new QLineEdit(opdBox_);
    diagnosis_->setObjectName(QStringLiteral("opdDiagnosis"));
    diagnosis_->setPlaceholderText(QStringLiteral("Routine review"));
    prescriptions_ = new QLineEdit(opdBox_);
    prescriptions_->setObjectName(QStringLiteral("opdPrescriptions"));
    prescriptions_->setPlaceholderText(QStringLiteral("None"));
    dispense_ = new QCheckBox(QStringLiteral("Dispense medicine from the pharmacy"), opdBox_);
    dispense_->setObjectName(QStringLiteral("opdDispense"));
    medicine_ = new QLineEdit(opdBox_);
    medicine_->setObjectName(QStringLiteral("opdMedicine"));
    quantity_ = new QSpinBox(opdBox_);
    quantity_->setObjectName(QStringLiteral("opdQuantity"));
    quantity_->setRange(1, 1000);
    unitPrice_ = new QDoubleSpinBox(opdBox_);
    unitPrice_->setObjectName(QStringLiteral("opdUnitPrice"));
    unitPrice_->setRange(0.0, 1000000.0);
    unitPrice_->setPrefix(QStringLiteral("Rs "));
    opdForm->addRow(QStringLiteral("Diagnosis"), diagnosis_);
    opdForm->addRow(QStringLiteral("Prescriptions"), prescriptions_);
    opdForm->addRow(QString(), dispense_);
    opdForm->addRow(QStringLiteral("Medicine"), medicine_);
    opdForm->addRow(QStringLiteral("Quantity"), quantity_);
    opdForm->addRow(QStringLiteral("Unit price"), unitPrice_);
    auto updateDispense = [this] {
        for (QWidget* w : {static_cast<QWidget*>(medicine_), static_cast<QWidget*>(quantity_),
                           static_cast<QWidget*>(unitPrice_)})
            w->setEnabled(dispense_->isChecked());
    };
    connect(dispense_, &QCheckBox::toggled, this, updateDispense);
    updateDispense();
    lv->addWidget(opdBox_);

    auto* treatButton = new QPushButton(QStringLiteral("Treat next patient"), left);
    treatButton->setObjectName(QStringLiteral("treatButton"));
    treatButton->setProperty("primary", true);
    connect(treatButton, &QPushButton::clicked, this, [this] { treatNext(); });
    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(treatButton);
    buttonRow->addStretch(1);
    lv->addLayout(buttonRow);

    auto* resultBox = new QGroupBox(QStringLiteral("Last treatment"), left);
    auto* rv = new QVBoxLayout(resultBox);
    // A read-only text area: it wraps long lines reliably inside the scroll area.
    result_ = new QPlainTextEdit(resultBox);
    result_->setObjectName(QStringLiteral("treatResult"));
    result_->setReadOnly(true);
    result_->setFrameShape(QFrame::NoFrame);
    result_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    result_->setFixedHeight(result_->fontMetrics().lineSpacing() * 6 + 8);
    result_->setPlainText(QStringLiteral("No patient treated in this window yet."));
    rv->addWidget(result_);
    lv->addWidget(resultBox);
    lv->addStretch(1);

    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), left);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(sc, &QShortcut::activated, this, [this] { treatNext(); });
    }
    split->addWidget(scrollable(left, split));

    // --------------------------------------------- middle: route area
    routeArea_ = new QWidget;
    auto* ra = new QHBoxLayout(routeArea_);
    ra->setContentsMargins(8, 8, 8, 8);
    route_ = new RoutePanel(hub_, Qt::Vertical, routeArea_);   // floor map above the steps
    route_->clear(QStringLiteral("The route appears here when a bed is assigned."));
    ra->addWidget(route_);
    split->addWidget(routeArea_);

    // ------------------------------------------- right: who is waiting
    auto* lines = new QSplitter(Qt::Vertical);
    lines->setChildrenCollapsible(false);
    emergencyModel_ = new EmergencyQueueModel(hub_, this);
    emergencyTable_ = new QTableView;
    emergencyTable_->setObjectName(QStringLiteral("doctorEmergencyQueue"));
    emergencyTable_->setModel(emergencyModel_);
    emergencyTable_->setItemDelegateForColumn(EmergencyQueueModel::Esi, new EsiDelegate(emergencyTable_));
    Theme::setupTable(emergencyTable_);
    for (int c : {EmergencyQueueModel::Age, EmergencyQueueModel::Complaint, EmergencyQueueModel::Registered})
        emergencyTable_->hideColumn(c);
    fitColumnsOnReset(emergencyTable_);
    lines->addWidget(tableSection(QStringLiteral("Emergency queue"), QString(), emergencyTable_, lines));
    opdModel_ = new OpdQueueModel(hub_, this);
    opdTable_ = new QTableView;
    opdTable_->setObjectName(QStringLiteral("doctorOpdQueue"));
    opdTable_->setModel(opdModel_);
    Theme::setupTable(opdTable_);
    for (int c : {OpdQueueModel::Age, OpdQueueModel::Reason})
        opdTable_->hideColumn(c);
    fitColumnsOnReset(opdTable_);
    lines->addWidget(tableSection(QStringLiteral("OPD line"), QString(), opdTable_, lines));
    lines->setStretchFactor(0, 3);
    lines->setStretchFactor(1, 2);
    split->addWidget(lines);

    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);
    split->setStretchFactor(2, 2);

    connect(&hub_, &SessionHub::queueChanged, this, &TreatPanel::refreshNext);
    connect(&hub_, &SessionHub::patientsChanged, this, &TreatPanel::refreshNext);
    connect(&hub_, &SessionHub::reloaded, this, &TreatPanel::refreshNext);
    refreshNext();
}

void TreatPanel::refreshNext()
{
    if (!hub_.isStarted())
        return;
    const HospitalSession& s = hub_.session();
    const TriageUnit& t = s.triage();
    const Patient* p = nullptr;
    if (t.hasEmergency())
        p = s.patients().tryFind(t.peekEmergency());
    else if (t.hasOpd())
        p = s.patients().tryFind(t.opdLine().front());

    auto* treatButton = findChild<QPushButton*>(QStringLiteral("treatButton"));
    if (!p) {
        nextId_->setText(QStringLiteral("Nobody is waiting."));
        for (QLabel* l : {nextName_, nextKind_, nextComplaint_, nextNeeds_})
            l->clear();
        nextBadge_->setLevel(0);
        opdBox_->setVisible(false);
        treatButton->setEnabled(false);
        return;
    }
    treatButton->setEnabled(true);
    nextId_->setText(qs(p->id()));
    nextName_->setText(QStringLiteral("%1, %2").arg(qs(p->name())).arg(p->age()));
    nextComplaint_->setText(qs(p->complaint()));
    if (p->isEmergency()) {
        nextBadge_->setLevel(p->severity());
        nextKind_->setText(QStringLiteral("Emergency, ESI %1 %2")
                               .arg(p->severity()).arg(Theme::esiName(p->severity())));
        switch (p->requiredUnit()) {
        case CareUnit::ICU:
            nextNeeds_->setText(QStringLiteral("Needs an ICU bed (%1 free)").arg(s.router().freeBeds(CareUnit::ICU)));
            break;
        case CareUnit::Ward:
            nextNeeds_->setText(QStringLiteral("Needs a ward bed (%1 free)").arg(s.router().freeBeds(CareUnit::Ward)));
            break;
        case CareUnit::None:
            nextNeeds_->setText(QStringLiteral("Treated in the emergency room, no bed"));
            break;
        }
        opdBox_->setVisible(false);
    } else {
        nextBadge_->setLevel(0);
        nextKind_->setText(QStringLiteral("Outpatient, %1").arg(qs(p->department())));
        nextNeeds_->setText(QStringLiteral("OPD consultation"));
        opdBox_->setVisible(true);
    }
}

bool TreatPanel::treatNext()
{
    using Outcome = HospitalSession::TreatResult::Outcome;
    host_.showMessage(QString());
    const TriageUnit& t = hub_.session().triage();
    const bool outpatientNext = !t.hasEmergency() && t.hasOpd();

    std::optional<HospitalSession::MedicineLine> medicine;
    if (outpatientNext && dispense_->isChecked()) {
        if (medicine_->text().trimmed().isEmpty()) {
            host_.showError(QStringLiteral("Enter the medicine to dispense, or untick the box."));
            medicine_->setFocus();
            return false;
        }
        medicine = HospitalSession::MedicineLine{ss(medicine_->text()), quantity_->value(),
                                                 unitPrice_->value()};
    }

    HospitalSession::TreatResult r;
    const bool ok = host_.attempt([&] {
        r = hub_.treatNext(ss(diagnosis_->text()), ss(prescriptions_->text()), medicine);
    });
    if (!ok)
        return false;

    // The same lines the console prints for option 3.
    QStringList lines;
    QString status;
    const Patient* p = r.patient;
    switch (r.outcome) {
    case Outcome::NobodyWaiting:
        result_->setPlainText(QStringLiteral("Nobody is waiting."));
        host_.showMessage(QStringLiteral("Nobody is waiting."));
        return false;
    case Outcome::Outpatient:
        lines << QStringLiteral("Outpatient: %1  %2 - %3 (%4)")
                     .arg(qs(p->id()), qs(p->name()), qs(p->complaint()), qs(p->department()));
        if (r.doctor)
            lines << QStringLiteral("Seen by %1 at %2").arg(qs(r.doctor->name()), Text::clock(r.doctorStart));
        lines << QStringLiteral("Consultation recorded. Bill so far: %1").arg(Text::money(r.billSoFar));
        status = QStringLiteral("%1 seen in OPD. Bill so far: %2").arg(qs(p->id()), Text::money(r.billSoFar));
        diagnosis_->clear();
        prescriptions_->clear();
        dispense_->setChecked(false);
        medicine_->clear();
        quantity_->setValue(1);
        unitPrice_->setValue(0.0);
        route_->clear(QStringLiteral("Outpatients are not given a bed, so there is no route."));
        break;
    default:
        lines << QStringLiteral("Patient: %1  %2, ESI %3 - %4")
                     .arg(qs(p->id()), qs(p->name())).arg(p->severity()).arg(qs(p->complaint()));
        if (r.outcome == Outcome::NoBed) {
            lines << QStringLiteral("Cannot admit yet: %1. The patient stays at the head of the queue.")
                         .arg(qs(r.error));
            status = lines.last();
            route_->clear(QStringLiteral("No bed was assigned, so there is no route."));
            break;
        }
        if (r.doctor)
            lines << QStringLiteral("Attending: %1 from %2").arg(qs(r.doctor->name()), Text::clock(r.doctorStart));
        if (r.outcome == Outcome::ReleasedFromEr) {
            lines << QStringLiteral("Treated in the ER and released - ESI %1 does not need a bed.")
                         .arg(p->severity());
            status = QStringLiteral("%1 treated in the ER and released.").arg(qs(p->id()));
            route_->clear(QStringLiteral("Treated in the emergency room, so there is no route."));
            break;
        }
        lines << QStringLiteral("Bed: %1 (%2)").arg(qs(r.bed.label), qs(Patient::unitName(r.bed.unit)));
        status = QStringLiteral("%1 admitted. Bed %2.").arg(qs(p->id()), qs(r.bed.label));
        if (!r.routeError.empty()) {
            lines << QStringLiteral("No route: %1.").arg(qs(r.routeError));
            route_->clear(lines.last());
        } else {
            route_->setRoute(r.route);
        }
        break;
    }
    result_->setPlainText(lines.join(QLatin1Char('\n')));
    host_.showMessage(status);
    Q_EMIT treated(qs(p->id()), r.outcome == Outcome::Admitted);
    return true;
}

QString TreatPanel::resultText() const
{
    return result_->toPlainText();
}

RouteStepsView* TreatPanel::route() const
{
    return route_->steps();
}
