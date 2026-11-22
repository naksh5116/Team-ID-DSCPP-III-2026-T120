# VitalRoute GUI: notes for the viva

Short and concrete. Read with the code open; file names are clickable on
GitHub. GUI lead: Devesh Saini.

## 1. Why the HospitalSession split was needed

Before the GUI, `src/main.cpp` held one class, `VitalRouteApp`, that mixed
two jobs:
- **the logic:** seeding, treatment, undo, save;
- **the console I/O:** `std::cin` prompts and `std::cout` output.

A GUI cannot reuse code that reads stdin in the middle of an action.

We moved the logic, unchanged, into `src/app/HospitalSession.{hpp,cpp}`:

- **Plain values in and out.** Methods take plain values and return values or result structs (`TreatResult`, `UndoResult`, `BedRelease`).
- **Errors are the existing exceptions** (`PatientNotFoundException`, ...). Nothing in it prints or reads input, and it does not depend on Qt.
- **Some console actions ask questions half-way through.** An outpatient's diagnosis comes after the doctor is booked; insurance comes after the bed is freed at discharge. These became two calls (`beginOpd` / `completeOpd`, `prepareDischarge` / `settleDischarge`), so the console keeps its exact prompt order.
- **`main.cpp` is now a thin console wrapper.** It only prompts and prints.

How we proved nothing changed:
- `tests/regression.sh` replays 8 scripted console sessions and compares them with output recorded from the **original** program before the split. They are still identical.
- The 17 original tests (`make test`) still pass.

Rules kept: no file in `src/engine`, `src/model` or `src/modules` was
changed, and no hand-written data structure was replaced.

## 2. How the windows stay in sync (SessionHub)

- **One process, one session.** There is one `HospitalSession`, owned by `src/gui/SessionHub`, a `QObject`. Every window holds a reference to the same hub.
- **Every change goes through the hub**, for example `hub.registerEmergency(...)`. After the change, the hub emits only the signals for what changed:

| Change | Signals |
|---|---|
| Register a patient | `queueChanged`, `patientsChanged`, `billsChanged` |
| Treat next patient | the three above + `bedsChanged`, `staffChanged`, then `routeAssigned(id)` if a bed was given |
| Visit note | `patientsChanged` |
| Theatre book / release | `theatresChanged` (+ `billsChanged` on booking) |
| Transfer desk | `transfersChanged` (+ `billsChanged` for a charged ambulance run) |
| Manual charge, insurance | `billsChanged` |
| Discharge | `patientsChanged`, `bedsChanged`, `billsChanged` |
| Undo, save, reload | all of them (+ `saved` / `reloaded`) |

- **Windows only listen.** Each window connects only to the signals it needs. The Doctor's queue table listens to `queueChanged`; a Patient window listens to `routeAssigned` and checks whether the ID is its own.
- **No polling and no timers.** A window never asks "has anything changed?"; it is told.
- **Exceptions still notify.** Signals are emitted by a small guard object, so they go out even when the session throws. No window can be left showing half a change.
- **Reload is safe.** It builds a complete new session before swapping it in, so a bad record file leaves the running session untouched.

## 3. Which screen uses which module and data structure

| Screen / panel | Module (src/modules) | Data structure (src/engine) |
|---|---|---|
| Nurse: emergency queue | `TriageUnit` | max-heap `vr_pqueue` (ESI, then arrival) |
| Nurse: OPD line | `TriageUnit` | FIFO `vr_queue` |
| Nurse: transfer desk | `TransferDesk` | deque `vr_deque` (urgent at front, routine at back) |
| Doctor: treat next patient | `TriageUnit`, `ResourceRouter`, `StaffRegistry`, `BillingEngine` | heap; Dijkstra on `vr_graph` with a `vr_pqueue` frontier |
| Doctor and Patient: route map and steps | `HospitalMap`, `Navigator` | weighted graph `vr_graph`, Dijkstra |
| Doctor: patient record | `PatientRegistry` | hash table `vr_hash` (ID lookup); doubly linked list `vr_dlist` (visit history, newest first by walking backwards) |
| Doctor: operation theatres | `OTScheduler` | circular queue `vr_cqueue` of free slots |
| Doctor: next available doctor | `StaffRegistry` | sorted array, merge sort + lower-bound binary search (`vr_sort`) |
| Super Admin: admissions by date | `PatientRegistry` | AVL tree `vr_avl` range walk |
| Super Admin: all bills | `BillingEngine` | merge sort (`vr_sort`) |
| Super Admin: undo | `HospitalSession` | stack `vr_stack` |
| Super Admin: indexes | `PatientRegistry` | `vr_hash` stats, `vr_avl` height and rotations |
| Bills everywhere | `BillingEngine`, `Bill` | polymorphic `Charge` subclasses (OOP) |

The tables show what the structures hold, in the structures' own order.
They never sort or store patients themselves (`SnapshotTableModel`).

## 4. How the route map converts metres to pixels

The floor plan stores landmarks in metres, with y growing north
(`data/hospital_graph.txt`, loaded into `HospitalMap`). `FloorPlanWidget`
converts them in four steps:

1. **One bounding box for all floors:** `minX..maxX`, `minY..maxY`, plus a 4 m margin. Because it is shared, a lift shaft is at the same place on every floor tab.
2. **Scale:** `scale = min(plotWidth / (maxX - minX), plotHeight / (maxY - minY))` pixels per metre. The smaller ratio keeps the whole plan visible without distortion.
3. **x:** `px = left + (x - minX) * scale`.
4. **y:** `py = top + (maxY - y) * scale`. The y is flipped because north is up on screen, but pixel rows count downwards.

Worked example. On a plan 78 m wide, in a 520 px plot, the scale is
520 / 78 = 6.67 px per metre. The ER (0, 0) and the triage desk (10, 0) are
then 66.7 px apart. The 10 m scale bar under the map is exactly that length.

Step highlighting: `RouteSegments` calls `Navigator::describe()` on each
prefix of the path. It works left to right, so the last step before
"Arrive" in each prefix is the step that holds the prefix's last edge.
Clicking a step draws its edges wider, or rings the landmark for a turn.

## 5. Qt concepts used

- **Signals and slots.** `SessionHub` emits `queueChanged()` and other signals; models and windows connect slots (often lambdas) to them. `Q_OBJECT` plus the moc code generator (CMake `AUTOMOC`) makes this work.
- **Models and views.** Every table is a `QTableView` showing a model derived from `QAbstractTableModel`. On a signal the model calls `beginResetModel()` / `endResetModel()` and the view redraws. Delegates customise a cell: `EsiDelegate` paints the ESI badge.
- **QPainter.** `FloorPlanWidget::paintEvent` draws corridors (`drawLine`), landmarks (`drawRect`), labels (`drawText`) and the route (a wider accent `QPen`). `VrStyle` (a `QProxyStyle`) paints check boxes. Everything is in logical coordinates with antialiasing.
- **Multi-screen handling.**
  - `QGuiApplication::screens()` gives each monitor's `availableGeometry()`.
  - `ScreenArranger::plan()` decides where each window goes. It is pure, so it can be tested with made-up screen layouts.
  - `QWindow::setScreen()` and `showMaximized()` / `showFullScreen()` place the windows.
  - The `screenAdded` / `screenRemoved` signals keep windows reachable when a projector is unplugged.
  - High-DPI: Qt 6 scales logical pixels per monitor, and we set the `PassThrough` rounding policy so 125 % and 150 % stay sharp.
- **Stylesheet with tokens.** `style.qss` uses names like `@accent`; `Theme.cpp` replaces them with the DESIGN.md colours at start-up.

## 6. Questions an examiner might ask

- **Why not Qt containers or Qt's own sorting?** The syllabus requires our own structures. Qt is only the presentation layer; every queue, tree, table and graph is still the C engine.
- **Is the role selector a login?** No. It is a prototype role selector, labelled as such. There are no passwords or user accounts.
- **What happens if two windows act at once?** It is one thread and one event loop, so actions run one after another and each finishes (and notifies) before the next starts.
- **What is not saved?** Theatre bookings and transfer requests. `PersistenceManager` does not store them, in the console or the GUI.
- **How do you know the GUI matches the console?**
  - `docs/GUI_TEST.md` lists every action with the console equivalent.
  - 104 automated GUI checks compare messages with the console's sentences.
  - The restart check shows the console reading files the GUI saved.

## 7. The web version, compared with the GUI

The web version (`docs/WEB.md`) is a third front end over the same
`HospitalSession`, added after the GUI. It is a separate program
(`bin/vitalroute_web`); the GUI and the console still have no network code.

| | Qt GUI | Web version |
|---|---|---|
| Who holds the session | `SessionHub` in the GUI process | `WebHub` in the server process |
| How screens hear about changes | Qt signals (`queueChanged`, ...) inside one process | The same event names, sent to every browser over a Server-Sent Events stream |
| Screens | Windows on the computer's monitors | Browser tabs on any device on the network |
| Map | `QPainter` on a `QWidget`, metres to pixels by hand | SVG with a `viewBox` in metres; the browser scales it |
| Shared code | `src/app/RouteSteps`, `src/app/TheatreSchedule` | the same files |

- **If asked "why not use the GUI over the network?"** Qt Widgets is built to draw on the computer's own screens. Showing it on phones would need a remote-desktop style tool (one shared picture, not separate role screens per device), or rewriting the screens for a browser, which is what the web version does.
- **Same rules.** Both front ends call the same session methods, so the messages, refusals and data structures are identical. The web checks compare the web replies with the console's sentences too.
