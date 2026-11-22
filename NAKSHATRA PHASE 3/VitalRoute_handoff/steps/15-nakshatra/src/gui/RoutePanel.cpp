// RoutePanel.cpp
#include "RoutePanel.hpp"

#include "FloorPlanWidget.hpp"
#include "RouteStepsView.hpp"
#include "SessionHub.hpp"
#include "Theme.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QSet>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QString floorTitle(int floor)
{
    return floor == 0 ? QStringLiteral("Ground floor") : QStringLiteral("Floor %1").arg(floor);
}

// Floors in the plan, lowest first.
QList<int> floorsOf(const HospitalMap& map)
{
    QSet<int> seen;
    const int total = map.nodeCount();
    int found = 0;
    for (int id = 0; found < total && id < 100000; ++id)
        if (map.hasNode(id)) {
            ++found;
            seen.insert(map.node(id).floor);
        }
    QList<int> floors(seen.begin(), seen.end());
    std::sort(floors.begin(), floors.end());
    return floors;
}

} // namespace

RoutePanel::RoutePanel(SessionHub& hub, Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), hub_(hub)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    split_ = new QSplitter(orientation, this);
    split_->setChildrenCollapsible(false);
    layout->addWidget(split_);

    // Page 0: one tab per floor. Page 1 (wall display): route floors side by side.
    mapStack_ = new QStackedWidget(split_);
    floors_ = new QTabWidget(mapStack_);
    floors_->setObjectName(QStringLiteral("floorTabs"));
    auto* wall = new QWidget(mapStack_);
    wall->setObjectName(QStringLiteral("wallFloors"));
    auto* wallRow = new QBoxLayout(QBoxLayout::LeftToRight, wall);
    wallRow_ = wallRow;
    wallRow->setContentsMargins(0, 0, 0, 0);
    wallRow->setSpacing(12);
    for (int floor : floorsOf(hub_.session().map())) {
        auto* plan = new FloorPlanWidget(hub_, floor, floors_);
        plans_.append(plan);
        floors_->addTab(plan, floorTitle(floor));

        auto* box = new QWidget(wall);
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(4);
        auto* heading = new QLabel(floorTitle(floor), box);
        heading->setFont(Theme::scaledFont(1.4, true));
        v->addWidget(heading);
        auto* wallPlan = new FloorPlanWidget(hub_, floor, box);
        wallPlan->setObjectName(QStringLiteral("wallFloorPlan%1").arg(floor));
        v->addWidget(wallPlan, 1);
        wallRow->addWidget(box, 1);
        wallPlans_.append(wallPlan);
        wallBoxes_.append(box);
    }
    mapStack_->addWidget(floors_);
    mapStack_->addWidget(wall);

    steps_ = new RouteStepsView(split_);
    split_->addWidget(mapStack_);
    split_->addWidget(steps_);
    split_->setStretchFactor(0, 3);
    split_->setStretchFactor(1, 2);

    connect(steps_, &RouteStepsView::stepSelected, this, &RoutePanel::onStepSelected);
    connect(&hub_, &SessionHub::reloaded, this, [this] { clear(); });
    updateWallFloors();
}

FloorPlanWidget* RoutePanel::floorPlan(int floor) const
{
    for (FloorPlanWidget* p : plans_)
        if (p->floor() == floor)
            return p;
    return nullptr;
}

void RoutePanel::setRoute(const Route& route)
{
    route_ = route;
    segments_ = mapRouteSegments(hub_.session().navigator(), route);
    for (FloorPlanWidget* p : plans_)
        p->setRoute(route_, segments_);
    for (FloorPlanWidget* p : wallPlans_)
        p->setRoute(route_, segments_);
    steps_->setRoute(route_);
    retitleFloors();
    updateWallFloors();
    // Open on the floor where the route starts.
    if (!route_.path.empty())
        if (FloorPlanWidget* p = floorPlan(hub_.session().map().node(route_.path.front()).floor))
            floors_->setCurrentWidget(p);
}

void RoutePanel::clear(const QString& note)
{
    route_ = Route();
    segments_ = RouteSegments();
    for (FloorPlanWidget* p : plans_)
        p->clearRoute();
    for (FloorPlanWidget* p : wallPlans_)
        p->clearRoute();
    steps_->clear(note);
    retitleFloors();
    updateWallFloors();
}

void RoutePanel::retitleFloors()
{
    QSet<int> onRoute;
    if (!route_.path.empty() && hub_.isStarted())
        for (int v : route_.path)
            onRoute.insert(hub_.session().map().node(v).floor);
    for (int i = 0; i < plans_.size(); ++i) {
        const int floor = plans_.at(i)->floor();
        floors_->setTabText(i, onRoute.contains(floor) ? floorTitle(floor) + QStringLiteral(" (route)")
                                                       : floorTitle(floor));
    }
}

void RoutePanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateWallLayout();
}

void RoutePanel::updateWallLayout()
{
    if (!wallRow_ || !hub_.isStarted())
        return;
    int shown = 0;
    for (QWidget* box : wallBoxes_)
        shown += box->isVisibleTo(mapStack_->widget(1)) ? 1 : 0;
    if (shown < 2)
        return;
    // The plan's proportions in metres (all floors share one bounding box).
    const HospitalMap& map = hub_.session().map();
    double minX = 0, maxX = 1, minY = 0, maxY = 1;
    bool first = true;
    int found = 0;
    for (int id = 0; found < map.nodeCount() && id < 100000; ++id) {
        if (!map.hasNode(id))
            continue;
        ++found;
        const vr_node& n = map.node(id);
        minX = first ? n.x : std::min(minX, n.x);
        maxX = first ? n.x : std::max(maxX, n.x);
        minY = first ? n.y : std::min(minY, n.y);
        maxY = first ? n.y : std::max(maxY, n.y);
        first = false;
    }
    const double pw = maxX - minX + 1.0, ph = maxY - minY + 1.0;
    const double w = mapStack_->width(), h = mapStack_->height();
    const double sideBySide = std::min(w / shown / pw, h / ph);
    const double stacked = std::min(w / pw, h / shown / ph);
    wallRow_->setDirection(stacked > sideBySide ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
}

void RoutePanel::updateWallFloors()
{
    // Only the floors the route crosses; without a route, the ground floor.
    QSet<int> onRoute;
    if (!route_.path.empty() && hub_.isStarted())
        for (int v : route_.path)
            onRoute.insert(hub_.session().map().node(v).floor);
    for (int i = 0; i < wallPlans_.size(); ++i) {
        const int floor = wallPlans_.at(i)->floor();
        wallBoxes_.at(i)->setVisible(onRoute.isEmpty() ? i == 0 : onRoute.contains(floor));
    }
    updateWallLayout();
}

QList<FloorPlanWidget*> RoutePanel::visibleWallFloors() const
{
    QList<FloorPlanWidget*> out;
    for (int i = 0; i < wallPlans_.size(); ++i)
        if (wallBoxes_.at(i)->isVisibleTo(mapStack_->widget(1)))
            out.append(wallPlans_.at(i));
    return out;
}

void RoutePanel::selectStep(int step)
{
    if (step < 0 || step >= steps_->stepCount())
        return;
    steps_->list()->setCurrentIndex(steps_->list()->model()->index(step, 0));
}

void RoutePanel::onStepSelected(int step)
{
    for (FloorPlanWidget* p : plans_)
        p->setSelectedStep(step);
    for (FloorPlanWidget* p : wallPlans_)
        p->setSelectedStep(step);
    if (step < 0 || route_.path.empty())
        return;
    const int floor = segments_.floorOfStep(hub_.session().map(), route_, step);
    if (FloorPlanWidget* p = floorPlan(floor))
        floors_->setCurrentWidget(p);
}

void RoutePanel::setTextScale(double factor)
{
    steps_->setTextScale(factor);
    // The map grows less than the text, so its labels stay clear of each other.
    for (FloorPlanWidget* p : plans_)
        p->setScale(1.0 + (factor - 1.0) * 0.3);
    for (FloorPlanWidget* p : wallPlans_)
        p->setScale(1.0 + (factor - 1.0) * 0.3);
}

void RoutePanel::setOrientation(Qt::Orientation orientation)
{
    split_->setOrientation(orientation);
}

void RoutePanel::setWallMode(bool on)
{
    wall_ = on;
    mapStack_->setCurrentIndex(on ? 1 : 0);
    updateWallFloors();
}
