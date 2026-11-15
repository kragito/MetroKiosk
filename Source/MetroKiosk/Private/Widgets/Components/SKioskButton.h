// Touch-sized button: rounded fill + optional icons + label, with accessible text for the Slate inspector.
#pragma once

#include "CoreMinimal.h"
#include "KioskStyle.h"
#include "SKioskIcon.h"
#include "Widgets/SCompoundWidget.h"

/** Default label size and minimum height for each button role. */
float KioskButtonRoleFont(EKioskButtonKind Kind);
float KioskButtonRoleHeight(EKioskButtonKind Kind);

class SKioskButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskButton)
		: _Kind(EKioskButtonKind::Primary)
		, _FontPx(0.f)
		, _Width(0.f)
		, _Height(0.f)
		, _MinWidth(0.f)
		, _MinHeight(-1.f)
		, _MaxWidth(0.f)
		, _Padding(FMargin(28.f, 10.f))
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
		/** Exact size (icon buttons, keys). Prefer MinWidth/MinHeight so labels can grow or wrap. */
		SLATE_ARGUMENT(float, Width)
		SLATE_ARGUMENT(float, Height)
		SLATE_ARGUMENT(float, MinWidth)
		/** Defaults to the height of the button's role (see KioskLayout); 0 turns the minimum off. */
		SLATE_ARGUMENT(float, MinHeight)
		SLATE_ARGUMENT(float, MaxWidth)
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
