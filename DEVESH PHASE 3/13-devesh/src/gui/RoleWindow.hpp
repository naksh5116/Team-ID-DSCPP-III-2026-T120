// RoleWindow.hpp - what every role window has in common.
//
//  - a status bar: waiting emergency / OPD counts, next patient, free ICU
//    and ward beds, the role; results and errors on the left
//  - menus: File (Save Ctrl+S, Close window Ctrl+W), View (Refresh F5,
//    Full screen F11), Role (switch to another role)
//  - attempt(): runs a session action and shows its exception message the
//    way the console does ("Not found: ...")
//
// Closing a role window never quits the application; the Launcher owns that.
#ifndef VR_GUI_ROLE_WINDOW_HPP
#define VR_GUI_ROLE_WINDOW_HPP

#include "Roles.hpp"

#include <QMainWindow>

#include <exception>
#include <functional>

class QLabel;
class SessionHub;

class RoleWindow : public QMainWindow {
    Q_OBJECT
public:
    RoleWindow(SessionHub& hub, Role role, QWidget* parent = nullptr);

    Role role() const { return role_; }

    // Message area on the left of the status bar. Stays until replaced.
    void showMessage(const QString& text);
    void showError(const QString& text);

    // Runs `action`; if it throws, shows the console-style message and
    // returns false. Clears the previous message first.
    bool attempt(const std::function<void()>& action);

    // "Not found: no patient with ID ...", same prefixes as the console.
    static QString describe(const std::exception& e);

    // F11: full screen on and off (back to maximised if it was maximised).
    void toggleFullScreen();

    // While tiled with other windows on one screen the minimum size shrinks
    // to the tile; an empty size restores the normal 1100 x 700 minimum.
    void setCompactMinimum(const QSize& tile);

    // The window title: "VitalRoute - <role>[ <suffix>]".
    void updateTitle();

Q_SIGNALS:
    void switchRoleRequested(Role role);
    void titleChanged();

protected:
    SessionHub& hub() { return hub_; }
    const SessionHub& hub() const { return hub_; }

    // Installs the role's own content below the menu bar.
    void setWorkArea(QWidget* widget);
    // Extra title text, e.g. the patient ID in a Patient window.
    virtual QString titleSuffix() const { return QString(); }
    // Called after F11 switches full screen on or off.
    virtual void fullScreenChanged(bool fullScreen) { Q_UNUSED(fullScreen); }


private:
    void buildMenus();
    void buildStatusBar();
    void updateStatus();
    void save();

    SessionHub& hub_;
    Role role_;
    QLabel* message_ = nullptr;
    QLabel* emergency_ = nullptr;
    QLabel* opd_ = nullptr;
    QLabel* next_ = nullptr;
    QLabel* icu_ = nullptr;
    QLabel* ward_ = nullptr;
    QLabel* roleLabel_ = nullptr;
    bool wasMaximized_ = false;
};

#endif // VR_GUI_ROLE_WINDOW_HPP
