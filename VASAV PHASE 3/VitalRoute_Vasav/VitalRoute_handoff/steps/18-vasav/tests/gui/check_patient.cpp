// check_patient.cpp - Patient window: read-only view, routeAssigned, independent windows.
#include "GuiCheck.hpp"

void patientChecks(const QString& dataDir, const QString& shots)
{
    section("Patient window");
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* mine = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    auto* other = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    report(mine && other && mine->windowTitle() == QStringLiteral("VitalRoute - Patient") &&
               mine->record()->fieldText("recordTitle") == QStringLiteral("No patient selected"),
           "P1  Patient window opens empty and asks for the patient's ID");
    if (!mine || !other)
        return;

    bool readOnly = true;
    for (QAction* a : mine->findChildren<QAction*>())
        readOnly = readOnly && a->text() != QStringLiteral("&Save");
    report(readOnly && mine->findChildren<QPushButton*>().size() == 1,
           "P2  Read-only: no Save action, one button (Show my record)");

    mine->showPatient(QStringLiteral("VR-2026-999999"));
    report(labelText(mine, "statusMessage") == QStringLiteral("Not found: no patient with ID VR-2026-999999.") &&
               mine->windowTitle() == QStringLiteral("VitalRoute - Patient"),
           "P3  Unknown ID: console message, title unchanged");

    mine->showPatient(QStringLiteral("VR-2026-000004"));
    other->showPatient(QStringLiteral("VR-2026-000010"));
    QCoreApplication::processEvents();
    report(mine->windowTitle() == QStringLiteral("VitalRoute - Patient VR-2026-000004") &&
               mine->record()->fieldText("recordStatus") == QStringLiteral("Waiting") &&
               mine->record()->fieldText("recordBed") == QStringLiteral("None") &&
               mine->route()->stepCount() == 0 &&
               mine->route()->summary().startsWith(QStringLiteral("You do not have a bed yet")) &&
               mine->bill()->lines()->rowCount() == 0,
           "P4  Waiting patient: title names the ID, status Waiting, no bed, no route, no charges yet",
           str(mine->windowTitle()));

    // The doctor treats the patient (any window); this window reacts to routeAssigned.
    hub.treatNext();
    QCoreApplication::processEvents();
    const Bill* bill = hub.session().findBill("VR-2026-000004");
    report(mine->record()->fieldText("recordBed") == QStringLiteral("ICU-1, Bed 1") &&
               mine->record()->fieldText("recordStatus") == QStringLiteral("Admitted") &&
               mine->route()->stepCount() == 14 &&
               mine->route()->summary() == QStringLiteral("Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s).") &&
               labelText(mine, "statusMessage").startsWith(QStringLiteral("A bed has been assigned: ICU-1, Bed 1.")) &&
               bill && mine->bill()->lines()->rowCount() == static_cast<int>(bill->chargeCount()) &&
               mine->bill()->totalText() == QStringLiteral("Due %1 (estimate until discharge)").arg(Text::money(bill->projectedTotal())),
           "P5  routeAssigned: bed, 14-step route and the new charges appear at once",
           str(mine->bill()->totalText()));
    shoot(mine, shots, "patient_window.png");

    report(other->windowTitle() == QStringLiteral("VitalRoute - Patient VR-2026-000010") &&
               other->record()->history()->rowCount() == 2 && other->route()->stepCount() == 0 &&
               labelText(other, "statusMessage").isEmpty(),
           "P6  A second Patient window for another patient is independent");

    const auto titles = [&launcher] {
        QStringList t;
        for (RoleWindow* w : launcher.roleWindows())
            t << w->windowTitle();
        return t;
    }();
    report(titles.contains(QStringLiteral("VitalRoute - Patient VR-2026-000004")) &&
               titles.contains(QStringLiteral("VitalRoute - Patient VR-2026-000010")),
           "P7  The Launcher's window list names each Patient window by ID");

    hub.discharge("VR-2026-000004", vrclock::today(), std::nullopt);
    QCoreApplication::processEvents();
    report(mine->record()->fieldText("recordStatus") == QStringLiteral("Discharged") &&
               mine->route()->stepCount() == 0 &&
               mine->bill()->totalText().startsWith(QStringLiteral("Paid ")) &&
               mine->bill()->headerText().contains(QStringLiteral("settled on")),
           "P8  After discharge: status Discharged, route cleared, bill shows the amount paid",
           str(mine->bill()->totalText()));

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

