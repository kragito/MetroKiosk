// Simplified schematic metro map: coloured line polylines with tappable station dots.
#pragma once

#include "CoreMinimal.h"
#include "KioskSession.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FOnKioskStationTapped, int32 /*StationIndex*/);

class SKioskMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskMap) {}
		SLATE_ARGUMENT(int32, SelectedStation)
		SLATE_ARGUMENT(float, Zoom)
		SLATE_EVENT(FOnKioskStationTapped, OnStationTapped)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(900.f, 600.f); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	/** Maps schematic space to widget-local space. */
	FVector2f ToLocal(const FVector2f& MapPos, const FVector2f& LocalSize) const;

	int32 SelectedStation = INDEX_NONE;
	float Zoom = 1.f;
	FOnKioskStationTapped OnStationTapped;
};
