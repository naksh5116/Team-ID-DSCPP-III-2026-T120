// check_restart.cpp - save in the GUI, quit, start again, compare (as T8).
//
// Two separate runs of the check program on one data folder:
//   vitalroute_gui_check --restart save DIR   work through the windows, save
//                                             from the Super Admin toolbar,
//                                             write DIR/fingerprint.txt, exit
//   vitalroute_gui_check --restart load DIR   new process: start from the
//                                             saved records, compare
// The fingerprint lists every patient, bill and doctor as the session holds
// them, so any field that did not survive the restart shows up as a diff.
#include "GuiCheck.hpp"

#include <QTextStream>

namespace {

QString money2(double v)
{
    return QString::number(v, 'f', 2);
}

QStringList fingerprint(const HospitalSession& s)
{
    QStringList lines;
    const TriageUnit& t = s.triage();
    lines << QStringLiteral("waiting emergency=%1 opd=%2 next=%3")
                 .arg(t.emergencyCount()).arg(t.opdCount()).arg(Text::qs(t.peekEmergency()));
    QStringList order;
    for (const Patient* p : s.emergencyOrder())
        order << Text::qs(p->id());
    lines << QStringLiteral("heap order=%1").arg(order.join(QLatin1Char(' ')));
    QStringList opd;
    for (const std::string& id : t.opdLine())
        opd << Text::qs(id);
    lines << QStringLiteral("opd line=%1").arg(opd.join(QLatin1Char(' ')));
    for (const Patient* p : s.patients().all())
        lines << QStringLiteral("patient %1|%2|%3|%4|bed=%5|doctor=%6|visits=%7|discharged=%8")
                     .arg(Text::qs(p->id()), Text::qs(p->name()), Text::statusText(p->status()))
                     .arg(p->severity()).arg(Text::qs(p->bedCode()), Text::qs(p->doctorId()))
                     .arg(p->visitCount()).arg(p->dischargeDate());
    for (const Bill* b : s.listBills(false))
        lines << QStringLiteral("bill %1|%2|charges=%3|subtotal=%4|total=%5|settled=%6|insurance=%7")
                     .arg(Text::qs(b->billId()), Text::qs(b->patientId())).arg(b->chargeCount())
                     .arg(money2(b->subtotal()), money2(b->total()))
                     .arg(b->settled() ? 1 : 0)
                     .arg(b->insurance() ? Text::qs(b->insurance()->summary()) : QStringLiteral("none"));
    for (const Doctor* d : s.staff().doctors())
        lines << QStringLiteral("doctor %1|next free=%2|seen=%3").arg(Text::qs(d->id())).arg(d->nextFree()).arg(d->patientsSeen());
    QStringList beds;
    for (const Bed& b : s.router().beds())
        if (!b.occupant.empty())
            beds << Text::qs(b.code) + QLatin1Char('=') + Text::qs(b.occupant);
    lines << QStringLiteral("occupied beds=%1").arg(beds.join(QLatin1Char(' ')));
    return lines;
}

} // namespace

int restartSave(const QString& dir)
{
    section("Restart, part 1: work in the GUI and save");
    setAutoConfirmForChecks(true);
    SessionHub hub(dir);
    const HospitalSession::StartupReport start = hub.start();
    report(!start.loadedFromRecords, "R1  Starts from the sample data (no saved records yet)");
    Launcher launcher(hub);
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    auto* doctor = qobject_cast<DoctorWindow*>(launcher.openRole(Role::Doctor));
    auto* admin = qobject_cast<AdminWindow*>(launcher.openRole(Role::SuperAdmin));
    QCoreApplication::processEvents();

    // Nurse: an ESI 1 patient with insurance.
    fillPatient(nurse, "er", QStringLiteral("Restart Patient"), 58, QStringLiteral("Chest pain at rest"));
    child<QRadioButton>(nurse, "esi1")->setChecked(true);
    auto* form = nurse->registration()->emergencyForm();
    child<QCheckBox>(form, "insuranceCheck")->setChecked(true);
    child<QLineEdit>(form, "insuranceProvider")->setText(QStringLiteral("CGHS"));
    child<QLineEdit>(form, "insurancePolicy")->setText(QStringLiteral("CG-1"));
    child<QDoubleSpinBox>(form, "insuranceAmount")->setValue(500);
    form->submit();
    // Doctor: three treatments (VR-2026-000004, 000007, then the new patient), a visit note.
    for (int i = 0; i < 3; ++i)
        doctor->treatment()->treatNext();
    doctor->lookup()->lookUp(QStringLiteral("VR-2026-000012"));
    child<QComboBox>(doctor, "noteDoctor")->setCurrentText(QStringLiteral("Dr. Rohan Mehta"));
    child<QLineEdit>(doctor, "noteDiagnosis")->setText(QStringLiteral("Unstable angina"));
    child<QLineEdit>(doctor, "notePrescriptions")->setText(QStringLiteral("Aspirin 75 mg"));
    doctor->lookup()->addVisitNote();
    // Super Admin: a manual charge, a discharge, then Save from the toolbar.
    BillingPanel* billing = admin->billing();
    billing->openBill(QStringLiteral("VR-2026-000012"));
    child<QComboBox>(admin, "chargeType")->setCurrentIndex(2);
    child<QLineEdit>(admin, "chargeDescription")->setText(QStringLiteral("Troponin test"));
    child<QDoubleSpinBox>(admin, "chargeTestPrice")->setValue(1200);
    billing->addCharge();
    billing->openBill(QStringLiteral("VR-2026-000004"));
    billing->discharge();
    admin->findChild<QAction*>(QStringLiteral("saveAction"))->trigger();
    const QString saved = labelText(admin, "statusMessage");
    report(saved.startsWith(QStringLiteral("Saved 12 patients, ")) &&
               QFile::exists(dir + QStringLiteral("/records/patients.dat")),
           "R2  Saved from the Super Admin toolbar", str(saved));

    QFile out(dir + QStringLiteral("/fingerprint.txt"));
    const QStringList lines = fingerprint(hub.session());
    const bool written = out.open(QIODevice::WriteOnly | QIODevice::Text) &&
                         out.write((lines.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8()) > 0;
    out.close();
    report(written, "R3  State fingerprint written for the second run",
           str(QStringLiteral("%1 lines").arg(lines.size())));

    setAutoConfirmForChecks(false);
    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return checksFailed() == 0 ? 0 : 1;
}

int restartLoad(const QString& dir)
{
    section("Restart, part 2: a new process loads the saved records");
    SessionHub hub(dir);
    const HospitalSession::StartupReport start = hub.start();
    report(start.loadedFromRecords && start.loaded.patients == 12 && start.warnings.empty(),
           "R4  Starts from the saved records, no warnings",
           str(QStringLiteral("%1 patients, %2 staff, %3 visits, %4 bills")
                   .arg(start.loaded.patients).arg(start.loaded.staff).arg(start.loaded.visits).arg(start.loaded.bills)));

    QFile in(dir + QStringLiteral("/fingerprint.txt"));
    QStringList before;
    if (in.open(QIODevice::ReadOnly | QIODevice::Text))
        before = QString::fromUtf8(in.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QStringList after = fingerprint(hub.session());
    QString firstDiff;
    for (int i = 0; i < std::max(before.size(), after.size()) && firstDiff.isEmpty(); ++i)
        if (before.value(i) != after.value(i))
            firstDiff = QStringLiteral("before: %1 / after: %2").arg(before.value(i), after.value(i));
    report(!before.isEmpty() && firstDiff.isEmpty(),
           "R5  Every patient, bill, doctor, queue and bed is the same after the restart",
           firstDiff.isEmpty() ? str(QStringLiteral("%1 lines identical").arg(after.size())) : str(firstDiff));

    // The windows show the reloaded state.
    Launcher launcher(hub);
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    auto* patient = qobject_cast<PatientWindow*>(launcher.openRole(Role::Patient));
    QCoreApplication::processEvents();
    patient->showPatient(QStringLiteral("VR-2026-000012"));
    QCoreApplication::processEvents();
    const Patient& p = hub.session().findPatient("VR-2026-000012");
    report(nurse->registration()->emergencyModel()->rowCount() == static_cast<int>(hub.session().triage().emergencyCount()) &&
               patient->record()->fieldText("recordBed") == Text::qs(p.bedLabel()) && !p.bedLabel().empty() &&
               patient->route()->stepCount() > 0 && patient->record()->history()->rowCount() == 2 &&
               patient->bill()->lines()->rowCount() == static_cast<int>(hub.session().findBill(p.id())->chargeCount()),
           "R6  The reloaded windows show the queue, the bed and route, the visit note and the bill",
           str(QStringLiteral("%1, %2 visits").arg(Text::qs(p.bedLabel())).arg(patient->record()->history()->rowCount())));

    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return checksFailed() == 0 ? 0 : 1;
}
