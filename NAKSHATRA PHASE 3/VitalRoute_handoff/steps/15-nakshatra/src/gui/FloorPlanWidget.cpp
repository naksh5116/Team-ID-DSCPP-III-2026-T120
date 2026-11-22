// FloorPlanWidget.cpp
#include "FloorPlanWidget.hpp"

#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QSet>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

using Text::qs;

namespace {

constexpr double kMarginMetres = 4.0;

// Every node id present in the map (ids need not be contiguous).
std::vector<int> nodeIds(const HospitalMap& map)
{
    std::vector<int> ids;
    const int total = map.nodeCount();
    for (int id = 0; static_cast<int>(ids.size()) < total && id < 100000; ++id)
        if (map.hasNode(id))
            ids.push_back(id);
    return ids;
}

bool isVertical(EdgeType t)
{
    return t == EDGE_LIFT || t == EDGE_STAIRS;
}

} // namespace

FloorPlanWidget::FloorPlanWidget(SessionHub& hub, int floor, QWidget* parent)
    : QWidget(parent), hub_(hub), floor_(floor)
{
    setObjectName(QStringLiteral("floorPlan%1").arg(floor));
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(&hub_, &SessionHub::bedsChanged, this, qOverload<>(&QWidget::update));
    connect(&hub_, &SessionHub::reloaded, this, [this] {
        clearRoute();   // the old route's vertices belong to the old session
    });
}

void FloorPlanWidget::setRoute(const Route& route, const RouteSegments& segments)
{
    route_ = route;
    segments_ = segments;
    hasRoute_ = !route.path.empty();
    selected_ = -1;
    update();
}

void FloorPlanWidget::clearRoute()
{
    route_ = Route();
    segments_ = RouteSegments();
    hasRoute_ = false;
    selected_ = -1;
    update();
}

void FloorPlanWidget::setSelectedStep(int step)
{
    selected_ = step;
    update();
}

void FloorPlanWidget::setScale(double factor)
{
    scale_ = factor;
    update();
}

FloorPlanWidget::Bounds FloorPlanWidget::bounds() const
{
    Bounds b;
    if (!hub_.isStarted())
        return b;
    const HospitalMap& map = hub_.session().map();
    bool first = true;
    for (int id : nodeIds(map)) {   // all floors, so lift shafts line up across tabs
        const vr_node& n = map.node(id);
        if (first) {
            b.minX = b.maxX = n.x;
            b.minY = b.maxY = n.y;
            first = false;
        }
        b.minX = std::min(b.minX, n.x);
        b.maxX = std::max(b.maxX, n.x);
        b.minY = std::min(b.minY, n.y);
        b.maxY = std::max(b.maxY, n.y);
    }
    b.minX -= kMarginMetres;
    b.maxX += kMarginMetres;
    b.minY -= kMarginMetres;
    b.maxY += kMarginMetres;
    return b;
}

QRectF FloorPlanWidget::plotRect() const
{
    // Room for labels on the right and the scale bar underneath.
    const double pad = 16.0 * scale_;
    const double labelRoom = 110.0 * scale_;
    return QRectF(pad, pad, std::max(10.0, width() - 2 * pad - labelRoom),
                  std::max(10.0, height() - 2 * pad - 28.0 * scale_));
}

double FloorPlanWidget::pixelsPerMetre() const
{
    const Bounds b = bounds();
    const QRectF r = plotRect();
    return std::min(r.width() / (b.maxX - b.minX), r.height() / (b.maxY - b.minY));
}

QPointF FloorPlanWidget::toPixels(double x, double y) const
{
    const Bounds b = bounds();
    const QRectF r = plotRect();
    const double s = pixelsPerMetre();
    // Centre the plan in the plot area.
    const double offX = r.left() + (r.width() - (b.maxX - b.minX) * s) / 2.0;
    const double offY = r.top() + (r.height() - (b.maxY - b.minY) * s) / 2.0;
    return QPointF(offX + (x - b.minX) * s, offY + (b.maxY - y) * s);
}

int FloorPlanWidget::routeEdgesOnFloor() const
{
    if (!hasRoute_ || !hub_.isStarted())
        return 0;
    const HospitalMap& map = hub_.session().map();
    int n = 0;
    for (std::size_t i = 0; i + 1 < route_.path.size(); ++i)
        if (map.node(route_.path[i]).floor == floor_ && map.node(route_.path[i + 1]).floor == floor_)
            ++n;
    return n;
}

void FloorPlanWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Theme::color("surface"));
    if (!hub_.isStarted())
        return;

    const HospitalMap& map = hub_.session().map();
    const ResourceRouter& router = hub_.session().router();
    const std::vector<int> ids = nodeIds(map);
    const QColor text = Theme::color("text");
    const QColor muted = Theme::color("textMuted");
    const QColor corridor = Theme::color("corridor");
    const QColor accent = Theme::color("accent");
    const QFont labelFont = Theme::scaledFont(0.85 * scale_);
    const QFont strongFont = Theme::scaledFont(0.85 * scale_, true);
    const QFontMetricsF fm(labelFont);

    // ---- corridors and doors on this floor (thin lines)
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const vr_node& a = map.node(ids[i]);
        if (a.floor != floor_)
            continue;
        for (std::size_t j = i + 1; j < ids.size(); ++j) {
            const vr_node& b = map.node(ids[j]);
            if (b.floor != floor_)
                continue;
            const vr_edge* e = map.edge(a.id, b.id);
            if (!e)
                e = map.edge(b.id, a.id);
            if (!e || isVertical(e->type))
                continue;
            QPen pen(corridor, 1.0 * scale_);
            if (e->type == EDGE_DOOR)
                pen.setStyle(Qt::DashLine);
            p.setPen(pen);
            p.drawLine(toPixels(a.x, a.y), toPixels(b.x, b.y));
        }
    }

    // ---- the route on this floor
    QSet<int> selectedEdges;
    int selectedVertex = -1;
    if (hasRoute_ && selected_ >= 0 && selected_ < segments_.stepEdges.size()) {
        for (int k : segments_.stepEdges.at(selected_))
            selectedEdges.insert(k);
        selectedVertex = segments_.stepVertex.at(selected_);
    }
    if (hasRoute_) {
        for (std::size_t k = 0; k + 1 < route_.path.size(); ++k) {
            const vr_node& a = map.node(route_.path[k]);
            const vr_node& b = map.node(route_.path[k + 1]);
            if (a.floor != floor_ || b.floor != floor_)
                continue;
            const bool sel = selectedEdges.contains(static_cast<int>(k));
            QPen pen(accent, (sel ? 5.0 : 3.0) * scale_);
            pen.setCapStyle(Qt::RoundCap);
            p.setPen(pen);
            p.drawLine(toPixels(a.x, a.y), toPixels(b.x, b.y));
        }
    }

    // Beds per ward/ICU node: numbers and how many are free.
    std::map<int, std::vector<const Bed*>> bedsAt;
    for (const Bed& bed : router.beds())
        bedsAt[bed.node].push_back(&bed);

    // ---- landmarks: squares first, then labels placed so they do not overlap
    const double small = 5.0 * scale_;
    const double big = 9.0 * scale_;
    QSet<int> onRoute;
    if (hasRoute_)
        for (int v : route_.path)
            onRoute.insert(v);

    struct Label {
        int id;
        QPointF centre;
        double side;
        bool strong;
        int priority;      // lower is placed first
        QString first;
        QString second;
    };
    QList<Label> labels;
    QList<QRectF> taken;   // marker squares and placed labels
    for (int id : ids) {
        const vr_node& n = map.node(id);
        if (n.floor != floor_)
            continue;
        const QPointF c = toPixels(n.x, n.y);
        const bool major = n.type == NODE_WARD || n.type == NODE_ICU || n.type == NODE_OT ||
                           n.type == NODE_ER || n.type == NODE_RECEPTION;
        const bool vertical = n.type == NODE_LIFT || n.type == NODE_STAIRS;
        const double side = (major || vertical) ? big : small;
        p.setPen(QPen(major || vertical ? text : muted, 1.0 * scale_));
        p.setBrush(Theme::color("surface"));
        const QRectF square(c.x() - side / 2, c.y() - side / 2, side, side);
        p.drawRect(square);
        taken.append(square.adjusted(-2, -2, 2, 2));

        Label l{id, c, side, major || vertical, 0, QString(), QString()};
        // Lifts and stairs say so; wards and ICUs list their beds.
        if (n.type == NODE_LIFT)
            l.first = QString::fromUtf8(n.name).startsWith(QStringLiteral("Lift B")) ? QStringLiteral("Lift B")
                                                                                     : QStringLiteral("Lift A");
        else if (n.type == NODE_STAIRS)
            l.first = QStringLiteral("Stairs");
        else
            l.first = qs(n.name);
        if (auto it = bedsAt.find(id); it != bedsAt.end() && !it->second.empty()) {
            int lo = it->second.front()->number, hi = lo, free = 0;
            for (const Bed* b : it->second) {
                lo = std::min(lo, b->number);
                hi = std::max(hi, b->number);
                if (b->occupant.empty())
                    ++free;
            }
            l.second = QStringLiteral("Beds %1-%2, %3 free").arg(lo).arg(hi).arg(free);
        }
        const bool endPoint = hasRoute_ && (id == route_.path.front() || id == route_.path.back());
        l.priority = endPoint ? 0 : onRoute.contains(id) ? 1 : (major || vertical) ? 2 : 3;
        labels.append(l);
    }
    // The route itself is an obstacle too: sample its lines every few pixels,
    // and reserve the lift/stairs rings and the start and destination marks.
    if (hasRoute_) {
        const double step = 5.0 * scale_;
        const double pad = 3.0 * scale_;
        for (std::size_t k = 0; k + 1 < route_.path.size(); ++k) {
            const vr_node& a = map.node(route_.path[k]);
            const vr_node& b = map.node(route_.path[k + 1]);
            if (a.floor != floor_ && b.floor != floor_)
                continue;
            if (a.floor != b.floor) {
                const vr_node& here = a.floor == floor_ ? a : b;
                const QPointF c = toPixels(here.x, here.y);
                const double r = 10.0 * scale_;
                taken.append(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r));
                continue;
            }
            const QPointF pa = toPixels(a.x, a.y);
            const QPointF pb = toPixels(b.x, b.y);
            const double len = std::hypot(pb.x() - pa.x(), pb.y() - pa.y());
            const int n = std::max(1, static_cast<int>(len / step));
            for (int i = 0; i <= n; ++i) {
                const QPointF q = pa + (pb - pa) * (static_cast<double>(i) / n);
                taken.append(QRectF(q.x() - pad, q.y() - pad, 2 * pad, 2 * pad));
            }
        }
        for (int v : {route_.path.front(), route_.path.back()}) {
            const vr_node& n = map.node(v);
            if (n.floor != floor_)
                continue;
            const QPointF c = toPixels(n.x, n.y);
            const double r = 8.0 * scale_;
            taken.append(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r));
        }
    }
    std::stable_sort(labels.begin(), labels.end(),
                     [](const Label& a, const Label& b) { return a.priority < b.priority; });

    const double gap = 4.0 * scale_;
    for (const Label& l : labels) {
        const QFontMetricsF lfm(l.strong ? strongFont : labelFont);
        const double w = std::max(lfm.horizontalAdvance(l.first),
                                  l.second.isEmpty() ? 0.0 : fm.horizontalAdvance(l.second));
        const double h = lfm.height() + (l.second.isEmpty() ? 0.0 : fm.lineSpacing());
        const double half = l.side / 2;
        // Right, left, above, below the square, then above/below starting at
        // the square's left edge. Each is pushed back inside the widget.
        const QRectF area = QRectF(rect()).adjusted(2, 2, -2, -2);
        auto inside = [&area](QRectF r) {
            if (r.left() < area.left())
                r.moveLeft(area.left());
            if (r.right() > area.right())
                r.moveRight(area.right());
            return r;
        };
        const QRectF candidates[] = {
            inside(QRectF(l.centre.x() + half + gap, l.centre.y() - lfm.height() / 2, w, h)),
            inside(QRectF(l.centre.x() - half - gap - w, l.centre.y() - lfm.height() / 2, w, h)),
            inside(QRectF(l.centre.x() - w / 2, l.centre.y() - half - gap - h, w, h)),
            inside(QRectF(l.centre.x() - w / 2, l.centre.y() + half + gap, w, h)),
            inside(QRectF(l.centre.x() - half, l.centre.y() - half - gap - h, w, h)),
            inside(QRectF(l.centre.x() - half, l.centre.y() + half + gap, w, h)),
        };
        QRectF chosen;
        for (const QRectF& r : candidates) {
            bool clear = area.contains(r);
            for (const QRectF& t : taken)
                clear = clear && !r.intersects(t);
            if (clear) {
                chosen = r;
                break;
            }
        }
        if (chosen.isNull()) {
            // Junction and corridor names with no room are left out; the start,
            // the destination and major landmarks are always labelled.
            if (!l.strong && l.priority != 0)
                continue;
            chosen = candidates[0];
        }
        taken.append(chosen);
        p.setFont(l.strong ? strongFont : labelFont);
        p.setPen(l.strong ? text : muted);
        p.drawText(QPointF(chosen.left(), chosen.top() + lfm.ascent()), l.first);
        if (!l.second.isEmpty()) {
            p.setFont(labelFont);
            p.setPen(muted);
            p.drawText(QPointF(chosen.left(), chosen.top() + lfm.height() + fm.ascent()), l.second);
        }
    }

    // ---- route markers: lift/stairs used, start, destination, selected landmark
    if (hasRoute_) {
        for (std::size_t k = 0; k + 1 < route_.path.size(); ++k) {
            const vr_node& a = map.node(route_.path[k]);
            const vr_node& b = map.node(route_.path[k + 1]);
            if (a.floor == b.floor)
                continue;
            for (const vr_node* n : {&a, &b}) {
                if (n->floor != floor_)
                    continue;
                const QPointF c = toPixels(n->x, n->y);
                const bool sel = selectedEdges.contains(static_cast<int>(k));
                p.setPen(QPen(accent, (sel ? 3.0 : 2.0) * scale_));
                p.setBrush(Qt::NoBrush);
                const double r = 9.0 * scale_;
                p.drawRect(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r));
            }
        }
        const vr_node& start = map.node(route_.path.front());
        const vr_node& end = map.node(route_.path.back());
        const double m = 14.0 * scale_;
        if (start.floor == floor_) {
            const QPointF c = toPixels(start.x, start.y);
            p.setPen(QPen(accent, 2.5 * scale_));
            p.setBrush(Theme::color("surface"));
            p.drawRect(QRectF(c.x() - m / 2, c.y() - m / 2, m, m));   // hollow: start
        }
        if (end.floor == floor_) {
            const QPointF c = toPixels(end.x, end.y);
            p.setPen(Qt::NoPen);
            p.setBrush(accent);
            p.drawRect(QRectF(c.x() - m / 2, c.y() - m / 2, m, m));   // filled: destination
        }
        if (selectedVertex >= 0 && map.node(selectedVertex).floor == floor_) {
            const vr_node& n = map.node(selectedVertex);
            const QPointF c = toPixels(n.x, n.y);
            p.setPen(QPen(accent, 2.0 * scale_));
            p.setBrush(Qt::NoBrush);
            const double r = 12.0 * scale_;
            p.drawEllipse(c, r, r);
        }
    }

    // ---- scale bar (10 m) and north
    const double s = pixelsPerMetre();
    const QRectF r = plotRect();
    const QPointF bar(r.left(), height() - 14.0 * scale_);
    p.setPen(QPen(text, 1.0 * scale_));
    p.drawLine(bar, bar + QPointF(10.0 * s, 0));
    p.drawLine(bar + QPointF(0, -4 * scale_), bar + QPointF(0, 4 * scale_));
    p.drawLine(bar + QPointF(10.0 * s, -4 * scale_), bar + QPointF(10.0 * s, 4 * scale_));
    p.setFont(labelFont);
    p.drawText(bar + QPointF(10.0 * s + 6 * scale_, fm.ascent() / 2 - 1), QStringLiteral("10 m"));
    const QPointF north(width() - 28.0 * scale_, 30.0 * scale_);
    p.drawLine(north, north + QPointF(0, -14 * scale_));
    QPainterPath head;
    head.moveTo(north + QPointF(0, -18 * scale_));
    head.lineTo(north + QPointF(-4 * scale_, -11 * scale_));
    head.lineTo(north + QPointF(4 * scale_, -11 * scale_));
    head.closeSubpath();
    p.fillPath(head, text);
    p.drawText(QPointF(north.x() - fm.horizontalAdvance(QStringLiteral("N")) / 2, north.y() + fm.ascent() + 2),
               QStringLiteral("N"));
}
