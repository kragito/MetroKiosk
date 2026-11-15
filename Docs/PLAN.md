# Plan: align MetroKiosk with the TDP Part 2 feedback

Inputs: instructor feedback (see `Docs/FEEDBACK_TODO.md`), the submitted documentation
`Team05_TDP_Part2/Team05_TDP_Part2_Documentation.pdf`, and the current build (commit e1d645b).
No code changes have been made yet. This is the plan only.

## 1. Gaps found when checking the documentation against the app

The PDF makes promises the app must keep, and the feedback asks for changes on top of them.

| Doc claim (section) | Current app | Gap |
|---|---|---|
| Tourists "pick stations from a list or map, **in their own language**" (03) | Only chrome and headings are translated; station names and most body text are English | Partial; tourists still need English and network knowledge (ties into feedback #3) |
| One-way "**$1.28 – $2.57, prices are calculated based on distance**" (05, step 2); Home figure says "from $1.38" | Ticket card says a flat **$1.68**; popular stations are hard-coded to $1.68 / 13 min / 1 change; others are $1.18–$2.38 by distance | Prices are inconsistent between doc, Home, ticket card and the real fare; every popular trip shows the same time |
| "**Footer keeps Back, Start over, languages and Help in fixed spots**" (04) | Home has no footer; Card options and Done have no Back | Fine per design, but the doc claim must be worded "where applicable", or the footer made truly constant |
| "Recognition over recall: taps HarbourFront in the popular list **instead of spelling it**" (05, step 3) | True for 4 popular stations only; everything else needs exact spelling or line knowledge | This is exactly what feedback #3 attacks |
| "**One decision per screen**" (07) | Destination screen has 4 competing ways to choose (popular list, search, line filter, map) | Contradicts the claim; simplify |
| "Colours never alone" (07) | Line badges carry text codes; status pills have text | OK |
| Help at every step pauses the purchase (02, 06) | Implemented and restores state | OK |
| Correcting selections before paying (04) | Back / Change / Start over exist | OK |
| Ticket in hand matches the on-screen summary (05, step 7) | Receipt card mirrors ticket fields | OK (printed ticket not shown in-app; it's hardware) |

## 2. Work items (priority order)

### P1: Destination step for people who don't know the network (feedback #3, doc 03/05/07)
- **"Where are you going?" is goal-first**: the primary input is a single search that accepts **places as
  well as stations** (landmarks, attractions, malls, hospitals, airport, e.g. "Gardens by the Bay",
  "VivoCity / Sentosa", "Changi Airport", "Orchard shopping"). Each place maps to its nearest station.
- **Error-tolerant search**: case/space-insensitive, fuzzy matching (edit distance), common alternate spellings
  ("harbor front", "dhobi"), and results that update while typing, with "Did you mean…" instead of an empty list.
- **Popular destinations show what's there**: "HarbourFront: VivoCity · Sentosa", with a small category icon.
  Use the same tile style everywhere.
- **Real per-destination journey facts**: correct time, number of changes, which line(s) to take, and the real
  fare. Remove the hard-coded 13 min / $1.68.
- **One decision on screen**: drop "Filter by line" as a primary control (move it into the map view as an
  optional overlay). Keep "See it on the map" as a secondary link.
- **Confirm step clarity**: the selected destination card shows the route summary before "Confirm".
- **Translations**: station and place names keep their English name, with the translated name beneath
  (for 中文/Melayu/தமிழ்), and the destination screen text is translated.

### P2: Pricing consistency (doc 05 vs app)
- A single source of truth for fares: distance-based adult fare per station, child/senior = 70%.
- Ticket-type card shows **"from $X"** (or the range) computed from data; Home chip and docs use the same value.
- Day pass and round-trip text derived from the same rule.

### P3: Home card arrows (feedback #1)
- Remove the right-edge arrows on Top up / Concession. Put the affordance inside each card (a chevron beside the
  title, or a "Top up →" / "Buy or renew →" button matching "Buy a ticket →"), so all three Home
  columns use the same call-to-action pattern.

### P4: App-wide consistency pass (feedback #2)
- Define in FKioskStyle and apply everywhere: type scale (H1/H2/body/caption), button sizes per role
  (primary, secondary, outlined, pill), corner radii, card padding, icon sizes, tag style, price/strike layout,
  left edge and gutters.
- Colour roles: green = proceed/pay, navy = choose, blue = help, amber = warning. Audit every screen against them.
- Produce a before/after capture set and a short consistency checklist the reviewer signs off.

### P5: Documentation updates (feedback #4). Text only; the PDF source isn't in the repo.
- For every decision: **principle → what it means → why it fits this user here → the concrete design choice.**
  Start with steps 5 & 6 (marked by the instructor), then 2–4 and section 06.
- Update the doc to match the app after P1–P4 (new destination flow, fare wording, footer "where applicable",
  "one decision per screen").
- Deliverable: `Docs/TDP_Part2_Revised_Text.md`, ready to paste into the team's document.

### P6: Video plan (feedback #5, #6)
- Persona-driven storyboard: (1) Sam, a first-time tourist, going to "Gardens by the Bay" without knowing the station;
  (2) an elderly commuter topping up with cash and getting change; (3) a student renewing a concession and checking
  dates; (4) someone who gets stuck and uses Help, then resumes.
- A required-interactions checklist covering all three flows, all payment methods, search with a typo, map, riders,
  language switch, Back / Start over, receipt and the lift-card reminder. Each item maps to a harness journey so
  screenshots and recordings are reproducible.
- Deliverable: `Docs/VIDEO_STORYBOARD.md` + `Tools/journeys/*.txt` scripts.

## 3. Process (same loop as before)
For each of P1–P4: Sonnet implements → code reviewer must approve → captures + journeys regenerated
→ neutral UX agent reviews (now including the instructor's three app comments as explicit criteria)
→ iterate until acceptable. P5/P6 are written after P1–P4 land, so they describe the final app.
Commit locally after each approved item; push only when you say so.

## 4. Decisions needed before starting
1. **Scope of P1**: change the app's destination flow (recommended, since it is the instructor's main criticism),
   or keep the app faithful to the original Figma and only discuss it in the docs?
2. **Assignment brief**: is there a list of "required interactions" for the video? If so, where is it?
   P6 currently uses our own list.
3. **Docs format**: is the documentation edited in Figma, Word or Google Docs? P5 currently produces paste-ready text only.
