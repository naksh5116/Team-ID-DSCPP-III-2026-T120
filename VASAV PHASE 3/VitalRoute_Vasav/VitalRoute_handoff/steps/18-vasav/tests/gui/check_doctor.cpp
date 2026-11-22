// check_doctor.cpp - Doctor window: treatment and route, patient record, theatres, availability.
#include "GuiCheck.hpp"

void doctorChecks(const QString& dataDir, const QString& shots)
{
    section("Doctor window");
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* doctor = qobject_cast<DoctorWindow*>(launcher.openRole(Role::Doctor));
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    report(doctor != nullptr, "D1  Doctor button opens a DoctorWindow");
    if (!doctor || !nurse)
        return;
    QCoreApplication::processEvents();
    TreatPanel* treat = doctor->treatment();
    report(child<QLabel>(doctor, "nextId")->text() == QStringLiteral("VR-2026-000004") &&
               child<QLabel>(doctor, "nextNeeds")->text().startsWith(QStringLiteral("Needs an ICU bed")),
           "D2  Next patient box shows the top of the heap and the bed type it needs");

    // First treatment: the README's sample output for option 3.
    treat->treatNext();
    const QStringList lines = treat->resultText().split(QLatin1Char('\n'));
    const RouteStepsView* route = treat->route();
    report(lines.size() == 3 &&
               lines.at(0) == QStringLiteral("Patient: VR-2026-000004  Suresh Kumar, ESI 1 - Cardiac "
                                             "arrest, pulse restored in ambulance") &&
               lines.at(1).startsWith(QStringLiteral("Attending: Dr. Meera Iyer from ")) &&
               lines.at(2) == QStringLiteral("Bed: ICU-1, Bed 1 (ICU)"),
           "D3  Treat next patient prints the console's lines (patient, attending, bed)",
           str(treat->resultText().replace(QLatin1Char('\n'), QStringLiteral(" | "))));
    report(route->stepCount() == 14 &&
               route->title() == QStringLiteral("Route: Central Emergency Room to ICU-1, Bed 1") &&
               route->stepText(0) == QStringLiteral("1. From the Central Emergency Room, head towards ER Triage Desk") &&
               route->stepText(6) == QStringLiteral("7. Take the lift to floor 1") &&
               route->stepText(13) == QStringLiteral("14. Arrive at ICU-1, Bed 1 - on your left") &&
               route->summary() == QStringLiteral("Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s)."),
           "D4  Directions match the console route: 14 steps, lift to floor 1, 89 m, 74 s",
           str(route->summary()));
    selectKey(treat->emergencyTable(), QStringLiteral("VR-2026-000007"));
    shoot(doctor, shots, "doctor_treatment.png");

    report(child<QLabel>(doctor, "nextId")->text() == QStringLiteral("VR-2026-000007") &&
               nurse->registration()->emergencyModel()->rowCount() == 5,
           "D5  Next patient moves on; the Nurse window's queue drops the patient at once");

    // 000007 ICU, 000006 ICU, 000008 ward, then 000005 (ESI 4) released from the ER.
    for (int i = 0; i < 4; ++i)
        treat->treatNext();
    report(treat->resultText().endsWith(QStringLiteral("Treated in the ER and released - ESI 4 does not need a bed.")) &&
               route->stepCount() == 0,
           "D6  ESI 4 is treated in the ER and released; no route is shown",
           str(treat->resultText().section(QLatin1Char('\n'), -1)));

    treat->treatNext();   // 000009, ESI 5
    QCoreApplication::processEvents();
    auto* opdBox = child<QLineEdit>(doctor, "opdDiagnosis")->parentWidget();
    child<QCheckBox>(doctor, "opdDispense")->setChecked(true);
    const bool refused = !treat->treatNext() &&
                         labelText(doctor, "statusMessage").startsWith(QStringLiteral("Enter the medicine"));
    child<QLineEdit>(doctor, "opdMedicine")->setText(QStringLiteral("Amlodipine 5 mg"));
    child<QSpinBox>(doctor, "opdQuantity")->setValue(30);
    child<QDoubleSpinBox>(doctor, "opdUnitPrice")->setValue(4.5);
    treat->treatNext();   // 000010, outpatient, blank diagnosis -> "Routine review"
    const Bill* opdBill = hub.session().findBill("VR-2026-000010");
    const Patient& opdPatient = hub.session().findPatient("VR-2026-000010");
    report(opdBox->isVisibleTo(doctor) && refused && opdBill &&
               treat->resultText().endsWith(QStringLiteral("Consultation recorded. Bill so far: ") +
                                            Text::money(opdBill->projectedTotal())) &&
               opdPatient.visitsNewestFirst().front().diagnosis == std::string("Routine review"),
           "D7  Outpatient: notes form shown, medicine checked, blank diagnosis becomes Routine review",
           str(treat->resultText().section(QLatin1Char('\n'), -1)));

    // Patient record.
    PatientLookupPanel* lookup = doctor->lookup();
    lookup->lookUp(QStringLiteral("VR-2026-000010"));
    const VisitHistoryModel* history = lookup->record()->history();
    bool newestFirst = history->rowCount() == static_cast<int>(opdPatient.visitCount());
    for (int r = 1; r < history->rowCount(); ++r)
        newestFirst = newestFirst && history->textAt(r - 1, 0) >= history->textAt(r, 0);
    report(newestFirst && lookup->record()->fieldText("recordBill").startsWith(QStringLiteral("BL-")) &&
               lookup->record()->fieldText("recordStatus") == QStringLiteral("Treated") &&
               labelText(doctor, "hashStats").startsWith(QStringLiteral("Patient ID index (hash table): 11 entries")),
           "D8  Lookup shows status, bill and visit history newest first, with hash index figures",
           str(labelText(doctor, "hashStats")));

    const int older = vrclock::addDays(vrclock::today(), -60);
    child<QDateEdit>(doctor, "noteDate")->setDate(Text::qdate(older));
    child<QComboBox>(doctor, "noteDoctor")->setCurrentText(QStringLiteral("Dr. Farah Siddiqui"));
    child<QLineEdit>(doctor, "noteDiagnosis")->setText(QStringLiteral("BP review"));
    const int rowsBefore = history->rowCount();
    lookup->addVisitNote();
    int placed = -1;
    for (int r = 0; r < history->rowCount(); ++r)
        if (history->textAt(r, 2) == QStringLiteral("BP review"))
            placed = r;
    report(history->rowCount() == rowsBefore + 1 && placed > 0 &&
               history->textAt(placed, 0) == Text::date(older) &&
               labelText(doctor, "statusMessage") == QStringLiteral("Visit added in date order to VR-2026-000010."),
           "D9  A visit note dated 60 days ago lands in date order, not at the top",
           str(QStringLiteral("row %1 of %2").arg(placed).arg(history->rowCount())));
    doctor->tabs()->setCurrentIndex(1);
    shoot(doctor, shots, "doctor_record.png");

    lookup->lookUp(QStringLiteral("VR-2026-999999"));
    report(labelText(doctor, "statusMessage") == QStringLiteral("Not found: no patient with ID VR-2026-999999."),
           "D10 Unknown ID: the console's Not found message");

    // Theatres.
    TheatrePanel* ot = doctor->theatres();
    child<QComboBox>(doctor, "theatreName")->setCurrentText(QStringLiteral("OT-1"));
    child<QLineEdit>(doctor, "theatrePatient")->setText(QStringLiteral("VR-2026-000004"));
    child<QLineEdit>(doctor, "theatreProcedure")->setText(QStringLiteral("Coronary angiography"));
    child<QDoubleSpinBox>(doctor, "theatreFee")->setValue(25000);
    ot->book();
    const TheatreModel* tm = ot->model();
    const int row = tm->rowOfKey(QStringLiteral("OT-1|0"));
    report(labelText(doctor, "statusMessage") == QStringLiteral("Booked OT-1 slot 1, 08:00-09:00 for Suresh Kumar.") &&
               tm->textAt(row, TheatreModel::Procedure) == QStringLiteral("Coronary angiography") &&
               ot->ringText().startsWith(QStringLiteral("OT-1: 7 of 8 slots free. Next free in cycle order: 2 3 4 5 6 7 8")),
           "D11 Booking takes slot 1, shows the procedure, ring moves on to slot 2",
           str(labelText(doctor, "statusMessage")));
    ot->table()->selectRow(row);
    ot->releaseSelected();
    const QString released = labelText(doctor, "statusMessage");
    const QString ringAfter = ot->ringText().section(QLatin1Char('\n'), 0, 0);
    ot->table()->selectRow(row);
    ot->releaseSelected();
    report(released == QStringLiteral("Released OT-1 slot 1; it is back in the free-slot ring.") &&
               ringAfter.endsWith(QStringLiteral("2 3 4 5 6 7 8 1")) &&
               labelText(doctor, "statusMessage") == QStringLiteral("Theatre: slot 1 in OT-1 is not booked."),
           "D12 Release puts slot 1 at the back of the ring; releasing again is refused",
           str(ringAfter));
    child<QLineEdit>(doctor, "theatrePatient")->setText(QStringLiteral("VR-2026-000006"));
    child<QLineEdit>(doctor, "theatreProcedure")->setText(QStringLiteral("Bronchoscopy"));
    ot->book();
    doctor->tabs()->setCurrentIndex(2);
    shoot(doctor, shots, "doctor_theatres.png");

    // Next available doctor.
    DoctorAvailabilityPanel* avail = doctor->availability();
    avail->specialty()->setCurrentText(QStringLiteral("Cardiology"));
    const QString next = avail->findNext();
    report(next.startsWith(QStringLiteral("Next available in Cardiology: Dr. Rohan Mehta - ")) &&
               avail->model()->rowCount() == static_cast<int>(hub.session().staff().doctors().size()),
           "D13 Next available doctor: the console sentence, and the whole availability array",
           str(next));
    doctor->tabs()->setCurrentIndex(3);
    shoot(doctor, shots, "doctor_availability.png");

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

