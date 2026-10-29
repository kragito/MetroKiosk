// Declarations for every kiosk screen. Each is a plain SCompoundWidget built from the shared session.
#pragma once

#include "CoreMinimal.h"
#include "KioskSession.h"
#include "Widgets/SCompoundWidget.h"

#define KIOSK_SCREEN_CLASS(ClassName) \
	class ClassName : public SCompoundWidget \
	{ \
	public: \
		SLATE_BEGIN_ARGS(ClassName) {} \
		SLATE_END_ARGS() \
		void Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& InSession); \
	};

KIOSK_SCREEN_CLASS(SHomeScreen)
KIOSK_SCREEN_CLASS(STicketTypeScreen)
KIOSK_SCREEN_CLASS(SDestinationScreen)
KIOSK_SCREEN_CLASS(SMapScreen)
KIOSK_SCREEN_CLASS(SReviewScreen)
KIOSK_SCREEN_CLASS(SPayMethodScreen)
KIOSK_SCREEN_CLASS(SWaitScreen)       // tap card / chip card / cash / processing, chosen from the session screen
KIOSK_SCREEN_CLASS(SReceiptScreen)
KIOSK_SCREEN_CLASS(SHelpCallScreen)
KIOSK_SCREEN_CLASS(SScanCardScreen)
KIOSK_SCREEN_CLASS(SCardOptionsScreen)
KIOSK_SCREEN_CLASS(STopUpScreen)
KIOSK_SCREEN_CLASS(SConcessionScreen)
KIOSK_SCREEN_CLASS(SPassDatesScreen)
KIOSK_SCREEN_CLASS(SDoneScreen)

#undef KIOSK_SCREEN_CLASS

/** Builds the widget for a screen. */
TSharedRef<SWidget> MakeKioskScreen(EKioskScreen Screen, const TSharedRef<FKioskSession>& Session);
