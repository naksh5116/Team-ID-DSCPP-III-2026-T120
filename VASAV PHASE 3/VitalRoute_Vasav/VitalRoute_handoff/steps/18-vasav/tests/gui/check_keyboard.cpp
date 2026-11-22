// check_keyboard.cpp - keyboard use: Enter submits forms, Tab order follows
// reading order, Ctrl+S saves, F5 refreshes, F11 toggles full screen.
//
// Keys are delivered with QCoreApplication::sendEvent(). For a key press
// that does not come from the hardware, QApplication runs the shortcut
// lookup first (Qt replays recorded input this way), so the forms' Enter
// shortcuts and the windows' Ctrl+S / F5 / F11 actions fire exactly as for
// a real key press. Shortcuts only reach the active window: when a window
// cannot be activated (some desktops refuse focus to background programs)
// its checks are reported as SKIP, not passed.
#include "GuiCheck.hpp"

#include <QEventLoop>

namespace {

bool activate(QWidget* window)
{
    window->show();
    window->raise();
    window->activateWindow();
    for (int i = 0; i < 50 && QApplication::activeWindow() != window; ++i)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return QApplication::activeWindow() == window;
}

void press(QWidget* target, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    target->setFocus(Qt::TabFocusReason);
    QKeyEvent down(QEvent::KeyPress, key, mods);
    QCoreApplication::sendEvent(target, &down);
    QKeyEvent up(QEvent::KeyRelease, key, mods);
    QCoreApplication::sendEvent(target, &up);
    QCoreApplication::processEvents();
}

// Names of the focusable, visible, enabled widgets in Tab order, starting
// at `start` and going once round the window's focus chain.
QStringList tabOrder(QWidget* start)
{
    QStringList names;
    QWidget* w = start;
    for (int i = 0; i < 2000; ++i) {
        if ((w->focusPolicy() & Qt::TabFocus) && w->isEnabled() && w->isVisibleTo(start->window()) &&
            !w->objectName().isEmpty())
            names << w->objectName();
        w = w->nextInFocusChain();
        if (w == start)
            break;
    }
    return names;
}

// True when `expected` appear in this relative order in `order`.
bool inOrder(const QStringList& order, const QStringList& expected, QString* detail)
{
    int last = -1;
    for (const QString& name : expected) {
        const int at = static_cast<int>(order.indexOf(name));
        if (at <= last) {
            *detail = QStringLiteral("%1 out of order (or missing)").arg(name);
            return false;
        }
        last = at;
    }
    return true;
}

} // namespace

void keyboardChecks(const QString& dataDir, const QString& shots)
{
    Q_UNUSED(shots);
    section("Keyboard");
    setAutoConfirmForChecks(true);   // a wrong Enter must never sit waiting on a dialog
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    auto* doctor = qobject_cast<DoctorWindow*>(launcher.openRole(Role::Doctor));
    auto* patient = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    auto* admin = qobject_cast<AdminWindow*>(launcher.openRole(Role::SuperAdmin));
    QCoreApplication::processEvents();

    // ---- Tab order: reading order in every form (no key presses needed)
    QString why;
    const bool registration = inOrder(tabOrder(child<QWidget>(nurse, "erName")),
                                      {QStringLiteral("erName"), QStringLiteral("erAge"), QStringLiteral("erPhone"),
                                       QStringLiteral("erComplaint"), QStringLiteral("esi1"),
                                       QStringLiteral("insuranceCheck"), QStringLiteral("erSubmit")},
                                      &why);
    nurse->tabs()->setCurrentIndex(1);
    const bool transfer = registration &&
                          inOrder(tabOrder(child<QWidget>(nurse, "transferUrgent")),
                                  {QStringLiteral("transferUrgent"), QStringLiteral("transferAmbulance"),
                                   QStringLiteral("transferPatient"), QStringLiteral("transferFrom"),
                                   QStringLiteral("transferTo"), QStringLiteral("transferAdd")},
                                  &why);
    nurse->tabs()->setCurrentIndex(0);
    doctor->tabs()->setCurrentIndex(1);
    doctor->lookup()->lookUp(QStringLiteral("VR-2026-000010"));
    const bool lookup = transfer &&
                        inOrder(tabOrder(child<QWidget>(doctor, "lookupId")),
                                {QStringLiteral("lookupId"), QStringLiteral("lookupButton"), QStringLiteral("noteDate"),
                                 QStringLiteral("noteDoctor"), QStringLiteral("noteDiagnosis"),
                                 QStringLiteral("notePrescriptions"), QStringLiteral("noteAdd")},
                                &why);
    admin->tabs()->setCurrentIndex(3);
    admin->billing()->openBill(QStringLiteral("VR-2026-000004"));
    const bool billing = lookup &&
                         inOrder(tabOrder(child<QWidget>(admin, "billingId")),
                                 {QStringLiteral("billingId"), QStringLiteral("billingOpen"),
                                  QStringLiteral("chargeAuthoriser"), QStringLiteral("chargeType"),
                                  QStringLiteral("chargeDescription"), QStringLiteral("chargeAdd"),
                                  QStringLiteral("insuranceProvider"), QStringLiteral("insuranceRecord"),
                                  QStringLiteral("dischargeDate"), QStringLiteral("dischargeButton")},
                                 &why);
    report(billing, "K1  Tab order follows reading order (registration, transfer desk, lookup, billing)", str(why));

    // ---- Enter submits (needs the window to be active for shortcuts)
    if (!activate(nurse)) {
        skip("K2  Enter in the emergency form registers the patient", "the window could not be made active");
        skip("K3  Keypad Enter in the outpatient form registers the patient", "the window could not be made active");
        skip("K4  Enter in the transfer form adds the request", "the window could not be made active");
    } else {
        fillPatient(nurse, "er", QStringLiteral("Keyboard Patient"), 44, QStringLiteral("Chest pain"));
        child<QRadioButton>(nurse, "esi2")->setChecked(true);
        press(child<QWidget>(nurse, "erComplaint"), Qt::Key_Return);
        report(labelText(nurse, "statusMessage")
                   .startsWith(QStringLiteral("Registered VR-2026-000012 (ESI 2).")),
               "K2  Enter in the emergency form registers the patient", str(labelText(nurse, "statusMessage")));

        child<QTabWidget>(nurse, "registrationTabs")->setCurrentIndex(1);
        fillPatient(nurse, "opd", QStringLiteral("Keypad Patient"), 30, QStringLiteral("Cough"));
        press(child<QWidget>(nurse, "opdName"), Qt::Key_Enter, Qt::KeypadModifier);
        report(labelText(nurse, "statusMessage").startsWith(QStringLiteral("Registered VR-2026-000013 for ")),
               "K3  Keypad Enter in the outpatient form registers the patient", str(labelText(nurse, "statusMessage")));

        nurse->tabs()->setCurrentIndex(1);
        child<QLineEdit>(nurse, "transferFrom")->setText(QStringLiteral("General Ward A"));
        child<QLineEdit>(nurse, "transferTo")->setText(QStringLiteral("Radiology"));
        press(child<QWidget>(nurse, "transferTo"), Qt::Key_Return);
        report(nurse->transfers()->model()->rowCount() == 1 &&
                   labelText(nurse, "statusMessage").startsWith(QStringLiteral("Queued TR-0001")),
               "K4  Enter in the transfer form adds the request", str(labelText(nurse, "statusMessage")));
    }

    if (!activate(patient)) {
        skip("K5  Enter in the Patient window's ID field shows the record", "the window could not be made active");
    } else {
        child<QLineEdit>(patient, "patientIdInput")->setText(QStringLiteral("VR-2026-000004"));
        press(child<QWidget>(patient, "patientIdInput"), Qt::Key_Return);
        report(patient->patientId() == QStringLiteral("VR-2026-000004") &&
                   patient->windowTitle() == QStringLiteral("VitalRoute - Patient VR-2026-000004"),
               "K5  Enter in the Patient window's ID field shows the record");
    }

    if (!activate(doctor)) {
        skip("K6  Enter in the lookup field looks the patient up", "the window could not be made active");
        skip("K7  Ctrl+S saves from any window", "the window could not be made active");
        skip("K8  F5 refreshes every table", "the window could not be made active");
    } else {
        child<QLineEdit>(doctor, "lookupId")->setText(QStringLiteral("VR-2026-000005"));
        press(child<QWidget>(doctor, "lookupId"), Qt::Key_Return);
        report(doctor->lookup()->record()->patientId() == QStringLiteral("VR-2026-000005"),
               "K6  Enter in the lookup field looks the patient up");

        press(child<QWidget>(doctor, "lookupId"), Qt::Key_S, Qt::ControlModifier);
        report(labelText(doctor, "statusMessage").startsWith(QStringLiteral("Saved ")) &&
                   QFile::exists(dataDir + QStringLiteral("/records/patients.dat")),
               "K7  Ctrl+S saves from any window", str(labelText(doctor, "statusMessage")));

        int refreshed = 0;
        const auto c = QObject::connect(&hub, &SessionHub::queueChanged, [&refreshed] { ++refreshed; });
        press(child<QWidget>(doctor, "lookupId"), Qt::Key_F5);
        QObject::disconnect(c);
        report(refreshed == 1, "K8  F5 refreshes every table (one round of change signals)");
    }

    if (!activate(admin)) {
        skip("K9  Enter in the discharge date does not discharge", "the window could not be made active");
    } else {
        hub.treatNext();   // VR-2026-000004 gets a bed, so a discharge would be possible
        admin->billing()->openBill(QStringLiteral("VR-2026-000004"));
        press(child<QWidget>(admin, "dischargeDate"), Qt::Key_Return);
        report(hub.session().findPatient("VR-2026-000004").status() == PatientStatus::Admitted,
               "K9  Enter in the discharge date does not discharge (destructive actions need their button)");
    }

    if (!activate(patient)) {
        skip("K10 F11 key turns the wall display on and off", "the window could not be made active");
    } else {
        press(patient->route()->list(), Qt::Key_F11);
        const bool on = patient->isFullScreen() && patient->isWallDisplay();
        press(patient->route()->list(), Qt::Key_F11);   // menu bar hidden: the shortcut must still work
        report(on && !patient->isFullScreen() && !patient->isWallDisplay(),
               "K10 F11 key turns the wall display on and off (works with the menu bar hidden)");
    }

    setAutoConfirmForChecks(false);
    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
