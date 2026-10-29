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
- Unreal Engine 5.8 (installed at `C:\Program Files\Epic Games\UE_5.8`)
- Visual Studio 2022 with the C++ game development workload
- Windows (Tamil text uses the system font `Nirmala.ttc`)

## Build and run
```powershell
powershell -File Tools/build.ps1          # builds the editor target (closes a running editor first)
powershell -File Tools/launch_editor.ps1  # opens the editor; press Play (PIE) to see the kiosk
```
The kiosk UI is hosted by `AKioskGameMode`, which is the project's default game mode.

## QA hooks
- `-KioskScreen=<Name>` opens a screen with sample state (for example `PayMethod`, `CardOptions`).
- `-KioskCaptureAll=<absolute dir>` saves a PNG of every screen and state, then quits.
  `powershell -File Tools/capture.ps1` runs it into `Saved/Captures`.

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
| `type:<text>` | Presses the on-screen keyboard or keypad keys one by one (`type:har`, `type:25`). Open the keyboard first (click `Search field`) or choose `Amount More` for the keypad. |
| `drag:<label>:<dx>,<dy>` | Touch-drags from the widget centre by design pixels, for example `drag:Map:-250,-120` (zoom in first). |
| `wait:<seconds>` | Lets real time pass; timers run normally in script mode. |
| `shot:<name>` | Saves `NN_<name>.png`. |
| `lang:<en\|zh\|ms\|ta>` | Clicks the language pill. |
| `dump` | Writes `NN_tree.txt`: every visible interactive widget with its name, enabled/disabled state and rect in design px (canvas 1917x1105). Use it to discover what can be clicked. |

Outputs go to `-Out`: `log.txt` (one line per step, `OK`/`FAIL`/`WARN`, then `DONE`), the PNGs and the trees.
Useful names: `Buy a ticket`, `Choose One-way`, `Station <name>`, `Confirm selected ticket`, `Add Adult|Child|Senior`,
`Go to payment`, `Pay with card|cash`, `Insert $2`, `Search field`, `Key H`, `Keypad 5`, `Amount More`,
`SIMULATE: tap card on reader`, `Help`, `Back to my purchase`, `Language English|Chinese|Malay|Tamil`.

Examples of verified journeys: cash ticket with riders, card top-up with the custom keypad, and help call
with the purchase restored afterwards (outputs under `Saved/Journeys/`, which is git-ignored).

## Layout
- `Source/MetroKiosk/Public` - session model, style tokens, data, localisation, game mode
- `Source/MetroKiosk/Private/Widgets` - root widget, shared components, one file group per screen set
- `Source/MetroKiosk/Private/KioskQA.*`, `KioskScript.*` - capture hook and the scripted harness
- `Docs/SPEC.md` - product spec; `Docs/ref/` - design reference images
- `Tools/` - build, launch, capture, journey and MCP helper scripts

## Notes for future sessions
- Non-English languages cover chrome and main headings only (table in `Private/KioskLoc.cpp`, keyed by the English text).
- The harness occasionally drops the very first click after the window opens (seen with some window managers).
  It retries up to four times and logs `WARN`; re-run the journey if a first step fails.
- PIE window placement on the author's machine is unreliable (the saved `NewWindowPosition` can be off-screen),
  which also breaks SlateInspector clicks. Prefer the scripted harness for UX runs.
- Editing source: `Tools/build.ps1` closes the editor; relaunch with `Tools/launch_editor.ps1`.
- Review items still open: tile and label layouts were only checked at 1917x1105; no automated tests beyond the journeys.
