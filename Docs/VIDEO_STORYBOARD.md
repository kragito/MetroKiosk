# TDP Part 2: video storyboard

Addresses the instructor's video comments: "use cases could be improved through more context and telling the
story of a specific user, rather than generically describing the features" and "not all required interactions
are demonstrated."

Format: four short stories (about 60–90 s each), each with **who, goal, setting, problem, what the kiosk does, outcome**.
Narrate the *user's* intent, not the feature list. Every interaction in the checklist at the end appears at least once.

## Recording

The app now starts in the 3D metro concourse, so there is no cut to an illustration and nothing is simulated. The hardware
is the real 3D kiosk: the person walks up, steps up to it, takes things from a wallet and uses the kiosk's own parts.

**Live (the main recording).** Run `powershell -File Tools/play_3d.ps1` and screen-record the 1917x1105 window (add
`-StartAtKiosk` to start already stepped up). Controls:
- Walk with WASD or the arrow keys, look with the mouse. Near the kiosk press **E**: the camera locks on and the mouse becomes a
  cursor that clicks the 3D screen and the kiosk parts. **Esc** steps back.
- Wallet along the bottom: Metro card (1), bank card (2), notes $2 $5 $10 $50 (3–6), coins 10¢ 20¢ 50¢ $1 (7, 8, 9, 0).
  Choose an item (key or click), then click the matching part.
- Parts, with what each takes: the round **Metro card pad** under the screen (a Metro card on Home or Scan card; clicked with
  empty hands at the end it lifts the card), the **contactless reader** (bank or Metro card on "Pay with card"), the
  **chip/NETS slot** (bank card), the **note slot**, the **coin slot**, the **ticket/receipt slot**, and the **collection tray**
  (click it with empty hands to collect the ticket, receipt and change). A part's light turns green when the screen asks for it.
  A wrong item or wrong moment is refused with a hint line; show that once (Scene 1: a coin at the contactless reader, "Hold a card here").
- The Metro card on the pad is the student card by default (the adult card needs `-KioskCard=Adult` on the manual launch line below).

**Late-night clock (Scene 4).** `play_3d.ps1` has no switch for the fake clock. Launch by hand from the repo root in PowerShell
(same arguments as the script, plus the clock):
```powershell
. .\Tools\engine.ps1; $Engine = Join-Path (Get-UnrealRoot) "Engine"
& "$Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\MetroKiosk.uproject" -game -windowed -ResX=1917 -ResY=1105 -nosplash -nosound -Kiosk3DStart=kiosk -KioskFakeTime=21:30
```
Build first with `powershell -File Tools/build.ps1` if the sources changed, and close the Unreal editor.

**Stills.**
- 3D stages (cash, card, top-up, concession): `powershell -File Tools/play_3d.ps1 -SelfTest -ShotDir <dir>`.
- Kiosk close-up (card on the pad, lit parts, full tray): `powershell -File Tools/play_3d.ps1 -Hardware -Shot <dir>\kiosk.png`.
- Concourse overview: `powershell -File Tools/play_3d.ps1 -Wide -Shot <dir>\concourse.png`.
- Flat screens: `powershell -File Tools/journey.ps1 -Script (Get-Content Tools/journeys/<name>.txt -Raw) -Out <dir>`
  (it adds `-Kiosk2D`; payments and card taps are the hidden `pay:` / `card:` steps, not controls on screen; add
  `-Extra "-KioskFakeTime=21:30"` for the late-night screens), or every screen at once with `powershell -File Tools/capture.ps1`
  (writes `Saved/Captures`).

Each scene's screens also have a matching script in `Tools/journeys/`, so they are reproducible; when recording live, play the
same steps by hand in the 3D window.

Figures to read off the screen when recording (not fixed here): the route sentence on Gardens by the Bay's map card, and the
late-night warning's hours at 21:30.

---

## Scene 0: Context (10 s)
City Hall MRT, evening rush. Wide shot of the concourse (`-Wide`), then walk up to the kiosk, press **E**, and the Home screen fills
the view. VO: "Four people, one kiosk."

## Scene 1: Sam, first-time tourist (no card, knows the place but not the station)
- **Goal:** dinner at Gardens by the Bay. **Problem:** doesn't know the station name, the lines, or the exact spelling.
- Home → **Buy a ticket** → **One-way** (VO: the cards differ only in icon, validity and price).
- Destination: types **"gardn"** (typo) → "Gardens by the Bay" appears → selects it. Gardens by the Bay is its own station
  (Thomson-East Coast Line, TE): the card reads **18 min · 1 change · $1.52**.
- Opens **Open metro map view**: it opens zoomed in on City Hall; presses **−** to see the whole network; the route card spells out
  each leg. Then **Back** to the list (optional view, not required).
- Review: "Valid for 2 hours after use." and the short trip line "About 18 min · 1 change."; adds **1 child** (30% off, **$1.06**) →
  total **$2.58**. Taps the **Change** link to show the destination can be corrected, then confirms again.
- Pay → **Pay with card** → the screen and the lit contactless reader show where to tap. Sam first tries a coin at the reader and gets the hint, then chooses the bank card (2) and clicks the contactless reader → Processing →
  **Payment approved**; the ticket and receipt drop into the tray, which glows. Clicks the tray with empty hands to collect them.
  Presses **I need more time**, then **Print receipt**.
- **Outcome:** Sam never needed to know "Bayfront" or the line colours.
- Script base: `a_tourist_gardens`, `b_typo_harbourfront`, `k7_map_back_to_list`, `t060_map_opening_view`, `l10_review_figma`,
  `k4_receipt_more_time`. (`a_tourist_gardens` types the full name; for the typo use `type:gardn`.)

## Scene 2: Mdm Tan, 72, prefers Chinese, pays cash
- **Goal:** top up her card before the bus. **Problem:** reads English poorly, isn't sure where to put her card or money.
- Home → **中文** (whole UI switches, including hardware instructions; station and place names show in Chinese too, on the map as well).
  Places her Metro card on the round pad under the screen (wallet 1, click the pad) → Card options shows her balance and "Low balance".
- **+$10** quick top-up shows the new balance (**$13.20**) **before** paying → **Pay with cash** → the note slot lights; inserts
  **$5, $5** (wallet 4 twice, click the note slot) → "Still to pay" reaches $0 → "Don't forget your card" → clicks the pad with empty
  hands to **lift the card**. Optionally a **$10** note plus a **$5** to overpay and show change landing in the tray; add a coin to show the coin slot.
- Shows the **"Are you still there?"** prompt if she pauses (cut in from `k5_idle_prompt`), and that Continue keeps her place.
- **Outcome:** done without English and without leaving her card behind.
- Script base: `k2_zh_topup_cash`, `j2_topup_keypad` (show **More → 25** once for "any amount"), `k5_idle_prompt`.
- Optional search beat for a Chinese reader: pinyin works too, for example "niuchesui" finds Chinatown (`l9_pinyin_search`).

## Scene 3: Priya, student commuter in a hurry
- **Goal:** renew her monthly Bus + Train concession before class. **Problem:** time.
- Places her card on the pad → **Renew concession pass** → her **current pass is pre-selected** → **Continue · check dates** → the two-month
  calendar shows the new pass starts the day after the old one ends → **Go to payment** → **Pay with chip card/NETs** → chooses the bank
  card (2) and clicks the **chip slot** (the camera can stay on the close-up) → "Don't forget your card" → clicks the pad to lift the card, then the tray for the receipt.
- Taps only 5 times. VO: the expert shortcut (card first) vs. the novice path (buttons).
- Script base: `k6_concession_renewal` (it pays by tap; pick the chip option by hand for the chip slot).

## Scene 4: Ahmad gets confused mid-purchase
- **Goal:** buy a **Day pass** at 9:30 p.m. **Problem:** the late-night warning makes him unsure whether it's worth it; he has already put in $2 cash.
- Ticket type shows "Only {n} h of service left today. A one-way may be cheaper." (launch with `-KioskFakeTime=21:30`, see Recording).
- Chooses Day pass anyway ($8.00; it skips Destination), inserts a **$2** note at the note slot, then presses **Help · talk to staff** →
  the call screen: "Your $2.00 is safe…", captions on, the payment timer paused → **Back to my purchase** → returns to the exact payment
  step with the call in a small pill in the step bar.
- Decides against it → **Start over** → "Your $2.00 has been returned to the collection tray." This is on screen only: in 3D the tray
  does not fill when cash is returned on Start over (it only fills with the ticket, receipt and change after a payment), so let the
  screen message carry it.
- **Outcome:** help never costs him money or progress.
- Script base: `d_daypass_skips_destination`, `k3_help_during_cash`, `j3_help_restore`.

## Scene 5: Wrap-up (10 s)
Quick montage: round trip in தமிழ், Melayu Home (Malay and Tamil wording now complete, no English left over), the whole-network map with a line
highlighted, then step back from the kiosk with **Esc** into the concourse. VO restates the one question ("Do I have a card?") and the outcome.
Script base: `l7_map_labels_languages`, `t060_map_opening_view`.

---

## Required-interactions checklist (tick each in the edit)

| Interaction | Scene |
|---|---|
| Walk up to the kiosk (WASD, mouse); press **E** to step up, **Esc** to step back | 0, 5 |
| Wallet: choose an item (key or click), then click the matching part | 1, 2, 3, 4 |
| Wrong item or wrong moment refused with a hint | 1 |
| Metro card pad: tap on Home / Scan card, and lift the card with empty hands | 2, 3 |
| Contactless reader (bank card) | 1 |
| Chip/NETS slot (bank card) | 3 |
| Note slot and coin slot (cash) | 2, 4 (use one coin in Scene 2 so the coin slot is shown) |
| Collection tray: collect ticket, receipt and change with empty hands | 1, 2, 3 |
| Home: three groups; card-first shortcut | 0, 2, 3 |
| Language switch (中文 / தமிழ் / Melayu), names in the language, also on the map | 2, 5 |
| One-way / Round trip / Day pass selection; late-night Day pass warning | 1, 4, 5 |
| Destination: popular list, search with typo, place search, pinyin search | 1, 2 |
| Map: opens zoomed in on City Hall; − to the whole network; pan/zoom; line highlight; back to list | 1, 5 |
| Review: riders stepper (+/−), Change link, trip line, totals | 1 |
| Pay by tap (bank card on the contactless reader) | 1 |
| Pay by chip card / NETS | 3 |
| Pay by cash with progress and change; cash returned on Start over | 2, 4 |
| Processing → approved; receipt print/email; "I need more time" | 1 |
| Top-up: quick amount with new balance; More → custom amount | 2 |
| Concession: pre-selected pass, dates check | 3 |
| Lift-your-card reminder | 2, 3 |
| Help call: pause, money status, captions, back to purchase, call pill, end call | 4 |
| Back / Start over / idle "Are you still there?" | 1, 2, 4 |

If the assignment brief lists required interactions, compare it with this table and add any that are missing.
