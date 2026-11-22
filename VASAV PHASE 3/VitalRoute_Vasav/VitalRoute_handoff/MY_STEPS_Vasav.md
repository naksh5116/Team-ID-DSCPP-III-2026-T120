# Vasav - your steps

Your part of the VitalRoute commit plan, in order. The full plan, with the
exact commands for every step, is in COMMITS.md next to this file.

1. Unzip this package next to your clone, so that
   `VitalRoute_handoff/steps/` sits beside the clone folder (the other
   members' packages unzip into the same folder without clashing).
2. Do your steps in this order. Wait for the step before yours in
   COMMITS.md ("Order at a glance") to be pushed, then `git pull`.
3. Skip any step that is already on GitHub.

| Step | Branch | Commit message | Folder |
|---|---|---|---|
| 02b | `feature/engine` | `feat(engine): doubly linked list, hash table and AVL tree for patient records` | 02b-vasav |
| 03 | `feature/engine` | `test(engine): unit tests for the queue family, AVL rotations and hash collisions` | 03-vasav |
| 05 | `feature/triage` | `feat(records): hash index for constant-time lookup by patient identifier` | 05-vasav |
| 11 | `feature/billing` | `feat(persistence): fstream record writer and index rebuild on startup` | 11-vasav |
| 11a | `feature/billing` | `test(scenarios): triage, routing, navigation, billing and full-journey tests` | 11a-vasav |
| 12a | `feature/gui` | `test(regression): replay recorded console sessions to prove output is unchanged` | 12a-vasav |
| 16a | `feature/gui` | `feat(gui): patient record view with visit history and ID lookup` | 16a-vasav |
| 16b | `feature/gui` | `feat(gui): admissions-by-date, staff roster and index statistics panels` | 16b-vasav |
| 18 | `feature/gui` | `test(gui): automated checks that drive the real windows on copies of the data` | 18-vasav |
| 25 | `feature/web` | `test(web): end-to-end checks of the server, pages, API and live events` | 25-vasav |
