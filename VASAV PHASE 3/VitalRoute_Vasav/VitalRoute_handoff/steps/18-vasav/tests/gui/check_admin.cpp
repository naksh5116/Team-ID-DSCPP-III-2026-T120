// check_admin.cpp - Super Admin window: billing, discharge, undo, admissions, staff, indexes, save and reload.
#include "GuiCheck.hpp"

void adminChecks(const QString& dataDir, const QString& shots)
{
    section("Super Admin window");
    setAutoConfirmForChecks(true);   // destructive actions would otherwise open a dialog
    SessionHub hub(dataDir);
    hub.start();
    Launcher launcher(hub);
    auto* admin = qobject_cast<AdminWindow*>(launcher.openRole(Role::SuperAdmin));
    auto* nurse = qobject_cast<NurseWindow*>(launcher.openRole(Role::Nurse));
    if (!admin || !nurse) {
        report(false, "A1  Super Admin button opens an AdminWindow");
        return;
    }
    QCoreApplication::processEvents();
    auto* undo = admin->findChild<QAction*>(QStringLiteral("undoAction"));
    report(admin->tabs()->count() == 9 && undo && !undo->isEnabled(),
           "A1  Super Admin window: 9 tabs, undo disabled while the journal is empty");

    const HospitalSession& s = hub.session();
    const vr_avl& idx = s.patients().dateIndex();
    report(admin->indexes()->value("hashEntries") == QStringLiteral("11") &&
               admin->indexes()->value("avlNodes") == QString::number(idx.nodes) &&
               admin->indexes()->value("avlValues") == QStringLiteral("11") &&
               admin->indexes()->value("avlHeight") == QString::number(vr_avl_height(&idx)) &&
               admin->indexes()->value("avlValid") == QStringLiteral("Yes"),
           "A2  Index panel: hash entries/buckets/load/collisions/resizes and AVL height, checked balanced",
           str(QStringLiteral("%1 entries, %2 buckets, AVL height %3")
                   .arg(admin->indexes()->value("hashEntries"), admin->indexes()->value("hashBuckets"),
                        admin->indexes()->value("avlHeight"))));

    const int today = vrclock::today();
    const auto expected = s.admittedBetween(vrclock::addDays(today, -30), today);
    report(admin->admissions()->model()->rowCount() == static_cast<int>(expected.size()) &&
               admin->admissions()->summary() ==
                   QStringLiteral("%1 patient(s) registered %2 to %3.")
                       .arg(expected.size()).arg(Text::date(vrclock::addDays(today, -30)), Text::date(today)) &&
               admin->admissions()->avlText().startsWith(QStringLiteral("AVL index: %1 dates, height").arg(idx.nodes)),
           "A3  Admissions: console defaults (last 30 days), count and AVL line as the console prints",
           str(admin->admissions()->summary()));

    // Manual charge, then undo it.
    BillingPanel* billing = admin->billing();
    billing->openBill(QStringLiteral("VR-2026-000004"));
    child<QComboBox>(admin, "chargeType")->setCurrentIndex(2);   // Test
    child<QLineEdit>(admin, "chargeDescription")->setText(QStringLiteral("Chest X-ray"));
    child<QDoubleSpinBox>(admin, "chargeTestPrice")->setValue(900);
    child<QCheckBox>(admin, "chargeTestUrgent")->setChecked(true);
    billing->addCharge();
    const QString added = labelText(admin, "statusMessage");
    report(added == QStringLiteral("Added Rs 1125.00. New subtotal Rs 1125.00.") &&
               billing->bill()->lines()->rowCount() == 1 && undo->isEnabled() &&
               undo->toolTip() == QStringLiteral("Undo the charge 'Chest X-ray' on BL-000004"),
           "A4  Manual charge by AD-01: console sentence, bill line added, undo names the entry", str(added));
    admin->tabs()->setCurrentIndex(3);
    shoot(admin, shots, "admin_billing.png");

    admin->undoLast();
    report(labelText(admin, "statusMessage") == QStringLiteral("Undone: charge 'Chest X-ray' on BL-000004 (Rs 1125.00).") &&
               billing->bill()->lines()->rowCount() == 0,
           "A5  Undo removes the charge with the console's sentence", str(labelText(admin, "statusMessage")));

    // Registration from the Super Admin window, seen by the Nurse window, then undone.
    fillPatient(admin, "er", QStringLiteral("Asha Rawat"), 40, QStringLiteral("Collapse"));
    child<QRadioButton>(admin, "esi2")->setChecked(true);
    admin->registration()->emergencyForm()->submit();
    const int afterRegister = nurse->registration()->emergencyModel()->rowCount();
    admin->undoLast();
    report(afterRegister == 7 && nurse->registration()->emergencyModel()->rowCount() == 6 &&
               labelText(admin, "statusMessage") == QStringLiteral("Undone: registration of Asha Rawat (VR-2026-000012).") &&
               admin->indexes()->value("hashEntries") == QStringLiteral("11") &&
               labelText(nurse, "statusEmergency") == QStringLiteral("Emergency waiting: 6"),
           "A6  Undo of a registration updates the Nurse window, its status bar and the index figures");

    // Insurance.
    billing->openBill(QStringLiteral("VR-2026-000005"));
    child<QLineEdit>(billing, "insuranceProvider")->setText(QStringLiteral("ICICI Lombard"));
    child<QLineEdit>(billing, "insurancePolicy")->setText(QStringLiteral("IL-5"));
    child<QDoubleSpinBox>(billing, "insuranceAmount")->setValue(10000);
    billing->recordInsurance();
    const Bill* b5 = s.findBill("VR-2026-000005");
    report(b5 && b5->insurance() && b5->insurance()->provider() == "ICICI Lombard" &&
               labelText(admin, "statusMessage") ==
                   QStringLiteral("Insurance recorded; it is deducted once, when the bill is settled."),
           "A7  Insurance recorded on the bill with the console sentence");

    // Discharge refusals, then a real discharge after treatment.
    billing->discharge();
    const QString waiting = labelText(admin, "statusMessage");
    billing->openBill(QStringLiteral("VR-2026-000001"));
    const bool settledLocked = !child<QPushButton>(admin, "dischargeButton")->isEnabled();
    billing->discharge();
    const QString already = labelText(admin, "statusMessage");
    report(waiting == QStringLiteral("VR-2026-000005 is still waiting to be seen.") && settledLocked &&
               already == QStringLiteral("VR-2026-000001 was already discharged."),
           "A8  Discharge refused for a waiting patient and an already discharged one (console wording)");

    const int icuFree = s.router().freeBeds(CareUnit::ICU);
    hub.treatNext();   // VR-2026-000004 to ICU-1, Bed 1
    billing->openBill(QStringLiteral("VR-2026-000004"));
    billing->discharge();
    report(labelText(admin, "statusMessage") ==
                   QStringLiteral("ICU-1, Bed 1 is free again. Bill BL-000004 settled: paid Rs 8400.00.") &&
               s.router().freeBeds(CareUnit::ICU) == icuFree &&
               s.findPatient("VR-2026-000004").status() == PatientStatus::Discharged,
           "A9  Discharge after confirmation: bed freed, insurance applied once, Rs 8400.00 paid",
           str(labelText(admin, "statusMessage")));

    // All bills, merge-sorted.
    child<QComboBox>(admin, "billsSort")->setCurrentIndex(1);
    const BillsListModel* bills = billing->allBills();
    auto amountAt = [bills](int r) {
        return bills->textAt(r, BillsListModel::Total).mid(3).toDouble();
    };
    bool descending = bills->rowCount() == 11;
    for (int r = 1; r < bills->rowCount(); ++r)
        descending = descending && amountAt(r - 1) >= amountAt(r);
    child<QComboBox>(admin, "billsSort")->setCurrentIndex(0);
    bool ascending = true;
    for (int r = 1; r < bills->rowCount(); ++r)
        ascending = ascending && bills->textAt(r - 1, BillsListModel::Opened) <= bills->textAt(r, BillsListModel::Opened);
    report(descending && ascending, "A10 All bills: by amount largest first, by date oldest first");

    auto* authoriser = child<QComboBox>(admin, "chargeAuthoriser");
    report(admin->staff()->roster()->rowCount() == static_cast<int>(s.staff().size()) &&
               authoriser->count() == 1 && authoriser->itemText(0).startsWith(QStringLiteral("AD-01")),
           "A11 Staff roster lists everyone; only administrators can authorise charges");
    admin->tabs()->setCurrentIndex(5);
    shoot(admin, shots, "admin_staff.png");
    admin->tabs()->setCurrentIndex(4);
    shoot(admin, shots, "admin_admissions.png");
    admin->tabs()->setCurrentIndex(8);
    shoot(admin, shots, "admin_indexes.png");

    // Save from the toolbar, reload from the Launcher: everything comes back (as T8).
    const std::size_t patients = s.patients().size();
    const std::size_t waitingE = s.triage().emergencyCount();
    admin->findChild<QAction*>(QStringLiteral("saveAction"))->trigger();
    const QString saved = labelText(admin, "statusMessage");
    launcher.reload();
    const HospitalSession& s2 = hub.session();
    report(saved.startsWith(QStringLiteral("Saved 11 patients, ")) && s2.patients().size() == patients &&
               s2.triage().emergencyCount() == waitingE &&
               s2.findPatient("VR-2026-000004").status() == PatientStatus::Discharged &&
               s2.findBill("VR-2026-000005")->insurance() &&
               nurse->registration()->emergencyModel()->rowCount() == static_cast<int>(waitingE) &&
               admin->indexes()->value("hashEntries") == QStringLiteral("11"),
           "A12 Save, then reload: patients, queue, discharge, insurance and indexes all come back",
           str(saved));

    setAutoConfirmForChecks(false);
    for (RoleWindow* w : launcher.roleWindows())
        w->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

