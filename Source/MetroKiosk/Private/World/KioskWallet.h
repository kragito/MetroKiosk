// The visitor's wallet (Metro card, bank card, notes, coins) and the rules for which 3D kiosk part takes which item on which screen.
#pragma once

#include "CoreMinimal.h"

class FKioskSession;

/** The clickable hardware on the 3D kiosk cabinet (see Docs/ref/figma/64-75_kiosk_hardware.png). */
enum class EKioskPart : uint8
{
	None,
	Screen,       // the touch screen: used by pointing at it
	Contactless,  // contactless reader: bank card or phone tap
	ChipSlot,     // chip card / NETS slot
	NoteSlot,
	CoinSlot,
	TicketSlot,   // where the ticket and receipt come out (they land in the tray)
	ChangeTray,   // collection tray: ticket, receipt and change
	CardPad,      // Metro card pad
};

enum class EWalletKind : uint8 { MetroCard, BankCard, Note, Coin };

struct FKioskWalletItem
{
	EWalletKind Kind = EWalletKind::Note;
	int32 Cents = 0; // notes and coins only
	FString CashLabel; // "$5", "10¢" (cash only)
};

namespace KioskWallet
{
	/** Metro card, bank card, then the notes and coins the kiosk accepts. Index 0-9 are the number keys 1-9, 0. */
	const TArray<FKioskWalletItem>& Items();
	FText ItemLabel(const FKioskWalletItem& Item);

	struct FUseResult
	{
		bool bAccepted = false;
		FText Message; // what the visitor is told: why a part refused, or what just happened
	};

	/** Uses Item (may be null: nothing held) on an input part. The part decides from the item and the current screen; an accepted item drives the session. */
	FUseResult Use(FKioskSession& Session, const FKioskWalletItem* Item, EKioskPart Part);

	/** True when the screen is asking for this part now (its light goes green). */
	bool IsPartWanted(const FKioskSession& Session, EKioskPart Part);
}
