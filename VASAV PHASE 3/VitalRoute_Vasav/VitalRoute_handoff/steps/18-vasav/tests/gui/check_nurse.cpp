// check_nurse.cpp - Nurse window: registration, waiting lines, transfer desk.
#include "GuiCheck.hpp"

void nurseChecks(const QString& dataDir, const QString& shots)
{
    section("Nurse window");
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    auto* nurse2 = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    report(nurse && nurse2, "N1  Nurse button opens a NurseWindow");
    if (!nurse || !nurse2)
        return;
    QCoreApplication::processEvents();
    RegistrationPanel* reg = nurse->registration();

    // Heap order = the order the console's option 3 treats them in.
    const QStringList heapOrder = {QStringLiteral("VR-2026-000004"), QStringLiteral("VR-2026-000007"),
                                   QStringLiteral("VR-2026-000006"), QStringLiteral("VR-2026-000008"),
                                   QStringLiteral("VR-2026-000005"), QStringLiteral("VR-2026-000009")};
    report(keysOf(reg->emergencyModel()) == heapOrder &&
               keysOf(reg->opdModel()) == QStringList{QStringLiteral("VR-2026-000010"),
                                                      QStringLiteral("VR-2026-000011")},
           "N2  Waiting tables show heap order (ESI, then arrival) and OPD FIFO order",
           str(keysOf(reg->emergencyModel()).join(QLatin1Char(' '))));

    // Validation before anything reaches the session.
    const std::size_t before = hub.session().patients().size();
    fillPatient(nurse, "er", QString(), 40, QStringLiteral("Collapse"));
    reg->emergencyForm()->submit();
    const QString noName = labelText(nurse, "statusMessage");
    fillPatient(nurse, "er", QStringLiteral("Asha Rawat"), 40, QStringLiteral("Collapse"));
    reg->emergencyForm()->submit();
    const QString noEsi = labelText(nurse, "statusMessage");
    report(noName == QStringLiteral("Enter the patient's name.") &&
               noEsi == QStringLiteral("Choose an ESI level, 1 (most critical) to 5.") &&
               hub.session().patients().size() == before,
           "N3  Missing name or ESI level is reported; nothing is registered");

    // Register an ESI 1 with insurance: lands after the two earlier ESI 1s.
    child<QRadioButton>(nurse, "esi1")->setChecked(true);
    child<QCheckBox>(nurse->registration()->emergencyForm(), "insuranceCheck")->setChecked(true);
    child<QLineEdit>(nurse->registration()->emergencyForm(), "insuranceProvider")->setText(QStringLiteral("CGHS"));
    const bool registered = reg->emergencyForm()->submit();
    const QString msg = labelText(nurse, "statusMessage");
    const QStringList after = keysOf(reg->emergencyModel());
    const Bill* bill = hub.session().findBill("VR-2026-000012");
    report(registered && after.size() == 7 && after.at(2) == QStringLiteral("VR-2026-000012") &&
               msg.startsWith(QStringLiteral(
                   "Registered VR-2026-000012 (ESI 1). Position in the emergency queue: 3 of 7.")) &&
               bill && bill->insurance() &&
               reg->emergencyTable()->currentIndex().row() == 2,
           "N4  ESI 1 registration: console sentence, position 3 of 7, insurance on the bill, row selected",
           str(msg));

    // The second Nurse window updated without anyone touching it.
    report(keysOf(nurse2->registration()->emergencyModel()) == after &&
               labelText(nurse2, "statusEmergency") == QStringLiteral("Emergency waiting: 7"),
           "N5  A second window shows the new patient at once (queueChanged)");

    // Outpatient.
    fillPatient(nurse, "opd", QStringLiteral("Kiran Bisht"), 52, QStringLiteral("Chest discomfort on exertion"));
    auto* dept = child<QComboBox>(nurse, "opdDepartment");
    dept->setCurrentIndex(dept->findText(QStringLiteral("Cardiology")));
    reg->opdForm()->submit();
    report(keysOf(reg->opdModel()).size() == 3 &&
               reg->opdModel()->keyAt(2) == QStringLiteral("VR-2026-000013") &&
               labelText(nurse, "statusMessage") ==
                   QStringLiteral("Registered VR-2026-000013 for Cardiology. Position in the OPD line: 3."),
           "N6  OPD registration joins the back of the line with the console sentence",
           str(labelText(nurse, "statusMessage")));

    // Insurance ticked but no insurer.
    fillPatient(nurse, "opd", QStringLiteral("Test"), 30, QStringLiteral("Cough"));
    child<QCheckBox>(reg->opdForm(), "insuranceCheck")->setChecked(true);
    reg->opdForm()->submit();
    report(labelText(nurse, "statusMessage").startsWith(QStringLiteral("Enter the insurer")) &&
               keysOf(reg->opdModel()).size() == 3,
           "N7  Insurance ticked without an insurer is refused");

    // Transfer desk: routine to the back, urgent to the front.
    TransferDeskPanel* desk = nurse->transfers();
    child<QLineEdit>(desk, "transferFrom")->setText(QStringLiteral("General Ward A"));
    child<QLineEdit>(desk, "transferTo")->setText(QStringLiteral("Radiology"));
    child<QRadioButton>(desk, "transferInternal")->setChecked(true);
    desk->addRequest();
    child<QRadioButton>(desk, "transferUrgent")->setChecked(true);
    child<QRadioButton>(desk, "transferAmbulance")->setChecked(true);
    child<QLineEdit>(desk, "transferPatient")->setText(QStringLiteral("VR-2026-000005"));
    child<QLineEdit>(desk, "transferFrom")->setText(QStringLiteral("Roadside"));
    child<QLineEdit>(desk, "transferTo")->setText(QStringLiteral("Central Emergency Room"));
    desk->addRequest();
    const QString queued = labelText(nurse, "statusMessage");
    const TransferModel* tm = desk->model();
    report(tm->rowCount() == 2 && tm->textAt(0, TransferModel::Priority) == QStringLiteral("Urgent") &&
               tm->textAt(1, TransferModel::Priority) == QStringLiteral("Routine") &&
               queued.startsWith(QStringLiteral("Queued ")),
           "N8  Urgent request goes to the front, routine stays at the back", str(queued));

    desk->dispatchNext();
    const QString dispatched = labelText(nurse, "statusMessage");
    report(tm->rowCount() == 1 && dispatched.startsWith(QStringLiteral("Dispatched ")) &&
               dispatched.contains(QStringLiteral("Ambulance charge added to")),
           "N9  Dispatch takes the front request and charges the ambulance run", str(dispatched));

    desk->cancelNewestRoutine();
    desk->dispatchNext();
    report(tm->rowCount() == 0 && labelText(nurse, "statusMessage") == QStringLiteral("No pending requests."),
           "N10 Cancel newest routine empties the desk; dispatching then says so");

    child<QLineEdit>(desk, "transferPatient")->setText(QStringLiteral("VR-2026-999999"));
    child<QLineEdit>(desk, "transferFrom")->setText(QStringLiteral("A"));
    child<QLineEdit>(desk, "transferTo")->setText(QStringLiteral("B"));
    desk->addRequest();
    report(tm->rowCount() == 0 && labelText(nurse, "statusMessage") ==
                                      QStringLiteral("Not found: no patient with ID VR-2026-999999."),
           "N11 Unknown patient ID on a transfer: console message, nothing queued");

    // Screenshots: registration with a selection, and the transfer desk with two requests.
    child<QRadioButton>(desk, "transferRoutine")->setChecked(true);
    child<QLineEdit>(desk, "transferPatient")->clear();
    child<QLineEdit>(desk, "transferFrom")->setText(QStringLiteral("General Ward B"));
    child<QLineEdit>(desk, "transferTo")->setText(QStringLiteral("Physiotherapy"));
    desk->addRequest();
    child<QRadioButton>(desk, "transferUrgent")->setChecked(true);
    child<QLineEdit>(desk, "transferPatient")->setText(QStringLiteral("VR-2026-000008"));
    child<QLineEdit>(desk, "transferFrom")->setText(QStringLiteral("Acute Medical Unit"));
    child<QLineEdit>(desk, "transferTo")->setText(QStringLiteral("City Hospital"));
    desk->addRequest();
    selectKey(reg->emergencyTable(), QStringLiteral("VR-2026-000012"));
    nurse->showMessage(msg);
    nurse->tabs()->setCurrentIndex(0);
    shoot(nurse, shots, "nurse_registration.png");
    nurse->tabs()->setCurrentIndex(1);
    shoot(nurse, shots, "nurse_transfers.png");

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

