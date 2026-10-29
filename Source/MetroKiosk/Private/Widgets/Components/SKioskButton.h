// Touch-sized button: rounded fill + optional icons + label, with accessible text for the Slate inspector.
#pragma once

#include "CoreMinimal.h"
#include "KioskStyle.h"
#include "SKioskIcon.h"
#include "Widgets/SCompoundWidget.h"

class SKioskButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskButton)
		: _Kind(EKioskButtonKind::Primary)
		, _FontPx(24.f)
		, _Width(0.f)
		, _Height(0.f)
		, _Padding(FMargin(24.f, 0.f))
		, _LeftIcon(EKioskIcon::None)
		, _RightIcon(EKioskIcon::None)
		, _IconPx(0.f)
		, _Enabled(true)
		, _Bold(true)
		, _Spacing(12.f)
	{}
		SLATE_ARGUMENT(FText, Label)
		SLATE_ARGUMENT(EKioskButtonKind, Kind)
		SLATE_ARGUMENT(float, FontPx)
		SLATE_ARGUMENT(float, Width)
		SLATE_ARGUMENT(float, Height)
		SLATE_ARGUMENT(FMargin, Padding)
		SLATE_ARGUMENT(EKioskIcon, LeftIcon)
		SLATE_ARGUMENT(EKioskIcon, RightIcon)
		SLATE_ARGUMENT(float, IconPx)
		SLATE_ARGUMENT(TOptional<FLinearColor>, TextColor)
		SLATE_ARGUMENT(FName, ButtonTag)
		SLATE_ATTRIBUTE(bool, Enabled)
		SLATE_ARGUMENT(bool, Bold)
		SLATE_ARGUMENT(float, Spacing)
		SLATE_EVENT(FOnClicked, OnClicked)
		/** Optional custom content that replaces the icon + label row. */
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
