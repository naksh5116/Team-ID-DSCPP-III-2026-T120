# VitalRoute GUI: manual test checklist

GUI lead: Devesh Saini. A checklist for every action on every role screen. For each action it gives
the expected result and how it compares with the console program on the
same seeded data. Run it before the demo and before the viva.

## How to use this checklist

1. Start from the sample data: delete the files in `data/records/` (keep
   `.gitkeep`). The first start then seeds the same 11 patients as the
   console (see README, "First run").
2. Start the GUI from the project folder (see README, "GUI").
3. Work down each table. Tick **You** when the result matches **Expected**.

### What the "Observed" column means

The **Observed** column records what I saw on 2026-11-22. I drove the real
windows through the automated check program (`tests/gui/`). Each entry
names the check that confirmed it, so it can be rerun:

| Mark | Meaning |
|---|---|
| `N4` (any check ID) | Confirmed by that automated check, which fills the real form widgets, presses the real buttons or keys, and reads the tables and status bar. `sh tests/run_gui_check.sh` reruns them all (104 checks) |
| `shot: name` | Seen in the screenshot `docs/screenshots/name.png` |
| **Not verified by me** | I could not check this here. A person must run it (usually real monitors or real hands) |

Test machine: Windows 11, MSYS2 UCRT64, Qt 6.10.1, GCC 15.2, **one** monitor
at 125 % scaling (the screenshots are 1.25 times the logical size). Linux was
**not** tested.

Where the GUI wording differs from the console, the table says so. Every
difference is deliberate and listed under "Wording differences" below.

---

## 1. Launcher

| # | Action | Expected | Console | Observed | You |
|---|---|---|---|---|---|
| L1 | Start the GUI | Launcher titled "VitalRoute - Launcher": "Open a role window", the note that there is no sign-in, buttons Doctor, Nurse, Patient, Super Admin, empty window list, data folder path | Console prints "First run: sample patients..." | W2, shot: launcher | |
| L2 | Click each role button | A new window per click; the Launcher stays open; the list shows each window and its screen | none | W2 | |
| L3 | Click Patient twice | Two separate Patient windows | none | W2, P6 | |
| L4 | Double-click a row in the window list | That window comes to the front | none | **Not verified by me** | |
| L5 | Save | Status: "Saved N patients, ... to data/records/" | Option 12 prints the same sentence | A12, K7 | |
| L6 | Reload saved records | Asks first. After Reload, every open window shows the saved state. Theatre bookings and transfers are cleared (they are not saved, as in the console) | A console restart | A12, Y10 | |
| L7 | Arrange on screens (one screen) | Open windows tiled 2 x 2: Nurse top left, Doctor top right, Patient bottom left, Super Admin bottom right. Status bars stay readable | none | S4, S7, shot: tiled_nurse | |
| L8 | Arrange on screens (2-4 screens) | One window per screen in the order Nurse, Doctor, Patient, Super Admin, each maximised. Extra windows wrap round and share a screen | none | Rules: S2, S3, S5. **Real monitors not verified by me** | |
| L9 | Close the Launcher | Asks once: "Save and close" / "Close without saving" / Cancel. Then every window closes | Option 12 saves and exits | **Not verified by me** (modal dialog) | |

## 2. Every role window (frame)

| # | Action | Expected | Console | Observed | You |
|---|---|---|---|---|---|
| F1 | Look at the title | "VitalRoute - Nurse", "VitalRoute - Doctor", "VitalRoute - Super Admin"; Patient windows add the ID ("VitalRoute - Patient VR-2026-000004") | none | W2, P4 | |
| F2 | Look at the status bar | Emergency waiting, OPD waiting, Next (patient ID), ICU beds free, Ward beds free, Role | Menu footer "Waiting: 6 emergency, 2 OPD (next: VR-2026-000004)" | W4 | |
| F3 | Resize to the minimum | Cannot go below 1100 x 700 (except while tiled); everything still usable | none | W3, shot: min_* in the run | |
| F4 | Role, Switch to ... | The window is replaced by the other role, same place and size | none | W6 | |
| F5 | File, Close window (Ctrl+W) | Only that window closes; the app keeps running | none | W7 | |
| F6 | Ctrl+S | Saves (not offered in Patient windows) | Option 12 | K7, P2 | |
| F7 | F5 | Every table in every window refreshes | none | K8 | |
| F8 | F11 | Full screen on the current screen; F11 again returns (to maximised if it was) | none | S10, K10 | |
| F9 | Cause an error (unknown ID) | Message on the left of the status bar, worded as the console: "Not found: no patient with ID ..." | Same sentence | W8, D10, P3 | |

## 3. Nurse

| # | Action | Expected | Console | Observed | You |
|---|---|---|---|---|---|
| N-a | Open the Nurse window | Emergency queue: 000004, 000007, 000006, 000008, 000005, 000009 (heap order). OPD line: 000010, 000011 | Option 3 treats in exactly this order | N2, shot: nurse_registration | |
| N-b | Register emergency with no name, or no ESI level | Status "Enter the patient's name." / "Choose an ESI level, 1 (most critical) to 5."; nothing registered | Console re-asks the name; a bad ESI gives "Registration cancelled: severity 9 is outside the ESI range..." | N3 | |
| N-c | Register "Asha Rawat", 40, ESI 1, insurance CGHS | "Registered VR-2026-000012 (ESI 1). Position in the emergency queue: 3 of 7." plus the insurance sentence; the new row is selected | Option 1 prints the same two sentences | N4 | |
| N-d | Each ESI choice | Badge 1-5 with the standard name (Resuscitation, Emergent, Urgent, Less urgent, Non-urgent) and decision point | Console asks for a number only | shot: nurse_registration | |
| N-e | Register an outpatient for Cardiology | "Registered VR-2026-000013 for Cardiology. Position in the OPD line: 3."; joins the back of the OPD line | Option 2, same sentence | N6 | |
| N-f | Tick insurance but leave the insurer blank | "Enter the insurer, or untick ..."; nothing registered | Console requires the insurer | N7 | |
| N-g | Press Enter in the form | Registers (as the button) | Console: each Enter answers one prompt | K2, K3 | |
| N-h | Transfer desk: add routine, then urgent | Urgent row at the top, routine at the bottom; "Queued TR-0002 URGENT ambulance ..." | Option 8, same "Queued ..." line | N8, shot: nurse_transfers | |
| N-i | Dispatch next request | Takes the top row; for an ambulance with a patient: "Ambulance charge added to BL-000005." | Option 8 "Dispatched ..." | N9 | |
| N-j | Cancel newest routine request | Removes the newest routine request; if the newest is urgent or none: "The newest request is not a routine one (or none is pending)." | Same | N10 | |
| N-k | Dispatch with nothing pending | "No pending requests." | Same | N10 | |
| N-l | Transfer for an unknown patient ID | "Not found: no patient with ID VR-2026-999999."; nothing queued | Same | N11 | |

## 4. Doctor

| # | Action | Expected | Console | Observed | You |
|---|---|---|---|---|---|
| D-a | Open the Doctor window | Next patient: VR-2026-000004, ESI 1, "Needs an ICU bed (8 free)"; queues on the right | Menu footer "next: VR-2026-000004" | D2 | |
| D-b | Treat next patient | "Patient: VR-2026-000004  Suresh Kumar, ESI 1 - Cardiac arrest, pulse restored in ambulance", "Attending: Dr. Meera Iyer from HH:MM", "Bed: ICU-1, Bed 1 (ICU)"; map and 14 steps; "Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s)." | Option 3: the same lines, except the console's first line starts "Next patient:"; the route matches step for step | D3, D4, shot: doctor_treatment | |
| D-c | Treat until ESI 4 | "Treated in the ER and released - ESI 4 does not need a bed."; no route | Same sentence | D6 | |
| D-d | Treat an outpatient | Notes form appears; blank diagnosis becomes "Routine review"; medicine needs a name; "Consultation recorded. Bill so far: Rs ..." | Option 3 asks the same questions | D7 | |
| D-e | Treat with the ICU full | "Cannot admit yet: no free ICU bed. The patient stays at the head of the queue." | Same | Console scenario 08; GUI shows the session's message (not separately checked) | |
| D-f | Look up VR-2026-000010 | Record with status, bed, doctor, bill, insurance; visit history newest first; hash index figures | Option 4 | D8, shot: doctor_record | |
| D-g | Add a visit note dated 60 days ago | "Visit added in date order to VR-2026-000010."; lands in date order, not at the top | Option 4 "Visit added in date order." | D9 | |
| D-h | Look up an unknown ID | "Not found: no patient with ID VR-2026-999999." | Same | D10 | |
| D-i | Book OT-1 for VR-2026-000004 | "Booked OT-1 slot 1, 08:00-09:00 for Suresh Kumar."; procedure in the table; ring line "Next free in cycle order: 2 3 4 5 6 7 8" | Option 7, same sentence | D11, shot: doctor_theatres | |
| D-j | Release the selected slot; release it again | "Released OT-1 slot 1; it is back in the free-slot ring."; ring now ends "... 8 1"; second time "Theatre: slot 1 in OT-1 is not booked." | Same | D12 | |
| D-k | Find next available doctor (Cardiology) | "Next available in Cardiology: Dr. Rohan Mehta - free now"; the whole availability array | Option 6, same sentence | D13 | |

## 5. Patient

| # | Action | Expected | Console | Observed | You |
|---|---|---|---|---|---|
| P-a | Open a Patient window | Asks for the ID; read-only (no Save, one button) | none | P1, P2 | |
| P-b | Enter an unknown ID | "Not found: ..."; title unchanged | Option 4 | P3 | |
| P-c | Enter VR-2026-000004 (waiting) | Title gains the ID; status Waiting; "You do not have a bed yet..."; bill with no charges | Option 4 | P4 | |
| P-d | A doctor treats that patient (other window) | At once: "A bed has been assigned: ICU-1, Bed 1."; map and 14-step route; bill lines and "Due Rs 8400.00 (estimate until discharge)" | Option 9 shows the same bill | P5, shot: patient_window | |
| P-e | Open a second Patient window for VR-2026-000010 | Independent: its own record, two earlier visits, no route | none | P6 | |
| P-f | After discharge | Status Discharged; route cleared; "Paid Rs 8400.00" | Option 10 prints the settled bill | P8 | |
| P-g | F11 (wall display) | Only the map (every floor on the route), directions 1.8x, distance and time; header "Patient VR-2026-000004: walking directions to ICU-1, Bed 1" (no name); F11 returns | none | S8, S9, K10, shot: patient_wall | |

## 6. Super Admin

| # | Action | Expected | Console | Observed | You |
|---|---|---|---|---|---|
| A-a | Open the window | 9 tabs (all the screens above, plus Billing and discharge, Admissions, Staff, Indexes); toolbar Undo last entry (disabled) and Save | none | A1 | |
| A-b | Indexes tab | Hash table: entries, buckets, buckets in use, load, longest chain, collisions, resizes; AVL: dates, patients, height, rotations, "Balanced: Yes" | Options 4 and 5 print these figures | A2, shot: admin_indexes | |
| A-c | Admissions (default dates) | "10 patient(s) registered <30 days ago> to <today>."; "AVL index: 4 dates, height 3, rotations LL 0 RR 1 LR 0 RL 0." | Option 5 with blank dates: the same count and AVL line (reference output `02-register-and-undo`) | A3, shot: admin_admissions | |
| A-d | Manual charge: Test, Chest X-ray, Rs 900, urgent | "Added Rs 1125.00. New subtotal Rs 1125.00."; Undo shows what it would undo | Option 9, same sentence | A4, shot: admin_billing | |
| A-e | Undo last entry | Asks first; "Undone: charge 'Chest X-ray' on BL-000004 (Rs 1125.00)." | Option 11, same sentence | A5 | |
| A-f | Register, then undo | "Undone: registration of Asha Rawat (VR-2026-000012)."; the Nurse window's queue and the index figures go back | Option 11 | A6 | |
| A-g | Record insurance | "Insurance recorded; it is deducted once, when the bill is settled." | Same | A7 | |
| A-h | Discharge a waiting / an already discharged patient | "VR-2026-000005 is still waiting to be seen." / "VR-2026-000001 was already discharged."; a settled bill's forms are locked | Option 10, same sentences | A8 | |
| A-i | Discharge a treated patient | Asks first, naming bed, bill and insurance; "ICU-1, Bed 1 is free again. Bill BL-000004 settled: paid Rs 8400.00." | Option 10: the same bed line and settled bill | A9 | |
| A-j | All bills, sorted by amount / by date | Largest first / oldest first | Option 9 with a blank ID | A10 | |
| A-k | Staff tab | All 9 staff; "Authorised by" lists administrators only | Option 9 accepts only an admin's ID | A11 | |
| A-l | Save, then Reload | Everything comes back | Option 12, restart | A12, Y10 | |
| A-m | Enter in the discharge date | Nothing happens (destructive actions need their button) | none | K9 | |

## 7. Route map

| # | Action | Expected | Observed | You |
|---|---|---|---|---|
| M-a | After a treatment | Tabs "Ground floor (route)", "Floor 1 (route)", "Floor 2"; route in the accent colour; start hollow square, destination filled square; lifts and stairs labelled; wards and ICUs show "Beds 1-4, n free" | M1, M2, shot: doctor_treatment | |
| M-b | Click a step | Its floor is shown; a walk is drawn wider, a turn gets a ring | M3, M4, shot: route_map_floor1 | |
| M-c | Resize the window | The plan rescales and stays inside the map | M5, M6 | |
| M-d | Another patient changes | An open Patient map keeps its floor and selected step | M8 | |

## 8. Screens and display

| # | Action | Expected | Observed | You |
|---|---|---|---|---|
| S-a | Unplug a screen with a window on it | The window moves to the primary screen, resized to fit; full screen stays full screen | Rule: S6. **Not verified by me** (one monitor) | |
| S-b | Plug a screen in | Launcher: "Screen ... added. Press Arrange on screens to use it." | **Not verified by me** | |
| S-c | Move a window between monitors with different scaling | Text and map stay sharp | **Not verified by me** (one monitor) | |

## 9. Keyboard

| # | Action | Expected | Observed | You |
|---|---|---|---|---|
| K-a | Tab through each form | Reading order (name, age, phone, complaint, ESI, insurance, button ...) | K1 | |
| K-b | Enter in any form | Submits that form | K2-K6 | |
| K-c | Ctrl+S, F5, F11 | Save, refresh, full screen (F11 also with the wall view's hidden menu bar) | K7, K8, K10 | |

---

## Cross-window sync

Open the four role windows, ideally on separate screens. Each row makes a
change in one window and watches a **different** window. There must be no
refresh, no delay, and no click in the watched window.

| SessionHub signal | Do this | Watch | Expected | Observed | You |
|---|---|---|---|---|---|
| `queueChanged` | Nurse registers an ESI 1 patient | Doctor's emergency queue; any status bar | New patient at position 3; "Emergency waiting: 7" everywhere | Y1, N5, W5 | |
| `routeAssigned(id)` | Doctor treats the next patient | The Patient window open on that ID | Bed and 14-step route appear; other Patient windows unchanged | Y2, P5, P6 | |
| `bedsChanged` | Doctor treats (ICU bed) | Every status bar; the floor plan | "ICU beds free" drops by one; the ward's free count drops | Y3 | |
| `staffChanged` | Doctor treats | Super Admin, Staff tab | The attending doctor's "Patients seen" goes up, "Busy until ..." | Y4 | |
| `patientsChanged` | Doctor adds a visit note | Patient window for that patient | New row in the visit history | Y5 | |
| `billsChanged` | Super Admin adds a manual charge | Patient window's bill | New bill line, new total | Y6 | |
| `theatresChanged` | Doctor books a theatre slot | Super Admin, Theatres tab | Slot shows Booked with the procedure | Y8 | |
| `transfersChanged` | Nurse adds a transfer request | Super Admin, Transfers tab | Request appears | Y9 | |
| Undo (all signals) | Super Admin undoes the charge | Patient window's bill | The line disappears | Y7, A6 | |
| Save / Reload (all signals) | Super Admin saves; Launcher reloads | Every window | Every window shows the reloaded session; transfers and theatre bookings empty | Y10 | |

## Persistence: save in the GUI, restart, reload (as test T8)

`sh tests/restart_check.sh` runs this in **two separate processes**.

1. The first process works through the windows:
   - registers an ESI 1 patient with insurance;
   - treats three patients;
   - adds a visit note;
   - adds a manual charge;
   - discharges VR-2026-000004;
   - saves from the Super Admin toolbar.

   It then writes a fingerprint of the whole state: every patient, bill and doctor, the heap and FIFO order, and the occupied beds.
2. A new process starts from the saved records and compares the fingerprint.
3. The console program reads the same GUI-saved records.

| # | Check | Observed |
|---|---|---|
| R1-R3 | Work in the GUI, save from the toolbar, write the fingerprint | PASS: "Saved 12 patients, 9 staff, 13 visits, 12 bills (16 charges)" |
| R4 | New process starts from the records, no warnings | PASS: 12 patients, 9 staff, 13 visits, 12 bills |
| R5 | The state matches the fingerprint | PASS: all 34 lines identical |
| R6 | The reloaded windows show the queue, bed and route, visit note and bill | PASS: ICU-1, Bed 3, 2 visits |
| R7 | The console program loads the GUI-saved records | PASS: "Loaded 12 patients, 9 staff, 13 visits and 12 bills"; shows the new patient's bed and visit note |

Theatre bookings and transfer requests are **not** in the saved records.
That is true in the console too (`PersistenceManager` does not save them).
After a restart or reload they start empty.

## Wording differences from the console (deliberate)

| GUI | Console | Why |
|---|---|---|
| Treatment result starts "Patient:" / "Outpatient:" | "Next patient:" / "Next outpatient:" | In the GUI the line sits under "Last treatment", beside a box that already shows the *next* patient |
| Status "Treated", "Admitted" (sentence case) | "TREATED", "ADMITTED" | The model's upper-case names are for the console and the record files; the GUI uses sentence case |
| Route footer as one sentence: "Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s)." | Two parts on one line | Same figures |
| Validation messages ("Enter the patient's name.") | The console re-asks the question | A form can say what is missing |

## Visual review (2026-11-22)

I reviewed the screenshots of every screen against DESIGN.md at three sizes: the normal size, the 1100 x 700 minimum, and a 2 x 2 tile. I followed the web-review checklist (hierarchy, generic look, consistency, copy, contrast, states).

| Finding | Severity | Status |
|---|---|---|
| Tiled windows cut the status bar counts ("Emergency waitin...") | Must fix | Fixed: the message area gives way first |
| Empty bill view showed blank Subtotal / Insurance / Total rows | Should fix | Fixed: totals hidden until a bill is open |
| Labels overlapped on the floor plan | Should fix | Fixed in piece 9 (priority placement, route kept clear) |
| Empty tables show no "nothing here" line | Nice to have | Not done; the status bar and messages say it |
| At 1100 x 700 the Doctor queue column cuts long names | Nice to have | Not done; full names in tooltips |
| ESI 2 badge contrast 3.8:1 | Known | Kept as specified; the level name is always beside the badge (DESIGN.md) |

## Not verified by me

These need a person and the real hardware:

- Real placement on 2-4 monitors or a projector (L8), and unplugging and plugging screens (S-a, S-b).
- Moving a window between monitors with different scaling (S-c).
- The Launcher's close dialog (L9) and double-clicking the window list (L4).
- Building and running on Linux.
