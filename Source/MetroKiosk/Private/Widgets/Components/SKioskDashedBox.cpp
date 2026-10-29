#include "SKioskDashedBox.h"

#include "KioskStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBorder.h"

void SKioskDashedBox::Construct(const FArguments& InArgs)
{
	Fill = InArgs._Fill;
	DashColor = InArgs._DashColor;
	Radius = InArgs._Radius;
	ChildSlot
	.Padding(InArgs._Padding)
	[
		InArgs._Content.Widget
	];
}

int32 SKioskDashedBox::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = AllottedGeometry.GetLocalSize();
	FKioskStyle& Style = FKioskStyle::Get();

	if (Fill.A > 0.f)
	{
		const FSlateBrush* FillBrush = Style.Round(Fill, Radius);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), FillBrush, ESlateDrawEffect::None, FKioskStyle::BrushTint(FillBrush));
	}

	const float Inset = 1.5f;
	const float DashLength = 11.f;
	const float GapLength = 8.f;
	const float Thickness = 2.f;

	auto DrawEdge = [&](FVector2f From, FVector2f To)
	{
		const float Length = FVector2f::Distance(From, To);
		const FVector2f Dir = (To - From) / FMath::Max(Length, 1.f);
		for (float Pos = 0.f; Pos < Length; Pos += DashLength + GapLength)
		{
			const float End = FMath::Min(Pos + DashLength, Length);
			TArray<FVector2f> Points = { From + Dir * Pos, From + Dir * End };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, DashColor, true, Thickness);
		}
	};

	const FVector2f A(Inset, Inset), B(Size.X - Inset, Inset), C(Size.X - Inset, Size.Y - Inset), D(Inset, Size.Y - Inset);
	DrawEdge(A, B);
	DrawEdge(B, C);
	DrawEdge(C, D);
	DrawEdge(D, A);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 2, InWidgetStyle, bParentEnabled);
}
