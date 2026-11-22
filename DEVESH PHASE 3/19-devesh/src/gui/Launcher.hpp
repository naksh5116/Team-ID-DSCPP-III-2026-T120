// Launcher.hpp - the small start window: open role windows, save, reload.
//
// A prototype role selector, not a login. Each button opens a NEW top-level
// window for that role on the shared SessionHub; the Launcher stays open and
// the same role can be opened more than once. Closing the Launcher asks once
// whether to save, then closes every window and quits.
#ifndef VR_GUI_LAUNCHER_HPP
#define VR_GUI_LAUNCHER_HPP

#include "Roles.hpp"

#include <QList>
#include <QMainWindow>
#include <QPointer>

class QLabel;
class QScreen;
class QStandardItemModel;
class QTableView;
class RoleWindow;
class SessionHub;

class Launcher : public QMainWindow {
    Q_OBJECT
public:
    explicit Launcher(SessionHub& hub, QWidget* parent = nullptr);

    // Opens and shows a new window for `role`.
    RoleWindow* openRole(Role role);
    // Open role windows, oldest first.
    QList<RoleWindow*> roleWindows() const;

    // Replaces `from` with a new window for `to` at the same place and size.
    RoleWindow* switchRole(RoleWindow* from, Role to);

    // Save; returns false (and shows why) if it failed.
    bool save();
    // Asks, then reloads the session from data/records (every window refreshes).
    void reload();
    // Places the role windows on the screens (ScreenArranger) and reports.
    QString arrangeOnScreens();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void adopt(RoleWindow* window);
    void refreshWindowList();
    void showMessage(const QString& text, bool error = false);
    void refreshScreens();
    void rescueWindows(QScreen* removed);

    SessionHub& hub_;
    QList<QPointer<RoleWindow>> windows_;
    QStandardItemModel* windowModel_ = nullptr;
    QTableView* windowTable_ = nullptr;
    QLabel* message_ = nullptr;
    QLabel* screens_ = nullptr;
};

#endif // VR_GUI_LAUNCHER_HPP
