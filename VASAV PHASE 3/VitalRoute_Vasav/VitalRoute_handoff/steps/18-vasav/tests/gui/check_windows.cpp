// check_windows.cpp - Launcher, shared window frame, stylesheet; control sheet and ESI badge screenshots.
#include "GuiCheck.hpp"

void windowChecks(const QString& dataDir, const QString& shots)
{
    section("Launcher and window frame");
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    launcher.show();

    const QString qss = Theme::styleSheet();
    report(!qss.isEmpty() && !qss.contains(QRegularExpression(QStringLiteral("@[A-Za-z]"))),
           "W1  Stylesheet loads and every @token is replaced");

    RoleWindow* doctor = launcher.openRole(Role::Doctor);
    RoleWindow* nurse = launcher.openRole(Role::Nurse);
    RoleWindow* patientA = launcher.openRole(Role::Patient);
    RoleWindow* patientB = launcher.openRole(Role::Patient);
    RoleWindow* admin = launcher.openRole(Role::SuperAdmin);
    QCoreApplication::processEvents();
    report(launcher.roleWindows().size() == 5 && patientA != patientB && launcher.isVisible() &&
               doctor->windowTitle() == QStringLiteral("VitalRoute - Doctor") &&
               admin->windowTitle() == QStringLiteral("VitalRoute - Super Admin"),
           "W2  Each button opens a new window; Patient twice gives two; Launcher stays open");

    bool sizes = true;
    for (RoleWindow* w : launcher.roleWindows())
        sizes = sizes && w->minimumSize() == QSize(1100, 700) &&
                labelText(w, "statusRole") == QStringLiteral("Role: ") + roleName(w->role());
    report(sizes, "W3  Every role window: minimum 1100x700, status bar names the role");

    const int icu = hub.session().router().freeBeds(CareUnit::ICU);
    report(labelText(nurse, "statusEmergency") == QStringLiteral("Emergency waiting: 6") &&
               labelText(nurse, "statusNext") == QStringLiteral("Next: VR-2026-000004") &&
               labelText(admin, "statusIcu") == QStringLiteral("ICU beds free: %1").arg(icu),
           "W4  Status bar shows waiting counts, next patient and free beds",
           str(labelText(nurse, "statusEmergency") + QStringLiteral(" | ") + labelText(nurse, "statusNext")));

    hub.treatNext();   // an action from any window
    report(labelText(nurse, "statusEmergency") == QStringLiteral("Emergency waiting: 5") &&
               labelText(doctor, "statusNext") == QStringLiteral("Next: VR-2026-000007") &&
               labelText(admin, "statusIcu") == QStringLiteral("ICU beds free: %1").arg(icu - 1) &&
               labelText(patientB, "statusIcu") == QStringLiteral("ICU beds free: %1").arg(icu - 1),
           "W5  One treatment updates the status bar of every open window at once",
           str(labelText(admin, "statusIcu")));

    shoot(&launcher, shots, "launcher.png");
    shoot(nurse, shots, "role_window_frame.png");
    // Review shots at the minimum window size, 1100 x 700, with real screens.
    if (!shots.isEmpty()) {
        RoleWindow* minNurse = launcher.openRole(Role::Nurse);
        RoleWindow* minDoctor = launcher.openRole(Role::Doctor);
        RoleWindow* minAdmin = launcher.openRole(Role::SuperAdmin);
        for (RoleWindow* w : {minNurse, minDoctor, minAdmin})
            w->resize(1100, 700);
        hub.treatNext();
        QCoreApplication::processEvents();
        shoot(minNurse, shots, "min_nurse.png");
        shoot(minDoctor, shots, "min_doctor.png");
        qobject_cast<AdminWindow*>(minAdmin)->tabs()->setCurrentIndex(3);
        shoot(minAdmin, shots, "min_admin_billing.png");
        for (RoleWindow* w : {minNurse, minDoctor, minAdmin})
            w->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // Switch role: same place, new role, old window gone.
    const QRect geo = nurse->geometry();
    QPointer<RoleWindow> oldNurse = nurse;
    RoleWindow* switched = launcher.switchRole(nurse, Role::Doctor);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    report(!oldNurse && switched->role() == Role::Doctor && switched->geometry() == geo &&
               launcher.roleWindows().size() == 5,
           "W6  Switch role replaces the window in place");

    // Closing a role window leaves the application and the other windows running.
    bool quitRequested = false;
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&quitRequested] { quitRequested = true; });
    patientA->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    report(!quitRequested && launcher.roleWindows().size() == 4 && launcher.isVisible(),
           "W7  Closing a role window does not quit the application");

    // Errors appear in the status bar with the console's wording.
    const bool ok = admin->attempt([&hub] { hub.session().findPatient("VR-2026-999999"); });
    report(!ok && labelText(admin, "statusMessage") ==
                      QStringLiteral("Not found: no patient with ID VR-2026-999999."),
           "W8  An exception is shown in the status bar as the console words it",
           str(labelText(admin, "statusMessage")));

    // Control sheet for the visual review: every input state in one place.
    if (!shots.isEmpty()) {
        QWidget sheet;
        auto* form = new QFormLayout(&sheet);
        form->setContentsMargins(12, 12, 12, 12);
        auto* unchecked = new QCheckBox(QStringLiteral("Unchecked"));
        auto* checked = new QCheckBox(QStringLiteral("Checked"));
        checked->setChecked(true);
        auto* disabledCheck = new QCheckBox(QStringLiteral("Disabled, checked"));
        disabledCheck->setChecked(true);
        disabledCheck->setEnabled(false);
        auto* checks = new QHBoxLayout;
        checks->addWidget(unchecked);
        checks->addWidget(checked);
        checks->addWidget(disabledCheck);
        form->addRow(QStringLiteral("Check boxes"), checks);
        auto* r1 = new QRadioButton(QStringLiteral("Selected"));
        r1->setChecked(true);
        auto* r2 = new QRadioButton(QStringLiteral("Not selected"));
        auto* radios = new QHBoxLayout;
        radios->addWidget(r1);
        radios->addWidget(r2);
        form->addRow(QStringLiteral("Radio buttons"), radios);
        auto* spin = new QSpinBox;
        spin->setValue(42);
        form->addRow(QStringLiteral("Spin box"), spin);
        auto* money = new QDoubleSpinBox;
        money->setPrefix(QStringLiteral("Rs "));
        money->setRange(0, 100000);
        money->setValue(1500);
        form->addRow(QStringLiteral("Amount"), money);
        auto* combo = new QComboBox;
        combo->addItems({QStringLiteral("General Medicine"), QStringLiteral("Cardiology")});
        form->addRow(QStringLiteral("Combo box"), combo);
        auto* line = new QLineEdit(QStringLiteral("Text field"));
        form->addRow(QStringLiteral("Line edit"), line);
        auto* off = new QLineEdit(QStringLiteral("Disabled field"));
        off->setEnabled(false);
        form->addRow(QStringLiteral("Disabled"), off);
        auto* primary = new QPushButton(QStringLiteral("Primary action"));
        primary->setProperty("primary", true);
        auto* secondary = new QPushButton(QStringLiteral("Secondary"));
        auto* disabledButton = new QPushButton(QStringLiteral("Disabled"));
        disabledButton->setEnabled(false);
        auto* buttons = new QHBoxLayout;
        buttons->addWidget(primary);
        buttons->addWidget(secondary);
        buttons->addWidget(disabledButton);
        buttons->addStretch(1);
        form->addRow(QStringLiteral("Buttons"), buttons);
        sheet.resize(520, 360);
        sheet.show();
        shoot(&sheet, shots, "controls.png");
    }

    // ESI badges for the visual review.
    if (!shots.isEmpty()) {
        QStandardItemModel model(5, 1);
        model.setHorizontalHeaderLabels({QStringLiteral("ESI")});
        for (int level = 1; level <= 5; ++level) {
            auto* item = new QStandardItem(Theme::esiName(level));
            item->setData(level, EsiDelegate::LevelRole);
            model.setItem(level - 1, 0, item);
        }
        QTableView table;
        table.setModel(&model);
        table.setItemDelegateForColumn(0, new EsiDelegate(&table));
        Theme::setupTable(&table);
        table.resizeColumnsToContents();
        table.resize(260, 190);
        table.selectRow(1);
        table.show();
        shoot(&table, shots, "esi_badges.png");
    }

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    launcher.hide();
}

