# Commit plan

Every member makes their own commits from their own GitHub account, so each
of us shows up as a contributor. This file lists every commit in the order it
must be made: who makes it, on which branch, the exact message, and the files
to stage. The eleven commits cited in our report keep their messages word for
word (column "Report #"). A few additional commits carry files the report
table does not mention; they are marked "additional".

The order matters. Each step only compiles on top of the ones before it, and
every step was checked to build without warnings and pass `make test` when
applied in this order.

Steps 12-26 add the two front ends built after the report's commits: the Qt 6
GUI (branch `feature/gui`) and the web version (branch `feature/web`). They
are all "additional". Their files come in a second handoff package (see
below). Owners follow the roles in our Phase-III report and slides:
* **Devesh is the GUI lead** ("Shared: GUI (lead: Devesh)"), so Devesh takes most GUI steps, plus the web pages.
* **Nakshatra** (graph, navigation, integration) takes the session split, the route map, and the web server and API.
* **Vasav** (records, indexes, persistence, tests) takes the record and index screens and every test step.

The report and slides do not mention the web version; its steps were assigned
by the same roles. They were checked the same way, by replaying them one by one on top
of the state after step M4:
* every step builds without warnings and passes `make test`;
* from step 12a, `make regression` reports the console output unchanged;
* from step 19, the GUI builds and its checks pass;
* from step 23, `make web` builds;
* from step 25, the web checks pass.

## Before your first step (Devesh and Vasav)

1. **Get push access.** Nakshatra adds you on GitHub under *Settings ->
   Collaborators -> Add people*; accept the invitation email.
2. **Get the handoff package.** Nakshatra shares `VitalRoute_handoff.zip`.
   Unzip it **next to** your clone:

   ```
   some-folder/
   ├── Team-ID-DSCPP-III-2026-T120/     <- your clone
   └── VitalRoute_handoff/
       └── steps/
           ├── 02b-vasav/
           ├── 03-vasav/
           └── ...
   ```

   Each `steps/<step>-<owner>/` folder holds exactly the files for that step,
   laid out as they sit in the repository.

   For steps 00c and 12-26, Nakshatra also shares
   `VitalRoute_handoff_part2.zip`. Unzip it in the **same** `some-folder/`:
   it adds `00c-nakshatra/` and `12-nakshatra/` to `26-nakshatra/` to the same
   `VitalRoute_handoff/steps/` folder. A file can appear in more than one
   step folder. `Makefile`, `README.md`, `docs/DEMO.md` and
   `docs/GUI_NOTES.md` grow in stages, so always copy from your own step's
   folder.
3. **Clone and set your identity** - use the email address on your GitHub
   account, otherwise the commit is not linked to your profile:

   ```bash
   git clone https://github.com/naksh5116/Team-ID-DSCPP-III-2026-T120.git
   cd Team-ID-DSCPP-III-2026-T120
   git config user.name  "Your Name"
   git config user.email "the-email-on-your-github-account@example.com"
   ```

## Rules for every step

* Wait until the previous step is pushed, then `git pull` before you start.
* Stage only the files listed for your step (no `git add .`).
* Use the commit message exactly as written.
* `make && make test` should pass before you commit. On Windows, run it in
  the MSYS2 UCRT64 shell; if you have no compiler set up, you can skip this
  line - the step has already been verified.
* Each commit also records the co-author trailer shown in its command.
* Nakshatra merges each feature branch into `main` with `--no-ff` once its
  last step is pushed, and creates the next branch.
* Steps 12-26 add more checks. Run each check only if you have the tools for
  it; every step has already been verified.
  * `make regression`, from step 12a on.
  * The GUI build and its checks, from step 19 on. This needs Qt 6 (README, "GUI").
  * `make web`, from step 23 on.
  * `sh tests/web_check.sh`, from step 25 on (MSYS2 or Linux shell, needs `curl`).
* Steps 13-18 add parts of the GUI that only build once step 19 adds the
  Launcher and `CMakeLists.txt`. `make && make test` passes at every one of
  them, because the console build does not compile the GUI.

## Order at a glance

| Step | Report # | Owner | Branch | Commit message | Status |
|---|---|---|---|---|---|
| 00 | - | Nakshatra | `main` | `chore: project skeleton and Makefile` | done |
| 00b | - | Nakshatra | `main` | `docs: README and team commit plan` | done |
| 00c | - | Nakshatra | `main` | `docs: extend the commit plan with the GUI and web version` | to do (can be done now) |
| 01 | 1 | Nakshatra | `feature/engine` | `feat(engine): priority queue keyed on severity with arrival-time tie-break` | done |
| 02 | 2 | Nakshatra | `feature/engine` | `feat(engine): circular queue for OT slots and deque for transfer requests` | done |
| 02a | - | Nakshatra | `feature/engine` | `feat(engine): stack for undo, FIFO queue for OPD, merge sort and lower-bound search` | done |
| 02b | - | Vasav | `feature/engine` | `feat(engine): doubly linked list, hash table and AVL tree for patient records` | to do |
| 03 | 3 | Vasav | `feature/engine` | `test(engine): unit tests for the queue family, AVL rotations and hash collisions` | to do |
| M1 | - | Nakshatra | main | *merge `feature/engine` into `main` (`--no-ff`), then create `feature/triage`* | to do |
| 04 | 4 | Devesh | `feature/triage` | `feat(model): Person, Patient and Staff hierarchy with role behaviour` | to do |
| 05 | 5 | Vasav | `feature/triage` | `feat(records): hash index for constant-time lookup by patient identifier` | to do |
| 05a | - | Nakshatra | `feature/triage` | `feat(triage): emergency heap and OPD queue behind the triage unit` | to do |
| 05b | - | Nakshatra | `feature/triage` | `feat(ops): next-available doctor search, OT slot ring and transfer desk` | to do |
| M2 | - | Nakshatra | main | *merge `feature/triage` into `main` (`--no-ff`), then create `feature/navigation`* | to do |
| 06 | 6 | Nakshatra | `feature/navigation` | `feat(graph): hospital layout loader with floors, coordinates and edge types` | to do |
| 07 | 7 | Nakshatra | `feature/navigation` | `feat(routing): Dijkstra selection of the nearest free bed` | to do |
| 08 | 8 | Nakshatra | `feature/navigation` | `fix(routing): filter candidates by clinical type before comparing distance` | to do |
| 09 | 9 | Nakshatra | `feature/navigation` | `feat(nav): turn-by-turn direction generator with lift and stairway steps` | to do |
| M3 | - | Nakshatra | main | *merge `feature/navigation` into `main` (`--no-ff`), then create `feature/billing`* | to do |
| 10 | 10 | Devesh | `feature/billing` | `feat(billing): Charge hierarchy, operator+= accumulation, insurance deduction` | to do |
| 11 | 11 | Vasav | `feature/billing` | `feat(persistence): fstream record writer and index rebuild on startup` | to do |
| 11a | - | Vasav | `feature/billing` | `test(scenarios): triage, routing, navigation, billing and full-journey tests` | to do |
| 11b | - | Nakshatra | `feature/billing` | `feat(app): console menu wiring every module, with seeded sample data` | to do |
| M4 | - | Nakshatra | main | *merge `feature/billing` into `main` (`--no-ff`), then create `feature/gui`* | to do |
| 12 | - | Nakshatra | `feature/gui` | `refactor(app): move console logic into HospitalSession with no I/O` | to do |
| 12a | - | Vasav | `feature/gui` | `test(regression): replay recorded console sessions to prove output is unchanged` | to do |
| 13 | - | Devesh | `feature/gui` | `feat(gui): session hub, theme, style and shared widgets for the Qt front end` | to do |
| 14 | - | Devesh | `feature/gui` | `feat(gui): Nurse window with registration, insurance and transfer desk` | to do |
| 15 | - | Nakshatra | `feature/gui` | `feat(gui): floor-plan route map with numbered walking steps` | to do |
| 16 | - | Devesh | `feature/gui` | `feat(gui): Doctor window with treatment, theatres and doctor search` | to do |
| 16a | - | Vasav | `feature/gui` | `feat(gui): patient record view with visit history and ID lookup` | to do |
| 16b | - | Vasav | `feature/gui` | `feat(gui): admissions-by-date, staff roster and index statistics panels` | to do |
| 17 | - | Devesh | `feature/gui` | `feat(gui): Patient window with wall display, and Super Admin billing and discharge` | to do |
| 18 | - | Vasav | `feature/gui` | `test(gui): automated checks that drive the real windows on copies of the data` | to do |
| 19 | - | Devesh | `feature/gui` | `feat(gui): Launcher, role factory and CMake build for the Qt GUI` | to do |
| 20 | - | Devesh | `feature/gui` | `docs(gui): demo script, viva notes, test checklist and screenshots` | to do |
| M5 | - | Nakshatra | main | *merge `feature/gui` into `main` (`--no-ff`), then create `feature/web`* | to do |
| 21 | - | Nakshatra | `feature/web` | `feat(web): hand-written HTTP/1.1 server with a server-sent event stream` | to do |
| 22 | - | Nakshatra | `feature/web` | `feat(web): JSON API and live change events over the shared session` | to do |
| 23 | - | Nakshatra | `feature/web` | `feat(web): vitalroute_web program, make web target and run_web.bat` | to do |
| 24 | - | Devesh | `feature/web` | `feat(web): role pages in plain JavaScript with live updates and an SVG route map` | to do |
| 25 | - | Vasav | `feature/web` | `test(web): end-to-end checks of the server, pages, API and live events` | to do |
| 26 | - | Nakshatra | `feature/web` | `docs(web): web version guide, README section and demo fallback` | to do |
| M6 | - | Nakshatra | main | *merge `feature/web` into `main` (`--no-ff`)* | to do |

## Commands for each remaining step

Run these from the root of your clone, in Git Bash (Windows) or any POSIX shell,
with the handoff package unpacked next to the clone (see above).

### Step 00c - Nakshatra - `main` (additional - can be done now)

This updated plan, so that everyone pulls the new steps.

```bash
git fetch
git switch main
git pull
cp -r ../VitalRoute_handoff/steps/00c-nakshatra/. .
git add \
    COMMITS.md
git commit -m "docs: extend the commit plan with the GUI and web version" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

Feature branches made before this step pick it up when they are merged into
`main`. Until then, read the plan on `main`.

### Step 02b - Vasav - `feature/engine` (additional - not in the report table)

```bash
git fetch
git switch feature/engine
git pull
cp -r ../VitalRoute_handoff/steps/02b-vasav/. .
git add \
    src/engine/vr_dlist.h \
    src/engine/vr_dlist.c \
    src/engine/vr_hash.h \
    src/engine/vr_hash.c \
    src/engine/vr_avl.h \
    src/engine/vr_avl.c
make && make test
git commit -m "feat(engine): doubly linked list, hash table and AVL tree for patient records" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 03 - Vasav - `feature/engine`

```bash
git fetch
git switch feature/engine
git pull
cp -r ../VitalRoute_handoff/steps/03-vasav/. .
git add \
    tests/test_engine.c \
    tests/run_tests.sh
git update-index --chmod=+x tests/run_tests.sh
make && make test
git commit -m "test(engine): unit tests for the queue family, AVL rotations and hash collisions" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step M1 - Nakshatra - merge `feature/engine`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/engine -m "Merge branch 'feature/engine'"
make && make test
git push
git switch -c feature/triage
git push -u origin feature/triage
```

### Step 04 - Devesh - `feature/triage`

```bash
git fetch
git switch feature/triage
git pull
cp -r ../VitalRoute_handoff/steps/04-devesh/. .
git add \
    src/model/Exceptions.hpp \
    src/model/Person.hpp \
    src/model/Person.cpp \
    src/model/Patient.hpp \
    src/model/Patient.cpp \
    src/model/Staff.hpp \
    src/model/Staff.cpp
make && make test
git commit -m "feat(model): Person, Patient and Staff hierarchy with role behaviour" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 05 - Vasav - `feature/triage`

```bash
git fetch
git switch feature/triage
git pull
cp -r ../VitalRoute_handoff/steps/05-vasav/. .
git add \
    src/modules/PatientRegistry.hpp \
    src/modules/PatientRegistry.cpp
make && make test
git commit -m "feat(records): hash index for constant-time lookup by patient identifier" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 05a - Nakshatra - `feature/triage` (additional - not in the report table)

```bash
git fetch
git switch feature/triage
git pull
cp -r ../VitalRoute_handoff/steps/05a-nakshatra/. .
git add \
    src/modules/TriageUnit.hpp \
    src/modules/TriageUnit.cpp
make && make test
git commit -m "feat(triage): emergency heap and OPD queue behind the triage unit" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 05b - Nakshatra - `feature/triage` (additional - not in the report table)

```bash
git fetch
git switch feature/triage
git pull
cp -r ../VitalRoute_handoff/steps/05b-nakshatra/. .
git add \
    src/modules/StaffRegistry.hpp \
    src/modules/StaffRegistry.cpp \
    src/modules/OTScheduler.hpp \
    src/modules/OTScheduler.cpp \
    src/modules/TransferDesk.hpp \
    src/modules/TransferDesk.cpp \
    data/doctors.txt \
    data/resources.txt
make && make test
git commit -m "feat(ops): next-available doctor search, OT slot ring and transfer desk" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step M2 - Nakshatra - merge `feature/triage`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/triage -m "Merge branch 'feature/triage'"
make && make test
git push
git switch -c feature/navigation
git push -u origin feature/navigation
```

### Step 06 - Nakshatra - `feature/navigation`

```bash
git fetch
git switch feature/navigation
git pull
cp -r ../VitalRoute_handoff/steps/06-nakshatra/. .
git add \
    src/engine/vr_graph.h \
    src/engine/vr_graph.c \
    src/modules/HospitalMap.hpp \
    src/modules/HospitalMap.cpp \
    data/hospital_graph.txt
make && make test
git commit -m "feat(graph): hospital layout loader with floors, coordinates and edge types" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 07 - Nakshatra - `feature/navigation`

```bash
git fetch
git switch feature/navigation
git pull
cp -r ../VitalRoute_handoff/steps/07-nakshatra/. .
git add \
    src/modules/ResourceRouter.hpp \
    src/modules/ResourceRouter.cpp
make && make test
git commit -m "feat(routing): Dijkstra selection of the nearest free bed" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 08 - Nakshatra - `feature/navigation`

```bash
git fetch
git switch feature/navigation
git pull
cp -r ../VitalRoute_handoff/steps/08-nakshatra/. .
git add \
    src/modules/ResourceRouter.hpp \
    src/modules/ResourceRouter.cpp
make && make test
git commit -m "fix(routing): filter candidates by clinical type before comparing distance" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 09 - Nakshatra - `feature/navigation`

```bash
git fetch
git switch feature/navigation
git pull
cp -r ../VitalRoute_handoff/steps/09-nakshatra/. .
git add \
    src/modules/Navigator.hpp \
    src/modules/Navigator.cpp
make && make test
git commit -m "feat(nav): turn-by-turn direction generator with lift and stairway steps" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step M3 - Nakshatra - merge `feature/navigation`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/navigation -m "Merge branch 'feature/navigation'"
make && make test
git push
git switch -c feature/billing
git push -u origin feature/billing
```

### Step 10 - Devesh - `feature/billing`

```bash
git fetch
git switch feature/billing
git pull
cp -r ../VitalRoute_handoff/steps/10-devesh/. .
git add \
    src/model/Charge.hpp \
    src/model/Charge.cpp \
    src/model/Bill.hpp \
    src/model/Bill.cpp \
    src/model/Insurance.hpp \
    src/model/Insurance.cpp \
    src/modules/BillingEngine.hpp \
    src/modules/BillingEngine.cpp
make && make test
git commit -m "feat(billing): Charge hierarchy, operator+= accumulation, insurance deduction" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 11 - Vasav - `feature/billing`

```bash
git fetch
git switch feature/billing
git pull
cp -r ../VitalRoute_handoff/steps/11-vasav/. .
git add \
    src/modules/PersistenceManager.hpp \
    src/modules/PersistenceManager.cpp
make && make test
git commit -m "feat(persistence): fstream record writer and index rebuild on startup" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 11a - Vasav - `feature/billing` (additional - not in the report table)

```bash
git fetch
git switch feature/billing
git pull
cp -r ../VitalRoute_handoff/steps/11a-vasav/. .
git add \
    tests/test_scenarios.cpp
make && make test
git commit -m "test(scenarios): triage, routing, navigation, billing and full-journey tests" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 11b - Nakshatra - `feature/billing` (additional - not in the report table)

```bash
git fetch
git switch feature/billing
git pull
cp -r ../VitalRoute_handoff/steps/11b-nakshatra/. .
git add \
    src/main.cpp
make && make test
git commit -m "feat(app): console menu wiring every module, with seeded sample data" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step M4 - Nakshatra - merge `feature/billing`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/billing -m "Merge branch 'feature/billing'"
make && make test
git push
git switch -c feature/gui
git push -u origin feature/gui
```

### Step 12 - Nakshatra - `feature/gui` (additional - not in the report table)

Integration. Everything the console menu did (registration, triage, treatment, billing, undo, save) now lives in `HospitalSession`, which prints nothing; `src/main.cpp` only reads input and prints. The GUI and the web version call the same class.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/12-nakshatra/. .
git add \
    Makefile \
    src/app/HospitalSession.cpp \
    src/app/HospitalSession.hpp \
    src/main.cpp
make && make test
git commit -m "refactor(app): move console logic into HospitalSession with no I/O" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 12a - Vasav - `feature/gui` (additional - not in the report table)

Tests. Eight scripted console sessions, recorded from the program before step 12, are replayed and compared (clock times and dates masked).

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/12a-vasav/. .
git add \
    Makefile \
    tests/regression.sh \
    tests/regression
git update-index --chmod=+x tests/regression.sh
make && make test && make regression
git commit -m "test(regression): replay recorded console sessions to prove output is unchanged" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 13 - Devesh - `feature/gui` (additional - not in the report table)

Devesh (GUI lead). The GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/13-devesh/. .
git add \
    DESIGN.md \
    src/gui/EsiDelegate.cpp \
    src/gui/EsiDelegate.hpp \
    src/gui/RoleWindow.cpp \
    src/gui/RoleWindow.hpp \
    src/gui/Roles.hpp \
    src/gui/ScreenArranger.cpp \
    src/gui/ScreenArranger.hpp \
    src/gui/SessionHub.cpp \
    src/gui/SessionHub.hpp \
    src/gui/SnapshotTableModel.cpp \
    src/gui/SnapshotTableModel.hpp \
    src/gui/Text.hpp \
    src/gui/Theme.cpp \
    src/gui/Theme.hpp \
    src/gui/UiHelpers.cpp \
    src/gui/UiHelpers.hpp \
    src/gui/VrStyle.cpp \
    src/gui/VrStyle.hpp \
    src/gui/gui.qrc \
    src/gui/style.qss
make && make test && make regression
git commit -m "feat(gui): session hub, theme, style and shared widgets for the Qt front end" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 14 - Devesh - `feature/gui` (additional - not in the report table)

Devesh (GUI lead). The GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/14-devesh/. .
git add \
    src/gui/InsuranceFields.cpp \
    src/gui/InsuranceFields.hpp \
    src/gui/NurseWindow.cpp \
    src/gui/NurseWindow.hpp \
    src/gui/QueueModels.cpp \
    src/gui/QueueModels.hpp \
    src/gui/RegistrationForm.cpp \
    src/gui/RegistrationForm.hpp \
    src/gui/RegistrationPanel.cpp \
    src/gui/RegistrationPanel.hpp \
    src/gui/TransferDeskPanel.cpp \
    src/gui/TransferDeskPanel.hpp
make && make test && make regression
git commit -m "feat(gui): Nurse window with registration, insurance and transfer desk" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 15 - Nakshatra - `feature/gui` (additional - not in the report table)

Navigation. `src/app/RouteSteps` is compiled by the console build too; the GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/15-nakshatra/. .
git add \
    src/app/RouteSteps.cpp \
    src/app/RouteSteps.hpp \
    src/gui/FloorPlanWidget.cpp \
    src/gui/FloorPlanWidget.hpp \
    src/gui/RoutePanel.cpp \
    src/gui/RoutePanel.hpp \
    src/gui/RouteSegments.cpp \
    src/gui/RouteSegments.hpp \
    src/gui/RouteStepsView.cpp \
    src/gui/RouteStepsView.hpp
make && make test && make regression
git commit -m "feat(gui): floor-plan route map with numbered walking steps" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 16 - Devesh - `feature/gui` (additional - not in the report table)

Devesh (GUI lead). `src/app/TheatreSchedule` is compiled by the console build too; the GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/16-devesh/. .
git add \
    src/app/TheatreSchedule.cpp \
    src/app/TheatreSchedule.hpp \
    src/gui/DoctorAvailabilityPanel.cpp \
    src/gui/DoctorAvailabilityPanel.hpp \
    src/gui/DoctorWindow.cpp \
    src/gui/DoctorWindow.hpp \
    src/gui/StaffTheatreModels.cpp \
    src/gui/StaffTheatreModels.hpp \
    src/gui/TheatrePanel.cpp \
    src/gui/TheatrePanel.hpp \
    src/gui/TreatPanel.cpp \
    src/gui/TreatPanel.hpp
make && make test && make regression
git commit -m "feat(gui): Doctor window with treatment, theatres and doctor search" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 16a - Vasav - `feature/gui` (additional - not in the report table)

Records. The GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/16a-vasav/. .
git add \
    src/gui/PatientLookupPanel.cpp \
    src/gui/PatientLookupPanel.hpp \
    src/gui/PatientRecordView.cpp \
    src/gui/PatientRecordView.hpp
make && make test && make regression
git commit -m "feat(gui): patient record view with visit history and ID lookup" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 16b - Vasav - `feature/gui` (additional - not in the report table)

Records and indexes (AVL range query, hash statistics; also the bills list model the billing screen uses). The GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/16b-vasav/. .
git add \
    src/gui/AdminModels.cpp \
    src/gui/AdminModels.hpp \
    src/gui/AdminPanels.cpp \
    src/gui/AdminPanels.hpp
make && make test && make regression
git commit -m "feat(gui): admissions-by-date, staff roster and index statistics panels" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 17 - Devesh - `feature/gui` (additional - not in the report table)

Devesh (GUI lead) and billing. The GUI itself builds from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/17-devesh/. .
git add \
    src/gui/AdminWindow.cpp \
    src/gui/AdminWindow.hpp \
    src/gui/BillView.cpp \
    src/gui/BillView.hpp \
    src/gui/BillingPanel.cpp \
    src/gui/BillingPanel.hpp \
    src/gui/PatientWindow.cpp \
    src/gui/PatientWindow.hpp
make && make test && make regression
git commit -m "feat(gui): Patient window with wall display, and Super Admin billing and discharge" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 18 - Vasav - `feature/gui` (additional - not in the report table)

Tests. The checks are built and run by CMake from step 19 on.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/18-vasav/. .
git add \
    tests/restart_check.sh \
    tests/run_gui_check.sh \
    tests/gui
git update-index --chmod=+x tests/restart_check.sh
git update-index --chmod=+x tests/run_gui_check.sh
make && make test && make regression
git commit -m "test(gui): automated checks that drive the real windows on copies of the data" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 19 - Devesh - `feature/gui` (additional - not in the report table)

Devesh (GUI lead). First step where the GUI builds. Needs Qt 6 (README, "GUI"); skip the cmake lines if you do not have it.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/19-devesh/. .
git add \
    .gitattributes \
    .gitignore \
    CMakeLists.txt \
    run_gui.bat \
    src/gui/Launcher.cpp \
    src/gui/Launcher.hpp \
    src/gui/RoleFactory.cpp \
    src/gui/RoleFactory.hpp \
    src/gui/main.cpp
make && make test && make regression
cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui
ctest --test-dir build-gui --output-on-failure
git commit -m "feat(gui): Launcher, role factory and CMake build for the Qt GUI" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 20 - Devesh - `feature/gui` (additional - not in the report table)

Devesh (GUI lead). `README.md`, `docs/DEMO.md` and `docs/GUI_NOTES.md` here are the GUI-only versions; step 26 adds their web parts.

```bash
git fetch
git switch feature/gui
git pull
cp -r ../VitalRoute_handoff/steps/20-devesh/. .
git add \
    README.md \
    docs/DEMO.md \
    docs/GUI_NOTES.md \
    docs/GUI_TEST.md \
    docs/screenshots
make && make test && make regression
git commit -m "docs(gui): demo script, viva notes, test checklist and screenshots" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step M5 - Nakshatra - merge `feature/gui`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/gui -m "Merge branch 'feature/gui'"
make && make test && make regression
git push
git switch -c feature/web
git push -u origin feature/web
```

### Step 21 - Nakshatra - `feature/web` (additional - not in the report table)

Integration. `make web` works from step 23 on.

```bash
git fetch
git switch feature/web
git pull
cp -r ../VitalRoute_handoff/steps/21-nakshatra/. .
git add \
    src/web/HttpServer.cpp \
    src/web/HttpServer.hpp \
    src/web/Json.hpp
make && make test && make regression
git commit -m "feat(web): hand-written HTTP/1.1 server with a server-sent event stream" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 22 - Nakshatra - `feature/web` (additional - not in the report table)

Integration. `make web` works from step 23 on.

```bash
git fetch
git switch feature/web
git pull
cp -r ../VitalRoute_handoff/steps/22-nakshatra/. .
git add \
    src/web/Api.cpp \
    src/web/Api.hpp \
    src/web/ApiActions.cpp \
    src/web/WebHub.cpp \
    src/web/WebHub.hpp \
    src/web/WebText.hpp
make && make test && make regression
git commit -m "feat(web): JSON API and live change events over the shared session" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 23 - Nakshatra - `feature/web` (additional - not in the report table)

Integration. First step where `make web` builds the server.

```bash
git fetch
git switch feature/web
git pull
cp -r ../VitalRoute_handoff/steps/23-nakshatra/. .
git add \
    Makefile \
    run_web.bat \
    src/web/main.cpp
make && make test && make web && make regression
git commit -m "feat(web): vitalroute_web program, make web target and run_web.bat" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 24 - Devesh - `feature/web` (additional - not in the report table)

Front end (as for the GUI).

```bash
git fetch
git switch feature/web
git pull
cp -r ../VitalRoute_handoff/steps/24-devesh/. .
git add \
    web
make && make test && make web && make regression
git commit -m "feat(web): role pages in plain JavaScript with live updates and an SVG route map" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 25 - Vasav - `feature/web` (additional - not in the report table)

Tests. Starts the server with --local-only on a temporary copy of `data/`; never touches `data/records`.

```bash
git fetch
git switch feature/web
git pull
cp -r ../VitalRoute_handoff/steps/25-vasav/. .
git add \
    tests/web_check.sh
git update-index --chmod=+x tests/web_check.sh
make && make test && make regression && make web
sh tests/web_check.sh
git commit -m "test(web): end-to-end checks of the server, pages, API and live events" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step 26 - Nakshatra - `feature/web` (additional - not in the report table)

Integration and docs.

```bash
git fetch
git switch feature/web
git pull
cp -r ../VitalRoute_handoff/steps/26-nakshatra/. .
git add \
    README.md \
    docs/DEMO.md \
    docs/GUI_NOTES.md \
    docs/WEB.md
make && make test && make regression
git commit -m "docs(web): web version guide, README section and demo fallback" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

### Step M6 - Nakshatra - merge `feature/web`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/web -m "Merge branch 'feature/web'"
make && make test && make regression && make web
git push
```
