// Vector icons drawn with Slate primitives (no texture assets needed).
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

enum class EKioskIcon : uint8
{
	None,
	ArrowRight,
	ArrowLeft,
	ArrowUp,
	ArrowDown,
	ChevronLeft,
	ChevronRight,
	ChevronDown,
	DoubleChevronDown,
	Check,
	Close,
	Plus,
	Minus,
	Ticket,
	TopUpCard,
	IdCard,
	Headset,
	Contactless,
	Globe,
	Map,
	List,
	Search,
	Clock,
	Swap,
	Person,
	Mic,
	Pause,
	Printer,
	Mail,
	Backspace,
	Bus,
	Train,
	Restart,
	Tray,
	Captions,
	Cash,
	ChipCard,
	Reader,
};

class SKioskIcon : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskIcon)
		: _Icon(EKioskIcon::None)
		, _Size(40.f)
		, _Color(FLinearColor::White)
		, _Thickness(0.f)
	{}
		SLATE_ARGUMENT(EKioskIcon, Icon)
		SLATE_ARGUMENT(float, Size)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(float, Thickness)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	EKioskIcon Icon = EKioskIcon::None;
	float Size = 40.f;
	FLinearColor Color = FLinearColor::White;
	float Thickness = 0.f;
};
