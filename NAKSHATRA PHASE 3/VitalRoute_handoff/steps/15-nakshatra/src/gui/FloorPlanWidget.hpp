// FloorPlanWidget.hpp - one floor of the hospital, drawn with QPainter.
//
// Everything comes from the running session: landmarks and corridors from
// HospitalMap (the vr_graph loaded from data/hospital_graph.txt - the GUI
// never reads the file), bed numbers and occupancy from ResourceRouter.
//
// Metres to pixels: one bounding box is taken over ALL floors (so a lift
// shaft is at the same place on every tab), then
//     scale = min(width_px / width_m, height_px / height_m)
//     px = left + (x - minX) * scale
//     py = top  + (maxY - y) * scale      (y grows north, pixels grow down)
// All drawing is in logical pixels with antialiasing; Qt maps them to each
// monitor's device pixels, so the plan stays sharp at any display scale.
#ifndef VR_GUI_FLOOR_PLAN_WIDGET_HPP
#define VR_GUI_FLOOR_PLAN_WIDGET_HPP

#include "Navigator.hpp"
#include "RouteSegments.hpp"

#include <QPointF>
#include <QRectF>
#include <QWidget>

class SessionHub;

class FloorPlanWidget : public QWidget {
    Q_OBJECT
public:
    FloorPlanWidget(SessionHub& hub, int floor, QWidget* parent = nullptr);

    int floor() const { return floor_; }
    void setRoute(const Route& route, const RouteSegments& segments);
    void clearRoute();
    void setSelectedStep(int step);        // -1 for none
    // Wall display: larger labels and lines (factor of the UI font).
    void setScale(double factor);

    // Metres to logical pixels for the current size (public for the checks).
    QPointF toPixels(double xMetres, double yMetres) const;
    double pixelsPerMetre() const;
    // Number of route edges drawn on this floor.
    int routeEdgesOnFloor() const;

    QSize sizeHint() const override { return QSize(480, 360); }
    QSize minimumSizeHint() const override { return QSize(240, 180); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Bounds {
        double minX = 0, maxX = 1, minY = 0, maxY = 1;
    };
    Bounds bounds() const;
    QRectF plotRect() const;

    SessionHub& hub_;
    int floor_;
    Route route_;
    RouteSegments segments_;
    bool hasRoute_ = false;
    int selected_ = -1;
    double scale_ = 1.0;
};

#endif // VR_GUI_FLOOR_PLAN_WIDGET_HPP
