# Instructor feedback (SyncSketch, Team05 TDP Part 2) vs. current build

Source: SyncSketch review `S2gV9JYjG504`. The PDF (Team05_TDP_Part2_Documentation.pdf) and
the video (Team05_TDP_Part2_Video.mp4) were both reviewed by the DigiPen instructor about 27–28 days ago.
The feedback was given on the original Figma prototype, before the Unreal recreation.
Checked against the current build at commit e1d645b (captures in Saved/Captures).

## Status summary

| # | Feedback | Where | Current build | Status |
|---|----------|-------|---------------|--------|
| 1 | Arrows on the edges of the Top up / Concession cards look like they connect to the options on their right | Video, frame 0957 (Home) | Same arrows, same position (01_Home.png) | **Not addressed** |
| 2 | Keep the UI consistent down to the smallest details: button/shape sizes, colours, positioning, alignment | Video, frame 3535 (Choose a ticket, One-way card circled) | Ticket cards are now consistent (same type sizes, prices on one baseline), but there has been no app-wide consistency pass | **Partly addressed** |
| 3 | Destination step relies on too much assumed knowledge: knowing exactly where to go, how lines work, how to spell station names. Reduce errors and serve the user's actual goal | Video, frame 4223 (Where are you going?, popular list + line filter marked) | Unchanged: station-name list, line filter, exact-text search, map | **Not addressed (biggest item)** |
| 4 | Citations don't show understanding or justify the specific application. Needs more detailed, principle-based justification | PDF, frame 0002 (steps 5 & 6 citations underlined) | Documentation, not the app | **Docs task** |
| 5 | Video use cases should tell a specific user's story with context, not describe features generically | Video, frame 10748 | Video deliverable | **Video task** |
| 6 | Not all required interactions are demonstrated in the video | Video, frame 15008 | Video deliverable | **Video task** |
| + | Positive: physical-kiosk visual first, overview before detail, clear document structure, generally good use of design principles | PDF frames 0001/0002, video 3535 | Keep | Keep as is |

## What needs to be done before further development

### A. App changes (in priority order)

1. **Rethink "Where are you going?" for users who don't know the network (#3)**
   - Search by **place, not only station**: landmarks and areas (e.g. "Gardens by the Bay", "Changi Airport",
     "VivoCity / Sentosa", "Orchard shopping", hospitals, attractions) mapped to the nearest station.
   - **Typo- and partial-tolerant search** (fuzzy match, ignore spaces/case, common misspellings,
     e.g. "harbor front", "dhobi"), with suggestions instead of "no results".
   - Popular destinations shown as **what's there**, not just the station name (e.g. "HarbourFront: VivoCity, Sentosa"),
     maybe with a small icon or photo for tourists.
   - **De-emphasise or remove "Filter by line"** as a primary control. It assumes users know the lines.
     Move it under "More ways to find" or into the map.
   - Show **real, per-destination journey info** (time, changes, which line to take) and **confirm before commit**.
     The current data says "13 min · 1 change" for every station, which is wrong and misleading.
   - Make it easy to recover from a wrong pick (clear "Change destination" on Review already exists; keep it obvious).
2. **Home card arrows (#1)**: remove the right-edge arrows on Top up / Concession, or move the affordance
   inside the card (e.g. a "Top up →" button or chevron next to the title), so the cards don't appear to point
   at the Help & Settings column.
3. **App-wide consistency pass (#2)**: audit every screen and normalise:
   - button heights and widths per role (primary / secondary / outlined), corner radii, padding;
   - type scale (one set of heading, body and caption sizes, used everywhere);
   - card styles, icon sizes, tag/pill styles, price/strike-through placement;
   - alignment grid and margins (shared left edge, consistent gutters);
   - colour usage per role (green = proceed/pay, navy = choose, blue = help).
   Put the values in FKioskStyle and remove per-screen overrides.

### B. Documentation (#4)
- For each design decision, name the principle, explain what it means, and explain **why it applies here**
  (cause → effect for this user), e.g. signifiers, feedback, constraints, mapping, recognition over recall,
  slips vs. mistakes. Don't just append "(Norman, 2013, pp. x–y)".
- Rewrite the step 5/6 justifications first (they were the ones marked).

### C. Video (#5, #6)
- Structure it around **specific personas with context** (e.g. Sam the first-time tourist going to HarbourFront;
  an elderly commuter topping up; a student renewing a concession), each with a goal, problem and outcome.
- Show **every required interaction** end to end: all three flows, every payment method (tap, chip, cash with change),
  search/keyboard, map, riders stepper, language switch, help call and resume, Back / Start over, receipt / lift-card reminder.
- The scripted harness (`Tools/journey.ps1`) can produce reproducible per-step screenshots for each persona journey.

### Open questions for the team
- Is the redesigned destination step (A1) in scope for this assignment, or should the app stay faithful to the
  original Figma and changes go into the docs only?
- Are the required interactions listed in the assignment brief? (#6 refers to them; we don't have the brief.)
