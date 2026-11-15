# TDP Part 2: video storyboard

Addresses the instructor's video comments: "use cases could be improved through more context and telling the
story of a specific user, rather than generically describing the features" and "not all required interactions
are demonstrated."

Format: four short stories (about 60–90 s each), each with **who, goal, setting, problem, what the kiosk does, outcome**.
Narrate the *user's* intent, not the feature list. Every interaction in the checklist at the end appears at least once.

Recording: each scene has a matching script in `Tools/journeys/` (or a variant), so the screens are reproducible.
Run one with `powershell -File Tools/journey.ps1 -Script (Get-Content Tools/journeys/<name>.txt -Raw) -Out <dir>`
for stills, or play the same steps by hand in a `-game -windowed -ResX=1917 -ResY=1105` window while screen-recording.
Hardware is simulated with the dashed "SIMULATE" controls; in the edit, cut to the kiosk illustration (Figure 3) when
a card is tapped or cash is inserted.

---

## Scene 0: Context (10 s)
City Hall MRT, evening rush. Show Figure 3 (the physical kiosk), then the Home screen. VO: "Four people, one kiosk."

## Scene 1: Sam, first-time tourist (no card, knows the place but not the station)
- **Goal:** dinner at Gardens by the Bay. **Problem:** doesn't know the station name, the lines, or the exact spelling.
- Home → **Buy a ticket** → **One-way** (VO: the cards differ only in icon, validity and price).
- Destination: types **"gardn"** (typo) → "Gardens by the Bay, near Bayfront" appears → selects it.
  The card shows **Take North-South Line → change at Marina Bay → Circle Line · 12 min · $1.64**.
- Opens **See it on the map** briefly, then **Back** to the list (optional view, not required).
- Review: adds **1 child** → total updates to $2.79. Presses **Change** to show the destination can be corrected, then confirms again.
- Pay → **Tap card or phone** → the screen and diagram point to the reader, right top → simulate the tap → Processing → **Payment approved**,
  ticket in the tray. Presses **I need more time**, then **Print receipt**.
- **Outcome:** Sam never needed to know "Bayfront" or the line colours.
- Script base: `a_tourist_gardens`, `b_typo_harbourfront`, `k7_map_back_to_list`, `k4_receipt_more_time`.

## Scene 2: Mdm Tan, 72, prefers Chinese, pays cash
- **Goal:** top up her card before the bus. **Problem:** reads English poorly, isn't sure where to put her card or money.
- Home → **中文** (whole UI switches, including hardware instructions) → places her card on the reader below the screen (simulate) →
  Card options shows her balance and "Low balance".
- **+$10** quick top-up shows the new balance **before** paying → **Pay with cash** → inserts **$5, $5** (and one extra note to show change)
  → "Still to pay" reaches $0 → change message → **Lift your card off the reader** reminder.
- Shows the **"Are you still there?"** prompt if she pauses (cut in from `k5_idle_prompt`), and that Continue keeps her place.
- **Outcome:** done without English and without leaving her card behind.
- Script base: `k2_zh_topup_cash`, `j2_topup_keypad` (show **More → 25** once for "any amount"), `k5_idle_prompt`.

## Scene 3: Priya, student commuter in a hurry
- **Goal:** renew her monthly Bus + Train concession before class. **Problem:** time.
- Places her card → **Renew concession pass** → her **current pass is pre-selected** → **Continue · check dates** → the two-month calendar shows
  the new pass starts the day after the old one ends → **Go to payment** → **Insert chip card** (show the chip/NETS slot direction) → done.
- Taps only 5 times. VO: the expert shortcut (card first) vs. the novice path (buttons).
- Script base: `k6_concession_renewal` (switch the payment step to the chip card for coverage).

## Scene 4: Ahmad gets confused mid-purchase
- **Goal:** buy a **Day pass** at 9:30 p.m. **Problem:** the late-night warning makes him unsure whether it's worth it; he has already put in $2 cash.
- Ticket type shows "Only 2 h of service left today. A one-way may be cheaper." (run with `-KioskFakeTime=21:30`).
- Chooses Day pass anyway (it skips Destination), inserts **$2**, then presses **Help · talk to staff** → the call screen: "Your $2.00 is safe…",
  captions on, the payment timer paused → **Back to my purchase** → returns to the exact payment step with the call in picture-in-picture.
- Decides against it → **Start over** → "Your $2.00 has been returned to the collection tray."
- **Outcome:** help never costs him money or progress.
- Script base: `d_daypass_skips_destination`, `k3_help_during_cash`, `j3_help_restore`.

## Scene 5: Wrap-up (10 s)
Quick montage: round trip in தமிழ், Melayu Home, the map with a line highlighted. VO restates the one question ("Do I have a card?") and the outcome.

---

## Required-interactions checklist (tick each in the edit)

| Interaction | Scene |
|---|---|
| Home: three groups; card-first shortcut | 0, 2, 3 |
| Language switch (中文 / தமிழ் / Melayu) | 2, 5 |
| One-way / Round trip / Day pass selection; late-night Day pass warning | 1, 4, 5 |
| Destination: popular list, search with typo, place search, map (pan/zoom, line highlight), back to list | 1, 5 |
| Review: riders stepper (+/−), Change destination, totals | 1 |
| Pay by tap (card/phone) | 1 |
| Pay by chip card / NETS | 3 |
| Pay by cash with progress and change; cash returned on Start over | 2, 4 |
| Processing → approved; receipt print/email; "I need more time" | 1 |
| Top-up: quick amount with new balance; More → custom amount | 2 |
| Concession: pre-selected pass, dates check | 3 |
| Lift-your-card reminder | 2, 3 |
| Help call: pause, money status, captions, back to purchase, PIP, end call | 4 |
| Back / Start over / idle "Are you still there?" | 1, 2, 4 |

If the assignment brief lists required interactions, compare it with this table and add any that are missing.
