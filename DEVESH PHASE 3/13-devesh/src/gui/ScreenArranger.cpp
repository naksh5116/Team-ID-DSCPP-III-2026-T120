// ScreenArranger.cpp
#include "ScreenArranger.hpp"

#include "RoleWindow.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

#include <algorithm>
#include <cmath>

namespace ScreenArranger {

namespace {

int roleRank(Role r)
{
    switch (r) {
    case Role::Nurse: return 0;
    case Role::Doctor: return 1;
    case Role::Patient: return 2;
    case Role::SuperAdmin: return 3;
    }
    return 4;
}

} // namespace

QList<QRect> grid(const QRect& area, int count, bool alwaysTwoByTwo)
{
    QList<QRect> cells;
    if (count <= 0)
        return cells;
    int cols = 1, rows = 1;
    if (alwaysTwoByTwo && count <= 4) {
        cols = rows = 2;
    } else if (count == 2) {
        cols = 2;
    } else if (count <= 4 && count > 1) {
        cols = rows = 2;
    } else if (count > 4) {
        cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
        rows = (count + cols - 1) / cols;
    }
    const int w = area.width() / cols;
    const int h = area.height() / rows;
    for (int i = 0; i < count; ++i) {
        const int c = i % cols;
        const int r = (i / cols) % rows;
        // The last column/row takes any leftover pixels.
        const int cw = c == cols - 1 ? area.width() - c * w : w;
        const int ch = r == rows - 1 ? area.height() - r * h : h;
        cells.append(QRect(area.left() + c * w, area.top() + r * h, cw, ch));
    }
    return cells;
}

QList<int> roleOrder(const QList<Role>& roles)
{
    QList<int> order;
    for (int i = 0; i < roles.size(); ++i)
        order.append(i);
    std::stable_sort(order.begin(), order.end(),
                     [&roles](int a, int b) { return roleRank(roles.at(a)) < roleRank(roles.at(b)); });
    return order;
}

QList<Slot> plan(const QList<Role>& roles, const QList<QRect>& screens)
{
    QList<Slot> slots;
    if (screens.isEmpty() || roles.isEmpty())
        return slots;
    const QList<int> order = roleOrder(roles);

    if (screens.size() == 1 && order.size() == 1) {
        slots.append(Slot{order.first(), 0, screens.first(), true});   // one window: maximised
        return slots;
    }
    if (screens.size() == 1) {
        // One screen: a 2 x 2 grid (more windows: a larger grid).
        const QList<QRect> cells = grid(screens.first(), static_cast<int>(order.size()), true);
        for (int i = 0; i < order.size(); ++i)
            slots.append(Slot{order.at(i), 0, cells.at(i), false});
        return slots;
    }

    // Several screens: window i on screen i mod N.
    QList<QList<int>> perScreen(screens.size());
    for (int i = 0; i < order.size(); ++i)
        perScreen[i % screens.size()].append(order.at(i));
    for (int s = 0; s < screens.size(); ++s) {
        const QList<int>& here = perScreen.at(s);
        if (here.size() == 1) {
            slots.append(Slot{here.first(), s, screens.at(s), true});
            continue;
        }
        const QList<QRect> cells = grid(screens.at(s), static_cast<int>(here.size()));
        for (int i = 0; i < here.size(); ++i)
            slots.append(Slot{here.at(i), s, cells.at(i), false});
    }
    // Back in role order, so slot k describes the k-th window placed.
    std::stable_sort(slots.begin(), slots.end(), [&order](const Slot& a, const Slot& b) {
        return order.indexOf(a.window) < order.indexOf(b.window);
    });
    return slots;
}

bool visibleOnAny(const QRect& frame, const QList<QRect>& screens)
{
    const QPoint centre = frame.center();
    for (const QRect& s : screens)
        if (s.contains(centre))
            return true;
    return false;
}

QRect rescue(const QRect& frame, const QRect& primary)
{
    QSize size = frame.size().boundedTo(primary.size());
    QRect r(QPoint(0, 0), size);
    r.moveCenter(primary.center());
    return r;
}

QString apply(const QList<RoleWindow*>& windows)
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (windows.isEmpty())
        return QStringLiteral("No role windows are open.");
    if (screens.isEmpty())
        return QStringLiteral("No screen is available.");

    QList<Role> roles;
    for (RoleWindow* w : windows)
        roles.append(w->role());
    QList<QRect> areas;
    for (QScreen* s : screens)
        areas.append(s->availableGeometry());

    for (const Slot& slot : plan(roles, areas)) {
        RoleWindow* w = windows.at(slot.window);
        QScreen* screen = screens.at(slot.screen);
        if (w->isFullScreen() || w->isMaximized())
            w->showNormal();
        if (QWindow* handle = w->windowHandle())
            handle->setScreen(screen);
        if (slot.maximise) {
            w->setCompactMinimum(QSize());   // back to the full minimum
            w->setGeometry(rescue(w->geometry(), slot.area));
            w->showMaximized();
        } else {
            w->setCompactMinimum(slot.area.size());
            // setGeometry() places the client area; leave room for the frame.
            const QRect frame = w->frameGeometry();
            const QRect client = w->geometry();
            const QMargins m(client.left() - frame.left(), client.top() - frame.top(),
                             frame.right() - client.right(), frame.bottom() - client.bottom());
            w->setGeometry(slot.area.marginsRemoved(m));
            w->show();
        }
        w->raise();
    }

    if (screens.size() == 1)
        return QStringLiteral("Tiled %1 window(s) in a 2 x 2 grid on the one screen.").arg(windows.size());
    return QStringLiteral("Placed %1 window(s) on %2 screens: Nurse, Doctor, Patient, Super Admin in that order.")
        .arg(windows.size()).arg(screens.size());
}

} // namespace ScreenArranger
