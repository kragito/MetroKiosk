# Metro Kiosk — product spec (recreated from Figma)

Source: Figma file `dkiBatPeqyuACIr2XvjdyK`, page "Prototype" (node 51:1505).
Reference screenshots (1024×591 renders of 1917×1105 frames) live in
`Docs/ref/*.png`. They are the visual target. Text in this spec is quoted from the design.

## What the app is

A self-service **metro ticket machine touchscreen** for a Singapore-style
metro (lines NS, EW, NE, CC, DT, TE). The kiosk is at **City Hall (NS, EW)**.
It is landscape, 1917×1105 design px, touch-only, for the general public:
tourists, elderly people, wheelchair users, and speakers of four languages
(English, 中文, Melayu, தமிழ்). Design priorities from the file: large tap targets,
plain language, always-visible Back / Start over / Help, a step indicator,
and telling people **where on the physical kiosk** to act (right column: contactless reader top,
chip slot middle, cash slots bottom; collection tray below the screen, left; Metro card reader below the screen, right of centre).

## Three entry flows (from the Home screen)

1. **No card → One-time ticket**
   Home → 1 Ticket type → 2 Destination (one search box for places and stations, popular destinations, summary card with route and fare; the map is a secondary link)
   → 3 Review (riders) → 4 Pay method → Wait (tap card | chip card | cash)
   → Processing → Payment approved / Receipt → auto-return to Home.
2. **Have a card → Top up**
   Home → Scan card ("Place your transit card on the reader") → Card options
   (balance, quick top-up +$10/+$20/+$50, "Top up another amount",
   "Renew concession pass", "Buy a ticket instead") → Top-up amount
   ($10 $20 $30 $50 $100 More, shows "After top-up" balance) → Pay → Wait for card
   → Processing → "Your top-up is done. Lift your Metro card off the reader".
3. **Have a card → Concession**
   Home → Scan card → Choose concession pass (Bus $29 / Train / Bus + Train $55,
   "Student card detected · student concession prices shown", adult price
   struck as reference) → Check your pass dates (November 2026 calendar,
   "Your new pass starts the day after it ends, so you don't lose any days",
   15 Nov – 14 Dec 2026) → Go to payment → Wait for card → Processing →
   "Your purchase is done. Lift your Metro card off the reader".

Placing a card on the reader from Home (bottom banner "Or place your card on
the reader to begin") also goes to Card options. In this simulation the card
reader / chip slot / cash slots are **simulated with on-screen controls**
(for example a "Simulate tap" affordance or the clickable note/coin chips on the cash
screen). Keep these clearly marked as simulation and not part of the real UI design.

## Global chrome (every screen except Help call)

- **Header bar** (teal `--k-brand`, ~107px tall): navy "M" logo tile, divider,
  "You are at **City Hall**" + NS (red) / EW (green) line badges; right side
  "Wed 14 Oct · 7:48 pm" (it's fine to use the live clock in this format).
- **Step bar** (light band under the header), only inside a flow: 4 steps with
  circles and connecting lines. Done = green circle with a check plus the chosen value
  as its label (for example "One-way", "Harbourfront"). Current = navy filled
  circle. Future = outlined. Ticket flow: Ticket, Destination, Review, Pay.
  Card flow: Tap card, What to do?, Review, Pay.
- **Footer** (white, ~127px): left "‹ Back" (outlined) and "⟲ Start over"
  (outlined, not on the first step), centre-right language pills (English /
  中文 / Melayu / தமிழ், the active one navy filled), far right blue
  "🎧 Help · talk to staff" button (`--k-help`).

## Screens (reference image → content)

| # | Screen | Ref | Key content |
|---|--------|-----|-------------|
| 1 | Home | 01_home | 3 columns: NO CARD (big green "One-time ticket" card with ticket icon, chips "One-way · from $1.28" (the lowest one-way fare, computed), "Day pass · $8.00", navy "Buy a ticket →"), HAVE A CARD ("Top up", "Concession" white cards with icons and →), HELP & SETTINGS (Language card with 4 pills; blue "Call for assistance" card with "Staff online" pill). Bottom banner: contactless icon, "Or place your card on the reader to begin", "We'll show your balance and what you can do with it", "Reader here ⌄⌄⌄" at the right. |
| 2 | Choose a ticket | 02_ticket_type | 3 cards: One-way "from $1.28"; Round trip "$1.79 - $3.60" (struck "$2.56 - $5.14", 30% discount); Day pass $8.00 with tag "Best for 3+ rides". The ranges are computed from the route-based fares (see Prices). Each has a navy "Choose" button. |
| 3 | Where are you going? | 03_destination | Left: one prominent search field ("Type a place or station, e.g. Gardens by the Bay") and, beneath it, "Popular from City Hall" tiles: HarbourFront "VivoCity · Sentosa", Bayfront "Gardens by the Bay · Marina Bay Sands", Changi Airport "Flights · Jewel", Orchard "Shopping · ION Orchard", Chinatown "Heritage · food · Chinatown Point". Every tile shows line badges and "N min · M changes · $fare" computed from the route. Right: a summary card (empty prompt until something is chosen), a secondary "See it on the map" link and a green "Confirm destination" button. No line filter on this screen. |
| 3b | Search keyboard | 03b_search_keyboard | On-screen QWERTY keyboard with Space / Clear / Done. Results update live: places ("Gardens by the Bay · near Bayfront") and stations, ranked exact > prefix > alias > fuzzy; case, spaces and punctuation are ignored, typos are tolerated ("harbor frnt" finds HarbourFront), translated names and pinyin aliases match. With no good match the list says "Did you mean…" (or offers the popular places). Never an empty list. |
| 3c | Map | 03c_map | A schematic metro map with tappable stations and a "You are here" marker at City Hall. Right: "Search for a place or station", the same summary card, an optional "Highlight one line" row (dims the other lines), "Open station list view", "Confirm destination". |
| 3d | Destination selected | 03d_destination_selected | The summary card shows the place and its station ("Gardens by the Bay · near Bayfront"), "Take North-South Line → change at Marina Bay → Circle Line", "12 min · 1 change", "One-way: $1.64 per adult", a "Change" button, and the green "Confirm destination". |
| 4 | Check your trip | 04_review | Left: ticket summary card ("One-way ticket" tag, Change link, From City Hall NS EW → To HarbourFront NE CC, dashed note "Valid for 2 hours after use. About 14 min on the Blue line."). Right: "Who is riding?" Adult $1.68 each, Child (5–17) $1.18 each · 30% off · under 5 ride free, Senior (65+) $1.18 each · 30% off, each with − count + steppers (− outlined, + navy). "Total · N rider(s)" with total amount, green "Go to payment →". Can't go below 0 riders total (at least 1 rider). |
| 5 | Pay $X | 05_pay_method | 3 cards: "Tap card or phone" (Fastest tag, "Pay with card" navy), "Insert chip card" ("Pay with chip card/NETs"), "Pay with cash" ("Notes up to $50 and coins…", "Pay with cash"). Subtitle "Use any method below. The lit slot on the kiosk shows you where." and right-aligned summary "One-way · City Hall → Harbourfront · 1 adult". |
| 6a | Wait for card | 06_wait_tap_card | Pill "Paying **$1.64** · one-way to Bayfront", "Hold your card or phone on the reader", explanation of where the reader is, status pill "● Waiting for your card · 9:59 left" (a real countdown), 3 tips (phone unlock / one card only / NETS reader below). Right: illustration of the reader with "Bank card". |
| 6b | Wait for chip card | 06_wait_chip_card | "Insert your card, chip first" with the chip-slot illustration "Chip first, facing up", "Chip slot · middle right". |
| 6c | Wait for cash | 06_wait_cash | Panel: To pay / Put in / Still to pay (navy), progress bar, Notes $2 $5 $10, Coins 10¢ 20¢ 50¢ $1 (tapping simulates inserting money), tray note. Change is shown when overpaying. |
| 6d | Processing | (no ref) | Same as 6a/6b/6c but the status pill reads "Processing..." and inputs are disabled; it advances automatically after ~2 s. |
| 7 | Payment approved | 09_receipt | Green check "Payment approved", "Your ticket is printing. **Take it from the collection tray.** (Tray is below the screen, left; see Revision 2 Physical layout.)", ticket summary card (ONE-WAY · ADULT, WED 14 OCT, City Hall → HarbourFront, Valid until 9:49 pm, $1.68). Countdown bar "Returning to start in 9 s". Right card "Want a receipt?": Print receipt, Email receipt, green "No receipt · I'm done". Footer has "+ Buy another ticket" instead of Back. |
| 8 | Help call | 10_help_video_call | Blue header "Help · In call with staff". Dark video placeholder "[Staff video feed]" with "● Connected · 0:24" (counting up). LIVE CAPTIONS box. Right: "Your purchase is paused. Nothing has been charged. You can pick up where you left off.", "Mute microphone", "Ask staff to come here", "Captions are on" toggles, footnote about the staffed office at Gate B. Footer: green "‹ Back to my purchase" and red outlined "✕ End call". Returning restores the exact previous screen and state. |
| 9 | Scan card | (no ref) | "Place your transit card on the reader", "The Metro card reader is the curved bowl below the screen, right of centre. Rest the card on it. It glows green when it's ready. Hold the card still until this screen changes.", "Waiting for your card…", "Not reading? Take the card out of your wallet or phone case, then try again.", Metro card illustration. Simulated tap advances. |
| 10 | Card options | 11_card_options | "Card found. What would you like to do?", right "Card options only · **Buy a ticket instead**". Left card panel: Student card •••• 4821, Balance $3.20, amber "Low balance · one trip left", Concession pass "Bus + Train · ends 14 Nov", "Keep your card on the reader". Quick top up (green tiles +$10 → New balance $13.20, +$20 → $23.20, +$50 → $53.20). "Other card options": Top up another amount ($30, $100 or any amount), Renew concession pass. |
| 11 | Top-up amount | 12_topup_amount_selected | "How much would you like to add?", Adult card, Balance now $3.20, after selection "After top-up $13.20 · +$10 added", amount grid $10 $20 $30 $50 $100 More, "Continue →" (disabled until an amount is chosen). |
| 12 | Choose a concession pass | 13_concession_selection | 3 option cards Bus / Train / Bus + Train with student prices and struck adult prices, selection state, "Every pass lasts 30 days with unlimited rides. Next, you'll see the exact dates it covers before you pay.", "Continue · check dates →". |
| 13 | Check your pass dates | (no ref) | November 2026 month calendar (Mon-first), highlighting the current pass (ends 14 Nov) and the new pass range 15 Nov – 14 Dec. Legend: Current pass · ends 14 Nov / New pass validity. Summary "Bus + Train – Student · 30 days · unlimited rides · $55.00", "Go to payment →". |
| 14 | Done · lift card | 14_purchase_done_lift_card | Warning-style "!" "Your purchase is done / Your top-up is done. Lift your Metro card off the reader below the screen before you go.", "Card still on the reader · Don't forget your card", Current Balance (for top-up: $13.20, "+$10.00 added · was $3.20"), illustration of the reader. Returns home after a timeout or when the card is lifted (simulated). |

## Design tokens (from the Figma "Colour palettes" frame)

| Token | Hex | Use |
|---|---|---|
| --k-bg | #EEF4FB | page background |
| --k-surface | #FFFFFF | cards |
| --k-border-strong | #B8C7DA | card and button borders |
| --k-ink | #1B2A4A | text, dark buttons |
| --k-muted | #4B5B74 | captions |
| --k-brand | #0B7A75 | header bar |
| --k-logo | #1F3A6B | "M" logo tile |
| --k-primary | #1F8A43 | primary/pay buttons, done step |
| --k-primary-tint | #E3F4E8 | tags like "Fastest", chips |
| --k-help | #1E5AA8 | help button, help header |
| --k-help-tint | #E3EDF8 | help/info panels, step band |
| --k-strong | #1B2A4A | dark button |
Extra: disabled button fill #D5DFEA; danger #C62828; amber warning #FFF3CD / #E0A100.
Line colours: NS #D42E12, EW #009645, NE #9900AA, CC #FA9E0D, DT #005EC4, TE #9D5B25.
Corner radius ~16px on cards, ~10–12px on buttons. Typography: bold geometric sans.
Use the engine's Roboto (Bold/Regular). Headings ~40px, card titles ~28px, body ~18–20px (design px).

## Prices / sample data

Fares come from one source of truth (`KioskRoutes.*`): the fastest route from City Hall over the line graph
(per-hop minutes from the schematic distance, 5 min per line change), and a distance-based fare.

- **Adult one-way** = 128c for the first 3.2 km, plus 6c per further km (rounded up), capped at 257c.
  Range **$1.28 - $2.57**. Examples from City Hall: Orchard 5 min, direct, $1.46; Bayfront 12 min, 1 change, $1.64;
  HarbourFront 16 min, 1 change, $1.82; Changi Airport 28 min, 1 change (East-West, change at Tanah Merah), $2.54.
- **Child and senior** = 70% of the adult fare, integer cents `(c*7+5)/10` (Bayfront $1.64 -> $1.15).
- **Round trip** = 2 x one-way less 30%, `(c*2*7+5)/10` (range $1.79 - $3.60).
- **Day pass** $8.00 (child/senior $5.60), no destination.
- The Home chip ("One-way - from $1.28") and the ticket cards are computed from the same data.
  Review, Pay, Wait, Receipt and the scripted harness all use `FKioskSession::AdultFareCents()`.
Places: about 60 real places (attractions, malls, hospitals, universities, the airport, hawker centres), each mapped
to its nearest station in `KioskPlaces.cpp`, with a category, alias keywords and translated names.

Station list: include at least ~40 real station names across the 6 lines (the map frame lists Boon Lay,
Jurong East, Buona Vista, Outram Park, Raffles Place, Bugis, Paya Lebar, Tanah Merah,
Tampines, Pasir Ris, Expo, Changi Airport, Woodlands, Yishun, Ang Mo Kio, Bishan,
Newton, Orchard, Dhoby Ghaut, Marina Bay, Marina South Pier, HarbourFront, Chinatown,
Little India, Serangoon, Sengkang, Punggol, Promenade, MacPherson, Caldecott,
Botanic Gardens, Bukit Panjang, Beauty World, Woodlands North, Gardens by the Bay,
Bayshore, Marsiling, Marymount, Marine Parade, Marine Terrace, Hume).
Card: Student card •••• 4821, balance $3.20, Bus + Train pass ends 14 Nov 2026.

## Technical architecture (Unreal Engine 5.8, C++)

- Project `MetroKiosk` (C++ module `MetroKiosk`). The UI is **pure C++ Slate**
  (`SCompoundWidget` subclasses, declarative `SNew` syntax), so it is fully code-reviewable with no
  binary .uasset editing.
- A `AKioskGameMode` (set as GlobalDefaultGameMode in DefaultEngine.ini) adds the root
  `SKioskRoot` widget to the game viewport on BeginPlay, shows the mouse cursor, and
  uses UI-only input.
- Fixed 1917×1105 design canvas inside `SScaleBox` (ScaleToFit), so it renders identically at any window size.
- A central `FKioskStyle` (tokens, fonts, brushes with rounded corners via
  `FSlateRoundedBoxBrush`) is used by every widget. No hardcoded colours outside it.
- A `FKioskState`/session model (flow type, ticket type, destination, riders, amount, cash inserted,
  language, previous screen for help) plus a simple navigator (screen enum + history stack for Back,
  Start over resets the session).
- Reusable components: header, step bar, footer, primary/secondary/outlined buttons, line badge,
  option card, stepper, tag/pill.
- Localisation: all user-facing strings via `NSLOCTEXT`/`LOCTEXT`. Switching language must at least
  change the static chrome strings (provide a small in-code string table for 中文 / Melayu / தமிழ்
  for chrome and main headings; full translation is not required, but nothing may break).
  CJK and Tamil glyphs must render (use a fallback font that covers them; check
  `Engine/Content/Slate/Fonts` and the engine's default composite font).
- **Review/QA hooks** (required):
  - `-KioskScreen=<ScreenName>` command-line arg opens directly to that screen with sample state.
  - `-KioskCaptureAll=<absolute dir>` cycles every screen (and key states such as destination
    selected, cash part-paid, processing, other language), saves a PNG per screen named
    `NN_<ScreenName>.png` via `FScreenshotRequest`, then quits. Used by the UX reviewer.
  - Run with: `UnrealEditor.exe <uproject> -game -windowed -ResX=1917 -ResY=1105 -KioskCaptureAll=<dir>`.

## Tooling (build, run, and Unreal MCP)

- Build (closes the editor first, since Live Coding blocks external builds):
  `powershell -File Tools/build.ps1` (add `-Relaunch` to reopen the editor and wait for MCP).
- Launch editor + wait for MCP: `powershell -File Tools/launch_editor.ps1`.
- Unreal MCP server: `http://localhost:8000/mcp` (Epic's ModelContextProtocol plugin,
  auto-starts with the editor). CLI bridge usable from Bash:
  - `python Tools/ue_mcp.py toolsets`
  - `python Tools/ue_mcp.py describe SlateInspectorToolset.SlateInspectorToolset`
  - `python Tools/ue_mcp.py call EditorToolset.EditorAppToolset StartPIE '{"options":{"bSimulate":false,"playMode":"PlayMode_InEditorFloating","warmupSeconds":2}}'`
  - `python Tools/ue_mcp.py call SlateInspectorToolset.SlateInspectorToolset Snapshot '{"ref":""}'`
    (then `Click` with a ref, `Screenshot` with `--out file.png`, `WaitFor`, `Type`, `PressKey`)
  - `python Tools/ue_mcp.py call EditorToolset.LogsToolset <tool> '{...}'` for the output log
  - `python Tools/ue_mcp.py call EditorToolset.EditorAppToolset StopPIE '{}'`
- Give interactive widgets meaningful `.Tag()` / `ToolTipText` / accessible text so the Slate
  inspector snapshot shows readable names (for example the button's label).

## Revision 2: product decisions (these override any earlier row or sentence above)

**Physical layout (single source of truth; one shared diagram, `KioskUI::HardwareDiagram`/`HardwareHint`).**
Follows the Figma kiosk drawing (node 64:75), which overrides the earlier positions. Collection tray: below the screen,
left. Metro card reader: below the screen, right of centre, a protruding bowl the card rests on (top-up/concession;
forgotten-card detection). Right column, top to bottom: contactless reader (bank cards, phones), chip/NETS slot with PIN
pad, note and coin slots. The Home banner, ScanCard, CardOptions
("Keep your card on the reader") and DoneLiftCard point at the Metro card reader; card-payment screens point at the
contactless reader (tap) or chip slot (chip); cash points at the note/coin slots and the tray. Wording and arrows agree.

**Cash.** Notes $2, $5, $10, $50 ("Notes up to $50"); coins 10c, 20c, 50c, $1. Same wording on PayMethod and the cash screen.
The cash screen also shows the 10-minute countdown.

**Validity (same sentence on ticket type, Review, Pay, Receipt).** One-way: "Valid for 2 hours after use."
Assumption (reversible): the two hours run from first use (tap-in), so the one-way receipt prints no clock time such as
"Valid until 9:49 pm"; it shows the same sentence as the other screens. Round trip: return valid until end of service
tonight (12:00 a.m.). Day pass: unlimited rides until end of service tonight (12:00 a.m.); their receipt keeps the
"Valid until 12:00 a.m. tonight" line. From 9:00 p.m. the Day pass card shows
"Only N h of service left today. A one-way may be cheaper." and loses "Best for 3+ rides". QA: `-KioskFakeTime=HH:MM`.

**Step bar.** Ticket flow: Ticket, Destination, Review, Pay (PayMethod, waits and Processing are step 4; a day pass reads
"All lines" for the skipped step). Card flow: Tap card, What to do?, Amount (top-up) or Pass (concession, including the dates
check), Pay.
Once a ticket step is passed it shows the rider's choice instead of the generic name: step 1 reads the ticket ("One-way",
"Round trip", "Day pass") and step 2 the station name in the active language (local name, else English). The current and later
steps keep the generic label, so Back restores it. Labels wrap within a capped width so later steps stay on the bar. The card-flow
bar is unchanged.

**Help call.** With cash inserted: "Your $X.XX is safe. Nothing more will be charged. You can continue or get it back."
otherwise "Nothing has been charged." All countdowns and auto-returns pause while any call is open (full screen or
picture-in-picture); only the 2 s "Processing" step of a payment already under way still finishes. Footnote: "Or visit the
staffed office at Gate B, or use the Help button on the front of the kiosk." (no phone numbers).

**Timeouts.** Receipt and done screens return Home after 30 s with a visible countdown and an "I need more time" button (+30 s).
Idle: 90 s without touches on a purchase screen (not Home, not hardware waits with their own countdown, not during a call)
shows "Are you still there?" with a 20 s countdown, "Continue" and "Start over"; on expiry the kiosk returns any cash (existing
banner) and goes Home.

**Home.** No right-edge arrows. Ticket, Top up and Concession tiles share one form: icon, title, description, full-width navy
bottom action button ("Buy a ticket", "Top up", "Buy or renew" with an arrow). The "One-way · from $1.28" and "Day pass · $8.00"
lines are plain text, not pills. **Receipt.** Never "MIXED": "One-way · 1 adult, 1 child". Top-up and concession done screens
also offer Print / Email receipt. **Concession renewal.** The current pass (Bus + Train) is pre-selected and labelled "Your
current pass"; the dates check shows both months (Nov and Dec) as flat, non-interactive calendars. **Footers.** CardOptions and
ScanCard have Back + Start over (CardOptions keeps "Buy a ticket instead"). **Destination.** Tapping the search field with
leftover text clears it; Back from the map returns to the list. **Processing.** Back/Start over are visibly disabled (flat grey).
The Bus pass uses a bus icon.

**Destination layout (T-006, Figma nodes 71:2301, 101:5387, 78:1978, 78:2919, 78:7148, 101:2308, 101:5064).** Rebuilt to the
Figma frames, keeping the app's place search (places and landmarks as well as stations, typo tolerance, "Did you mean").
Left: "Where are you going?" and the list, headed "Popular from City Hall" (or "Results for ..." / "Did you mean" while typing).
Right, top to bottom: a slim trip card ("Select a location", then "City Hall → HarbourFront · about 13 min · 1 change" once
chosen; there is no Change button, the customer just picks another tile); "Somewhere else?" with the "Search all stations"
field and the hint "A keyboard will popup. The list narrows with every letter you type."; "Filter by line" with six buttons
(NS, EW, NE, CC, DT, TE: badge plus short name); "Open metro map view"; and "Confirm selected ticket" (grey until a station
is chosen, then green). Tapping a line filters the left list to that line (nearest first, City Hall left out) and the heading
becomes "Filtered by North-South Line" with the line badge; tapping the active line again clears it. The filter also narrows
typed results (`FKioskSearch::Run(Query, LineFilter)`, "Did you mean" suggestions included) and is kept when the keyboard opens;
if nothing on the line matches, the list says so. With the keyboard open, the keyboard replaces the hint, filter grid and map
button (Search frames). The trip card shows time and changes only; the fare stays on each tile and on Review. The map screen keeps its own highlighter and
shares the same filter. Choices where the frames differ: the selected frame 78:7148 omits the filter and map button, but 78:2919
(same state) keeps them, so they stay visible when a station is chosen; the hint is always the "A keyboard will popup" wording.

**Metro card types (T-014).** The card flow reads the card type from the tapped card. Scan card's SIMULATE control offers a
Student card and an Adult card (`FKioskState::CardType`, sample data in `FKioskData::Card`); the QA switch `-KioskCard=Student|Adult`
sets the default card. Labels follow the card: "Student card" or "Adult card" on card options, top-up and done screens, and in the
pay summaries. Student card: concession prices with the adult price struck through and the "Student card detected" banner (as
before). Adult card: adult prices only, no strike-through and no banner. Assumptions (reversible): the sample student card
(•••• 4821, $3.20, low-balance banner) keeps its active Bus + Train pass ending 14 Nov, so its renewal dates are unchanged;
the sample adult card (•••• 7305, $18.50, no low-balance banner) has no active pass, so card options says "No active pass" and
offers "Buy a concession pass", nothing is pre-selected on the concession screen, and the dates screen starts the 30-day pass today
(start-date picking is T-011; the "has active pass" fields live on `FKioskCardInfo` for it to build on). "Pick an amount" and the top-up
amounts are the same for both cards. Captures `AdultCardOptions`, `AdultTopUpAmountSelected`, `AdultConcession`, `AdultPassDates`,
`AdultDoneLiftCard` and `AdultDoneLiftCardConcession` are appended to the end of the capture list.

**Card options header (T-018, Figma 156:29687).** "Card options only ·" sits beside the "Buy a ticket instead" link; both wrap.

**Design system (P4).** Type scale in `KioskType` (Display 52, H1 42, H2 32, H3 26, Body 22, Caption 18, map label 16, nothing
else below 18). Button roles with fixed minimum heights: primary green 104, navy choose / outlined secondary / help 88, compact
(Change) 60, pills 72, keys 76; labels are centred and wrap to two lines instead of clipping, so no control has a fixed width.
One card radius/padding, one "Change" control, equal-width language pills shared by footer and Home, one line-badge size.

**Localisation.** Every string goes through `KT()`; `Private/KioskLocTable.inl` holds 中文, Melayu and தமிழ் for all of them
(header date/time, rider labels, "each", totals, SIMULATE hints, overlays). `MetroKiosk.Loc.Coverage` fails on any missing entry,
placeholder mismatch, or English mixed into Chinese/Tamil. Station and place names stay English with the local name beneath.

**Map screen (T-016, T-017; Figma 101:8701, 140:15747, 101:8851).** The map screen has its own search field ("Find station on the map") that opens the same on-screen keyboard in place (it no longer jumps to the list). While text is typed, stations that match the text (places count for the station they are at; the line filter still applies; typo-guess suggestions do not count) stay at full strength with their name shown, every other station fades with the existing line-filter fade, and the map shows "N stations lit up · others greyed out" (plural by count, all four languages). If nothing matches the map is left as it is and says "No station matches · try fewer letters". Clearing the text restores the map. With nothing selected the right column says "No station selected" / "Tap a station on the map to see the route, or find it by name below."; with a station selected it shows From and To with line badges, "About N min · direct" or "About N min · N change(s)", the adult fare for the current ticket, and one row per leg with its line badge: "Take the East-West Line, 3 stops to Outram Park", then "Change at Outram Park to the North East Line, 1 stop to HarbourFront". Stop counts come from the route legs in `KioskRoutes`. The line highlighter moved from the right column onto the map panel (bottom left) to make room; the fare is kept on the card although the frames omit it.
