// Wallet contents and the accept / refuse rules of each input part on the 3D kiosk.
#include "KioskWallet.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskSession.h"

namespace
{
	FKioskWalletItem MakeCard(EWalletKind Kind)
	{
		FKioskWalletItem Item;
		Item.Kind = Kind;
		return Item;
	}
}

const TArray<FKioskWalletItem>& KioskWallet::Items()
{
	static TArray<FKioskWalletItem> Wallet;
	if (Wallet.Num() == 0)
	{
		Wallet.Add(MakeCard(EWalletKind::MetroCard));
		Wallet.Add(MakeCard(EWalletKind::BankCard));
		for (const FKioskCashDenom& Denom : FKioskData::CashDenominations())
		{
			FKioskWalletItem Item;
			Item.Kind = Denom.bCoin ? EWalletKind::Coin : EWalletKind::Note;
			Item.Cents = Denom.Cents;
			Item.CashLabel = Denom.Label;
			Wallet.Add(Item);
		}
	}
	return Wallet;
}

FText KioskWallet::ItemLabel(const FKioskWalletItem& Item)
{
	switch (Item.Kind)
	{
	case EWalletKind::MetroCard: return KT("Metro card");
	case EWalletKind::BankCard: return KT("Bank card");
	default: return FText::FromString(Item.CashLabel);
	}
}

bool KioskWallet::IsPartWanted(const FKioskSession& Session, EKioskPart Part)
{
	switch (Part)
	{
	case EKioskPart::Contactless: return Session.Current() == EKioskScreen::WaitTapCard;
	case EKioskPart::ChipSlot: return Session.Current() == EKioskScreen::WaitChipCard;
	case EKioskPart::NoteSlot:
	case EKioskPart::CoinSlot: return Session.Current() == EKioskScreen::WaitCash;
	case EKioskPart::CardPad: return Session.Current() == EKioskScreen::ScanCard || Session.Current() == EKioskScreen::DoneLiftCard;
	default: return false;
	}
}

KioskWallet::FUseResult KioskWallet::Use(FKioskSession& Session, const FKioskWalletItem* Item, EKioskPart Part)
{
	FUseResult Result;
	const EKioskScreen Screen = Session.Current();
	auto Refuse = [&Result](const FText& Message) { Result.bAccepted = false; Result.Message = Message; return Result; };
	auto Accept = [&Result](const FText& Message) { Result.bAccepted = true; Result.Message = Message; return Result; };

	// Taking the Metro card back off the pad needs no item in hand.
	if (Part == EKioskPart::CardPad && Screen == EKioskScreen::DoneLiftCard)
	{
		Session.SimulateCardLift();
		return Accept(KT("You took your Metro card back"));
	}
	if (Part == EKioskPart::TicketSlot)
	{
		return Refuse(KT("Nothing goes in here. Your ticket and receipt come out here and land in the tray."));
	}
	if (Part == EKioskPart::ChangeTray)
	{
		return Refuse(KT("The tray is for taking things out. Click it with nothing in hand."));
	}
	if (!Item)
	{
		return Refuse(KT("Choose something from your wallet first: press its number key or click it."));
	}

	if (Screen == EKioskScreen::Processing)
	{
		return Refuse(KT("Payment in progress. Please wait."));
	}

	const bool bIsCard = Item->Kind == EWalletKind::MetroCard || Item->Kind == EWalletKind::BankCard;
	switch (Part)
	{
	case EKioskPart::CardPad:
		if (Item->Kind != EWalletKind::MetroCard)
		{
			return Refuse(KT("Only a Metro card goes on this pad."));
		}
		if (Screen == EKioskScreen::Home || Screen == EKioskScreen::ScanCard)
		{
			Session.SimulateMetroCardTap(FKioskSession::DefaultCardType());
			return Accept(KT("Metro card on the pad"));
		}
		return Refuse(KT("The screen is not asking for your Metro card now."));

	case EKioskPart::Contactless:
		if (!bIsCard)
		{
			return Refuse(KT("Hold a card here. Cash goes in the note and coin slots."));
		}
		if (Screen == EKioskScreen::WaitTapCard)
		{
			Session.SimulateCardTap();
			return Accept(KT("Card tapped"));
		}
		if (Item->Kind == EWalletKind::MetroCard && (Screen == EKioskScreen::Home || Screen == EKioskScreen::ScanCard))
		{
			return Refuse(KT("Put your Metro card on the round pad below the screen."));
		}
		return Refuse(KT("Not now: choose Pay with card on the screen first."));

	case EKioskPart::ChipSlot:
		if (Item->Kind != EWalletKind::BankCard)
		{
			return Refuse(KT("The chip slot takes a bank card with a chip."));
		}
		if (Screen == EKioskScreen::WaitChipCard)
		{
			Session.SimulateCardTap();
			return Accept(KT("Chip card inserted"));
		}
		return Refuse(KT("Not now: choose Pay with chip card/NETs on the screen first."));

	case EKioskPart::NoteSlot:
		if (Item->Kind == EWalletKind::Coin)
		{
			return Refuse(KT("The note slot takes notes only. Coins go in the coin slot."));
		}
		if (Item->Kind != EWalletKind::Note)
		{
			return Refuse(KT("The note slot takes notes only."));
		}
		if (Screen == EKioskScreen::WaitCash)
		{
			Session.InsertCash(Item->Cents);
			return Accept(FText::Format(KT("Inserted {0}"), FText::FromString(Item->CashLabel)));
		}
		return Refuse(KT("Not now: choose Pay with cash on the screen first."));

	case EKioskPart::CoinSlot:
		if (Item->Kind == EWalletKind::Note)
		{
			return Refuse(KT("The coin slot takes coins only. Notes go in the note slot."));
		}
		if (Item->Kind != EWalletKind::Coin)
		{
			return Refuse(KT("The coin slot takes coins only."));
		}
		if (Screen == EKioskScreen::WaitCash)
		{
			Session.InsertCash(Item->Cents);
			return Accept(FText::Format(KT("Inserted {0}"), FText::FromString(Item->CashLabel)));
		}
		return Refuse(KT("Not now: choose Pay with cash on the screen first."));

	default:
		break;
	}
	return Refuse(FText::GetEmpty());
}
