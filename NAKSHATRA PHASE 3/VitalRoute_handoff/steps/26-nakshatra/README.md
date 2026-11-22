# VitalRoute

**Team ID:** DSCPP-III-2026-T120 · 3rd semester Project-Based Learning
**Courses:** Data Structures in C · Object-Oriented Programming in C++

VitalRoute is a console-based hospital management and emergency routing
simulator. It triages emergency and outpatient arrivals, sends each admitted
patient to the nearest free bed *of the right clinical type*, prints
turn-by-turn walking directions from the Central Emergency Room to that bed
across a three-floor hospital, schedules operation theatres, queues ambulance
and transfer requests, bills everything that is used (with insurance), and
saves all records to plain text files that are reloaded on the next start.

It is a simulator: every patient, doctor and floor plan is sample data. There
is no database engine, no network access and no third-party library - only
the C standard library and the C++ standard library. (Two optional front ends
sit on top: a Qt 6 GUI, and a separate web version that is the only part
using the network - see "GUI" and "Web version" below.)

## Two language layers

| Layer | Language | Where | Rule |
|---|---|---|---|
| Engine | C11 (`gcc -std=c11`) | `src/engine/` | Every data structure is hand-written. No C++ at all. |
| Model | C++17 (`g++ -std=c++17`) | `src/model/` | Domain classes, inheritance, polymorphic charges, exceptions. |
| Modules | C++17 | `src/modules/`, `src/app/`, `src/main.cpp` | Application logic built on the engine; STL only for bookkeeping. |

Every engine header is wrapped in `extern "C" { ... }` under `__cplusplus`,
so the C++ layers include them directly and link against the C objects.

## Build and run

Requirements: `gcc`, `g++` (GCC 9 or newer) and GNU `make`.

```sh
make            # builds bin/vitalroute
make test       # builds and runs both test suites (one PASS/FAIL line per test)
make clean      # removes build/ and bin/ (saved records are kept)
./bin/vitalroute
```

Run the program from the repository root; it reads `data/`. Another data
directory can be passed as the first argument: `./bin/vitalroute path/to/data`.

**Windows:** use the MSYS2 **UCRT64** shell (`pacman -S make mingw-w64-ucrt-x86_64-gcc`)
and run `make` there. Mixing Git Bash with MSYS2's `make` loses the `TMP`
variable and gcc fails with "Cannot create temporary file".

**Memory checking:** on Linux, `valgrind --leak-check=full ./bin/vitalroute`.

### First run

On the first start there are no saved records, so VitalRoute loads the staff
roster from `data/doctors.txt` and seeds 11 patients:

* 3 earlier admissions, already discharged with settled bills (IDs `...000001`-`...000003`);
* 6 emergencies waiting, ESI levels 1, 4, 2, 1, 3, 5 in arrival order (`...000004`-`...000009`);
* 2 outpatients waiting (`...000010`, `...000011`), one returning with an earlier visit history.

Choose **3** to treat the next patient: the two ESI-1 cases leave the heap
first, each is given an ICU bed and the route is printed. Choose **12** to
save; the next start reloads everything from `data/records/`.

## GUI

A Qt 6 Widgets front end over the same logic, for a multi-screen demo. GUI lead: Devesh Saini.

- **One process, many windows.** A Launcher opens role windows (Nurse, Doctor, Patient, Super Admin), one per screen.
- **Instant sync.** An action in one window appears in the others at once.
- **Route map.** The route to the patient's bed is drawn on a floor-plan map.

The console program is unchanged and reads and writes the same `data/records/` files. The GUI uses only Qt 6 Core, Gui and Widgets: no QML, no web views, no networking.

| Document | What it is |
|---|---|
| [docs/DEMO.md](docs/DEMO.md) | Step-by-step multi-screen demo (and a one-screen fallback) |
| [docs/GUI_NOTES.md](docs/GUI_NOTES.md) | How it works, for the viva |
| [docs/GUI_TEST.md](docs/GUI_TEST.md) | Manual test checklist with recorded results |
| [DESIGN.md](DESIGN.md) | Colours, type, spacing and components |

### Build on Windows (MSYS2 UCRT64)

In the **MSYS2 UCRT64** shell, once:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

Then, from the repository root:

```sh
cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui
./build-gui/vitalroute_gui.exe
```

The GUI finds `data/` the same way the console does: the first argument,
then `./data`, `../data`, next to the executable, then the source tree.
Starting it from the repository root is simplest.

To run it outside the MSYS2 shell (from PowerShell, Explorer or another
PC), Windows must find the Qt DLLs. Either put `C:\msys64\ucrt64\bin` on
`PATH`, or copy the DLLs next to the program once:

```sh
windeployqt6 build-gui/vitalroute_gui.exe
```

### Build on Linux

Install a C/C++ compiler, CMake, Ninja and the Qt 6 base development
package. For example, on Debian or Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build qt6-base-dev
```

On Fedora:

```sh
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel
```

Then:

```sh
cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui
./build-gui/vitalroute_gui
```

The CMake file needs Qt 6.2 or newer. The GUI was built and tested on
Windows (Qt 6.10.1); the Linux steps above have not been tried yet.

### Using it

1. The Launcher opens. It is a prototype role selector, not a login: each
   button opens a new window for that role, and the same role can be opened
   more than once (for example two Patient windows).
2. **Arrange on screens** places one window per screen in the order Nurse,
   Doctor, Patient, Super Admin. With one screen it tiles them 2 x 2.
3. Keys:

   | Key | Action |
   |---|---|
   | Enter | Submits the form you are in |
   | Ctrl+S | Saves |
   | F5 | Refreshes every table |
   | F11 | Full screen (a Patient window becomes a wall display of the route) |

4. Closing a role window never quits. Closing the Launcher asks once whether
   to save, then closes everything.

### GUI checks

The automated checks open the real windows on temporary copies of `data/`,
fill the forms, press the buttons and keys, and compare messages with the
console's (104 checks):

```sh
ctest --test-dir build-gui --output-on-failure     # off screen, no windows appear
sh tests/run_gui_check.sh --shots shots/            # also saves screenshots
sh tests/restart_check.sh                           # save in the GUI, restart, compare (as T8)
make regression                                     # console output unchanged since the GUI work
```

## Web version

The same hospital as a website, for phones, tablets and laptops on the same
network. Every open page works on one shared session, and changes appear on
all devices at once.

```sh
make web                              # builds bin/vitalroute_web (no extra libraries)
./bin/vitalroute_web                  # prints http://localhost:8080/ and the network link(s)
./bin/vitalroute_web --local-only     # this computer only
```

On Windows, double-click `run_web.bat`. It starts the server and opens the
Launcher in the browser.

- **Separate program.** It uses the same `HospitalSession` and the same hand-written data structures. The console program and the Qt GUI are unchanged and have no network code.
- **Plain C++ sockets.** The HTTP server, JSON and live updates (Server-Sent Events) are written by hand in `src/web/`; the pages in `web/` are plain HTML, CSS and JavaScript with no frameworks.
- **No sign-in.** Anyone who can open the link can change the data, so use a trusted network or `--local-only`. Do not run the GUI and the web server on the same data folder at the same time.

Details, the API and viva notes: [docs/WEB.md](docs/WEB.md). Checks (51):
`sh tests/web_check.sh`.

## The menu

```
=========== VitalRoute ===========
 1. Register patient (emergency)
 2. Register patient (OPD)
 3. Treat next patient          <- pops the heap, routes, prints directions
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

### Sample route (option 3 on an ESI-1 patient)

```
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

## Module map

```
src/
├── engine/                       PURE C - the data structures
│   ├── vr_stack.c/.h             growable stack            -> undo journal
│   ├── vr_queue.c/.h             linked FIFO queue          -> OPD line
│   ├── vr_pqueue.c/.h            binary max-heap            -> triage, Dijkstra frontier
│   ├── vr_cqueue.c/.h            circular queue + count     -> OT free-slot ring
│   ├── vr_deque.c/.h             circular-array deque       -> transfer desk
│   ├── vr_dlist.c/.h             doubly linked list         -> visit history
│   ├── vr_hash.c/.h              chained hash table         -> patient ID index
│   ├── vr_avl.c/.h               AVL tree                   -> admission-date index
│   ├── vr_graph.c/.h             adjacency list + Dijkstra  -> floor plan
│   └── vr_sort.c/.h              merge sort, lower bound    -> listings, doctor search
├── model/                        C++ domain classes
│   ├── Person                    abstract base (+ date/clock helpers)
│   ├── Patient                   Person subclass; owns a vr_dlist of visits
│   ├── Staff                     Staff -> Doctor, Nurse, Admin
│   ├── Charge                    abstract; Bed/Medicine/Test/ProcedureCharge
│   ├── Bill                      operator+=, operator<<, one-time insurance
│   ├── Insurance                 flat or percentage (with cap)
│   └── Exceptions.hpp            five std::runtime_error subclasses
├── modules/                      C++ application logic
│   ├── PatientRegistry           hash + AVL indexes, owns patients
│   ├── StaffRegistry             roster, next-available-doctor search
│   ├── TriageUnit                emergency heap, OPD queue
│   ├── HospitalMap               loads the floor plan into vr_graph
│   ├── ResourceRouter            type filter first, then Dijkstra distance
│   ├── Navigator                 turn-by-turn direction generator
│   ├── OTScheduler               per-theatre slot ring
│   ├── TransferDesk              urgent front / routine back
│   ├── BillingEngine             charges, manual entries, sorted listings
│   └── PersistenceManager        fstream records, index rebuild on load
├── app/
│   └── HospitalSession           all application logic and the undo journal (no I/O)
├── gui/                          Qt 6 Widgets front end (see the GUI section)
└── main.cpp                      console menu: prompts and printing over HospitalSession
```

`HospitalMap` is the one file beyond the original layout: the `HospitalMap`
class had no designated file, and both the router and the navigator need it.

## Syllabus coverage

### Data Structures in C

| # | Structure | Implementation | Used by |
|---|---|---|---|
| 1 | Stack (push, pop, peek, is_empty, growth) | `src/engine/vr_stack.c` | Undo journal in `src/app/HospitalSession.cpp` (`undoLast`) |
| 2 | Queue (FIFO) | `src/engine/vr_queue.c` | OPD line in `src/modules/TriageUnit.cpp` |
| 3 | Priority queue (max-heap, insert, extract_max, peek, heapify) | `src/engine/vr_pqueue.c` | Triage in `TriageUnit.cpp`; Dijkstra in `vr_graph.c`; `heapify` rebuilds the waiting list on load |
| 4 | Circular queue (modulo wrap, `count` field) | `src/engine/vr_cqueue.c` | Free-slot ring per theatre in `src/modules/OTScheduler.cpp` |
| 5 | Double-ended queue | `src/engine/vr_deque.c` | `src/modules/TransferDesk.cpp` |
| 6 | Doubly linked list | `src/engine/vr_dlist.c` | Visit history in `src/model/Patient.cpp` (newest-first by backward walk) |
| 7 | Hash table (chaining, load-factor resize, collision count) | `src/engine/vr_hash.c` | Patient ID index in `src/modules/PatientRegistry.cpp`; stats shown by option 4 |
| 8 | AVL tree (LL, RR, LR, RL, range walk) | `src/engine/vr_avl.c` | Admission-date index in `PatientRegistry.cpp`; option 5 |
| 9 | Weighted graph + Dijkstra | `src/engine/vr_graph.c` | `HospitalMap`, `ResourceRouter`, `Navigator` |
| 10 | Merge sort (by date, by amount) | `src/engine/vr_sort.c` | Bill listings in `BillingEngine.cpp`; patient listings in `PatientRegistry.cpp`; staff availability rebuild |
| 11 | Binary search (lower bound) | `src/engine/vr_sort.c` | Next available doctor in `src/modules/StaffRegistry.cpp` |

### Object-Oriented Programming in C++

| # | Concept | Where |
|---|---|---|
| 1 | Classes and objects | `Patient`, `Doctor`, `Nurse`, `Admin`, `Bill`, `Insurance`, `HospitalMap` and every module |
| 2 | Inheritance | `Person` -> `Patient`, `Staff`; `Staff` -> `Doctor`, `Nurse`, `Admin` (`src/model/`) |
| 3 | Polymorphism / virtual functions | `Charge::calculateCost() const = 0`, overridden by `BedCharge`, `MedicineCharge`, `TestCharge`, `ProcedureCharge`; `Bill` and `BillingEngine` only hold `Charge` pointers. Also `Person::role/describe`, `Staff::duty/canPrescribe/canAuthorizeCharges`. Virtual destructors on every polymorphic base. |
| 4 | Operator overloading | `Bill& Bill::operator+=(const Charge&)`, `bool Patient::operator<(const Patient&) const` (triage order), `operator<<` for `Bill`, `Route` and `Person` |
| 5 | Exception handling | `BedUnavailableException`, `PatientNotFoundException`, `InvalidSeverityException`, `NoRouteFoundException`, `SlotUnavailableException` in `src/model/Exceptions.hpp`, thrown by `HospitalSession`, caught in `main.cpp` (console) and `RoleWindow::attempt` (GUI) |
| 6 | File handling | `src/modules/PersistenceManager.cpp` (`ofstream`/`ifstream`); `HospitalMap::load`, `ResourceRouter::loadBeds`, `StaffRegistry::loadRoster` |
| 7 | STL | `vector`, `map`, `string`, `optional`, `algorithm` (`std::sort`, `std::find`, `std::clamp`) - in the C++ layers only, never replacing a structure above |

## How the headline features work

**Triage.** Severity follows the Emergency Severity Index (1 = most
critical). Emergencies go into the max-heap keyed on `(6 - severity,
-arrival_seq)`, so level 1 leaves first and equal levels leave in arrival
order. Severities outside 1-5 raise `InvalidSeverityException`. ESI 1-2 need
an ICU bed, ESI 3 a ward bed, ESI 4-5 are treated in the ER and released.
Outpatients wait in the FIFO queue and are seen once no emergency is waiting.

**Bed selection - two stages, in this order.** `ResourceRouter::allocate`
first keeps only free beds of the required clinical unit, then runs Dijkstra
from the Central Emergency Room and takes the nearest of those. The ground
floor Acute Medical Unit is far closer to the ER than any ICU bed, so if the
stages were reversed an ICU patient would be sent to a ward bed; test T5
checks exactly that.

**Directions.** `Navigator` takes the Dijkstra path and, for each same-floor
segment, computes `atan2(dy, dx)` in degrees. At each intermediate node the
change of bearing is folded into (-180, 180] and classified: under 20 degrees
straight, 20-60 bear slightly, 60-135 turn, over 135 turn sharply; positive is
left. Lift and stair edges become one "Take the lift/stairs to floor n" line
and bearings restart on the new floor. Straight runs merge into "Walk N m",
turns name the landmark, and the total distance gives the walking time at
1.2 m/s (lift rides are not counted as walking).

**Operation theatres.** Each theatre has eight one-hour slots and a circular
queue of free slot numbers. Booking dequeues the next free slot in cycle
order; releasing enqueues it, so it can be reused at once without moving any
other slot. `count` tells a full ring (all slots free) from an empty one
(fully booked).

**Billing.** Consultations, tests, beds, surgery and ambulance runs are
charged automatically as they happen (`bill += charge`). Administrators can
add manual charges of any type. Insurance (flat, or a percentage with an
optional cap) is applied once, when the bill is settled, and never takes the
total below zero. Option 11 undoes the last registration or manual charge.

**Persistence.** Option 12 writes `patients.dat`, `staff.dat`, `visits.dat` and
`bills.dat` under `data/records/`. On start-up they are read back, every
patient is re-registered (rebuilding the hash and AVL indexes), the waiting
lists are rebuilt with `heapify`, and occupied beds are marked again.

## Data files

`data/hospital_graph.txt` holds 36 landmarks on three floors. The ground floor
has the Central Emergency Room, Reception, the Acute Medical Unit, two lifts
and a stairwell; floor 1 has ICU-1, ICU-2 and two operation theatres; floor 2
has three general wards.

```
N <id> <floor> <x> <y> <type> <name with spaces>    # metres, y grows north
E <from> <to> <weight> <type> [D]                     # undirected unless D
```

`data/resources.txt` lists beds (`BED <code> <node> <number> <ICU|WARD> <L|R>`)
and theatres (`OT <name> <node> <slots> <first hour>`). `data/doctors.txt` is
the starting staff roster.

## Tests

`make test` runs `tests/run_tests.sh`, which builds and runs:

* `tests/test_engine.c` (C):
  * T1 priority queue ordering
  * T2 circular queue wrap-around
  * T3 AVL balance on 1000 sorted dates, with all four rotations
  * T4 hash lookup with forced collisions
  * X1-X5 stack, queue, deque, doubly linked list, and merge sort with lower bound
* `tests/test_scenarios.cpp` (C++):
  * T5 triage order and bed assignment, with the ICU-versus-nearer-ward regression
  * T6 turn-by-turn directions, including lift and stairs
  * T7 billing with insurance against a hand-calculated total
  * T8 full journey: register, triage, route, consult, bill, discharge, save, reload and compare
  * X6-X9 theatres, transfer desk, doctor search, and undo/visit ordering

`make regression` replays scripted console sessions (`tests/regression/*.in`)
against a fresh copy of `data/`, and compares them with output recorded from
the program before the GUI work began. Clock times and dates are masked, so
the expected files stay valid on any day.

The GUI has its own checks (`tests/gui/`, run by `ctest`); see the GUI section
above. The web version has `tests/web_check.sh`; see "Web version".

## Team

| Member | Owns |
|---|---|
| Nakshatra Agarwal (`naksh5116`), team lead | Hospital graph, severity triage, Dijkstra bed selection, direction generator, integration |
| Devesh Saini, GUI lead | `Person`/`Patient`/`Staff` hierarchy, `Charge` hierarchy, billing with operator overloading and insurance; lead of the Qt GUI and the web pages |
| Vasav Sharma | Visit records, hash index, AVL date-range search, file persistence, test suite (including the GUI and web checks) |

See `COMMITS.md` for the commit plan each member follows.
