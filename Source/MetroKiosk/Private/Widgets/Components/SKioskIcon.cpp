#include "SKioskIcon.h"

#include "KioskStyle.h"
#include "Rendering/DrawElements.h"

namespace
{
	/** Draws in a unit square scaled to the widget. All coordinates are 0..1. */
	struct FIconPainter
	{
		FSlateWindowElementList& List;
		int32 Layer;
		const FGeometry& Geo;
		FLinearColor Color;
		float Size;
		FVector2f Origin;
		float Thick;

		FVector2f P(float X, float Y) const { return Origin + FVector2f(X * Size, Y * Size); }

		void Poly(const TArray<FVector2f>& UnitPoints, bool bClose = false) const
		{
			TArray<FVector2f> Points;
			for (const FVector2f& Point : UnitPoints)
			{
				Points.Add(P(Point.X, Point.Y));
			}
			if (bClose && UnitPoints.Num() > 1)
			{
				const FVector2f First = Points[0];
				Points.Add(First);
			}
			FSlateDrawElement::MakeLines(List, Layer, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thick);
		}

		void Line(float X0, float Y0, float X1, float Y1) const
		{
			Poly({ FVector2f(X0, Y0), FVector2f(X1, Y1) });
		}

		void Arc(float Cx, float Cy, float RadiusX, float RadiusY, float DegFrom, float DegTo) const
		{
			TArray<FVector2f> Points;
			const int32 Steps = 24;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const float Angle = FMath::DegreesToRadians(FMath::Lerp(DegFrom, DegTo, static_cast<float>(Step) / Steps));
				Points.Add(FVector2f(Cx + RadiusX * FMath::Cos(Angle), Cy + RadiusY * FMath::Sin(Angle)));
			}
			Poly(Points);
		}

		void Circle(float Cx, float Cy, float Radius) const { Arc(Cx, Cy, Radius, Radius, 0.f, 360.f); }

		void RRect(float X, float Y, float W, float H, float Radius, bool bFill = false) const
		{
			FKioskStyle& Style = FKioskStyle::Get();
			const FSlateBrush* Brush = bFill
				? Style.Round(Color, Radius * Size)
				: Style.Round(Style.C.Transparent, Radius * Size, Color, Thick);
			FSlateDrawElement::MakeBox(List, Layer,
				Geo.ToPaintGeometry(FVector2f(W * Size, H * Size), FSlateLayoutTransform(P(X, Y))),
				Brush, ESlateDrawEffect::None, FKioskStyle::BrushTint(Brush));
		}

		void Dot(float Cx, float Cy, float Radius) const { RRect(Cx - Radius, Cy - Radius, Radius * 2.f, Radius * 2.f, Radius, true); }
	};

	void PaintIcon(EKioskIcon Icon, const FIconPainter& Ip)
	{
		using V = FVector2f;
		switch (Icon)
		{
		case EKioskIcon::ArrowRight:
			Ip.Line(0.08f, 0.5f, 0.92f, 0.5f);
			Ip.Poly({ V(0.58f, 0.18f), V(0.92f, 0.5f), V(0.58f, 0.82f) });
			break;
		case EKioskIcon::ArrowLeft:
			Ip.Line(0.08f, 0.5f, 0.92f, 0.5f);
			Ip.Poly({ V(0.42f, 0.18f), V(0.08f, 0.5f), V(0.42f, 0.82f) });
			break;
		case EKioskIcon::ArrowUp:
			Ip.Line(0.5f, 0.08f, 0.5f, 0.92f);
			Ip.Poly({ V(0.18f, 0.42f), V(0.5f, 0.08f), V(0.82f, 0.42f) });
			break;
		case EKioskIcon::ArrowDown:
			Ip.Line(0.5f, 0.08f, 0.5f, 0.92f);
			Ip.Poly({ V(0.18f, 0.58f), V(0.5f, 0.92f), V(0.82f, 0.58f) });
			break;
		case EKioskIcon::ChevronLeft:
			Ip.Poly({ V(0.65f, 0.15f), V(0.3f, 0.5f), V(0.65f, 0.85f) });
			break;
		case EKioskIcon::ChevronRight:
			Ip.Poly({ V(0.35f, 0.15f), V(0.7f, 0.5f), V(0.35f, 0.85f) });
			break;
		case EKioskIcon::ChevronDown:
			Ip.Poly({ V(0.15f, 0.35f), V(0.5f, 0.7f), V(0.85f, 0.35f) });
			break;
		case EKioskIcon::DoubleChevronDown:
			Ip.Poly({ V(0.2f, 0.18f), V(0.5f, 0.46f), V(0.8f, 0.18f) });
			Ip.Poly({ V(0.2f, 0.54f), V(0.5f, 0.82f), V(0.8f, 0.54f) });
			break;
		case EKioskIcon::Check:
			Ip.Poly({ V(0.18f, 0.54f), V(0.42f, 0.78f), V(0.84f, 0.26f) });
			break;
		case EKioskIcon::Close:
			Ip.Line(0.2f, 0.2f, 0.8f, 0.8f);
			Ip.Line(0.8f, 0.2f, 0.2f, 0.8f);
			break;
		case EKioskIcon::Plus:
			Ip.Line(0.15f, 0.5f, 0.85f, 0.5f);
			Ip.Line(0.5f, 0.15f, 0.5f, 0.85f);
			break;
		case EKioskIcon::Minus:
			Ip.Line(0.15f, 0.5f, 0.85f, 0.5f);
			break;
		case EKioskIcon::Ticket:
		{
			Ip.Line(0.1f, 0.2f, 0.9f, 0.2f);
			Ip.Line(0.1f, 0.8f, 0.9f, 0.8f);
			Ip.Line(0.9f, 0.2f, 0.9f, 0.38f);
			Ip.Arc(0.9f, 0.5f, 0.12f, 0.12f, 270.f, 90.f);
			Ip.Line(0.9f, 0.62f, 0.9f, 0.8f);
			Ip.Line(0.1f, 0.2f, 0.1f, 0.38f);
			Ip.Arc(0.1f, 0.5f, 0.12f, 0.12f, -90.f, 90.f);
			Ip.Line(0.1f, 0.62f, 0.1f, 0.8f);
			for (int32 Dash = 0; Dash < 5; ++Dash)
			{
				const float Y = 0.24f + Dash * 0.12f;
				Ip.Line(0.68f, Y, 0.68f, Y + 0.06f);
			}
			break;
		}
		case EKioskIcon::TopUpCard:
			Ip.RRect(0.06f, 0.14f, 0.88f, 0.72f, 0.1f);
			Ip.Line(0.06f, 0.4f, 0.94f, 0.4f);
			Ip.Line(0.76f, 0.6f, 0.76f, 0.78f);
			Ip.Line(0.67f, 0.69f, 0.85f, 0.69f);
			break;
		case EKioskIcon::IdCard:
			Ip.RRect(0.06f, 0.14f, 0.88f, 0.72f, 0.1f);
			Ip.Circle(0.3f, 0.43f, 0.1f);
			Ip.Arc(0.3f, 0.74f, 0.16f, 0.17f, 180.f, 360.f);
			Ip.Line(0.58f, 0.42f, 0.84f, 0.42f);
			Ip.Line(0.58f, 0.6f, 0.84f, 0.6f);
			break;
		case EKioskIcon::Headset:
			Ip.Arc(0.5f, 0.5f, 0.36f, 0.34f, 180.f, 360.f);
			Ip.Line(0.14f, 0.5f, 0.14f, 0.58f);
			Ip.Line(0.86f, 0.5f, 0.86f, 0.58f);
			Ip.RRect(0.08f, 0.52f, 0.17f, 0.3f, 0.06f);
			Ip.RRect(0.75f, 0.52f, 0.17f, 0.3f, 0.06f);
			break;
		case EKioskIcon::Contactless:
			Ip.Dot(0.2f, 0.5f, 0.045f);
			Ip.Arc(0.2f, 0.5f, 0.22f, 0.22f, -48.f, 48.f);
			Ip.Arc(0.2f, 0.5f, 0.4f, 0.4f, -48.f, 48.f);
			Ip.Arc(0.2f, 0.5f, 0.58f, 0.58f, -48.f, 48.f);
			break;
		case EKioskIcon::Globe:
			Ip.Circle(0.5f, 0.5f, 0.42f);
			Ip.Arc(0.5f, 0.5f, 0.2f, 0.42f, 0.f, 360.f);
			Ip.Line(0.08f, 0.5f, 0.92f, 0.5f);
			break;
		case EKioskIcon::Map:
			Ip.Poly({ V(0.1f, 0.25f), V(0.36f, 0.14f), V(0.64f, 0.26f), V(0.9f, 0.15f), V(0.9f, 0.75f), V(0.64f, 0.86f), V(0.36f, 0.74f), V(0.1f, 0.85f) }, true);
			Ip.Line(0.36f, 0.14f, 0.36f, 0.74f);
			Ip.Line(0.64f, 0.26f, 0.64f, 0.86f);
			break;
		case EKioskIcon::List:
			for (int32 Row = 0; Row < 3; ++Row)
			{
				const float Y = 0.25f + Row * 0.25f;
				Ip.Dot(0.14f, Y, 0.035f);
				Ip.Line(0.3f, Y, 0.88f, Y);
			}
			break;
		case EKioskIcon::Search:
			Ip.Circle(0.42f, 0.42f, 0.27f);
			Ip.Line(0.62f, 0.62f, 0.88f, 0.88f);
			break;
		case EKioskIcon::Clock:
			Ip.Circle(0.5f, 0.5f, 0.42f);
			Ip.Poly({ V(0.5f, 0.24f), V(0.5f, 0.5f), V(0.68f, 0.62f) });
			break;
		case EKioskIcon::Swap:
			Ip.Line(0.1f, 0.32f, 0.9f, 0.32f);
			Ip.Poly({ V(0.68f, 0.12f), V(0.9f, 0.32f), V(0.68f, 0.52f) });
			Ip.Line(0.1f, 0.7f, 0.9f, 0.7f);
			Ip.Poly({ V(0.32f, 0.5f), V(0.1f, 0.7f), V(0.32f, 0.9f) });
			break;
		case EKioskIcon::Person:
			Ip.Circle(0.5f, 0.32f, 0.18f);
			Ip.Arc(0.5f, 0.92f, 0.34f, 0.4f, 180.f, 360.f);
			break;
		case EKioskIcon::Mic:
			Ip.RRect(0.38f, 0.1f, 0.24f, 0.45f, 0.12f);
			Ip.Arc(0.5f, 0.45f, 0.26f, 0.26f, 0.f, 180.f);
			Ip.Line(0.5f, 0.71f, 0.5f, 0.9f);
			break;
		case EKioskIcon::Pause:
			Ip.RRect(0.22f, 0.18f, 0.2f, 0.64f, 0.05f);
			Ip.RRect(0.58f, 0.18f, 0.2f, 0.64f, 0.05f);
			break;
		case EKioskIcon::Printer:
			Ip.Poly({ V(0.28f, 0.34f), V(0.28f, 0.1f), V(0.72f, 0.1f), V(0.72f, 0.34f) });
			Ip.RRect(0.08f, 0.34f, 0.84f, 0.38f, 0.06f);
			Ip.RRect(0.26f, 0.58f, 0.48f, 0.32f, 0.03f);
			break;
		case EKioskIcon::Mail:
			Ip.RRect(0.06f, 0.2f, 0.88f, 0.6f, 0.06f);
			Ip.Poly({ V(0.08f, 0.26f), V(0.5f, 0.58f), V(0.92f, 0.26f) });
			break;
		case EKioskIcon::Backspace:
			Ip.Poly({ V(0.3f, 0.2f), V(0.9f, 0.2f), V(0.9f, 0.8f), V(0.3f, 0.8f), V(0.08f, 0.5f) }, true);
			Ip.Line(0.46f, 0.36f, 0.72f, 0.64f);
			Ip.Line(0.72f, 0.36f, 0.46f, 0.64f);
			break;
		case EKioskIcon::Bus:
			Ip.RRect(0.2f, 0.1f, 0.6f, 0.68f, 0.1f);
			Ip.Line(0.2f, 0.46f, 0.8f, 0.46f);
			Ip.Line(0.3f, 0.26f, 0.7f, 0.26f);
			Ip.Dot(0.34f, 0.62f, 0.04f);
			Ip.Dot(0.66f, 0.62f, 0.04f);
			Ip.Line(0.3f, 0.78f, 0.3f, 0.9f);
			Ip.Line(0.7f, 0.78f, 0.7f, 0.9f);
			break;
		case EKioskIcon::Train:
			Ip.RRect(0.2f, 0.08f, 0.6f, 0.7f, 0.12f);
			Ip.Line(0.2f, 0.44f, 0.8f, 0.44f);
			Ip.Line(0.3f, 0.24f, 0.7f, 0.24f);
			Ip.Dot(0.34f, 0.6f, 0.04f);
			Ip.Dot(0.66f, 0.6f, 0.04f);
			Ip.Line(0.28f, 0.78f, 0.2f, 0.92f);
			Ip.Line(0.72f, 0.78f, 0.8f, 0.92f);
			break;
		case EKioskIcon::Restart:
			Ip.Arc(0.5f, 0.52f, 0.34f, 0.34f, -60.f, 230.f);
			Ip.Poly({ V(0.18f, 0.16f), V(0.2f, 0.4f), V(0.42f, 0.34f) });
			break;
		case EKioskIcon::Tray:
			Ip.Poly({ V(0.1f, 0.58f), V(0.1f, 0.88f), V(0.9f, 0.88f), V(0.9f, 0.58f) });
			Ip.Line(0.5f, 0.1f, 0.5f, 0.62f);
			Ip.Poly({ V(0.3f, 0.42f), V(0.5f, 0.62f), V(0.7f, 0.42f) });
			break;
		case EKioskIcon::Captions:
			Ip.RRect(0.08f, 0.2f, 0.84f, 0.6f, 0.08f);
			Ip.Line(0.24f, 0.46f, 0.46f, 0.46f);
			Ip.Line(0.54f, 0.46f, 0.76f, 0.46f);
			Ip.Line(0.24f, 0.62f, 0.62f, 0.62f);
			break;
		case EKioskIcon::Cash:
			Ip.RRect(0.06f, 0.22f, 0.88f, 0.56f, 0.08f);
			Ip.Circle(0.5f, 0.5f, 0.14f);
			Ip.Dot(0.2f, 0.36f, 0.025f);
			Ip.Dot(0.8f, 0.64f, 0.025f);
			break;
		case EKioskIcon::ChipCard:
			Ip.RRect(0.06f, 0.2f, 0.88f, 0.6f, 0.08f);
			Ip.RRect(0.2f, 0.34f, 0.2f, 0.16f, 0.03f);
			Ip.Line(0.6f, 0.64f, 0.82f, 0.64f);
			break;
		case EKioskIcon::Coin:
			Ip.Circle(0.5f, 0.5f, 0.42f);
			Ip.Circle(0.5f, 0.5f, 0.24f);
			break;
		case EKioskIcon::Reader:
			Ip.RRect(0.06f, 0.3f, 0.88f, 0.4f, 0.1f);
			Ip.RRect(0.24f, 0.44f, 0.52f, 0.12f, 0.05f, true);
			break;
		default:
			break;
		}
	}
}

void SKioskIcon::Construct(const FArguments& InArgs)
{
	Icon = InArgs._Icon;
	Size = InArgs._Size;
	Color = InArgs._Color;
	Thickness = InArgs._Thickness;
	SetCanTick(false);
}

FVector2D SKioskIcon::ComputeDesiredSize(float) const
{
	return FVector2D(Size, Size);
}

int32 SKioskIcon::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Local = AllottedGeometry.GetLocalSize();
	const float Extent = FMath::Min(Size, FMath::Min(Local.X, Local.Y));
	const FVector2f Origin((Local.X - Extent) * 0.5f, (Local.Y - Extent) * 0.5f);
	const float Thick = Thickness > 0.f ? Thickness : FMath::Max(2.f, Extent * 0.065f);

	const FIconPainter Painter{ OutDrawElements, LayerId, AllottedGeometry, Color, Extent, Origin, Thick };
	PaintIcon(Icon, Painter);
	return LayerId;
}
