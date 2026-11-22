# VitalRoute GUI design

Design rules for the Qt 6 Widgets GUI (`src/gui/`). GUI lead: Devesh Saini. The look is a hospital
information system or operations console: dense, calm and legible, built for
repeated use by staff. It is not a marketing page.

Values live in one place: the token table in `src/gui/Theme.cpp`. The
stylesheet `src/gui/style.qss` uses the token names (`@accent`, `@border`, ...)
and `Theme::styleSheet()` substitutes them at start-up. Code that paints
(badges, the route map) asks `Theme` for colours by name. No raw colour
values anywhere else.

The direction comes from the team's brief and common clinical-software
practice. It is not based on a survey of real products.

## Colour

| Token | Value | Role |
|---|---|---|
| `surface` | `#FFFFFF` | Work surface: tables, forms, the map canvas |
| `panel` | `#F4F5F7` | Panels around the work: menu bar, status bar, table headers, inactive tabs |
| `border` | `#D9DCE1` | 1 px dividers: panels, tables, tabs, splitters |
| `control` | `#8C939D` | Outline of inputs and secondary buttons (3.1:1 on white, so the field edge is visible) |
| `text` | `#1B1F24` | All body text |
| `textMuted` | `#5B636E` | Hints, field captions, secondary columns (6.1:1 on white) |
| `textDisabled` | `#8A9099` | Disabled controls only |
| `hover` | `#ECEEF1` | Hover on neutral buttons and rows |
| `pressed` | `#E2E5E9` | Pressed neutral button |
| `accent` | `#143769` | Selection, focus outline, primary action, the highlighted route. Nothing else (11.8:1 on white) |
| `accentHover` | `#1D4A87` | Hover on the primary action |
| `accentPressed` | `#0E2850` | Pressed primary action |
| `onAccent` | `#FFFFFF` | Text on the accent |
| `corridor` | `#9AA1AB` | Corridor lines on the floor plan |

**ESI is the only other colour.** It appears only in the ESI badge.

| Level | Name | Badge | Number | Contrast |
|---|---|---|---|---|
| 1 | Resuscitation | `#C62828` | white | 5.6:1 |
| 2 | Emergent | `#E65100` | white, bold | 3.8:1 (see below) |
| 3 | Urgent | `#F9A825` | `#1B1F24` | 8.4:1 |
| 4 | Less urgent | `#2E7D32` | white | 5.1:1 |
| 5 | Non-urgent | `#1565C0` | white | 5.8:1 |

ESI 2 falls short of 4.5:1 for small text. The number is bold, and every
place that shows a badge also shows the level name next to it, so the level
is never read from the colour or the badge alone.

**There is no error colour.** Red, orange, yellow, green and blue already
mean ESI levels, so an error never borrows one of them. Errors are plain
text in the window's message area, prefixed the same way the console
prefixes them ("Not found: no patient with ID ...").

## Typography

- The system UI font at the system size (Segoe UI 9 pt on Windows, the
  desktop font on Linux). The stylesheet never sets a font size or family.
- Monospace (`QFontDatabase::systemFont(FixedFont)`, at the UI font's size)
  for patient IDs, bed codes, bill IDs, transfer codes and amounts. Amounts
  are right-aligned so the decimals line up. Everything else is left-aligned.
- Weights: regular, plus semibold (600) for table headers, group titles and
  the patient's name in a record header.
- Sizes are multiples of the system size, never fixed pixels:

| Use | Size |
|---|---|
| Body, tables, forms | 1.0x |
| Section heading in a window | 1.15x, semibold |
| Floor plan labels | 0.85x |
| Wall display: directions | 1.8x |
| Wall display: distance and time | 2.2x, semibold |

## Spacing

One 4 px scale, in logical pixels, so it stays sharp at any display scaling:
4, 8, 12, 16, 24.

- Layout margins 8, spacing between controls 8, between groups 12.
- Table cells: 2 px vertical, 6 px horizontal padding. Compact rows, no zebra
  striping, horizontal gridlines only.
- Role windows: minimum 1100 x 700 logical pixels. Splitters and tabs absorb
  resizing; nothing is fixed-position.

## Shape and depth

- Radius: 2 px on buttons, inputs and badges; 0 on panels, tables and tabs.
- No shadows, gradients, translucency or blur.

## Components

| Component | Rule |
|---|---|
| Primary button | Accent fill, white text. At most one per form: the action Enter triggers ("Register patient", "Treat next patient") |
| Secondary button | White fill, `control` outline. Every other action |
| Destructive action (discharge, undo, reload) | Secondary button plus a confirmation dialog that names exactly what will happen. Not red |
| Focus | Accent outline on inputs and buttons. Tab order follows reading order |
| Disabled | `textDisabled` on `panel`. A disabled action keeps its label |
| Inputs | White, `control` outline, accent outline on focus. The label sits left of the field, sentence case |
| Check box | Square, `control` outline; checked is an accent fill with a white tick. Drawn by `VrStyle` as vector shapes |
| Spin box / combo box arrows | Small `text` triangles (disabled: `textDisabled`) on a `panel` button area. `Theme` draws them from the tokens at 4x size at start-up, because stylesheets only take arrow images |
| Tables | `QTableView` with a model. Header on `panel`, semibold, left-aligned. Accent selection with white text |
| Tabs | Flat. The selected tab is white with a 2 px accent top edge |
| Status bar | One per role window, on `panel`. Fields: emergency waiting, OPD waiting, next patient, free ICU beds, free ward beds, role. The message area (results and errors) is on the left |
| ESI badge | A square one text line high with the number centred. The level name follows in normal text |
| Dialogs | Only for destructive actions (discharge, undo, reload, save on exit) |

## Floor plan (route map)

- Canvas on `surface`. Corridors are 1 px `corridor` lines. Landmarks are
  small squares outlined in `textMuted`.
- Lifts are marked "Lift" and stairs "Stairs" in text. There are no icons.
  Bed and ward landmarks are labelled with their names.
- The route is a 3 px `accent` line, drawn on every floor it crosses. Start
  is a hollow accent square, destination a filled accent square. A selected
  step is drawn 5 px wide.
- Drawn in logical coordinates with antialiasing; Qt maps them to device
  pixels on each monitor.

## Imagery and icons

None. No icon packs, emoji, illustrations, logos, splash screen or hero
banner. Words label every control.

## Wording

Sentence case, plain and specific: "Treat next patient", "Bed ICU-1, Bed 1",
"Open a role window". No marketing copy, no greetings, no exclamation marks.
Numbers are shown as they are ("8 ICU beds free"), never as giant dashboard
figures.
