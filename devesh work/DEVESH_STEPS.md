# Devesh Saini — your GitHub steps

You have **2 step(s)**: 04, 10.

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
| 1 | 02b | Vasav | `feature/engine` |
| 2 | 03 | Vasav | `feature/engine` |
| 3 | M1 | Nakshatra | `main` |
| 4 | 04 | Devesh  **← YOU** | `feature/triage` |
| 5 | 05 | Vasav | `feature/triage` |
| 6 | 05a | Nakshatra | `feature/triage` |
| 7 | 05b | Nakshatra | `feature/triage` |
| 8 | M2 | Nakshatra | `main` |
| 9 | 06 | Nakshatra | `feature/navigation` |
| 10 | 07 | Nakshatra | `feature/navigation` |
| 11 | 08 | Nakshatra | `feature/navigation` |
| 12 | 09 | Nakshatra | `feature/navigation` |
| 13 | M3 | Nakshatra | `main` |
| 14 | 10 | Devesh  **← YOU** | `feature/billing` |
| 15 | 11 | Vasav | `feature/billing` |
| 16 | 11a | Vasav | `feature/billing` |
| 17 | 11b | Nakshatra | `feature/billing` |
| 18 | M4 | Nakshatra | `main` |

## Your steps, in order

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
make test
git commit -m "feat(model): Person, Patient and Staff hierarchy with role behaviour" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
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
make test
git commit -m "feat(billing): Charge hierarchy, operator+= accumulation, insurance deduction" \
           -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

