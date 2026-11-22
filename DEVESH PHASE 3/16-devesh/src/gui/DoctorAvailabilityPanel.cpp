// DoctorAvailabilityPanel.cpp
#include "DoctorAvailabilityPanel.hpp"

#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "StaffTheatreModels.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QTableView>
#include <QVBoxLayout>

using Text::qs;
using Text::ss;

DoctorAvailabilityPanel::DoctorAvailabilityPanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(8);

    auto* row = new QWidget(this);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8);
    h->addWidget(new QLabel(QStringLiteral("Specialty"), row));
    specialty_ = new QComboBox(row);
    specialty_->setObjectName(QStringLiteral("availabilitySpecialty"));
    specialty_->setMinimumContentsLength(20);
    h->addWidget(specialty_);
    auto* find = new QPushButton(QStringLiteral("Find next available doctor"), row);
    find->setObjectName(QStringLiteral("availabilityFind"));
    find->setProperty("primary", true);
    connect(find, &QPushButton::clicked, this, [this] { findNext(); });
    h->addWidget(find);
    h->addStretch(1);
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), row);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(sc, &QShortcut::activated, this, [this] { findNext(); });
    }
    v->addWidget(row);

    result_ = new QLabel(QStringLiteral("Choose a specialty."), this);
    result_->setObjectName(QStringLiteral("availabilityResult"));
    result_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(result_);

    model_ = new DoctorAvailabilityModel(hub_, this);
    table_ = new QTableView;
    table_->setObjectName(QStringLiteral("availabilityTable"));
    table_->setModel(model_);
    Theme::setupTable(table_);
    fitColumnsOnReset(table_);
    v->addWidget(tableSection(QStringLiteral("Availability array"),
                              QStringLiteral("Kept sorted by specialty, then next-free time. The next "
                                             "available doctor is found with a lower-bound binary search. "
                                             "Press F5 to recheck who is free now."),
                              table_, this),
                 1);

    connect(&hub_, &SessionHub::staffChanged, this, &DoctorAvailabilityPanel::refreshSpecialties);
    connect(&hub_, &SessionHub::reloaded, this, &DoctorAvailabilityPanel::refreshSpecialties);
    refreshSpecialties();
}

void DoctorAvailabilityPanel::refreshSpecialties()
{
    if (!hub_.isStarted())
        return;
    const QString current = specialty_->currentText();
    specialty_->clear();
    for (const std::string& s : hub_.session().specialties())
        specialty_->addItem(qs(s));
    const int keep = specialty_->findText(current);
    if (keep >= 0)
        specialty_->setCurrentIndex(keep);
}

QString DoctorAvailabilityPanel::findNext()
{
    host_.showMessage(QString());
    const QString spec = specialty_->currentText();
    if (spec.isEmpty()) {
        result_->setText(QStringLiteral("No doctors on the roster."));
        return result_->text();
    }
    const int now = vrclock::minutesNow();
    QString text;
    if (const Doctor* d = hub_.session().nextAvailableDoctor(ss(spec))) {
        // The console's option 6 sentence.
        text = QStringLiteral("Next available in %1: %2 - ").arg(spec, qs(d->name()));
        if (d->nextFree() <= now)
            text += QStringLiteral("free now");
        else
            text += QStringLiteral("free at %1 (in %2 min)").arg(Text::clock(d->nextFree())).arg(d->nextFree() - now);
        if (const int row = model_->rowOfKey(qs(d->id())); row >= 0)
            table_->selectRow(row);
    } else {
        text = QStringLiteral("Nobody on the roster has the specialty %1.").arg(spec);
    }
    result_->setText(text);
    host_.showMessage(text);
    return text;
}
