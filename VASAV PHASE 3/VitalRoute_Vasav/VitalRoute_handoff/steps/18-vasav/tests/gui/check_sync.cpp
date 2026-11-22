// check_sync.cpp - cross-window sync: one check per SessionHub signal.
//
// Each check makes a change in one window and reads the result in a
// DIFFERENT window, with no refresh call in between - the SessionHub signal
// is the only link between them.
#include "GuiCheck.hpp"

void syncChecks(const QString& dataDir, const QString& shots)
{
    Q_UNUSED(shots);
    section("Cross-window sync");
    setAutoConfirmForChecks(true);
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    auto* doctor = qobject_cast<DoctorWindow*>(launcher.openRole(Role::Doctor));
    auto* patient = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    auto* admin = qobject_cast<AdminWindow*>(launcher.openRole(Role::SuperAdmin));
    QCoreApplication::processEvents();
    patient->showPatient(QStringLiteral("VR-2026-000004"));
    admin->billing()->openBill(QStringLiteral("VR-2026-000004"));

    // queueChanged: Nurse registers -> Doctor's queue and the Super Admin status bar.
    fillPatient(nurse, "er", QStringLiteral("Sync Patient"), 50, QStringLiteral("Collapse"));
    child<QRadioButton>(nurse, "esi1")->setChecked(true);
    nurse->registration()->emergencyForm()->submit();
    const auto* doctorQueue = qobject_cast<const SnapshotTableModel*>(doctor->treatment()->emergencyTable()->model());
    report(doctorQueue->rowOfKey(QStringLiteral("VR-2026-000012")) == 2 &&
               labelText(admin, "statusEmergency") == QStringLiteral("Emergency waiting: 7"),
           "Y1  queueChanged: Nurse registers; Doctor's queue and Super Admin's status bar update");

    // routeAssigned + bedsChanged + staffChanged: Doctor treats.
    doctor->treatment()->treatNext();   // VR-2026-000004 to ICU-1, Bed 1
    report(patient->route()->stepCount() == 14 &&
               patient->record()->fieldText("recordBed") == QStringLiteral("ICU-1, Bed 1"),
           "Y2  routeAssigned(id): Doctor treats; the Patient window for that ID shows bed and route");
    report(labelText(nurse, "statusIcu") == labelText(admin, "statusIcu") &&
               labelText(patient, "statusIcu") == QStringLiteral("ICU beds free: %1")
                                                      .arg(hub.session().router().freeBeds(CareUnit::ICU)),
           "Y3  bedsChanged: free ICU beds drop in every window's status bar",
           str(labelText(nurse, "statusIcu")));
    auto* availability = admin->findChild<DoctorAvailabilityPanel*>();
    const DoctorAvailabilityModel* avail = availability ? availability->model() : nullptr;
    const int meera = avail ? avail->rowOfKey(QStringLiteral("DR-01")) : -1;
    report(avail && meera >= 0 && avail->textAt(meera, DoctorAvailabilityModel::Seen) == QStringLiteral("1"),
           "Y4  staffChanged: the attending doctor's count rises in the Super Admin availability table");

    // patientsChanged: Doctor adds a visit note -> Patient window history.
    const int visits = patient->record()->history()->rowCount();
    doctor->lookup()->lookUp(QStringLiteral("VR-2026-000004"));
    child<QComboBox>(doctor, "noteDoctor")->setCurrentText(QStringLiteral("Dr. Meera Iyer"));
    child<QLineEdit>(doctor, "noteDiagnosis")->setText(QStringLiteral("Post-arrest review"));
    doctor->lookup()->addVisitNote();
    report(patient->record()->history()->rowCount() == visits + 1,
           "Y5  patientsChanged: a visit note added by the Doctor appears in the Patient window");

    // billsChanged: Super Admin adds a charge -> Patient window's bill.
    const int lines = patient->bill()->lines()->rowCount();
    child<QComboBox>(admin, "chargeType")->setCurrentIndex(1);   // Medicine
    child<QLineEdit>(admin, "chargeDescription")->setText(QStringLiteral("Heparin"));
    child<QDoubleSpinBox>(admin, "chargeMedUnit")->setValue(250);
    admin->billing()->addCharge();
    report(patient->bill()->lines()->rowCount() == lines + 1,
           "Y6  billsChanged: a manual charge added by the Super Admin appears on the Patient's bill");

    // Undo refreshes every window (straight after the charge: once anything
    // else is added to that bill, undo correctly refuses, as in the console).
    admin->undoLast();   // the Heparin charge
    report(patient->bill()->lines()->rowCount() == lines,
           "Y7  Undo in the Super Admin window takes the charge off the Patient's bill",
           str(labelText(admin, "statusMessage")));

    // theatresChanged: Doctor books -> Super Admin's theatre table.
    child<QComboBox>(doctor, "theatreName")->setCurrentText(QStringLiteral("OT-2"));
    child<QLineEdit>(doctor, "theatrePatient")->setText(QStringLiteral("VR-2026-000004"));
    child<QLineEdit>(doctor, "theatreProcedure")->setText(QStringLiteral("Angioplasty"));
    doctor->theatres()->book();
    const TheatreModel* adminTheatres = admin->theatres()->model();
    report(adminTheatres->textAt(adminTheatres->rowOfKey(QStringLiteral("OT-2|0")), TheatreModel::Status) ==
               QStringLiteral("Booked"),
           "Y8  theatresChanged: a booking by the Doctor shows in the Super Admin theatre table");

    // transfersChanged: Nurse adds -> Super Admin's transfer table.
    TransferDeskPanel* desk = nurse->transfers();
    child<QLineEdit>(desk, "transferFrom")->setText(QStringLiteral("ICU-1"));
    child<QLineEdit>(desk, "transferTo")->setText(QStringLiteral("Radiology"));
    desk->addRequest();
    report(admin->transfers()->model()->rowCount() == 1,
           "Y9  transfersChanged: a request added at the Nurse desk shows at the Super Admin desk");

    // Save, then reload: every window shows the reloaded session.
    admin->findChild<QAction*>(QStringLiteral("saveAction"))->trigger();
    launcher.reload();
    QCoreApplication::processEvents();
    const auto* nurseQueue = nurse->registration()->emergencyModel();
    report(nurseQueue->rowCount() == static_cast<int>(hub.session().triage().emergencyCount()) &&
               patient->record()->fieldText("recordBed") == QStringLiteral("ICU-1, Bed 1") &&
               patient->route()->stepCount() == 14 &&
               admin->transfers()->model()->rowCount() == 0 &&   // transfers are not saved (documented)
               labelText(doctor, "statusNext") == labelText(nurse, "statusNext"),
           "Y10 Save then Reload: every open window shows the reloaded session");

    setAutoConfirmForChecks(false);
    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
