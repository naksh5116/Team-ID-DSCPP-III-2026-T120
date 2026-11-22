// check_screens.cpp - Screen placement rules and the F11 wall display.
#include "GuiCheck.hpp"

void screenChecks(const QString& dataDir, const QString& shots)
{
    section("Screens and wall display");
    using namespace ScreenArranger;
    const QRect fullHd(0, 0, 1920, 1040);

    const QList<QRect> cells = grid(fullHd, 4, true);
    report(cells.size() == 4 && cells.at(0) == QRect(0, 0, 960, 520) && cells.at(3) == QRect(960, 520, 960, 520),
           "S1  2 x 2 grid splits a 1920 x 1040 screen into four 960 x 520 tiles");

    // Four screens; windows opened in a muddled order still go Nurse, Doctor, Patient, Super Admin.
    const QList<QRect> four = {QRect(0, 0, 1920, 1040), QRect(1920, 0, 1920, 1040),
                               QRect(3840, 0, 1280, 984), QRect(5120, 0, 1920, 1040)};
    const QList<Slot> p4 = plan({Role::SuperAdmin, Role::Patient, Role::Nurse, Role::Doctor}, four);
    report(p4.size() == 4 && p4.at(0).window == 2 && p4.at(0).screen == 0 && p4.at(1).window == 3 &&
               p4.at(1).screen == 1 && p4.at(2).window == 1 && p4.at(2).screen == 2 && p4.at(3).window == 0 &&
               p4.at(3).screen == 3 && p4.at(2).maximise && p4.at(2).area == four.at(2),
           "S2  Four screens: Nurse 1, Doctor 2, Patient 3, Super Admin 4, each maximised on its screen");

    const QList<QRect> two = {QRect(0, 0, 1920, 1040), QRect(1920, 0, 1366, 728)};
    const QList<Slot> p2 = plan({Role::Nurse, Role::Doctor, Role::Patient, Role::SuperAdmin}, two);
    report(p2.size() == 4 && p2.at(0).screen == 0 && p2.at(1).screen == 1 && p2.at(2).screen == 0 &&
               p2.at(3).screen == 1 && !p2.at(0).maximise && p2.at(0).area.width() == 960 &&
               p2.at(2).area.left() == 960,
           "S3  Two screens, four windows: wraps (Nurse and Patient share screen 1, side by side)");

    const QList<Slot> p1 = plan({Role::SuperAdmin, Role::Doctor, Role::Patient, Role::Nurse}, {fullHd});
    const QList<Slot> one = plan({Role::Doctor}, {fullHd});
    report(p1.size() == 4 && p1.at(0).window == 3 && p1.at(0).area == QRect(0, 0, 960, 520) &&
               p1.at(1).window == 1 && p1.at(1).area.topLeft() == QPoint(960, 0) &&
               p1.at(2).window == 2 && p1.at(2).area.topLeft() == QPoint(0, 520) &&
               p1.at(3).window == 0 && p1.at(3).area.topLeft() == QPoint(960, 520) &&
               one.size() == 1 && one.at(0).maximise,
           "S4  One screen: 2 x 2 grid, Nurse top left ... Super Admin bottom right; a lone window is maximised");

    const QList<Slot> p3 = plan({Role::Nurse, Role::Doctor, Role::Patient, Role::Patient, Role::SuperAdmin},
                                {fullHd, fullHd.translated(1920, 0), fullHd.translated(3840, 0)});
    report(p3.size() == 5 && p3.at(3).screen == 0 && p3.at(4).screen == 1 && p3.at(2).maximise,
           "S5  Five windows on three screens wrap round (second Patient on screen 1, Super Admin on 2)");

    const QRect lost(2500, 100, 1200, 800);     // on a monitor that has gone
    const QRect huge(-50, -50, 3000, 2000);
    const QRect saved = rescue(lost, fullHd);
    report(!visibleOnAny(lost, {fullHd}) && visibleOnAny(QRect(100, 100, 800, 600), {fullHd}) &&
               fullHd.contains(saved) && saved.size() == lost.size() && fullHd.contains(rescue(huge, fullHd)),
           "S6  A window left on a removed screen is detected and moved onto the primary screen, resized to fit");

    // The real screens of this machine.
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    launcher.show();
    auto* patient = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    auto* admin = launcher.openRole(Role::SuperAdmin);
    auto* nurse = launcher.openRole(Role::Nurse);
    auto* doctor = launcher.openRole(Role::Doctor);
    QCoreApplication::processEvents();
    const QList<QScreen*> screens = QGuiApplication::screens();
    const QString arranged = launcher.arrangeOnScreens();
    QCoreApplication::processEvents();
    // Review shots at tile size (one screen: a quarter of it).
    nurse->showMessage(QStringLiteral("Tiled view"));
    shoot(nurse, shots, "tiled_nurse.png");
    shoot(admin, shots, "tiled_admin.png");
    QCoreApplication::processEvents();
    bool placed = true;
    const QList<RoleWindow*> byRole = {nurse, doctor, patient, admin};
    for (int i = 0; i < byRole.size(); ++i) {
        QScreen* expected = screens.at(i % screens.size());
        placed = placed && byRole.at(i)->screen() == expected &&
                 expected->availableGeometry().adjusted(-12, -12, 12, 12).contains(byRole.at(i)->frameGeometry());
        if (screens.size() == 1)
            placed = placed && byRole.at(i)->minimumWidth() < 1100;
    }
    report(placed && (screens.size() == 1 ? arranged.startsWith(QStringLiteral("Tiled 4 window(s)"))
                                          : arranged.startsWith(QStringLiteral("Placed 4 window(s)"))),
           "S7  Arrange on screens on this machine's screens", str(arranged));

    // F11 on a Patient window: the wall display.
    patient->showPatient(QStringLiteral("VR-2026-000004"));
    hub.treatNext();
    QCoreApplication::processEvents();
    QAction* full = nullptr;
    for (QAction* a : patient->actions())
        if (a->shortcut() == QKeySequence(Qt::Key_F11))
            full = a;
    if (full)
        full->trigger();
    QCoreApplication::processEvents();
    const double ratio = patient->route()->list()->font().pointSizeF() / QApplication::font().pointSizeF();
    report(full && patient->isFullScreen() && patient->isWallDisplay() && !patient->menuBar()->isVisible() &&
               !patient->statusBar()->isVisible() && !patient->record()->isVisible() &&
               !patient->bill()->isVisible() && patient->route()->isVisible() &&
               patient->wallHeaderText() == QStringLiteral("Patient VR-2026-000004: walking directions to ICU-1, Bed 1") &&
               std::abs(ratio - 1.8) < 0.05 && patient->routePanel()->isWallMode() &&
               patient->routePanel()->visibleWallFloors().size() == 2,
           "S8  F11 on a Patient window: wall display with both route floors side by side, 1.8x directions, ID but no name",
           str(QStringLiteral("text %1x").arg(ratio, 0, 'f', 2)));
    shoot(patient, shots, "patient_wall.png");
    if (full)
        full->trigger();   // F11 still works with the menu bar hidden
    QCoreApplication::processEvents();
    report(!patient->isFullScreen() && !patient->isWallDisplay() && patient->menuBar()->isVisible() &&
               patient->record()->isVisible(),
           "S9  F11 again leaves the wall display and restores the normal Patient window");

    QAction* doctorFull = nullptr;
    for (QAction* a : doctor->actions())
        if (a->shortcut() == QKeySequence(Qt::Key_F11))
            doctorFull = a;
    if (doctorFull)
        doctorFull->trigger();
    QCoreApplication::processEvents();
    const bool doctorFullScreen = doctor->isFullScreen() && doctor->menuBar()->isVisible();
    if (doctorFull)
        doctorFull->trigger();
    QCoreApplication::processEvents();
    report(doctorFullScreen && !doctor->isFullScreen(),
           "S10 F11 on other roles: plain full screen, menus kept, F11 returns");

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    launcher.hide();
}

