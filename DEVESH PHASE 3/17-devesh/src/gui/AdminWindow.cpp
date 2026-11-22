// AdminWindow.cpp
#include "AdminWindow.hpp"

#include "AdminPanels.hpp"
#include "BillingPanel.hpp"
#include "PatientLookupPanel.hpp"
#include "RegistrationPanel.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "TheatrePanel.hpp"
#include "TransferDeskPanel.hpp"
#include "TreatPanel.hpp"
#include "UiHelpers.hpp"

#include <QAction>
#include <QTabWidget>
#include <QToolBar>

using Text::qs;

AdminWindow::AdminWindow(SessionHub& hub, QWidget* parent) : RoleWindow(hub, Role::SuperAdmin, parent)
{
    // Session actions, always in view (text only, no icons).
    QToolBar* bar = addToolBar(QStringLiteral("Session"));
    bar->setObjectName(QStringLiteral("sessionToolbar"));
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    undo_ = bar->addAction(QStringLiteral("Undo last entry"));
    undo_->setObjectName(QStringLiteral("undoAction"));
    connect(undo_, &QAction::triggered, this, [this] { undoLast(); });
    QAction* save = bar->addAction(QStringLiteral("Save"));
    save->setObjectName(QStringLiteral("saveAction"));
    save->setToolTip(QStringLiteral("Save the session to the records folder (Ctrl+S)"));
    connect(save, &QAction::triggered, this, [this] {
        attempt([this] {
            const PersistenceManager::Counts c = this->hub().save();
            showMessage(QStringLiteral("Saved %1 patients, %2 staff, %3 visits, %4 bills (%5 charges) to %6/")
                            .arg(c.patients).arg(c.staff).arg(c.visits).arg(c.bills).arg(c.charges)
                            .arg(qs(this->hub().session().recordsDir())));
        });
    });

    tabs_ = new QTabWidget(this);
    tabs_->setUsesScrollButtons(true);
    registration_ = new RegistrationPanel(hub, *this, tabs_);
    treat_ = new TreatPanel(hub, *this, tabs_);
    lookup_ = new PatientLookupPanel(hub, *this, tabs_);
    billing_ = new BillingPanel(hub, *this, tabs_);
    admissions_ = new AdmissionsPanel(hub, *this, tabs_);
    staff_ = new StaffPanel(hub, *this, tabs_);
    theatres_ = new TheatrePanel(hub, *this, tabs_);
    transfers_ = new TransferDeskPanel(hub, *this, tabs_);
    indexes_ = new IndexStatsPanel(hub, tabs_);
    tabs_->addTab(registration_, QStringLiteral("Registration"));
    tabs_->addTab(treat_, QStringLiteral("Treatment"));
    tabs_->addTab(lookup_, QStringLiteral("Patient record"));
    tabs_->addTab(billing_, QStringLiteral("Billing and discharge"));
    tabs_->addTab(admissions_, QStringLiteral("Admissions"));
    tabs_->addTab(staff_, QStringLiteral("Staff"));
    tabs_->addTab(theatres_, QStringLiteral("Theatres"));
    tabs_->addTab(transfers_, QStringLiteral("Transfers"));
    tabs_->addTab(indexes_, QStringLiteral("Indexes"));
    setWorkArea(tabs_);

    // The undo button names what it would undo; it follows every change.
    for (auto signal : {&SessionHub::patientsChanged, &SessionHub::billsChanged, &SessionHub::queueChanged})
        connect(&hub, signal, this, &AdminWindow::updateUndoAction);
    connect(&hub, &SessionHub::reloaded, this, &AdminWindow::updateUndoAction);
    updateUndoAction();
}

void AdminWindow::updateUndoAction()
{
    if (!hub().isStarted())
        return;
    const std::string note = hub().session().nextUndoNote();
    undo_->setEnabled(!note.empty());
    undo_->setToolTip(note.empty() ? QStringLiteral("Nothing to undo")
                                   : QStringLiteral("Undo the %1").arg(qs(note)));
}

bool AdminWindow::undoLast()
{
    using Kind = HospitalSession::UndoResult::Kind;
    showMessage(QString());
    const std::string note = hub().session().nextUndoNote();
    if (note.empty()) {
        showMessage(QStringLiteral("Nothing to undo."));
        return false;
    }
    if (!confirm(this, QStringLiteral("Undo last entry"),
                 QStringLiteral("Undo the %1?\n\nEvery open window is updated.").arg(qs(note)),
                 QStringLiteral("Undo")))
        return false;

    HospitalSession::UndoResult r;
    if (!attempt([&] { r = hub().undoLast(); }))
        return false;
    // The console's option 11 sentences.
    switch (r.kind) {
    case Kind::Nothing:
        showMessage(QStringLiteral("Nothing to undo."));
        return false;
    case Kind::RegistrationDropped:
        showMessage(QStringLiteral("Cannot undo the %1: %2 has already been seen. Entry dropped.")
                        .arg(qs(r.note), qs(r.patientId)));
        return false;
    case Kind::ChargeDropped:
        showMessage(QStringLiteral("Cannot undo the %1: the bill has changed since (or is settled). Entry dropped.")
                        .arg(qs(r.note)));
        return false;
    case Kind::RegistrationUndone:
        showMessage(QStringLiteral("Undone: %1 (%2).").arg(qs(r.note), qs(r.patientId)));
        return true;
    case Kind::ChargeUndone:
        showMessage(QStringLiteral("Undone: %1 (%2).").arg(qs(r.note), Text::money(r.amount)));
        return true;
    }
    return false;
}
