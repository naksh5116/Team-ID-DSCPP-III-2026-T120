// Launcher.cpp - role buttons, the list of open windows, save and reload.
#include "Launcher.hpp"

#include "RoleFactory.hpp"
#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "ScreenArranger.hpp"
#include "UiHelpers.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStyle>
#include <QTableView>
#include <QVBoxLayout>
#include <QWindow>

using Text::qs;

Launcher::Launcher(SessionHub& hub, QWidget* parent) : QMainWindow(parent), hub_(hub)
{
    setWindowTitle(QStringLiteral("VitalRoute - Launcher"));
    setMinimumSize(520, 380);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* heading = new QLabel(QStringLiteral("Open a role window"), central);
    heading->setProperty("textRole", QStringLiteral("heading"));
    layout->addWidget(heading);

    auto* note = new QLabel(QStringLiteral(
        "Prototype role selector. There is no sign-in: each button opens a new window "
        "for that role, and every window works on the same running session."), central);
    note->setProperty("textRole", QStringLiteral("hint"));
    note->setWordWrap(true);
    layout->addWidget(note);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);
    for (Role r : kLauncherOrder) {
        auto* b = new QPushButton(roleName(r), central);
        b->setObjectName(QStringLiteral("open") + roleName(r).remove(QLatin1Char(' ')));
        b->setAutoDefault(false);
        connect(b, &QPushButton::clicked, this, [this, r] { openRole(r); });
        buttons->addWidget(b);
    }
    buttons->addStretch(1);
    layout->addLayout(buttons);

    auto* listHeading = new QLabel(QStringLiteral("Open windows"), central);
    listHeading->setProperty("textRole", QStringLiteral("heading"));
    layout->addSpacing(4);
    layout->addWidget(listHeading);

    windowModel_ = new QStandardItemModel(0, 2, this);
    windowModel_->setHorizontalHeaderLabels({QStringLiteral("Window"), QStringLiteral("Screen")});
    windowTable_ = new QTableView(central);
    windowTable_->setObjectName(QStringLiteral("windowList"));
    windowTable_->setModel(windowModel_);
    Theme::setupTable(windowTable_);
    windowTable_->setToolTip(QStringLiteral("Double-click a window to bring it to the front"));
    connect(windowTable_, &QTableView::doubleClicked, this, [this](const QModelIndex& idx) {
        const QList<RoleWindow*> open = roleWindows();
        if (idx.row() >= 0 && idx.row() < open.size()) {
            RoleWindow* w = open.at(idx.row());
            w->raise();
            w->activateWindow();
        }
    });
    layout->addWidget(windowTable_, 1);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(8);
    auto* saveButton = new QPushButton(QStringLiteral("Save"), central);
    saveButton->setObjectName(QStringLiteral("saveButton"));
    saveButton->setAutoDefault(false);
    connect(saveButton, &QPushButton::clicked, this, [this] { save(); });
    auto* reloadButton = new QPushButton(QStringLiteral("Reload saved records"), central);
    reloadButton->setObjectName(QStringLiteral("reloadButton"));
    reloadButton->setAutoDefault(false);
    connect(reloadButton, &QPushButton::clicked, this, &Launcher::reload);
    auto* arrangeButton = new QPushButton(QStringLiteral("Arrange on screens"), central);
    arrangeButton->setObjectName(QStringLiteral("arrangeButton"));
    arrangeButton->setAutoDefault(false);
    arrangeButton->setToolTip(QStringLiteral("One window per screen: Nurse, Doctor, Patient, Super Admin. "
                                             "With one screen, a 2 x 2 grid."));
    connect(arrangeButton, &QPushButton::clicked, this, [this] { arrangeOnScreens(); });
    actions->addWidget(arrangeButton);
    actions->addWidget(saveButton);
    actions->addWidget(reloadButton);
    actions->addStretch(1);
    layout->addLayout(actions);

    screens_ = new QLabel(central);
    screens_->setObjectName(QStringLiteral("screenCount"));
    screens_->setProperty("textRole", QStringLiteral("hint"));
    layout->addWidget(screens_);

    auto* folder = new QLabel(QStringLiteral("Data folder: %1")
                                  .arg(QDir::toNativeSeparators(QDir(hub_.dataDir()).absolutePath())),
                              central);
    folder->setProperty("textRole", QStringLiteral("hint"));
    folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(folder);

    setCentralWidget(central);

    message_ = new QLabel(statusBar());
    message_->setObjectName(QStringLiteral("statusMessage"));
    statusBar()->addWidget(message_, 1);
    statusBar()->setSizeGripEnabled(false);

    // Screens come and go (a projector unplugged mid-demo): keep every
    // window reachable and the list current.
    auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    connect(app, &QGuiApplication::screenAdded, this, [this](QScreen* s) {
        refreshScreens();
        refreshWindowList();
        showMessage(QStringLiteral("Screen %1 added. Press Arrange on screens to use it.").arg(s->name()));
    });
    connect(app, &QGuiApplication::screenRemoved, this, &Launcher::rescueWindows);
    connect(app, &QGuiApplication::primaryScreenChanged, this, &Launcher::refreshScreens);

    refreshScreens();
    refreshWindowList();
}

RoleWindow* Launcher::openRole(Role role)
{
    RoleWindow* w = createRoleWindow(role, hub_);
    adopt(w);
    w->show();
    return w;
}

void Launcher::adopt(RoleWindow* w)
{
    windows_.append(w);
    connect(w, &QObject::destroyed, this, [this] { refreshWindowList(); });
    connect(w, &RoleWindow::titleChanged, this, &Launcher::refreshWindowList);
    connect(w, &RoleWindow::switchRoleRequested, this,
            [this, w](Role to) { switchRole(w, to); });
    w->winId();   // creates the native window, so its screen can be tracked
    if (QWindow* handle = w->windowHandle())
        connect(handle, &QWindow::screenChanged, this, &Launcher::refreshWindowList);
    refreshWindowList();
}

RoleWindow* Launcher::switchRole(RoleWindow* from, Role to)
{
    RoleWindow* w = createRoleWindow(to, hub_);
    adopt(w);
    if (from) {
        const bool full = from->isFullScreen();
        w->setGeometry(from->geometry());
        if (full)
            w->showFullScreen();
        else
            w->show();
        from->close();
    } else {
        w->show();
    }
    return w;
}

QList<RoleWindow*> Launcher::roleWindows() const
{
    QList<RoleWindow*> out;
    for (const QPointer<RoleWindow>& p : windows_)
        if (p)
            out.append(p.data());
    return out;
}

void Launcher::refreshWindowList()
{
    // Drop windows that have been closed (and deleted).
    for (int i = windows_.size() - 1; i >= 0; --i)
        if (!windows_.at(i))
            windows_.removeAt(i);

    const QList<QScreen*> screens = QGuiApplication::screens();
    windowModel_->setRowCount(0);
    for (RoleWindow* w : roleWindows()) {
        QString screenText;
        if (QScreen* s = w->screen()) {
            screenText = QStringLiteral("Screen %1").arg(screens.indexOf(s) + 1);
            if (!s->name().isEmpty())
                screenText += QStringLiteral(" (%1)").arg(s->name());
        }
        windowModel_->appendRow({new QStandardItem(w->windowTitle()), new QStandardItem(screenText)});
    }
    windowTable_->resizeColumnToContents(0);
}

bool Launcher::save()
{
    try {
        const PersistenceManager::Counts c = hub_.save();
        showMessage(QStringLiteral("Saved %1 patients, %2 staff, %3 visits, %4 bills (%5 charges) to %6/")
                        .arg(c.patients).arg(c.staff).arg(c.visits).arg(c.bills).arg(c.charges)
                        .arg(qs(hub_.session().recordsDir())));
        return true;
    } catch (const std::exception& e) {
        showMessage(RoleWindow::describe(e), true);
        return false;
    }
}

void Launcher::reload()
{
    if (!confirm(this, QStringLiteral("Reload saved records"),
                 QStringLiteral("Reload the session from %1/?\n\n"
                                "Changes since the last save are lost in every window. Theatre bookings "
                                "and transfer requests are cleared, because they are not part of the "
                                "saved records.")
                     .arg(QDir::toNativeSeparators(qs(hub_.session().recordsDir()))),
                 QStringLiteral("Reload")))
        return;
    try {
        const HospitalSession::StartupReport r = hub_.reload();
        if (r.loadedFromRecords)
            showMessage(QStringLiteral("Loaded %1 patients, %2 staff, %3 visits and %4 bills from %5/")
                            .arg(r.loaded.patients).arg(r.loaded.staff).arg(r.loaded.visits)
                            .arg(r.loaded.bills).arg(qs(hub_.session().recordsDir())));
        else
            showMessage(QStringLiteral("No saved records: sample patients and staff created again."));
    } catch (const std::exception& e) {
        showMessage(RoleWindow::describe(e), true);
    }
}

void Launcher::showMessage(const QString& text, bool error)
{
    message_->setProperty("textRole", error ? QVariant(QStringLiteral("error")) : QVariant());
    message_->setText(text);
    message_->style()->unpolish(message_);
    message_->style()->polish(message_);
}

void Launcher::closeEvent(QCloseEvent* event)
{
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Close VitalRoute"));
    box.setText(QStringLiteral("Save the session before closing?"));
    box.setInformativeText(QStringLiteral("Every role window closes with the Launcher."));
    QPushButton* saveButton = box.addButton(QStringLiteral("Save and close"), QMessageBox::AcceptRole);
    QPushButton* discard = box.addButton(QStringLiteral("Close without saving"), QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(saveButton);
    box.exec();

    if (box.clickedButton() == saveButton) {
        if (!save()) {   // keep everything open so the error can be read
            event->ignore();
            return;
        }
    } else if (box.clickedButton() != discard) {
        event->ignore();
        return;
    }

    for (RoleWindow* w : roleWindows())
        w->close();
    event->accept();
    QApplication::quit();
}

QString Launcher::arrangeOnScreens()
{
    const QString result = ScreenArranger::apply(roleWindows());
    refreshWindowList();
    showMessage(result);
    return result;
}

void Launcher::refreshScreens()
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    const QScreen* primary = QGuiApplication::primaryScreen();
    screens_->setText(QStringLiteral("Screens: %1%2")
                          .arg(screens.size())
                          .arg(primary ? QStringLiteral(" (primary: %1, %2 x %3)")
                                             .arg(primary->name())
                                             .arg(primary->availableGeometry().width())
                                             .arg(primary->availableGeometry().height())
                                       : QString()));
}

void Launcher::rescueWindows(QScreen* removed)
{
    // Areas of the screens that remain.
    QList<QRect> remaining;
    for (QScreen* s : QGuiApplication::screens())
        if (s != removed)
            remaining.append(s->availableGeometry());
    QScreen* primary = QGuiApplication::primaryScreen();
    if (primary == removed || !primary)
        primary = QGuiApplication::screens().isEmpty() ? nullptr : QGuiApplication::screens().first();
    if (!primary || primary == removed) {
        refreshScreens();
        return;
    }

    QList<QWidget*> windows;
    windows.append(this);
    for (RoleWindow* w : roleWindows())
        windows.append(w);
    int moved = 0;
    for (QWidget* w : windows) {
        const bool wasOnRemoved = w->windowHandle() && w->windowHandle()->screen() == removed;
        if (!wasOnRemoved && ScreenArranger::visibleOnAny(w->frameGeometry(), remaining))
            continue;
        const bool full = w->isFullScreen();
        if (full || w->isMaximized())
            w->showNormal();
        if (QWindow* handle = w->windowHandle())
            handle->setScreen(primary);
        w->setGeometry(ScreenArranger::rescue(w->geometry(), primary->availableGeometry()));
        if (full)
            w->showFullScreen();
        ++moved;
    }
    refreshScreens();
    refreshWindowList();
    showMessage(QStringLiteral("Screen %1 removed. %2 window(s) moved to %3.")
                    .arg(removed ? removed->name() : QString()).arg(moved).arg(primary->name()));
}
