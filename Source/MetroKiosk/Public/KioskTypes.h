// Shared enums and the session state struct for the kiosk.
#pragma once

#include "CoreMinimal.h"

enum class EKioskScreen : uint8
{
	Home,
	TicketType,
	Destination,
	DestinationMap,
	Review,
	PayMethod,
	WaitTapCard,
	WaitChipCard,
	WaitCash,
	Processing,
	Receipt,
	HelpCall,
	ScanCard,
	CardOptions,
	TopUpAmount,
	Concession,
	PassDates,
	DoneLiftCard,
	Count
};

enum class EKioskFlow : uint8 { Ticket, TopUp, Concession };
enum class ETicketType : uint8 { OneWay, RoundTrip, DayPass };
enum class EPayMethod : uint8 { Tap, Chip, Cash };
enum class EKioskLanguage : uint8 { English, Chinese, Malay, Tamil };

/** Everything a customer has chosen so far in one visit. */
struct FKioskState
{
	EKioskFlow Flow = EKioskFlow::Ticket;
	ETicketType Ticket = ETicketType::OneWay;
	int32 Destination = INDEX_NONE;
	int32 Adults = 1;
	int32 Children = 0;
	int32 Seniors = 0;
	EPayMethod PayMethod = EPayMethod::Tap;
	int32 CashInsertedCents = 0;
	int32 TopUpCents = 0;
	int32 Concession = INDEX_NONE;
	int32 LineFilter = INDEX_NONE;
	FString SearchText;
	bool bKeyboardOpen = false;
	bool bTopUpKeypad = false;   // custom amount entry instead of the preset grid
	FString TopUpDigits;
};
