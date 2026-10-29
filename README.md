# Metro Kiosk

A self-service metro ticket machine touchscreen UI, built for the Technical Design Principles
course. The brief: design a user-friendly interface for public transportation.

The app recreates a Figma design (see `Docs/SPEC.md` and the reference renders in `Docs/ref/`) as
pure C++ Slate in Unreal Engine 5.8. It is designed for the general public: large tap targets,
plain language, always-visible Back / Start over / Help, a step indicator, four languages
(English, 中文, Melayu, தமிழ்) and on-screen hints that show where the physical hardware is.

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

## Layout
- `Source/MetroKiosk/Public` - session model, style tokens, data, localisation, game mode
- `Source/MetroKiosk/Private/Widgets` - root widget, shared components, one file group per screen set
- `Docs/SPEC.md` - product spec; `Docs/ref/` - design reference images
- `Tools/` - build, launch, capture and MCP helper scripts

## Known issues
- The "You are here" marker on the schematic map slightly overlaps a neighbouring label.
- Tamil is only partly translated (chrome and main headings).
