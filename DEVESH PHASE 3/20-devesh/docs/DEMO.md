# VitalRoute GUI: multi-screen demo script

GUI lead: Devesh Saini. About 5 minutes. One computer, one running program, four windows on up to
four screens. Every action happens in one window and appears in the others
at once.

## Before the audience arrives

1. Connect the screens (or a laptop plus a projector). In Windows display
   settings choose **Extend**, not Duplicate.
2. Start from the sample data: delete the files in `data/records/` (keep
   `.gitkeep`).
3. Start the GUI from the project folder (README, "GUI"). The Launcher opens.
4. In the Launcher click **Nurse**, **Doctor**, **Patient**, **Super Admin**,
   then **Arrange on screens**:

   | Screen | Window |
   |---|---|
   | 1 | VitalRoute - Nurse |
   | 2 | VitalRoute - Doctor |
   | 3 | VitalRoute - Patient |
   | 4 | VitalRoute - Super Admin |

   With fewer screens the windows wrap round (screen 1 gets Nurse and
   Patient side by side, and so on). See the one-screen fallback below.
5. Prepare the windows: the Doctor window stays on Treatment, and the Super
   Admin window opens on the Indexes tab. Leave the Patient window empty.
   The patient ID does not exist until step 1.

## The demo

| Step | Screen | Do | The audience sees |
|---|---|---|---|
| 1 | **1: Nurse** | Emergency patient form: name "Demo Patient", age 60, complaint "Chest pain at rest", choose **1 Resuscitation**, press Enter | "Registered VR-2026-000012 (ESI 1). Position in the emergency queue: 3 of 7." The red badge row joins the queue below the two earlier ESI 1 patients (equal severity, so arrival order) |
| 2 | **2: Doctor**, then **3: Patient** | Point at the Doctor's queue. Then in the Patient window type `VR-2026-000012` and press Enter | The new patient is already in the Doctor's queue at position 3, and every status bar reads "Emergency waiting: 7". The Patient window shows "Waiting" and "You do not have a bed yet" |
| 3 | **2: Doctor** | Click **Treat next patient** three times. Pause after each: patients 000004 and 000007 come first, then the demo patient | Each time: attending doctor, bed (ICU-1, Bed 1, then Bed 2, then Bed 3), and the route drawn on the map with numbered steps |
| 4 | **3: Patient** | Nothing at first: on the third treatment its route appears by itself. Then press **F11** | Wall display: "Patient VR-2026-000012: walking directions to ICU-1, Bed 3", both floors of the route, large-type steps, "Total distance ... Estimated walking time ...". The name is not shown on a public screen |
| 5 | **2: Doctor** | Click step 7 "Take the lift to floor 1", then a turn | The map switches to floor 1 and highlights that step |
| 6 | **4: Super Admin** | Point at the status bar, then open **Billing and discharge**, type `VR-2026-000012`, press Enter | "ICU beds free" has dropped by three since the start (8 to 5). The new bill shows the emergency panel, the consultation and the ICU bed, each with its calculation |
| 7 | **4: Super Admin** | Indexes tab | Hash table entries 12, AVL tree height and rotations, "Balanced: Yes" |
| 8a | **4: Super Admin** | Add a manual charge (Test, "Troponin", Rs 1200), then **Undo last entry** and confirm | The charge appears on the Patient's bill (screen 3, after F11 to leave the wall view) and disappears again on undo |
| 8b | **4: Super Admin** | **Save** in the toolbar | "Saved 12 patients, ... to data/records/". Optional: Launcher, **Reload saved records**. Every screen redraws from the saved files |

### What to say at each step (one line each)

1. "Registration puts the patient in a max-heap keyed on ESI level, then arrival."
2. "One process, one session. The Nurse's action emitted `queueChanged`, and every window refreshed."
3. "The router keeps only free ICU beds, then runs Dijkstra from the ER; the Navigator turns the path into directions."
4. "`routeAssigned` told this Patient window its patient now has a bed."
5. "Each step maps to its part of the path, worked out with the Navigator itself."
6. "`bedsChanged` and `billsChanged`: the bill is a list of polymorphic charges."
7. "The hash table finds an ID in constant average time; the AVL tree answers date ranges."
8. "Undo pops a stack; save writes plain-text records the console can read too."

## One-screen fallback (2 x 2)

If only one display works:

1. Open the four windows as above and click **Arrange on screens**. The
   windows tile 2 x 2:

   | | |
   |---|---|
   | Nurse (top left) | Doctor (top right) |
   | Patient (bottom left) | Super Admin (bottom right) |

2. Run steps 1-3 and 6-8 the same way. All four status bars are visible at
   once, so the sync is easy to see.
3. For step 4, press F11 in the Patient window. It fills the screen; F11
   again returns it to its tile.
4. Click on a window before typing; the keyboard goes to the active window.

## If something goes wrong

| Problem | Do |
|---|---|
| A projector is unplugged | Its window moves to the main screen by itself. Plug it back in and click **Arrange on screens** |
| A window is closed by mistake | Launcher, click that role again (the session is unaffected) |
| The data looks wrong | Launcher, **Reload saved records** (or delete `data/records/*.dat` and restart for the sample data) |
| Text looks tiny on the projector | Windows display settings: set that screen's scale to 125-150 %. The GUI redraws sharply at any scale |
