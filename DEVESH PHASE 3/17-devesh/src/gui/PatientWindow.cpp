// PatientWindow.cpp
#include "PatientWindow.hpp"

#include "BillView.hpp"
#include "PatientRecordView.hpp"
#include "RoutePanel.hpp"
#include "RouteStepsView.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

using Text::qs;
using Text::ss;

namespace {

QWidget* section(const QString& heading, QWidget* content, QWidget* parent)
{
    auto* w = new QWidget(parent);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(4);
    v->addWidget(roleLabel(heading, "heading", w));
    content->setParent(w);
    v->addWidget(content, 1);
    return w;
}

} // namespace

PatientWindow::PatientWindow(SessionHub& hub, QWidget* parent) : RoleWindow(hub, Role::Patient, parent)
{
    auto* central = new QWidget(this);
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    // ---- "Your patient ID"
    // ---- wall display header (full screen only)
    wallBar_ = new QWidget(central);
    auto* wb = new QVBoxLayout(wallBar_);
    wb->setContentsMargins(16, 12, 16, 4);
    wb->setSpacing(2);
    wallHeader_ = new QLabel(wallBar_);
    wallHeader_->setObjectName(QStringLiteral("wallHeader"));
    wallHeader_->setFont(Theme::scaledFont(1.8, true));
    wallHeader_->setWordWrap(true);
    wb->addWidget(wallHeader_);
    wb->addWidget(roleLabel(QStringLiteral("Press F11 to leave full screen."), "hint", wallBar_));
    wallBar_->hide();
    v->addWidget(wallBar_);

    idRow_ = new QWidget(central);
    auto* h = new QHBoxLayout(idRow_);
    h->setContentsMargins(8, 8, 8, 8);
    h->setSpacing(8);
    h->addWidget(new QLabel(QStringLiteral("Your patient ID"), idRow_));
    idEdit_ = new QLineEdit(idRow_);
    idEdit_->setObjectName(QStringLiteral("patientIdInput"));
    idEdit_->setFont(Theme::monoFont());
    idEdit_->setPlaceholderText(QStringLiteral("VR-2026-000004"));
    idEdit_->setMaximumWidth(idEdit_->fontMetrics().horizontalAdvance(QStringLiteral("VR-2026-0000000")) + 24);
    h->addWidget(idEdit_);
    auto* show = new QPushButton(QStringLiteral("Show my record"), idRow_);
    show->setObjectName(QStringLiteral("patientShow"));
    show->setProperty("primary", true);
    connect(show, &QPushButton::clicked, this, [this] { showPatient(idEdit_->text()); });
    h->addWidget(show);
    h->addWidget(roleLabel(QStringLiteral("This view is read-only."), "hint", idRow_));
    h->addStretch(1);
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), idRow_);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(sc, &QShortcut::activated, this, [this] { showPatient(idEdit_->text()); });
    }
    v->addWidget(idRow_);

    // ---- record | route over bill
    auto* split = new QSplitter(Qt::Horizontal, central);
    split->setChildrenCollapsible(false);
    record_ = new PatientRecordView(hub);
    recordSection_ = section(QStringLiteral("Your record"), record_, split);
    split->addWidget(recordSection_);

    auto* right = new QSplitter(Qt::Vertical, split);
    right->setChildrenCollapsible(false);
    routeArea_ = new QWidget;
    auto* ra = new QHBoxLayout(routeArea_);
    ra->setContentsMargins(0, 0, 0, 0);
    route_ = new RoutePanel(hub, Qt::Horizontal, routeArea_);   // floor map beside the steps
    ra->addWidget(route_);
    routeSection_ = section(QStringLiteral("Walking directions to your bed"), routeArea_, right);
    routeHeading_ = routeSection_->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
    right->addWidget(routeSection_);
    bill_ = new BillView(hub);
    billSection_ = section(QStringLiteral("Your bill"), bill_, right);
    right->addWidget(billSection_);
    right->setStretchFactor(0, 3);
    right->setStretchFactor(1, 2);
    split->addWidget(right);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);
    v->addWidget(split, 1);
    setWorkArea(central);

    connect(record_, &PatientRecordView::patientGone, this, [this](const QString& id) {
        showMessage(QStringLiteral("%1 is no longer registered.").arg(id));
        refreshRoute();
    });
    connect(&hub, &SessionHub::routeAssigned, this, [this](const QString& id) {
        if (id != id_)
            return;
        refreshRoute();
        if (const Patient* p = this->hub().session().patients().tryFind(ss(id)))
            showMessage(QStringLiteral("A bed has been assigned: %1. The walking directions are shown.")
                            .arg(qs(p->bedLabel())));
    });
    connect(&hub, &SessionHub::bedsChanged, this, &PatientWindow::refreshRoute);
    connect(&hub, &SessionHub::patientsChanged, this, &PatientWindow::refreshRoute);
    connect(&hub, &SessionHub::reloaded, this, [this] {
        shownRoute_.clear();   // the route panel cleared itself; build it again
        refreshRoute();
    });
    refreshRoute();
    idEdit_->setFocus();
}

bool PatientWindow::showPatient(const QString& rawId)
{
    showMessage(QString());
    const QString id = rawId.trimmed();
    if (id.isEmpty()) {
        showError(QStringLiteral("Enter your patient ID, for example VR-2026-000004."));
        return false;
    }
    const bool ok = attempt([&] { hub().session().findPatient(ss(id)); });   // hash lookup
    id_ = ok ? id : QString();
    idEdit_->setText(id);
    record_->setPatientId(id_);
    bill_->setPatientId(id_);
    refreshRoute();
    updateTitle();
    return ok;
}

void PatientWindow::refreshRoute()
{
    // The key of what is shown: patient and bed. Rebuilding only when it
    // changes keeps the selected step and floor while other patients change.
    QString shown;
    const Patient* p = id_.isEmpty() ? nullptr : hub().session().patients().tryFind(ss(id_));
    if (p)
        shown = id_ + QLatin1Char('|') + qs(p->bedCode()) + QLatin1Char('|') +
                QString::number(static_cast<int>(p->status()));
    if (!shown.isEmpty() && shown == shownRoute_)
        return;
    shownRoute_ = shown;
    updateWallHeader();

    if (id_.isEmpty()) {
        route_->clear(QStringLiteral("Enter your patient ID to see your record."));
        return;
    }
    if (!p) {
        route_->clear(QStringLiteral("No record for this ID."));
        return;
    }
    if (p->bedCode().empty()) {
        switch (p->status()) {
        case PatientStatus::Waiting:
            route_->clear(QStringLiteral("You do not have a bed yet. The directions appear here as soon as "
                                         "a doctor assigns one."));
            break;
        case PatientStatus::Discharged:
            route_->clear(QStringLiteral("You have been discharged. No bed is assigned."));
            break;
        default:
            route_->clear(QStringLiteral("Your visit did not need a bed."));
            break;
        }
        return;
    }
    try {
        route_->setRoute(hub().session().routeForPatient(p->id()));
    } catch (const std::exception& e) {
        route_->clear(describe(e));
    }
}

RouteStepsView* PatientWindow::route() const
{
    return route_->steps();
}

void PatientWindow::setWallDisplay(bool on)
{
    wall_ = on;
    // Larger type only, no decoration: everything but the route goes away.
    menuBar()->setVisible(!on);
    statusBar()->setVisible(!on);
    idRow_->setVisible(!on);
    recordSection_->setVisible(!on);
    billSection_->setVisible(!on);
    if (routeHeading_)
        routeHeading_->setVisible(!on);
    wallBar_->setVisible(on);
    route_->setTextScale(on ? 1.8 : 1.0);   // DESIGN.md: directions 1.8x, distance and time 2.2x
    route_->setWallMode(on);               // every floor the route crosses, side by side
    updateWallHeader();
}

void PatientWindow::updateWallHeader()
{
    QString text;
    const Patient* p = id_.isEmpty() ? nullptr : hub().session().patients().tryFind(ss(id_));
    if (!p)
        text = QStringLiteral("Enter a patient ID first.");
    else if (p->bedLabel().empty())
        text = QStringLiteral("Patient %1: no bed assigned yet").arg(id_);
    else
        text = QStringLiteral("Patient %1: walking directions to %2").arg(id_, qs(p->bedLabel()));
    wallHeader_->setText(text);
}

QString PatientWindow::wallHeaderText() const
{
    return wallHeader_->text();
}
