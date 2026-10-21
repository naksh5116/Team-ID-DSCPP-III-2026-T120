# Nakshatra Agarwal — your GitHub steps

You have **11 step(s)**: M1, 05a, 05b, M2, 06, 07, 08, 09, M3, 11b, M4.

## One-time setup (do this once, before your first step)

1. **Add Devesh and Vasav as collaborators** (repo → *Settings → Collaborators → Add people*) and send them `devesh.zip` / `vasav.zip`.
2. **Install Git** (https://git-scm.com) and use **Git Bash** for every command below.
   PowerShell does not understand `cp -r` the same way, so do not use it here.
3. **Clone the repo and unzip this package next to it**, so the folders look like this:

   ```
   Desktop/pbl/
   ├── Team-ID-DSCPP-III-2026-T120/   <- your clone (open Git Bash HERE for every step)
   └── VitalRoute_handoff/            <- from this zip
       └── steps/...
   ```

   ```bash
   cd ~/Desktop/pbl
   git clone https://github.com/naksh5116/Team-ID-DSCPP-III-2026-T120.git
   cd Team-ID-DSCPP-III-2026-T120
   git config user.name  "Your Full Name"
   git config user.email "the-email-on-YOUR-github-account"
   ```
   The email **must** be the one on your own GitHub account, or the commit will not show under your profile.

## Rules for every step

* **Wait for your turn.** Check the turn table: the step before yours must be pushed first. Ask in the group chat.
* Run `git pull` before starting (the commands already include it).
* Stage **only** the files listed (never `git add .`).
* Copy the commit message **exactly**.
* `make test` checks the build; if you have no compiler on your laptop, skip that one line. Every step has already been build-tested in this order.
* If `git push` is rejected, run `git pull --rebase` and push again.
* **Read every file you commit before the evaluation.** The viva asks each member to explain their own code: what the data structure is, why it was chosen, and its time complexity.

## Whole-team turn order

| # | Step | Who | Branch |
|---|---|---|---|
| 1 | 02b | Vasav | `feature/engine` |
| 2 | 03 | Vasav | `feature/engine` |
| 3 | M1 | Nakshatra  **← YOU** | `main` |
| 4 | 04 | Devesh | `feature/triage` |
| 5 | 05 | Vasav | `feature/triage` |
| 6 | 05a | Nakshatra  **← YOU** | `feature/triage` |
| 7 | 05b | Nakshatra  **← YOU** | `feature/triage` |
| 8 | M2 | Nakshatra  **← YOU** | `main` |
| 9 | 06 | Nakshatra  **← YOU** | `feature/navigation` |
| 10 | 07 | Nakshatra  **← YOU** | `feature/navigation` |
| 11 | 08 | Nakshatra  **← YOU** | `feature/navigation` |
| 12 | 09 | Nakshatra  **← YOU** | `feature/navigation` |
| 13 | M3 | Nakshatra  **← YOU** | `main` |
| 14 | 10 | Devesh | `feature/billing` |
| 15 | 11 | Vasav | `feature/billing` |
| 16 | 11a | Vasav | `feature/billing` |
| 17 | 11b | Nakshatra  **← YOU** | `feature/billing` |
| 18 | M4 | Nakshatra  **← YOU** | `main` |

## Your steps, in order

### Step M1 - Nakshatra - merge `feature/engine`

```bash
git fetch
git switch main
git pull
git merge --no-ff origin/feature/engine -m "Merge branch 'feature/engine'"
make test
git push
git switch -c feature/triage
git push -u origin feature/triage
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
make test
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
make test
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
make test
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
make test
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
make test
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
make test
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
make test
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
make test
git push
git switch -c feature/billing
git push -u origin feature/billing
```

### Step 11b - Nakshatra - `feature/billing` (additional - not in the report table)

```bash
git fetch
git switch feature/billing
git pull
cp -r ../VitalRoute_handoff/steps/11b-nakshatra/. .
git add \
    src/main.cpp
make test
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
make test
git push
```

## After M4

`main` now holds the complete project. Check it once from a fresh clone:

```bash
make && make test && ./bin/vitalroute
```
