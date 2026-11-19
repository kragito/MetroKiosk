# Metro Kiosk

A self-service metro ticket machine touchscreen UI, built for the Technical Design Principles
course. The brief: design a user-friendly interface for public transportation.

The app recreates a Figma design (see `Docs/SPEC.md` and the reference renders in `Docs/ref/`) as
pure C++ Slate in Unreal Engine 5.8. It is designed for the general public: large tap targets,
plain language, Back / Start over / Help visible where applicable, a step indicator, four
languages (English, 中文, Melayu, தமிழ்) and on-screen hints that show where the physical hardware is.

Flows: one-time ticket, card top-up, and concession pass renewal. Hardware (card reader, chip
slot, cash slots) is simulated with clearly marked dashed "SIMULATE" controls.

## Requirements
- Unreal Engine 5.8. The `Tools/` scripts find it through `Tools/engine.ps1`, in this order: the
  `UE_ROOT` environment variable (the folder that contains `Engine`), then the Epic launcher's install
  record (registry `HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8`, then `LauncherInstalled.dat`), then
  `C:\Program Files\Epic Games\UE_5.8`. If none has `Engine\Build\BatchFiles\Build.bat`, the script
  stops with a message naming `UE_ROOT`. An invalid `UE_ROOT` is ignored with a warning.
- Visual Studio 2022 with the C++ game development workload
- Windows (Tamil text uses the system font `Nirmala.ttc`)

## Build and run
```powershell
powershell -File Tools/build.ps1          # builds the editor target (closes a running editor first)
powershell -File Tools/launch_editor.ps1  # opens the editor; press Play (PIE) to see the kiosk
```
`Tools/build.ps1` passes `-NoUBA` because the Unreal Build Accelerator hung locally (cl.exe idle, nothing finishing); add `-UBA` to turn it back on.
The kiosk UI is hosted by `AKioskGameMode`, which is the project's default game mode.

## QA hooks
- `-KioskScreen=<Name>` opens a screen with sample state (for example `PayMethod`, `CardOptions`).
- `-KioskCaptureAll=<absolute dir>` saves a PNG of every screen and state (including destination: empty, search "garden",
  typo "harbor frnt", "Did you mean", place selected, 中文 and தமிழ்), then quits.
  `powershell -File Tools/capture.ps1` runs it into `Saved/Captures`.
- `-KioskCaptureRes=WIDTHxHEIGHT` makes captures use that window size instead of 1917x1105 (the canvas scales to fit);
  `Tools/capture.ps1 -ResX 1280 -ResY 720 -Out Saved\Captures_1280x720` passes it for you.

## Scripted journeys (no editor, no PIE)
A UX tester can drive the app headlessly from the command line. It runs the standalone game, sends real
touch events through Slate (nothing moves your mouse), takes screenshots and writes a log.

```powershell
powershell -File Tools/journey.ps1 -Out C:\temp\j1 -Script 'click:Buy a ticket|click:Choose One-way|shot:tickets|dump'
```
Use single quotes in PowerShell/bash so `$` is not expanded. It does not close a running editor; if
the binaries are stale and the editor is open it stops and tells you to rebuild.

Steps are separated by `|`:

| Step | What it does |
|---|---|
| `click:<label>` | Touch-clicks the visible, enabled widget whose accessible name or label matches (exact match first, then case-insensitive contains). Fails with a reason if not found, disabled, or covered. A screenshot `NN_after_click_<label>.png` is taken automatically. |
| `type:<text>` | Presses the on-screen keyboard or keypad keys one by one (`type:har`, `type:25`). Open the keyboard first (click `Search`) or choose `Amount More` for the keypad. |
| `drag:<label>:<dx>,<dy>` | Touch-drags from the widget centre by design pixels, for example `drag:Map:-250,-120` (zoom in first). |
| `scroll:<label>:<dy>` | Mouse-wheel scrolls the named list by roughly `dy` design pixels (positive = down). Result lists with more than five rows are named `Station list`: `scroll:Station list:300`, then click the row. Rows scrolled out of view fail with a "not hittable" reason. |
| `expect:<text>` / `absent:<text>` | Passes if some visible text on screen contains (or no visible text contains) `<text>`, case-insensitive. Use it to assert countdowns, messages and screens. |
| `wait:<seconds>` | Lets real time pass; timers run normally in script mode. |
| `shot:<name>` | Saves `NN_<name>.png`. |
| `lang:<en\|zh\|ms\|ta>` | Clicks the language pill. |
| `dump` | Writes `NN_tree.txt`: every visible interactive widget with its name, enabled/disabled state and rect in design px (canvas 1917x1105). Use it to discover what can be clicked. |

A failed step (not found, disabled, covered, or a click that changed nothing) logs `FAIL`, saves a `NN_failure.png`, and aborts the run with an `ABORT` line so later steps do not cascade. Pass `-KioskScriptKeepGoing` to the game (not the wrapper) to continue instead. Matching is exact first; if an exact match exists but is covered or disabled the step fails rather than falling back to a looser match.

Outputs go to `-Out`: `log.txt` (one line per step, `OK`/`FAIL`/`WARN`, then `DONE`), the PNGs and the trees.
Useful names: `Buy a ticket`, `Choose One-way`, `Search`, `Place <name>` (for example `Place Gardens by the Bay`),
`Station <name>`, `Change destination` (map card), `Confirm selected ticket`, `Open metro map view`, `Filter NS|EW|NE|CC|DT|TE` (destination list and map),
`Add Adult|Child|Senior`, `Go to payment`, `Pay with card|cash`, `Insert $2`, `Key H`, `Key Space`, `Hide keyboard` (the Done key),
`Keypad 5`, `Amount More`, `SIMULATE: tap card on reader` (on Scan card this is the Student card; the Adult card is `SIMULATE: tap Adult card on reader`), `Help`, `Back to my purchase`, `Language English|Chinese|Malay|Tamil`.
Step names stay in English whatever language is active (the on-screen map labels follow the active language). The language pills live in the footer, so switch language after the first
screen (`click:Buy a ticket|lang:zh`), not on Home.

### Journey scripts
`Tools/journeys/*.txt` hold one script per file; `powershell -File Tools/run_journeys.ps1` runs them all (or `-Name a,b`, comma-separated, no spaces)
and prints PASS/FAIL, with screenshots and the log in `Saved/Journeys/<name>/` (git-ignored).

| File | Journey |
|---|---|
| `a_tourist_gardens` | types "gardens by the bay", picks the place, Confirm, Review, pays by card, receipt |
| `b_typo_harbourfront` | types the typo "harbor frnt" and still finds HarbourFront |
| `c_chinese_search` | switches to 中文, finds Chinatown by the pinyin alias "niuchesui" |
| `d_daypass_skips_destination` | Day pass goes straight to Review |
| `j1_cash_ticket`, `j2_topup_keypad`, `j3_help_restore` | cash ticket with a child rider, card top-up with the keypad, help call and restore |
| `j4_map_drag_search`, `j5_scroll_results` | map with line highlight, zoom, drag and search; scrolling a long result list |
| `k2_zh_topup_cash` | 中文 top-up paid with cash |
| `k3_help_during_cash` | help call during cash payment: "Your $1.00 is safe", countdown paused in the call and in picture-in-picture, then resumes |
| `k4_receipt_more_time` | receipt "I need more time" adds 30 s |
| `k5_idle_prompt` | 90 s idle shows "Are you still there?", Continue dismisses it (takes about 2 minutes) |
| `k6_concession_renewal` | renewal with the current pass pre-selected, two-month dates check, done screen with receipt buttons |
| `k7_map_back_to_list` | Back from the map returns to the list; leftover search text is cleared |
| `l5_adult_topup` | adult Metro card top-up: "Adult card", $18.50 balance, no low-balance banner or pass, +$20 to $38.50, done screen |
| `l6_adult_concession` | adult Metro card concession: adult prices (no struck price, no student banner), nothing pre-selected, "No pass active", "Go to payment" disabled until a day is picked (`click:Day +3` = three days from today; names are `Day YYYY-MM-DD`), done screen shows the chosen start |
| `l7_map_labels_languages` | map labels follow the language: selects HarbourFront from search, then switches en, zh, ta and ms on the map and zoomed in (check the shots: labels are painted, not visible to `expect:`), back to the list expects the zh name 港湾, confirm reaches Review |
| `l8_full_network_routes` | full MRT network routes from City Hall: Woodlands North (44 min, 1 change, $2.24, NS then TE), Changi Airport (38 min, $2.12, EW branch via Tanah Merah), Tuas Link (49 min, direct, $2.54, the maximum), each with map-route and review shots; then zh search "stevens" expects the Chinese name 史蒂芬 on the route and Review |

## 3D mode (optional)
`powershell -File Tools/play_3d.ps1` (or add `-Kiosk3D` to a `-game` launch) puts the kiosk in a small metro concourse built
at runtime from engine basic shapes only (`/Engine/BasicShapes`, no assets under `Content/`). The kiosk screen shows the live
kiosk UI. Without `-Kiosk3D` the app opens the 2D kiosk exactly as before.
- Controls: WASD or arrow keys walk, mouse looks, left click presses on the kiosk screen, **E** near the kiosk opens the
  full-screen kiosk view, **Esc** steps back out.
- Switches: `-Kiosk3DStart=kiosk` starts in the full-screen view (journey scripts can run as `-Kiosk3D -Kiosk3DStart=kiosk`);
  `-Kiosk3DShot=<abs path.png>` saves a screenshot of the scene from the spawn point after about 4 s, then quits
  (`Tools/play_3d.ps1 -Shot <png>`). `-StartAtKiosk` and `-NoBuild` are also script switches.
- Code: `Source/MetroKiosk/Private/World/` (`KioskEnvironment` scene, `KioskVisitorPawn` first-person pawn, `KioskWorld` mode switching).

## Automation tests
```powershell
powershell -File Tools/build.ps1
powershell -File Tools/run_tests.ps1            # MetroKiosk.Search.*, MetroKiosk.Loc.Coverage, MetroKiosk.Session.Rules
```
`MetroKiosk.Loc.Coverage` scans every `KT("...")` in the sources and builds every screen in 中文, Melayu and தமிழ் to catch
missing translations, placeholder mismatches and English mixed into Chinese or Tamil. Close the editor first.

QA switches: `-KioskCard=Student|Adult` sets which Metro card the reader detects when no card is chosen (default Student; Scan card's
SIMULATE buttons still pick either card explicitly). Sample names also take an `Adult` or `Student` prefix, for example
`-KioskScreen=AdultCardOptions`, `AdultTopUpAmountSelected`, `AdultConcession`, `AdultPassDates`, `AdultDoneLiftCard`,
`AdultDoneLiftCardConcession` (the capture run saves these last). `-KioskFakeTime=HH:MM` freezes the kiosk clock start (Tools/capture.ps1 uses 10:30; samples `TicketTypeLate`
and `IdlePrompt` show the late-night warning and the idle overlay). Captures cover every screen in English and the key screens
in 中文, Melayu and தமிழ் (`-KioskScreen=<Screen>_Chinese|_Malay|_Tamil` works too).

## Layout
- `Source/MetroKiosk/Public` - session model, style tokens, data, localisation, game mode
- `Source/MetroKiosk/Private/Widgets` - root widget, shared components, one file group per screen set
- `Source/MetroKiosk/Private/KioskQA.*`, `KioskScript.*` - capture hook and the scripted harness
- `Docs/SPEC.md` - product spec; `Docs/ref/` - design reference images
- `Tools/` - build, launch, capture, journey and MCP helper scripts

## Notes for future sessions
- Non-English languages cover every user-facing string (table in `Private/KioskLocTable.inl`, keyed by the English text,
  including format patterns such as `{0} min · direct`). Station and place names are not in that table: they live in
  `Private/KioskPlaces.cpp` (`GNames`), shown as the English name with the local-script name beneath.
- Destination data: `KioskPlaces.*` (about 60 places mapped to stations, aliases and pinyin), `KioskSearch.*` (normalisation,
  exact/prefix/alias/fuzzy ranking, "Did you mean"), `KioskRoutes.*` (Dijkstra from City Hall over the line paths: 1 min per
  22 map units per hop, min 2, +5 min per change; and the fare rule). The fare rule: 128c up to 3.2 km, +6c per extra km, capped
  at 257c; child/senior `(c*7+5)/10`; round trip 2x less 30%; day pass $8.00. Never hard-code a fare or time in a widget.
- The on-screen keyboard is Latin only, so Chinese, Malay and Tamil readers search with English names, aliases or pinyin
  (for example `niuchesui`); the search hint says so; the matcher also accepts typed Chinese/Tamil text if a physical keyboard is ever attached.
- The harness enables Slate input while the app is not the active OS window (`SetHandleDeviceInputWhenApplicationNotActive`).
  Without that, Slate refuses pointer capture for a background-launched window and every click is silently dropped.
  `Tools/journey.ps1 -StealFocusForTest` opens Notepad over the game to prove journeys pass while the game is inactive.
- Cursor parking was removed from the screenshot hooks (it moved the user's real mouse). It is harmless because
  buttons show no hover state (Hovered looks the same as Normal).
- PIE window placement on the author's machine is unreliable (the saved `NewWindowPosition` can be off-screen),
  which also breaks SlateInspector clicks. Prefer the scripted harness for UX runs.
- Editing source: `Tools/build.ps1` closes the editor; relaunch with `Tools/launch_editor.ps1`.
- Review items still open: layouts were only checked at 1917x1105.
