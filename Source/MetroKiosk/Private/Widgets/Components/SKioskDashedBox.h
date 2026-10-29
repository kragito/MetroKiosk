// A filled panel with a dashed outline. Used for notes and for the "SIMULATE" hardware affordances.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SKioskDashedBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskDashedBox)
		: _Fill(FLinearColor::Transparent)
		, _DashColor(FLinearColor::Black)
		, _Padding(FMargin(16.f))
		, _Radius(10.f)
	{}
		SLATE_ARGUMENT(FLinearColor, Fill)
		SLATE_ARGUMENT(FLinearColor, DashColor)
		SLATE_ARGUMENT(FMargin, Padding)
		SLATE_ARGUMENT(float, Radius)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FLinearColor Fill;
	FLinearColor DashColor;
	float Radius = 10.f;
};
