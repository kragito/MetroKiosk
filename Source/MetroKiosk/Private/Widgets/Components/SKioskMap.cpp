#include "SKioskMap.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
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
	Session = InArgs._Session;
	SetClipping(EWidgetClipping::ClipToBounds);
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
	const FVector2f Pan = (Zoom > 1.01f && Session.IsValid()) ? Session->MapPan : FVector2f::ZeroVector;
	return LocalSize * 0.5f + Pan + (MapPos - Focus) * (BaseScale * Zoom);
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
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
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
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(Draw * 2.f, Draw * 2.f), FSlateLayoutTransform(Pos - FVector2f(Draw, Draw))), Dot,
			ESlateDrawEffect::None, FKioskStyle::BrushTint(Dot));

		// City Hall shows the "You are here" marker instead of a plain label.
		if (bHere)
		{
			const FString Text = KT("You are here").ToString();
			const float TextWidth = FontMeasure->Measure(Text, MarkerFont).X;
			const FVector2f BoxSize(TextWidth + 22.f, 30.f);
			const FVector2f BoxPos = Pos + FVector2f(-Draw - 8.f - BoxSize.X, -BoxSize.Y * 0.5f);
			const FSlateBrush* MarkerBrush = Style.Round(C.Help, 6.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
				AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos)), MarkerBrush,
				ESlateDrawEffect::None, FKioskStyle::BrushTint(MarkerBrush));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
				AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos + FVector2f(11.f, 4.f))),
				Text, MarkerFont, ESlateDrawEffect::None, C.White);
		}
		else if (bSelected || bInterchange || Zoom > 1.2f)
		{
			// Crowded stations put their label on the left; every label sits on a soft white pill so lines never cut through it.
			static const TSet<FString> LeftLabels = { TEXT("Chinatown"), TEXT("Outram Park"), TEXT("Dhoby Ghaut"),
				TEXT("Botanic Gardens"), TEXT("Caldecott"), TEXT("Orchard"), TEXT("Buona Vista"), TEXT("HarbourFront") };
			const float Width = FontMeasure->Measure(Station.Name, LabelFont).X;
			const float LabelX = LeftLabels.Contains(Station.Name) ? (-Draw - 6.f - Width) : (Draw + 6.f);
			const FVector2f LabelPos = Pos + FVector2f(LabelX, -9.f);
			const FSlateBrush* PillBrush = Style.Round(FLinearColor(C.White.R, C.White.G, C.White.B, 0.82f), 5.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(Width + 8.f, 20.f), FSlateLayoutTransform(LabelPos + FVector2f(-4.f, -1.f))), PillBrush,
				ESlateDrawEffect::None, FKioskStyle::BrushTint(PillBrush));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(200.f, 20.f), FSlateLayoutTransform(LabelPos)),
				Station.Name, LabelFont, ESlateDrawEffect::None, bSelected ? C.Primary : C.Ink);
		}
	}
	return LayerId + 6;
}

FReply SKioskMap::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = true;
	bDragging = false;
	DragStart = DragLast = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return FReply::Handled().CaptureMouse(AsShared());
}

FReply SKioskMap::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bPressed)
	{
		return FReply::Unhandled();
	}
	const FVector2f Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (!bDragging && FVector2f::Distance(Local, DragStart) > 10.f)
	{
		bDragging = true;
	}
	if (bDragging && Zoom > 1.01f && Session.IsValid())
	{
		const FVector2f Limit = MyGeometry.GetLocalSize() * 0.5f * (Zoom - 0.5f);
		FVector2f Pan = Session->MapPan + (Local - DragLast);
		Pan.X = FMath::Clamp(Pan.X, -Limit.X, Limit.X);
		Pan.Y = FMath::Clamp(Pan.Y, -Limit.Y, Limit.Y);
		Session->MapPan = Pan;
	}
	DragLast = Local;
	return FReply::Handled();
}

FReply SKioskMap::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bPressed || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = false;
	const bool bWasDrag = bDragging;
	bDragging = false;
	FReply Reply = FReply::Handled().ReleaseMouseCapture();
	if (bWasDrag)
	{
		return Reply;
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
	}
	return Reply;
}

void SKioskMap::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bPressed = false;
	bDragging = false;
}

FCursorReply SKioskMap::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Hand);
}
