#include "SKioskMap.h"

#include "KioskData.h"
#include "KioskStyle.h"
#include "Rendering/DrawElements.h"

namespace
{
	const FVector2f GMapMin(40.f, 20.f);
	const FVector2f GMapMax(930.f, 580.f);
	constexpr float HitRadius = 30.f;
}

void SKioskMap::Construct(const FArguments& InArgs)
{
	SelectedStation = InArgs._SelectedStation;
	Zoom = FMath::Max(1.f, InArgs._Zoom);
	OnStationTapped = InArgs._OnStationTapped;
	SetCanTick(false);
}

FVector2f SKioskMap::ToLocal(const FVector2f& MapPos, const FVector2f& LocalSize) const
{
	const FVector2f MapSize = GMapMax - GMapMin;
	const float BaseScale = FMath::Min((LocalSize.X - 90.f) / MapSize.X, (LocalSize.Y - 110.f) / MapSize.Y);
	const FVector2f Center = (GMapMin + GMapMax) * 0.5f;
	FVector2f Focus = Center;
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	if (Zoom > 1.01f && Stations.IsValidIndex(SelectedStation))
	{
		Focus = Stations[SelectedStation].MapPos;
	}
	return LocalSize * 0.5f + (MapPos - Focus) * (BaseScale * Zoom);
}

int32 SKioskMap::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;
	const FVector2f Size = AllottedGeometry.GetLocalSize();
	const FPaintGeometry Paint = AllottedGeometry.ToPaintGeometry();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	const float LineWidth = 7.f * FMath::Sqrt(Zoom);

	// Lines
	for (int32 Line = 0; Line < FKioskData::NumLines; ++Line)
	{
		for (const TArray<int32>& Path : FKioskData::LinePaths(Line))
		{
			TArray<FVector2f> Points;
			for (int32 StationIndex : Path)
			{
				Points.Add(ToLocal(Stations[StationIndex].MapPos, Size));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Paint, Points, ESlateDrawEffect::None, C.Line[Line], true, LineWidth);
		}
	}

	// Station dots and labels
	const FSlateFontInfo LabelFont = Style.Font(14.f, true);
	const FSlateFontInfo MarkerFont = Style.Font(17.f, true);
	for (int32 Index = 0; Index < Stations.Num(); ++Index)
	{
		const FKioskStation& Station = Stations[Index];
		const FVector2f Pos = ToLocal(Station.MapPos, Size);
		const bool bInterchange = FMath::CountBits(Station.LineMask) > 1;
		const bool bSelected = Index == SelectedStation;
		const bool bHere = Index == FKioskData::CityHallIndex();
		const float Radius = (bSelected ? 17.f : (bInterchange ? 11.f : 7.f)) * FMath::Sqrt(Zoom);

		const FSlateBrush* Dot = bSelected
			? Style.Round(C.Primary, Radius, C.White, 4.f)
			: bHere
				? Style.Round(C.Help, Radius * 1.3f, C.White, 4.f)
				: Style.Round(C.White, Radius, bInterchange ? C.Ink : C.Line[Station.FirstLine()], bInterchange ? 4.f : 3.f);
		const float Draw = (bHere && !bSelected) ? Radius * 1.3f : Radius;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(Draw * 2.f, Draw * 2.f), FSlateLayoutTransform(Pos - FVector2f(Draw, Draw))), Dot,
			ESlateDrawEffect::None, FKioskStyle::BrushTint(Dot));

		if (bSelected || bInterchange || Zoom > 1.2f)
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(200.f, 20.f), FSlateLayoutTransform(Pos + FVector2f(Draw + 5.f, -9.f))),
				Station.Name, LabelFont, ESlateDrawEffect::None, bSelected ? C.Primary : C.Ink);
		}
		if (bHere)
		{
			const FVector2f BoxSize(124.f, 32.f);
			const FVector2f BoxPos = Pos + FVector2f(-BoxSize.X * 0.5f, -Radius * 1.3f - BoxSize.Y - 6.f);
			const FSlateBrush* MarkerBrush = Style.Round(C.Help, 6.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos)), MarkerBrush,
				ESlateDrawEffect::None, FKioskStyle::BrushTint(MarkerBrush));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
				AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos + FVector2f(10.f, 5.f))),
				TEXT("You are here"), MarkerFont, ESlateDrawEffect::None, C.White);
		}
	}
	return LayerId + 5;
}

FReply SKioskMap::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	const FVector2f Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2f Size = MyGeometry.GetLocalSize();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();

	int32 Best = INDEX_NONE;
	float BestDistance = HitRadius;
	for (int32 Index = 0; Index < Stations.Num(); ++Index)
	{
		if (Index == FKioskData::CityHallIndex())
		{
			continue; // you are already here
		}
		const float Distance = FVector2f::Distance(Local, ToLocal(Stations[Index].MapPos, Size));
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Index;
		}
	}
	if (Best != INDEX_NONE)
	{
		OnStationTapped.ExecuteIfBound(Best);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FCursorReply SKioskMap::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Hand);
}
