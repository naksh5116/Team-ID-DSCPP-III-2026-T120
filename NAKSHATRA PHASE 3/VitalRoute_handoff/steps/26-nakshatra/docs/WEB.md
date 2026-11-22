# VitalRoute Web

VitalRoute Web is the hospital as a website. One program, `vitalroute_web`,
holds the hospital in memory and serves it to any browser: on the same
computer at `http://localhost:8080/`, or on a phone, tablet or laptop on the
same network at the address the program prints. Every open page works on the
same hospital, and a change made on one device shows on all the others
straight away.

It is a **separate program**. The console program (`bin/vitalroute`) and the
Qt GUI (`vitalroute_gui`) are unchanged and contain no network code.

## Start it

**Windows, double-click:** `run_web.bat` in the project folder. It starts the
server and opens the Launcher in your browser. Keep its window open; close it
(or press Ctrl+C) to stop the server.

**Any shell:**

```
make web
bin/vitalroute_web                  # this computer and the local network
bin/vitalroute_web --local-only     # this computer only
bin/vitalroute_web --port 9000 data
```

| Option | Meaning |
|---|---|
| `--port N` | Port to listen on (default 8080). |
| `--local-only` | Accept connections from this computer only (listens on 127.0.0.1). |
| `--web DIR` | Folder with the pages (default: `web/` next to the data folder). |
| `DATA_DIR` | Hospital data folder (default `data`, as the console). |

When it starts it prints the links, for example:

```
VitalRoute Web is running.
  On this computer:     http://localhost:8080/
  On the same network:  http://192.168.1.23:8080/
```

## Open it on other devices

1. Connect the phone or laptop to **the same Wi-Fi** as the computer running
   the server.
2. Open the "On the same network" link. The Launcher page lists it too.
3. The first time the server runs without `--local-only`, Windows asks
   whether to let `vitalroute_web` through the firewall. Only you can answer
   that. Allow it on **private** networks only.

If other devices cannot connect:
- Check that both are on the same network, and use the IP address, not `localhost`.
- College and public Wi-Fi often block devices from reaching each other
  ("client isolation"). Use a phone hotspot or a home router instead.
- Check that the firewall prompt was allowed (Windows Security, Firewall,
  "Allow an app through firewall").

## Safety

- **There is no sign-in.** Anyone who can open the link can change the data.
  Run it only on a network you trust, or use `--local-only`.
- It is a teaching simulator. Do not enter real patient data.
- No HTTPS: traffic on the network is not encrypted.
- Do not run the Qt GUI and the web server **on the same data folder at the
  same time**. Each holds its own copy in memory, and whichever saves last
  overwrites the other.

> **Project rule change.** The original GUI brief said "no networking of any
> kind". That rule still holds for the console program and the Qt GUI. The
> network is used only by this separate web program, which was added later
> at the team's request. Tell your mentor about this change before the viva.

## The pages

| Page | Who | What |
|---|---|---|
| `index.html` | everyone | Launcher: role links, links for other devices, number of open role windows. |
| `nurse.html` | Nurse | Registration (emergency with ESI level, OPD) and waiting lists; transfer desk. |
| `doctor.html` | Doctor | Treat next patient with the walking route to the bed; patient record and visit notes; operation theatres; doctors on duty. |
| `patient.html` | Patient | Read-only: one patient's directions, record and bill. **Wall display** (button or F11): directions only, large, with no name, for a corridor screen. `patient.html?id=VR-2026-000004` opens a patient directly, so a wall screen can be bookmarked. |
| `admin.html` | Super Admin | Everything above, plus billing and discharge, admissions by date, staff, indexes, Undo, Save (Ctrl+S) and Reload. |

The pages are led by Devesh Saini, the GUI lead, and follow the same design
rules as the Qt GUI (`DESIGN.md`): white
work surface, one accent colour for selection and primary actions, ESI
colours only in ESI badges, monospace for IDs and amounts, no shadows or
gradients. At phone width (under 900 px) the columns stack, and no page
scrolls sideways.

## How it works

```
 browser tabs on any device                      vitalroute_web (one process)
 ┌──────────────┐  GET /api/queues  ───────►  ┌──────────────────────────────┐
 │ nurse.html   │  POST /api/register-... ──► │ HttpServer   select() loop   │
 │ doctor.html  │                             │   static files from web/     │
 │ patient.html │  ◄── event stream ────────  │   /api/* ──► Api ──► WebHub  │
 │ admin.html   │      (Server-Sent Events)   │                  │           │
 └──────────────┘                             │          HospitalSession     │
                                              │   (heap, hash table, AVL,    │
                                              │    lists, graph, Dijkstra)   │
                                              └──────────────────────────────┘
```

- **One session, many viewers.** `WebHub` owns the one `HospitalSession`,
  the same class the console and the Qt GUI use. The web layer never touches
  the data structures directly; it calls the session's methods, so the
  hand-written heap, hash table, AVL tree, lists and graph do all the work.
- **HTTP by hand.** `src/web/HttpServer.cpp` is a single-threaded `select()`
  loop over plain sockets: Winsock on Windows, POSIX sockets on Linux. It
  handles HTTP/1.1 keep-alive, static files, form-encoded POST bodies and
  JSON replies. Requests are handled one at a time, so the session needs no
  locks. Limits: 64 KB of headers (431), 1 MB body (413), and on Windows at
  most about 500 open connections. Each open page holds one, which is plenty
  for a classroom.
- **Live updates.** After every change, `WebHub` sends the same event names as
  the Qt `SessionHub` (`queueChanged`, `bedsChanged`, `routeAssigned`, ...) to
  every open page over one long-lived `GET /api/events` response (Server-Sent
  Events). Each page re-reads only what that event affects. Nothing polls.
  Server-Sent Events were chosen over WebSockets because updates only flow
  from the server to the pages, they are plain HTTP, and the browser
  reconnects by itself. A WebSocket would need a handshake with SHA-1 and
  frame encoding written by hand.
- **Never out of date.** Every event carries a growing version number. A page
  that sees a gap (it missed an event), or that reconnects after the server
  restarted, re-reads everything.
- **Changes must come from the pages.** Every `POST` needs the header
  `X-VitalRoute: 1`, and its `Origin` must match the server. Another website
  open in the same browser therefore cannot submit forms to VitalRoute
  (cross-site request forgery). Refused requests get 403.
- **Names stay text.** A name typed on one device appears on every other
  device, so the pages only ever insert text (`textContent`), never HTML. A
  patient called `<img src=x onerror=alert(1)>` is shown exactly like that.
- **The map** (`web/map.js`) is drawn as SVG from `GET /api/map` and
  `GET /api/route`. The SVG `viewBox` is in metres with y flipped, and the
  browser scales it to the panel. Labels are placed like the Qt map: major
  landmarks first, each at the first free spot around its marker. The route
  steps and the edges each step covers come from `src/app/RouteSteps`, the
  same code the Qt map uses.

## API

All replies are JSON. Errors are `{"error": "..."}` with the console's own
wording: 400 bad input, 404 unknown patient, 409 refused (no bed, nobody
waiting, already discharged), 403 refused POST, 405 wrong method.

| Read (GET) | Returns |
|---|---|
| `/api/ping` | Server is up; number of open pages; links for other devices. |
| `/api/state` | Waiting counts, free beds, next patient, undo note. |
| `/api/queues` | Emergency queue (heap order) and OPD line (FIFO). |
| `/api/patient?id=` | Record, doctor, bill summary, visit history, hash index stats. |
| `/api/bill?id=`, `/api/bills?sort=amount\|date` | One bill with lines and totals; all bills (merge sort). |
| `/api/route?id=` | Walking route to the patient's bed: steps, path, floors. |
| `/api/map` | Landmarks, corridors, beds. |
| `/api/theatres`, `/api/transfers` | Theatre schedule and free-slot rings; pending transfers. |
| `/api/doctors`, `/api/next-doctor?specialty=`, `/api/staff` | Availability array; binary search; roster. |
| `/api/admissions?from=&to=`, `/api/indexes` | AVL range query; hash table and AVL statistics. |
| `/api/events` | The live event stream. |

| Change (POST, form fields) | Does |
|---|---|
| `/api/register-emergency`, `/api/register-opd` | Register a patient (optional insurance). |
| `/api/treat-next` | Treat the next patient (admit, release, or OPD consultation). |
| `/api/visit-note` | Add a visit to a patient's history. |
| `/api/theatre/book`, `/api/theatre/release` | Book the next free slot; release a slot. |
| `/api/transfer`, `/api/transfer/dispatch`, `/api/transfer/cancel` | Transfer desk (deque). |
| `/api/charge`, `/api/insurance`, `/api/discharge` | Billing and discharge. |
| `/api/undo`, `/api/save`, `/api/reload` | Undo the last change; save to disk; reload from disk. |

## Tests

`sh tests/web_check.sh` (in the MSYS2 shell) builds the server, starts it
with `--local-only` on a temporary copy of `data/`, and runs 51 checks:

- the server core (`WB`): static files, refused paths, header and body limits;
- the pages (`WP`): served with the right types, and every imported name is exported;
- the read API (`WR`), including a second server on records saved by the console program;
- the actions and live events (`WA`).

It never touches `data/records`.

Checked by hand in a browser, with several tabs on one server:
- registering on the Nurse page updates the Doctor page's queue and status bar;
- treating on the Doctor page shows the route on an open Patient page for that patient, without a reload;
- a charge and a discharge on the Admin page update the Patient page's bill and directions;
- the wall display, and every page at phone width (375 px) with no sideways scrolling;
- stopping the server shows "Reconnecting"; restarting it brings every page back up to date.

## Files

| File | What |
|---|---|
| `src/web/HttpServer.*` | Sockets, HTTP/1.1, static files, event stream. |
| `src/web/WebHub.*` | Owns the session; numbers and broadcasts changes. |
| `src/web/Api.*`, `ApiActions.cpp` | The read and change endpoints. |
| `src/web/Json.hpp`, `WebText.hpp` | JSON writer; the console's wording for money, dates and statuses. |
| `src/web/main.cpp` | Options, startup messages, Ctrl+C. |
| `web/app.js`, `app.css` | Shared page code (API calls, event stream, tables, tabs, status bar) and styles. |
| `web/panels.js`, `map.js` | The screen panels shared by the pages; the SVG route map. |
| `web/*.html` | The Launcher and the four role pages. |
| `run_web.bat` | Double-click start on Windows. |

## Questions an examiner might ask

- *Why can two phones see the same queue?* There is one `HospitalSession`
  in the server process. The phones only display it, and the server tells
  them when it changes.
- *What happens if two people press Treat at the same moment?* The server
  handles requests one at a time. The first treats the next patient; the
  second treats the patient after that, or is told "Nobody is waiting."
- *Why no threads?* One `select()` loop is enough for a classroom, and it
  means the session never needs a lock.
- *Is it secure?* Not for real use: there is no sign-in and no HTTPS. It
  guards against other websites (the `X-VitalRoute` header and the Origin
  check) and against script injection (text only), and `--local-only` keeps
  it on one computer.
