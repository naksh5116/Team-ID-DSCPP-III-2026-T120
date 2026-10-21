# VitalRoute

**A Smart Multi-Specialty Hospital Management and Emergency Routing System**

| | |
|---|---|
| **Team ID** | DSCPP-III-2026-T120 |
| **Programme** | Project-Based Learning, 3rd semester, Graphic Era (Deemed to be University), Dehradun |
| **Courses** | Data Structures in C · Object-Oriented Programming in C++ |
| **Team** | Nakshatra Agarwal (lead) · Devesh Saini · Vasav Sharma |
| **Mentor** | Ms. Pratibha Waldia |

VitalRoute is a console simulator of hospital patient flow. It answers four questions a hospital cannot afford to get wrong:

1. **Who is treated first?** Emergency patients are ranked by clinical severity (Emergency Severity Index, levels 1–5), not by arrival order.
2. **Which bed is free and medically correct?** The nearest free bed *of the required clinical type* is chosen by shortest-path search over the hospital floor plan.
3. **How does the patient get there?** The shortest path is turned into turn-by-turn walking directions, including lift and stair steps across three floors.
4. **What does the patient owe?** Charges accumulate automatically, and insurance is deducted once at discharge.

Every record is saved to plain text files and reloaded on the next start.

> This is an academic simulator. All patients, staff and floor plans are sample data. There is no database engine, no network access and no third-party library: only the C standard library and the C++ standard library.

---

## Contents

- [Build and run](#build-and-run)
- [Using the program](#using-the-program)
- [Sample output](#sample-output)
- [Architecture](#architecture)
- [Repository layout](#repository-layout)
- [Syllabus coverage](#syllabus-coverage)
- [How the key features work](#how-the-key-features-work)
- [Data files](#data-files)
- [Tests](#tests)
- [Team and contributions](#team-and-contributions)
- [References](#references)

---

## Build and run

**Requirements:** `gcc` and `g++` (GCC 9 or newer) and GNU `make`.

Open a terminal **in the repository root** (the folder containing `Makefile`), then:

```bash
make              # builds bin/vitalroute
make test         # builds and runs all 17 tests
./bin/vitalroute  # starts the program
make clean        # removes build/ and bin/ (saved records are kept)
```

### Windows

Use the **MSYS2 UCRT64** shell:

```bash
pacman -S make mingw-w64-ucrt-x86_64-gcc      # one-time setup
cd /c/Users/<you>/Desktop/pbl/Team-ID-DSCPP-III-2026-T120
make
./bin/vitalroute.exe
```

Once built, the program also runs from PowerShell: `./bin/vitalroute.exe`.

Do not mix Git Bash with MSYS2's `make`; gcc then loses its temporary directory and fails with "Cannot create temporary file".

### Notes

- Always start the program from the repository root, because it reads `data/` relative to the current folder. A different data folder can be passed as the first argument: `./bin/vitalroute path/to/data`.
- To start again from the sample data, delete the files inside `data/records/`.
- Memory checking on Linux: `valgrind --leak-check=full ./bin/vitalroute`.

---

## Using the program

```
=========== VitalRoute ===========
 1. Register patient (emergency)
 2. Register patient (OPD)
 3. Treat next patient
 4. Look up patient by ID
 5. Patients admitted between two dates
 6. Find next available doctor
 7. Operation theatre schedule
 8. Ambulance / transfer requests
 9. View or edit a bill
10. Discharge and settle bill
11. Undo last entry
12. Save and exit
==================================
```

| Option | What happens | Structure behind it |
|---|---|---|
| 1 | Register an emergency with an ESI level (1 = most critical) | Max-heap insert |
| 2 | Register an outpatient | FIFO queue |
| 3 | Take the highest-priority patient, assign a doctor and a bed, print the route | Heap extract, Dijkstra, direction generator |
| 4 | Find a patient by ID, e.g. `VR-2026-000004`, with visit history | Hash table, doubly linked list |
| 5 | List admissions in a date range (dates as `2026-09-30` or `20260930`) | AVL tree range walk |
| 6 | Next available doctor for a specialty | Merge sort + lower-bound binary search |
| 7 | Book or release operation theatre slots | Circular queue |
| 8 | Add or dispatch transfers: urgent at the front, routine at the back | Deque |
| 9 | View a bill or add a manual charge | Polymorphic charges, `operator+=` |
| 10 | Apply insurance and settle the bill | `Insurance`, `Bill` |
| 11 | Undo the last registration or manual charge | Stack |
| 12 | Save everything to `data/records/` and exit | `fstream` |

**First run.** With no saved records, VitalRoute loads the staff roster from `data/doctors.txt` and seeds 11 patients:

- 3 earlier admissions, already discharged with settled bills (`VR-2026-000001` to `000003`);
- 6 emergencies waiting, ESI levels 1, 4, 2, 1, 3, 5 in arrival order (`000004` to `000009`);
- 2 outpatients waiting (`000010`, `000011`), one returning with an earlier visit history.

Choose **3** twice: both ESI-1 patients leave the heap first, in arrival order, and each is sent to an ICU bed with a printed route. Choose **12** to save; the next start reloads everything.

---

## Sample output

Option 3 on the first run:

```
Next patient: VR-2026-000004  Suresh Kumar, ESI 1 - Cardiac arrest, pulse restored in ambulance
Attending: Dr. Meera Iyer
Bed: ICU-1, Bed 1 (ICU)

ROUTE: Central Emergency Room  ->  ICU-1, Bed 1
------------------------------------------------------------
 1. From the Central Emergency Room, head towards ER Triage Desk
 2. Walk 34 m along the Reception Corridor
 3. Turn left at Main Corridor Junction
 4. Walk 12 m
 5. Bear slightly left at Pharmacy Corner
 6. Walk 9 m
 7. Take the lift to floor 1
 8. Exit the lift heading south towards Nursing Station 1A
 9. Walk 10 m
10. Turn right at Nursing Station 1A
11. Walk 12 m along the West Wing Corridor
12. Turn right at the West Wing Corridor
13. Walk 12 m
14. Arrive at ICU-1, Bed 1 - on your left
------------------------------------------------------------
Total distance: 89 m      Estimated walking time: 74 s (1 min 14 s)
```

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│ Presentation      console menu (src/main.cpp)                │
│                   Qt GUI planned for Phase III               │
├──────────────────────────────────────────────────────────────┤
│ Application       Patient & Staff · Triage & Routing ·       │
│ (C++)             Navigation · Billing     (src/modules/)    │
├──────────────────────────────────────────────────────────────┤
│ Domain model      Person, Patient, Staff, Charge, Bill,      │
│ (C++)             Insurance, exceptions    (src/model/)      │
├──────────────────────────────────────────────────────────────┤
│ Engine (C)        every data structure, hand-written         │
│                                            (src/engine/)     │
├──────────────────────────────────────────────────────────────┤
│ Persistence       plain-text records       (data/records/)   │
└──────────────────────────────────────────────────────────────┘
```

Patient flow: **Intake → Triage & Routing → Navigation → Records & Billing → Discharge**.

| Layer | Language | Rule |
|---|---|---|
| `src/engine/` | C11 (`gcc -std=c11`) | Every data structure is hand-written. No C++. |
| `src/model/` | C++17 (`g++ -std=c++17`) | Domain classes, inheritance, polymorphic charges, exceptions. |
| `src/modules/`, `src/main.cpp` | C++17 | Application logic on top of the engine. STL only for bookkeeping. |

Every engine header is wrapped in `extern "C" { ... }` under `__cplusplus`, so the C++ layers include the C headers directly and link against the C objects. The presentation layer can be replaced (for example by the Phase III GUI) without changing any module, the engine or the storage format.

---

## Repository layout

```
.
├── Makefile                      build rules for C and C++ layers
├── README.md                     this file
├── COMMITS.md                    team commit plan (who commits what, in which order)
├── data/
│   ├── hospital_graph.txt        floor plan: 36 landmarks on 3 floors
│   ├── resources.txt             ICU and ward beds, operation theatres
│   ├── doctors.txt               starting staff roster
│   └── records/                  saved state (written by option 12)
├── src/
│   ├── engine/                   PURE C - the data structures
│   │   ├── vr_stack.c/.h         growable stack            -> undo journal
│   │   ├── vr_queue.c/.h         linked FIFO queue         -> OPD line
│   │   ├── vr_pqueue.c/.h        binary max-heap           -> triage, Dijkstra frontier
│   │   ├── vr_cqueue.c/.h        circular queue + count    -> OT free-slot ring
│   │   ├── vr_deque.c/.h         circular-array deque      -> transfer desk
│   │   ├── vr_dlist.c/.h         doubly linked list        -> visit history
│   │   ├── vr_hash.c/.h          chained hash table        -> patient ID index
│   │   ├── vr_avl.c/.h           AVL tree                  -> admission-date index
│   │   ├── vr_graph.c/.h         adjacency list + Dijkstra -> floor plan
│   │   └── vr_sort.c/.h          merge sort, lower bound   -> listings, doctor search
│   ├── model/                    C++ domain classes
│   │   ├── Person                abstract base (+ date/clock helpers)
│   │   ├── Patient               Person subclass; owns a vr_dlist of visits
│   │   ├── Staff                 Staff -> Doctor, Nurse, Admin
│   │   ├── Charge                abstract; Bed/Medicine/Test/ProcedureCharge
│   │   ├── Bill                  operator+=, operator<<, one-time insurance
│   │   ├── Insurance             flat or percentage (with cap)
│   │   └── Exceptions.hpp        five domain exceptions
│   ├── modules/                  C++ application logic
│   │   ├── PatientRegistry       hash + AVL indexes, owns patients
│   │   ├── StaffRegistry         roster, next-available-doctor search
│   │   ├── TriageUnit            emergency heap, OPD queue
│   │   ├── HospitalMap           loads the floor plan into vr_graph
│   │   ├── ResourceRouter        type filter first, then Dijkstra distance
│   │   ├── Navigator             turn-by-turn direction generator
│   │   ├── OTScheduler           per-theatre slot ring
│   │   ├── TransferDesk          urgent front / routine back
│   │   ├── BillingEngine         charges, manual entries, sorted listings
│   │   └── PersistenceManager    fstream records, index rebuild on load
│   └── main.cpp                  console menu and undo journal
└── tests/
    ├── test_engine.c             unit tests for the C engine
    ├── test_scenarios.cpp        scenario and integration tests
    └── run_tests.sh              runs both suites (called by make test)
```

---

## Syllabus coverage

### Data Structures in C

| # | Structure | Implementation | Used by |
|---|---|---|---|
| 1 | Stack (push, pop, peek, dynamic growth) | `src/engine/vr_stack.c` | Undo journal in `src/main.cpp` |
| 2 | Queue (FIFO) | `src/engine/vr_queue.c` | OPD line in `TriageUnit` |
| 3 | Priority queue (max-heap, insert, extract-max, heapify) | `src/engine/vr_pqueue.c` | Triage in `TriageUnit`; Dijkstra in `vr_graph.c`; heapify rebuilds the waiting list on load |
| 4 | Circular queue (modulo wrap, `count` field) | `src/engine/vr_cqueue.c` | Free-slot ring per theatre in `OTScheduler` |
| 5 | Double-ended queue | `src/engine/vr_deque.c` | `TransferDesk` |
| 6 | Doubly linked list | `src/engine/vr_dlist.c` | Visit history in `Patient` (newest-first by backward walk) |
| 7 | Hash table (chaining, load-factor resize) | `src/engine/vr_hash.c` | Patient ID index in `PatientRegistry` |
| 8 | AVL tree (LL, RR, LR, RL rotations, range walk) | `src/engine/vr_avl.c` | Admission-date index in `PatientRegistry`; option 5 |
| 9 | Weighted graph + Dijkstra | `src/engine/vr_graph.c` | `HospitalMap`, `ResourceRouter`, `Navigator` |
| 10 | Merge sort (stable) | `src/engine/vr_sort.c` | Bill and patient listings; staff availability |
| 11 | Binary search (lower bound) | `src/engine/vr_sort.c` | Next available doctor in `StaffRegistry` |

### Object-Oriented Programming in C++

| # | Concept | Where |
|---|---|---|
| 1 | Classes and objects | `Patient`, `Doctor`, `Nurse`, `Admin`, `Bill`, `Insurance`, `HospitalMap` and every module |
| 2 | Inheritance | `Person` → `Patient`, `Staff`; `Staff` → `Doctor`, `Nurse`, `Admin` |
| 3 | Polymorphism / virtual functions | `Charge::calculateCost() const = 0`, overridden by `BedCharge`, `MedicineCharge`, `TestCharge`, `ProcedureCharge`; `Bill` and `BillingEngine` hold only `Charge` pointers. Virtual destructors on every polymorphic base. |
| 4 | Operator overloading | `Bill::operator+=(const Charge&)`, `Patient::operator<` (triage order), `operator<<` for `Bill`, `Route` and `Person` |
| 5 | Exception handling | `BedUnavailableException`, `PatientNotFoundException`, `InvalidSeverityException`, `NoRouteFoundException`, `SlotUnavailableException`, all caught in `main.cpp` |
| 6 | File handling | `PersistenceManager` (`ofstream` / `ifstream`); `HospitalMap::load`, `ResourceRouter::loadBeds`, `StaffRegistry::loadRoster` |
| 7 | STL | `vector`, `map`, `string`, `optional`, `algorithm`, in the C++ layers only, never replacing a structure above |

---

## How the key features work

**Triage.** Emergencies go into a max-heap keyed on `(6 − severity, −arrival order)`, so ESI 1 leaves first and patients with equal severity leave in arrival order. A severity outside 1–5 raises `InvalidSeverityException`. ESI 1–2 need an ICU bed, ESI 3 a ward bed, and ESI 4–5 are treated in the emergency room and released. Outpatients wait in a FIFO queue and are seen once no emergency is waiting.

**Bed selection, in two stages.** `ResourceRouter::allocate` first keeps only free beds of the required clinical type, then runs Dijkstra from the Central Emergency Room and takes the nearest of those. The order matters: the ground-floor Acute Medical Unit is much closer to the ER than any ICU bed, so comparing distance first would send an ICU patient to a ward bed. Our first scenario test caught exactly that bug, and test T5 now guards against it.

**Directions.** For each same-floor segment of the path, `Navigator` computes the bearing with `atan2(dy, dx)`. At every intermediate landmark the change in bearing is classified: under 20° straight on, 20–60° bear slightly, 60–135° turn, over 135° turn sharply. Lift and stair edges become a single "Take the lift/stairs to floor n" instruction. Straight runs are merged into "Walk N m", and walking time uses 1.2 m/s (lift rides excluded).

**Operation theatres.** Each theatre has eight one-hour slots and a circular queue of free slot numbers. Booking dequeues the next free slot; releasing enqueues it, so a freed slot is reusable at once without moving any other slot. The `count` field distinguishes a full ring from an empty one.

**Transfers.** Urgent ambulance and transfer requests are pushed at the front of a deque and routine ones at the back, both in O(1).

**Billing.** Consultations, tests, beds, surgery and ambulance runs are charged automatically as they happen (`bill += charge`). Administrators can add manual charges. Insurance (flat, or a percentage with an optional cap) is applied once, at settlement, and never takes the total below zero.

**Persistence.** Option 12 writes `patients.dat`, `staff.dat`, `visits.dat` and `bills.dat` under `data/records/`. On start-up they are read back, every patient is re-registered (rebuilding the hash and AVL indexes), the waiting lists are rebuilt with heapify, and occupied beds are marked again.

---

## Data files

`data/hospital_graph.txt` holds 36 landmarks on three floors. The ground floor has the Central Emergency Room, reception, the Acute Medical Unit, two lifts and a stairwell; floor 1 has ICU-1, ICU-2 and two operation theatres; floor 2 has three general wards.

```
N <id> <floor> <x> <y> <type> <name with spaces>    # coordinates in metres, y grows north
E <from> <to> <weight> <type> [D]                     # undirected unless D
```

`data/resources.txt` lists beds and theatres:

```
BED <code> <node> <number> <ICU|WARD> <L|R>
OT  <name> <node> <slots> <first hour>
```

`data/doctors.txt` is the starting staff roster.

---

## Tests

`make test` builds and runs both suites. Each test prints one `[PASS]` or `[FAIL]` line, and the run ends with `ALL TESTS PASSED` or `SOME TESTS FAILED`.

**`tests/test_engine.c` (C)**

| Test | Checks |
|---|---|
| T1 | Priority queue: severity order, arrival-time tie-break, heapify, removal |
| T2 | Circular queue: wrap-around, full vs empty, freed slot reused without shifting |
| T3 | AVL tree: 1000 ascending dates keep logarithmic height; all four rotations |
| T4 | Hash table: lookup after collisions are forced into one bucket |
| X1–X5 | Stack, queue, deque, doubly linked list, merge sort stability and lower bound |

**`tests/test_scenarios.cpp` (C++)**

| Test | Checks |
|---|---|
| T5 | Triage order and bed assignment; an ICU patient is never sent to a nearer ward bed |
| T6 | Turn-by-turn directions for a known route, including lift and stairs |
| T7 | Billing with insurance matches a hand-calculated total |
| T8 | Full journey: register, triage, route, consult, bill, discharge, save, reload, compare |
| X6–X9 | Theatre slot reuse, transfer desk ordering, doctor search, undo and visit order |

---

## Team and contributions

| Member | Role | Owns |
|---|---|---|
| Nakshatra Agarwal (`naksh5116`) | Team lead | Core queue family, hospital graph, severity triage, Dijkstra bed selection, direction generator, OT and transfer modules, console integration |
| Devesh Saini | Developer | `Person` / `Patient` / `Staff` hierarchy, `Charge` hierarchy, billing with operator overloading and insurance |
| Vasav Sharma | Developer | Doubly linked list, hash index, AVL date-range search, file persistence, test suites |

Each member commits from their own GitHub account on a feature branch (`feature/engine`, `feature/triage`, `feature/navigation`, `feature/billing`), and integration happens through reviewed merges into `main`. See `COMMITS.md` for the full plan.

---

## References

1. Reema Thareja, *Data Structures Using C*, Oxford University Press.
2. E. Balagurusamy, *Object-Oriented Programming with C++*, McGraw Hill.
3. *Emergency Severity Index Handbook*, Emergency Nurses Association.
4. cppreference.com, C++ language and standard library reference.
5. Qt Documentation, doc.qt.io (planned Phase III interface).
