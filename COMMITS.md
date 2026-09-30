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

## Order at a glance

| Step | Report # | Owner | Branch | Commit message | Status |
|---|---|---|---|---|---|
| 00 | - | Nakshatra | `main` | `chore: project skeleton and Makefile` | done |
| 00b | - | Nakshatra | `main` | `docs: README and team commit plan` | done |
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
| M4 | - | Nakshatra | main | *merge `feature/billing` into `main` (`--no-ff`)* | to do |

## Commands for each remaining step

Run these from the root of your clone, in Git Bash (Windows) or any POSIX shell,
with the handoff package unpacked next to the clone (see above).

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
```
