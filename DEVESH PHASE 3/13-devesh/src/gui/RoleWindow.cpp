// RoleWindow.cpp - status bar, menus and shortcuts shared by every role.
#include "RoleWindow.hpp"

#include "Exceptions.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"

#include <QAction>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>
#include <QStatusBar>
#include <QStyle>

using Text::qs;

namespace {

QLabel* statusField(QWidget* parent, const char* name)
{
    auto* l = new QLabel(parent);
    l->setObjectName(QLatin1String(name));
    l->setProperty("statusField", true);
    return l;
}

} // namespace

RoleWindow::RoleWindow(SessionHub& hub, Role role, QWidget* parent)
    : QMainWindow(parent), hub_(hub), role_(role)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(1100, 700);   // logical pixels (DESIGN.md)

    // Open at a comfortable size that still fits the current screen.
    if (const QScreen* screen = QGuiApplication::primaryScreen()) {
        const QSize avail = screen->availableGeometry().size();
        resize(qMax(1100, qMin(1280, avail.width() - 80)), qMax(700, qMin(800, avail.height() - 80)));
    }

    buildMenus();
    buildStatusBar();
    updateTitle();

    connect(&hub_, &SessionHub::queueChanged, this, &RoleWindow::updateStatus);
    connect(&hub_, &SessionHub::bedsChanged, this, &RoleWindow::updateStatus);
    connect(&hub_, &SessionHub::saved, this, [this] { updateStatus(); });
    connect(&hub_, &SessionHub::reloaded, this, [this] { updateStatus(); });
    updateStatus();
}

void RoleWindow::buildMenus()
{
    QMenu* file = menuBar()->addMenu(QStringLiteral("&File"));
    if (role_ != Role::Patient) {   // the patient view is read-only
        QAction* save = file->addAction(QStringLiteral("&Save"));
        save->setShortcut(QKeySequence::Save);
        connect(save, &QAction::triggered, this, &RoleWindow::save);
        file->addSeparator();
    }
    QAction* close = file->addAction(QStringLiteral("&Close window"));
    close->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    connect(close, &QAction::triggered, this, &QWidget::close);

    QMenu* view = menuBar()->addMenu(QStringLiteral("&View"));
    QAction* refresh = view->addAction(QStringLiteral("&Refresh"));
    refresh->setShortcut(QKeySequence(Qt::Key_F5));
    connect(refresh, &QAction::triggered, this, [this] { hub_.refreshAll(); });
    QAction* full = view->addAction(QStringLiteral("&Full screen"));
    full->setShortcut(QKeySequence(Qt::Key_F11));
    connect(full, &QAction::triggered, this, &RoleWindow::toggleFullScreen);

    QMenu* roleMenu = menuBar()->addMenu(QStringLiteral("&Role"));
    for (Role r : kLauncherOrder) {
        QAction* a = roleMenu->addAction(QStringLiteral("Switch to %1").arg(roleName(r)));
        a->setEnabled(r != role_);
        connect(a, &QAction::triggered, this, [this, r] { Q_EMIT switchRoleRequested(r); });
    }

    // The shortcuts must keep working when the menu bar is hidden (wall
    // display), so every menu action also belongs to the window itself.
    for (QAction* top : menuBar()->actions())
        if (QMenu* m = top->menu())
            addActions(m->actions());
}

void RoleWindow::buildStatusBar()
{
    QStatusBar* bar = statusBar();
    bar->setSizeGripEnabled(true);
    message_ = new QLabel(bar);
    // On a narrow (tiled) window the message gives way first, so the counts and
    // the next patient stay readable; its full text is in the tooltip.
    message_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    message_->setMinimumWidth(0);
    message_->setObjectName(QStringLiteral("statusMessage"));
    message_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bar->addWidget(message_, 1);

    emergency_ = statusField(bar, "statusEmergency");
    opd_ = statusField(bar, "statusOpd");
    next_ = statusField(bar, "statusNext");
    icu_ = statusField(bar, "statusIcu");
    ward_ = statusField(bar, "statusWard");
    roleLabel_ = statusField(bar, "statusRole");
    for (QLabel* l : {emergency_, opd_, next_, icu_, ward_, roleLabel_})
        bar->addPermanentWidget(l);
    roleLabel_->setText(QStringLiteral("Role: %1").arg(roleName(role_)));
}

void RoleWindow::updateStatus()
{
    if (!hub_.isStarted())
        return;
    const HospitalSession& s = hub_.session();
    const TriageUnit& t = s.triage();
    emergency_->setText(QStringLiteral("Emergency waiting: %1").arg(t.emergencyCount()));
    opd_->setText(QStringLiteral("OPD waiting: %1").arg(t.opdCount()));
    if (t.hasEmergency())
        next_->setText(QStringLiteral("Next: <span style=\"font-family:'%1'\">%2</span>")
                           .arg(Theme::monoFont().family(), qs(t.peekEmergency())));
    else if (t.hasOpd())
        next_->setText(QStringLiteral("Next: <span style=\"font-family:'%1'\">%2</span> (OPD)")
                           .arg(Theme::monoFont().family(), qs(t.opdLine().front())));
    else
        next_->setText(QStringLiteral("Next: nobody waiting"));
    icu_->setText(QStringLiteral("ICU beds free: %1").arg(s.router().freeBeds(CareUnit::ICU)));
    ward_->setText(QStringLiteral("Ward beds free: %1").arg(s.router().freeBeds(CareUnit::Ward)));
}

void RoleWindow::setWorkArea(QWidget* widget)
{
    setCentralWidget(widget);
}

void RoleWindow::updateTitle()
{
    QString title = QStringLiteral("VitalRoute - %1").arg(roleName(role_));
    const QString suffix = titleSuffix();
    if (!suffix.isEmpty())
        title += QLatin1Char(' ') + suffix;   // "VitalRoute - Patient VR-2026-000004"
    setWindowTitle(title);
    Q_EMIT titleChanged();
}

void RoleWindow::showMessage(const QString& text)
{
    message_->setProperty("textRole", QVariant());
    message_->setText(text);
    message_->setToolTip(text);   // full text when the status bar cuts it short
    message_->style()->unpolish(message_);
    message_->style()->polish(message_);
}

void RoleWindow::showError(const QString& text)
{
    message_->setProperty("textRole", QStringLiteral("error"));
    message_->setText(text);
    message_->setToolTip(text);
    message_->style()->unpolish(message_);
    message_->style()->polish(message_);
}

QString RoleWindow::describe(const std::exception& e)
{
    const QString what = qs(e.what());
    if (dynamic_cast<const InvalidSeverityException*>(&e))
        return QStringLiteral("Registration cancelled: %1.").arg(what);
    if (dynamic_cast<const PatientNotFoundException*>(&e))
        return QStringLiteral("Not found: %1.").arg(what);
    if (dynamic_cast<const BedUnavailableException*>(&e))
        return QStringLiteral("No bed: %1.").arg(what);
    if (dynamic_cast<const NoRouteFoundException*>(&e))
        return QStringLiteral("No route: %1.").arg(what);
    if (dynamic_cast<const SlotUnavailableException*>(&e))
        return QStringLiteral("Theatre: %1.").arg(what);
    return QStringLiteral("Error: %1.").arg(what);
}

bool RoleWindow::attempt(const std::function<void()>& action)
{
    showMessage(QString());
    try {
        action();
        return true;
    } catch (const std::exception& e) {
        showError(describe(e));
        return false;
    }
}

void RoleWindow::save()
{
    attempt([this] {
        const PersistenceManager::Counts c = hub_.save();
        showMessage(QStringLiteral("Saved %1 patients, %2 staff, %3 visits, %4 bills (%5 charges) to %6/")
                        .arg(c.patients).arg(c.staff).arg(c.visits).arg(c.bills).arg(c.charges)
                        .arg(qs(hub_.session().recordsDir())));
    });
}

void RoleWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        if (wasMaximized_)
            showMaximized();
        else
            showNormal();
    } else {
        wasMaximized_ = isMaximized();
        showFullScreen();   // on the screen the window is on
    }
    fullScreenChanged(isFullScreen());
}

void RoleWindow::setCompactMinimum(const QSize& tile)
{
    if (tile.isEmpty())
        setMinimumSize(1100, 700);
    else
        setMinimumSize(qMin(1100, tile.width() - 16), qMin(700, tile.height() - 40));
}
