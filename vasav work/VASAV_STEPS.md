# Vasav Sharma — your GitHub steps

You have **5 step(s)**: 02b, 03, 05, 11, 11a.

## One-time setup (do this once, before your first step)

1. **Accept the GitHub invite.** Nakshatra adds you under *Settings → Collaborators*; accept the email.
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
| 1 | 02b | Vasav  **← YOU** | `feature/engine` |
| 2 | 03 | Vasav  **← YOU** | `feature/engine` |
| 3 | M1 | Nakshatra | `main` |
| 4 | 04 | Devesh | `feature/triage` |
| 5 | 05 | Vasav  **← YOU** | `feature/triage` |
| 6 | 05a | Nakshatra | `feature/triage` |
| 7 | 05b | Nakshatra | `feature/triage` |
| 8 | M2 | Nakshatra | `main` |
| 9 | 06 | Nakshatra | `feature/navigation` |
| 10 | 07 | Nakshatra | `feature/navigation` |
| 11 | 08 | Nakshatra | `feature/navigation` |
| 12 | 09 | Nakshatra | `feature/navigation` |
| 13 | M3 | Nakshatra | `main` |
| 14 | 10 | Devesh | `feature/billing` |
| 15 | 11 | Vasav  **← YOU** | `feature/billing` |
| 16 | 11a | Vasav  **← YOU** | `feature/billing` |
| 17 | 11b | Nakshatra | `feature/billing` |
| 18 | M4 | Nakshatra | `main` |

## Your steps, in order

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
make test
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
make test
git commit -m "test(engine): unit tests for the queue family, AVL rotations and hash collisions" \
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
make test
git commit -m "feat(records): hash index for constant-time lookup by patient identifier" \
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
make test
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
make test
git commit -m "test(scenarios): triage, routing, navigation, billing and full-journey tests" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

