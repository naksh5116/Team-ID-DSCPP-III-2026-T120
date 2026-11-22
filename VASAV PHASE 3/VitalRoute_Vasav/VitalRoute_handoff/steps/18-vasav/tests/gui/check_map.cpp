// check_map.cpp - Route map: floors, step-to-segment mapping, metres to pixels.
#include "GuiCheck.hpp"

void mapChecks(const QString& dataDir, const QString& shots)
{
    section("Route map");
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* doctor = qobject_cast<DoctorWindow*>(launcher.openRole(Role::Doctor));
    auto* patient = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    if (!doctor || !patient)
        return;
    patient->showPatient(QStringLiteral("VR-2026-000004"));
    QCoreApplication::processEvents();
    doctor->treatment()->treatNext();   // VR-2026-000004 to ICU-1, Bed 1 (lift A to floor 1)
    QCoreApplication::processEvents();

    RoutePanel* panel = doctor->treatment()->routePanel();
    QTabWidget* floors = panel->floors();
    report(floors->count() == 3 && floors->tabText(0) == QStringLiteral("Ground floor (route)") &&
               floors->tabText(1) == QStringLiteral("Floor 1 (route)") &&
               floors->tabText(2) == QStringLiteral("Floor 2") && floors->currentIndex() == 0,
           "M1  One tab per floor; the floors the route crosses are marked; opens on the start floor",
           str(floors->tabText(0) + QStringLiteral(", ") + floors->tabText(1) + QStringLiteral(", ") + floors->tabText(2)));

    report(panel->floorPlan(0)->routeEdgesOnFloor() == 5 && panel->floorPlan(1)->routeEdgesOnFloor() == 3 &&
               panel->floorPlan(2)->routeEdgesOnFloor() == 0,
           "M2  Dijkstra path drawn per floor: 5 corridor edges on the ground floor, 3 on floor 1");

    // Step-to-segment mapping, against the README route.
    const RouteSegments& seg = panel->segments();
    report(seg.stepEdges.size() == 14 && seg.stepEdges.at(1) == QList<int>({0, 1, 2}) &&
               seg.stepVertex.at(0) == 0 && seg.stepVertex.at(2) == 4 &&     // turn left at Main Corridor Junction
               seg.stepEdges.at(6) == QList<int>({5}) &&                       // take the lift: edge 6 -> 13
               seg.stepVertex.at(7) == 13 &&                                   // exit the lift at Lift A, floor 1
               seg.stepVertex.at(13) == 18,                                    // arrive at ICU-1
           "M3  Each step maps to its edges (walks, lift) or its landmark (turns, exit, arrive)");

    panel->selectStep(7);   // "Exit the lift heading south towards Nursing Station 1A"
    const int afterExit = floors->currentIndex();
    panel->selectStep(1);   // "Walk 34 m along the Reception Corridor"
    report(afterExit == 1 && floors->currentIndex() == 0,
           "M4  Clicking a step switches to its floor (exit the lift: floor 1; first walk: ground floor)");

    // Metres to pixels.
    FloorPlanWidget* ground = panel->floorPlan(0);
    FloorPlanWidget* first = panel->floorPlan(1);
    const double ppm = ground->pixelsPerMetre();
    const QPointF er = ground->toPixels(0, 0);        // Central Emergency Room
    const QPointF triage = ground->toPixels(10, 0);   // ER Triage Desk, 10 m east
    const QPointF reception = ground->toPixels(22, -10);
    const QPointF corridor = ground->toPixels(22, 0);
    const bool sameSize = ground->size() == first->size() ||
                          (first->resize(ground->size()), true);
    report(ppm > 0 && std::abs((triage.x() - er.x()) - 10 * ppm) < 0.01 && std::abs(triage.y() - er.y()) < 0.01 &&
               reception.y() > corridor.y() &&   // y grows north, pixels grow down
               sameSize && ground->toPixels(30, 20) == first->toPixels(30, 20),   // Lift A lines up across floors
           "M5  Metres to pixels: 10 m = 10 x scale, south is lower on screen, Lift A at the same place on every floor",
           str(QStringLiteral("%1 px per metre").arg(ppm, 0, 'f', 2)));

    const QSize before = ground->size();
    ground->resize(before.width() / 2, before.height() / 2);
    const double ppmSmall = ground->pixelsPerMetre();
    bool inside = true;
    for (const QPointF& corner : {ground->toPixels(0, -14), ground->toPixels(66, 22)})
        inside = inside && ground->rect().contains(corner.toPoint());
    ground->resize(before);
    report(ppmSmall < ppm && inside, "M6  Resizing rescales the plan and keeps it inside the widget");

    RoutePanel* patientMap = patient->routePanel();
    report(patientMap->floorPlan(1)->routeEdgesOnFloor() == 3 && patient->route()->stepCount() == 14,
           "M7  The Patient window draws the same route on its own map");

    patientMap->selectStep(10);   // "Turn right at Nursing Station 1A", on floor 1
    hub.registerEmergency("Another Patient", 30, "", "Fall", 3, std::nullopt);   // someone else changes
    QCoreApplication::processEvents();
    report(patientMap->floors()->currentIndex() == 1 &&
               patient->route()->list()->currentIndex().row() == 10,
           "M8  The Patient map keeps its floor and selected step when other patients change");

    selectKey(doctor->treatment()->emergencyTable(), QStringLiteral("VR-2026-000007"));
    panel->selectStep(4);   // "Bear slightly left at Pharmacy Corner"
    shoot(doctor, shots, "doctor_treatment.png");
    panel->selectStep(10);
    shoot(panel, shots, "route_map_floor1.png");
    shoot(patient, shots, "patient_window.png");

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

