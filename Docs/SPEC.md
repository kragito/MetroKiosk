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
and telling people **where on the physical kiosk** to act (reader top-right,
chip slot middle-right, cash slots lower-right, collection tray bottom-right).

## Three entry flows (from the Home screen)

1. **No card → One-time ticket**
   Home → 1 Ticket type → 2 Destination (list / filter / search keyboard / map)
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
| 1 | Home | 01_home | 3 columns: NO CARD (big green "One-time ticket" card with ticket icon, chips "One-way · $1.68", "Day pass · $8.00", navy "Buy a ticket →"), HAVE A CARD ("Top up", "Concession" white cards with icons and →), HELP & SETTINGS (Language card with 4 pills; blue "Call for assistance" card with "Staff online" pill). Bottom banner: contactless icon, "Or place your card on the reader to begin", "We'll show your balance and what you can do with it", "Reader here ⌄⌄⌄" at the right. |
| 2 | Choose a ticket | 02_ticket_type | 3 cards: One-way $1.68; Round trip "$1.79 - $3.60" (struck "$2.56 - $5.14", 30% discount); Day pass $8.00 with tag "Best for 3+ rides". Each has a navy "Choose" button. Caption "Prices are for one adult. Add children or seniors on the Review step." Day pass skips Destination. |
| 3 | Where are you going? | 03_destination | Left: "Popular from City Hall" list (HarbourFront NE CC, Hume DT, Dhoby Ghaut NS NE CC, Marina South Pier NS CC), each "13 min · 1 change". Right: "Select a location" (selected display), "Somewhere else?" search field "Search all stations" + hint "A keyboard will popup. The list narrows with every letter you type.", "Filter by line" 6 pills (NS North–South, EW East–West, NE North East, CC Circle, DT Downtown, TE Thomson), "Open metro map view →", big "Confirm selected ticket →" (disabled grey until a station is picked, then primary). Filtering by line changes the list header to "Filtered by North-South Line". |
| 3b | Search keyboard | 03b_search_keyboard | On-screen QWERTY keyboard with Space / Clear. The typed text ("Har") filters the station list live. |
| 3c | Map | 03c_map | A schematic metro map with tappable stations, "You are here" marker at City Hall, "Find station on the map", "No station selected / Tap a station on the map to see the route, or find it by name below.", "Open station list view →". A simplified schematic is fine. |
| 3d | Destination selected | 03d_destination_selected_with_help_pip | The chosen station is shown ("City Hall → HarbourFront · about 13 min · 1 change"). |
| 4 | Check your trip | 04_review | Left: ticket summary card ("One-way ticket" tag, Change link, From City Hall NS EW → To HarbourFront NE CC, dashed note "Valid for 2 hours after purchase. About 14 min on the Blue line."). Right: "Who is riding?" Adult $1.68 each, Child (5–17) $1.18 each · 30% off · under 5 ride free, Senior (65+) $1.18 each · 30% off, each with − count + steppers (− outlined, + navy). "Total · N rider(s)" with total amount, green "Go to payment →". Can't go below 0 riders total (at least 1 rider). |
| 5 | Pay $X | 05_pay_method | 3 cards: "Tap card or phone" (Fastest tag, "Pay with card" navy), "Insert chip card" ("Pay with chip card/NETs"), "Pay with cash" ("Bills up to $20 and coins…", "Pay with cash"). Subtitle "Use any method below. The lit slot on the kiosk shows you where." and right-aligned summary "One-way · City Hall → Harbourfront · 1 adult". |
| 6a | Wait for card | 06_wait_tap_card | Pill "Paying **$1.68** · one-way to HarbourFront", "Hold your card or phone on the reader", explanation of where the reader is, status pill "● Waiting for your card · 9:59 left" (a real countdown), 3 tips (phone unlock / one card only / NETS reader below). Right: illustration of the reader with "Bank card". |
| 6b | Wait for chip card | 06_wait_chip_card | "Insert your card, chip first" with the chip-slot illustration "Chip first, facing up", "Chip slot · middle right". |
| 6c | Wait for cash | 06_wait_cash | Panel: To pay / Put in / Still to pay (navy), progress bar, Notes $2 $5 $10, Coins 10¢ 20¢ 50¢ $1 (tapping simulates inserting money), tray note. Change is shown when overpaying. |
| 6d | Processing | (no ref) | Same as 6a/6b/6c but the status pill reads "Processing..." and inputs are disabled; it advances automatically after ~2 s. |
| 7 | Payment approved | 09_receipt | Green check "Payment approved", "Your ticket is printing. **Take it from the collection tray, bottom right.**", ticket summary card (ONE-WAY · ADULT, WED 14 OCT, City Hall → HarbourFront, Valid until 9:49 pm, $1.68). Countdown bar "Returning to start in 9 s". Right card "Want a receipt?": Print receipt, Email receipt, green "No receipt · I'm done". Footer has "+ Buy another ticket" instead of Back. |
| 8 | Help call | 10_help_video_call | Blue header "Help · In call with staff". Dark video placeholder "[Staff video feed]" with "● Connected · 0:24" (counting up). LIVE CAPTIONS box. Right: "Your purchase is paused. Nothing has been charged. You can pick up where you left off.", "Mute microphone", "Ask staff to come here", "Captions are on" toggles, footnote about the staffed office at Gate B. Footer: green "‹ Back to my purchase" and red outlined "✕ End call". Returning restores the exact previous screen and state. |
| 9 | Scan card | (no ref) | "Place your transit card on the reader", "The reader is to the right of the screen, at the top. It glows green when it's ready. Hold the card still until this screen changes.", "Waiting for your card…", "Not reading? Take the card out of your wallet or phone case, then try again.", Metro card illustration. Simulated tap advances. |
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

One-way $1.68 adult, child/senior $1.18. Round trip $1.79–$3.60 depending on the
destination (use the one-way × 2 with 30% off rule: $1.68×2×0.7 → $2.35 for HarbourFront, fine to round).
Day pass $8.00 (no destination, no child pricing needed except as listed).
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
