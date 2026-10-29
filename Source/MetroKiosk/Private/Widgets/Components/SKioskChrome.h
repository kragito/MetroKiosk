// Global chrome: header bar, step indicator band and footer.
#pragma once

#include "CoreMinimal.h"
#include "KioskSession.h"
#include "Widgets/SCompoundWidget.h"

/** Teal header (blue in help mode): logo, "You are at City Hall" + line badges, live clock. */
class SKioskHeader : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskHeader) : _bHelpMode(false) {}
		SLATE_ARGUMENT(bool, bHelpMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

/** Light band with four numbered steps. */
class SKioskStepBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskStepBar) {}
		SLATE_ARGUMENT(FKioskStepInfo, Steps)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

struct FKioskFooterConfig
{
	bool bBack = false;
	bool bStartOver = false;
	bool bBuyAnother = false;
	bool bNavEnabled = true;
};

/** Footer with Back / Start over, language pills and the Help button. */
class SKioskFooter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskFooter) {}
		SLATE_ARGUMENT(FKioskFooterConfig, Config)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& InSession);
};

/** Footer shown on the full-screen help call: Back to my purchase / End call. */
class SKioskHelpFooter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskHelpFooter) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& InSession);
};

/** Row of four language pills (used by the footer and the Home language card). */
TSharedRef<SWidget> MakeLanguagePills(const TSharedRef<FKioskSession>& Session, float Height, float FontPx, float Gap);
