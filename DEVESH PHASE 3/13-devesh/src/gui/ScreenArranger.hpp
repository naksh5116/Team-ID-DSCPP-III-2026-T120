// ScreenArranger.hpp - where each role window goes on the available screens.
//
// The demo runs on one computer with 2-4 monitors (or a laptop plus a
// projector), one role window per screen. Rules:
//   - windows are taken in role order Nurse, Doctor, Patient, Super Admin
//     (then in the order they were opened)
//   - N screens: window i goes to screen i mod N (wrapping when there are
//     more windows than screens); a window alone on its screen is maximised
//     there, several windows sharing a screen are tiled on it
//   - one screen: the windows are tiled in a 2 x 2 grid
// While tiled, a window's minimum size shrinks to its tile (a 2 x 2 tile on
// a 1920 x 1080 screen is smaller than the usual 1100 x 700 minimum).
//
// plan() is pure (roles and rectangles in, rectangles out) so the checks
// can test 1, 2 and 4 screens on any machine; apply() moves real windows.
#ifndef VR_GUI_SCREEN_ARRANGER_HPP
#define VR_GUI_SCREEN_ARRANGER_HPP

#include "Roles.hpp"

#include <QList>
#include <QRect>
#include <QString>

class QScreen;
class RoleWindow;

namespace ScreenArranger {

struct Slot {
    int window = 0;        // index into the list passed to plan()
    int screen = 0;        // index into the screen list
    QRect area;            // frame rectangle on that screen
    bool maximise = false; // alone on its screen
};

// Rectangles of a grid over `area` for `count` windows: 1 = whole area,
// 2 = side by side, 3-4 = 2 x 2, more = ceil(sqrt(n)) columns.
QList<QRect> grid(const QRect& area, int count, bool alwaysTwoByTwo = false);

// Window order: by role (Nurse, Doctor, Patient, Super Admin), stable.
QList<int> roleOrder(const QList<Role>& roles);

// Where each window goes. `screens` are the screens' available areas.
QList<Slot> plan(const QList<Role>& roles, const QList<QRect>& screens);

// Moves the windows as planned on the real screens. Returns a sentence for
// the status bar ("Placed 4 windows on 4 screens.").
QString apply(const QList<RoleWindow*>& windows);

// True when the rectangle's centre is on one of the screens.
bool visibleOnAny(const QRect& frame, const QList<QRect>& screens);
// A frame that fits on `primary`, centred, no larger than it.
QRect rescue(const QRect& frame, const QRect& primary);

} // namespace ScreenArranger

#endif // VR_GUI_SCREEN_ARRANGER_HPP
