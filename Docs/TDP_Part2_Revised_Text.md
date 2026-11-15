# TDP Part 2: revised documentation text (paste-ready)

Purpose: replace the wording in `Team05_TDP_Part2_Documentation.pdf` so that it (a) matches the current
kiosk, and (b) answers the instructor's comment that the citations "attempt to reference studied design
principles, but do not clearly demonstrate understanding or justify a specific application."

Pattern used for every decision: **Principle → what it means → why it applies to this user here → the design choice.**
The citations are the ones already in the team's reference list. Check the page numbers against your copy before submitting.

---

## 01 Our approach

The kiosk is built around one question: **do I have a transit card?** The home screen has three labelled columns:
**No card** (one-time ticket), **Have a card** (top up or concession pass) and **Help & settings** (language and video call).

- **Common region and proximity (Gestalt; DES214 Lecture 3).** Elements inside one bounded area, or placed
  close together, are read as one group. The three columns are three separate cards with their own headings,
  so a passenger reads them as three choices, not one long list. Each card now ends in the same full-width action
  button ("Buy a ticket →", "Top up →", "Buy or renew →"). An earlier version put an arrow on the card's right edge,
  which pointed into the neighbouring column and suggested a link between the groups. Moving the action inside
  the card keeps the grouping intact.
- **Two paths for two levels of experience (Norman, 2013, p. 216).** A design should support both first-time users
  (who need guidance) and experts (who want speed). Newcomers press a button and follow the steps; regular
  commuters just place their Metro card on the reader below the screen and skip straight to their options.
  Both paths end in the same Review and Pay screens, so there is only one way of paying to learn.

## 02 The kiosk itself

- **Signifiers (Norman, 2013, pp. 13–20).** A signifier is any perceivable cue that tells people where an action
  should happen. A first-time user doesn't know which slot does what, so every part they must touch has a light
  strip that turns green when the screen asks for it. The same position is also shown on screen with a small
  diagram of the kiosk (Metro card reader below the screen, left; tap-to-pay reader right, top; chip/NETS slot right,
  middle; note and coin slots right, lower; collection tray below the screen, centre). The light, the diagram and
  the words all describe the same place, so the instruction and the hardware cannot disagree.

## 03 Who we designed for

- **Regular commuters:** in a hurry, so they make *slips*: correct goal, wrong action (Norman, 2013, p. 174).
  Slips are prevented with large separated targets, a confirm step that shows the total, and a "Don't forget your
  card" reminder.
- **Elderly travellers:** near vision declines with age (Psychology course notes, Lifespan Development). Body text
  is never smaller than 18 px on the 32-inch screen, targets are large, contrast is high, and there is one main
  decision per screen.
- **Tourists:** no card and **no mental model of the network**. They usually know *where* they want to go
  (a landmark), not *which station* or *which line*. They can search by place name, misspell it, and still
  get the right station, route and fare, in their own language.

## 04 What every screen shares

- **Knowledge in the world vs. in the head (Norman, 2013, pp. 74–79).** People shouldn't have to remember what
  the world can show them. The header always shows the current station, so the start of every trip is visible
  rather than remembered.
- **Feedforward: where am I and what's left (Norman, 2013, pp. 38–40).** The step bar names each step and shows the
  current one. It always matches the screen (Ticket → Destination → Review → Pay; or Tap card → What to do? →
  Amount/Pass → Pay), so users can predict how much is left.
- **Consistency and memory (Psychology course notes, Memory).** Things that stay in the same place are found
  without searching. Back, Start over, the language buttons and Help are always in the same footer positions
  (where the step allows them). Buttons have fixed roles (green = continue/pay, navy = choose, outlined =
  secondary, blue = help) and the same sizes, corners and type scale on every screen.
- **Recoverable errors (Norman, 2013, pp. 203–205).** People make mistakes, so every choice can be undone before
  paying: Back, Change on Review, and Start over. Cash already inserted is returned with a message.
  An idle screen asks "Are you still there?" before resetting.

## 05 Use case: a first-time visitor buys a one-way ticket

**User:** Sam, a first-time adult tourist with no card, paying by phone. **Goal:** dinner at Gardens by the Bay.
**Setting:** City Hall MRT, 7:48 p.m.

1. **Home: common region.** Sam has no card, so the "No card" column is the only one that applies;
   he presses **Buy a ticket**.
2. **Choose a ticket: signifiers.** One-way, Round trip and Day pass use the same card layout, so they are compared
   by their differences only: icon, validity and price ("from $1.28"). Because the layout is identical, the eye goes
   to what differs. That is why the cards were made strictly consistent.
3. **Where to?: recognition over recall; reducing knowledge in the head.**
   *Recall* (producing a name from memory) is much harder than *recognition* (picking it from options)
   (Psychology course notes, Memory). Sam doesn't know the station is called "Bayfront" or which line it is on.
   He types "gardens" (or "gardn"); the kiosk recognises the landmark, forgives the spelling, and shows
   "Gardens by the Bay, near Bayfront". Popular destinations list *what's there* ("HarbourFront: VivoCity · Sentosa"),
   so he can recognise a goal instead of recalling a station. Line knowledge is not needed: the kiosk works out
   the route. The line filter moved into the optional map, so the screen asks for one decision.
4. **Check the trip: feedforward (Norman, 2013, pp. 71–72).** Feedforward tells people what will happen *before*
   they act. Before confirming and before paying, Sam sees the route ("Take North-South Line → change at Marina Bay
   → Circle Line"), the time (12 min, 1 change) and the fare ($1.64). He can add a child or senior and watch the
   total change before he commits.
5. **Tap to pay: signifiers, mapped to the hardware.** The screen says "Hold your card or phone on the reader,
   right of the screen, at the top", the on-screen kiosk diagram highlights that spot, and the green light strip
   marks the real reader. Three matching cues remove the question "where do I tap?". This matters most for a
   first-time user, who has no prior map of the machine.
6. **Payment approved: feedback (Norman, 2013, pp. 23–25).** Feedback must be immediate and informative, or
   people repeat actions or walk away unsure. The tick and "Payment approved" confirm success at once; the
   message says *where* the ticket is ("collection tray, below the screen, centre") while the tray lights up,
   directing attention to the next action. A visible 30-second countdown with "I need more time" stops the
   screen leaving before he's ready.
7. **Ticket in hand: feedback that matches.** The printed ticket repeats the route, ticket type, riders and validity
   shown on screen, so Sam can check that what he holds is what he bought.

## 06 Other supporting tasks

- **Top up a card: feedforward and slip prevention (Norman, 2013, pp. 71–72, 176–177).** Each quick amount shows
  the *new balance* before paying (feedforward). Because the card stays on the reader during a top-up, the classic
  slip is walking away without it (a post-completion error); the done screen says "Lift your card off the reader below
  the screen" and the reader detects a forgotten card.
- **Concession pass: anchoring.** The student price is shown next to the struck-out adult price, so the saving is
  judged against a reference. The user's current pass is pre-selected ("Your current pass"), and a two-month
  calendar shows the new pass starts the day after the old one ends, so no days are lost.
- **Call staff: feedback and recoverability (Norman, 2013, ch. 5).** Asking for help is often a response to an
  error, so the call must not create new ones. The purchase pauses, every countdown stops, and the screen says
  exactly what has happened to the money ("Your $2.00 is safe. Nothing more will be charged"), then the user
  returns to the same step.

## 07 Accessibility support

- **Older riders:** minimum 18 px text, high contrast, large targets, one main decision per screen
  (Norman, 2013, pp. 243–247).
- **Language:** English, 中文, Melayu and தமிழ், covering every instruction (including hardware directions). Station
  names appear in the chosen language with the English signage name beneath, so they match station signs.
- **Colour never alone (Psychology course notes, Sensation and perception):** line colours always carry their code
  (NS, EW…), and states always have text or an icon as well as colour, for colour-blind users.
- **Reach and hearing:** controls between 59 and 109 cm high; an audio jack for the spoken guide.

## 08 References (unchanged)

DES214 Perception, Composition & Programme. (2026). *Lecture 3* [Lecture slides].
Norman, D. A. (2013). *The design of everyday things* (Rev. and expanded ed.). Basic Books.
Psychology course notes. (2026). *Sensation and perception; Memory; Thinking and language; Lifespan development* [Lecture notes].

---

### Figures to replace
Use the screenshots in `MetroKiosk/Saved/Captures/`. Figure 1 → `01_Home.png`. Step 2 → `02_TicketType.png`.
Step 3 → `06_DestinationTypo.png` or `09_DestinationSelected.png`. Step 4 → `10_Review.png` (or `11_ReviewMixed.png`).
Step 5 → `15_WaitTapCard.png`. Step 6 → `21_Receipt.png`. Section 06 → `27_CardOptions.png`, `32_PassDates.png`,
`23_HelpCallCash.png`. Section 07 (language) → `51_HelpCallCash_Chinese.png`, `66_Review_Tamil.png`.
File numbers can shift when captures are regenerated; match by name.
