// Simplified schematic metro map: coloured line polylines with tappable station dots.
#pragma once

#include "CoreMinimal.h"
#include "KioskSession.h"
#include "Widgets/SLeafWidget.h"

struct FKioskMapShapeCache;
struct FKioskMapLayout;

DECLARE_DELEGATE_OneParam(FOnKioskStationTapped, int32 /*StationIndex*/);

class SKioskMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SKioskMap) {}
		SLATE_ARGUMENT(int32, SelectedStation)
		SLATE_ARGUMENT(float, Zoom)
		SLATE_ARGUMENT(TSharedPtr<FKioskSession>, Session)
		/** Stations that match the typed search. Used only when bDimOthers is true. */
		SLATE_ARGUMENT(TSet<int32>, LitStations)
		/** True while the search text matches something: every station not in LitStations fades. */
		SLATE_ARGUMENT(bool, bDimOthers)
		SLATE_EVENT(FOnKioskStationTapped, OnStationTapped)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(900.f, 600.f); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	/** Maps schematic space to widget-local space. */
	FVector2f ToLocal(const FVector2f& MapPos, const FVector2f& LocalSize) const;
	/** Pixels per map unit on each axis (before zoom) and the map point shown at the widget centre (before pan). */
	void ViewParams(const FVector2f& LocalSize, FVector2f& OutScale, FVector2f& OutFocus) const;
	/** Keeps the network's bounding box overlapping the viewport (most of it) at every zoom and centre mode. */
	FVector2f ClampPan(const FVector2f& Pan, const FVector2f& LocalSize) const;

	int32 SelectedStation = INDEX_NONE;
	float Zoom = 1.f;
	TSharedPtr<FKioskSession> Session;
	TSet<int32> LitStations;
	bool bDimOthers = false;
	FVector2f DragLast = FVector2f::ZeroVector;
	FVector2f DragStart = FVector2f::ZeroVector;
	bool bPressed = false;
	bool bDragging = false;
	FOnKioskStationTapped OnStationTapped;

	/** Shaped label text and the last label/pill placement; owned by the widget so they die with it, rebuilt when an input changes. */
	mutable TSharedPtr<FKioskMapShapeCache> ShapeCache;
	mutable TSharedPtr<FKioskMapLayout> LayoutCache;
};
