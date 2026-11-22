# Nakshatra (team lead) - your steps

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
| 00c | `main` | `docs: extend the commit plan with the GUI and web version` | 00c-nakshatra |
| M1 | main | *merge `feature/engine` into `main` (`--no-ff`), then create `feature/triage`* | (no files: merge or done) |
| 05a | `feature/triage` | `feat(triage): emergency heap and OPD queue behind the triage unit` | 05a-nakshatra |
| 05b | `feature/triage` | `feat(ops): next-available doctor search, OT slot ring and transfer desk` | 05b-nakshatra |
| M2 | main | *merge `feature/triage` into `main` (`--no-ff`), then create `feature/navigation`* | (no files: merge or done) |
| 06 | `feature/navigation` | `feat(graph): hospital layout loader with floors, coordinates and edge types` | 06-nakshatra |
| 07 | `feature/navigation` | `feat(routing): Dijkstra selection of the nearest free bed` | 07-nakshatra |
| 08 | `feature/navigation` | `fix(routing): filter candidates by clinical type before comparing distance` | 08-nakshatra |
| 09 | `feature/navigation` | `feat(nav): turn-by-turn direction generator with lift and stairway steps` | 09-nakshatra |
| M3 | main | *merge `feature/navigation` into `main` (`--no-ff`), then create `feature/billing`* | (no files: merge or done) |
| 11b | `feature/billing` | `feat(app): console menu wiring every module, with seeded sample data` | 11b-nakshatra |
| M4 | main | *merge `feature/billing` into `main` (`--no-ff`), then create `feature/gui`* | (no files: merge or done) |
| 12 | `feature/gui` | `refactor(app): move console logic into HospitalSession with no I/O` | 12-nakshatra |
| 15 | `feature/gui` | `feat(gui): floor-plan route map with numbered walking steps` | 15-nakshatra |
| M5 | main | *merge `feature/gui` into `main` (`--no-ff`), then create `feature/web`* | (no files: merge or done) |
| 21 | `feature/web` | `feat(web): hand-written HTTP/1.1 server with a server-sent event stream` | 21-nakshatra |
| 22 | `feature/web` | `feat(web): JSON API and live change events over the shared session` | 22-nakshatra |
| 23 | `feature/web` | `feat(web): vitalroute_web program, make web target and run_web.bat` | 23-nakshatra |
| 26 | `feature/web` | `docs(web): web version guide, README section and demo fallback` | 26-nakshatra |
| M6 | main | *merge `feature/web` into `main` (`--no-ff`)* | (no files: merge or done) |
