// Builds the concourse (floor, walls, ceiling lights, platform edge, benches, pillars) and the kiosk cabinet from /Engine/BasicShapes.
#include "KioskEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "KioskStyle.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SKioskRoot.h"
#include "Widgets/SNullWidget.h"

namespace
{
	// Layout in centimetres. The kiosk screen faces +X; the concourse runs along +X from the kiosk wall to the platform edge.
	constexpr float XMin = -110.f;
	constexpr float XMax = 1520.f;
	constexpr float PlatformEdgeX = 1100.f;
	constexpr float HalfWidth = 820.f;
	constexpr float WallHeight = 420.f;
	constexpr float ScreenWidthCm = 100.f;
	constexpr float ScreenCentreZ = 129.f;
}

AKioskEnvironment::AKioskEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	BaseMaterial = MaterialFinder.Object;
}

UStaticMeshComponent* AKioskEnvironment::AddBlock(UStaticMesh* Mesh, const FName& Name, const FVector& Centre, const FVector& SizeCm, const FLinearColor& Colour)
{
	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this, Name);
	Block->SetMobility(EComponentMobility::Movable);
	Block->SetStaticMesh(Mesh);
	Block->SetupAttachment(RootComponent);
	Block->SetRelativeLocation(Centre);
	Block->SetRelativeScale3D(SizeCm / 100.f); // basic shapes are 100 cm
	Block->SetCastShadow(false);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), Colour);
		Block->SetMaterial(0, Material);
	}
	Block->RegisterComponent();
	return Block;
}

void AKioskEnvironment::AddLight(const FName& Name, const FVector& Location)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, Name);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(Location);
	Light->SetIntensity(400.f);
	Light->SetAttenuationRadius(900.f);
	Light->SetLightColor(FLinearColor(1.f, 0.97f, 0.92f));
	Light->SetCastShadows(false);
	Light->RegisterComponent();
}

void AKioskEnvironment::Build()
{
	if (!CubeMesh || !CylinderMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Kiosk3D: engine basic shapes are missing; the scene is empty"));
		return;
	}
	const FKioskColors& C = KC();
	const float Length = XMax - XMin;
	const float MidX = (XMax + XMin) * 0.5f;
	const float FloorLength = PlatformEdgeX - XMin;
	const float FloorMidX = (PlatformEdgeX + XMin) * 0.5f;
	const float FullWidth = HalfWidth * 2.f;

	// Shell.
	AddBlock(CubeMesh, TEXT("Floor"), FVector(FloorMidX, 0.f, -10.f), FVector(FloorLength, FullWidth, 20.f), C.Muted);
	AddBlock(CubeMesh, TEXT("Ceiling"), FVector(MidX, 0.f, WallHeight + 10.f), FVector(Length, FullWidth, 20.f), C.Surface);
	AddBlock(CubeMesh, TEXT("KioskWall"), FVector(XMin + 10.f, 0.f, WallHeight * 0.5f), FVector(20.f, FullWidth, WallHeight), C.Bg);
	AddBlock(CubeMesh, TEXT("FarWall"), FVector(XMax - 10.f, 0.f, WallHeight * 0.5f), FVector(20.f, FullWidth, WallHeight), C.BorderStrong);
	AddBlock(CubeMesh, TEXT("WallLeft"), FVector(MidX, -HalfWidth - 10.f, WallHeight * 0.5f), FVector(Length, 20.f, WallHeight), C.Bg);
	AddBlock(CubeMesh, TEXT("WallRight"), FVector(MidX, HalfWidth + 10.f, WallHeight * 0.5f), FVector(Length, 20.f, WallHeight), C.Bg);

	// Platform edge: tactile yellow strip, a railing, and the track bed below.
	const float TrackLength = XMax - PlatformEdgeX;
	const float TrackMidX = (PlatformEdgeX + XMax) * 0.5f;
	AddBlock(CubeMesh, TEXT("EdgeStrip"), FVector(PlatformEdgeX - 30.f, 0.f, 1.f), FVector(40.f, FullWidth, 2.f), C.Chip);
	AddBlock(CubeMesh, TEXT("TrackBed"), FVector(TrackMidX, 0.f, -110.f), FVector(TrackLength, FullWidth, 20.f), C.Ink);
	AddBlock(CubeMesh, TEXT("RailA"), FVector(TrackMidX, -70.f, -96.f), FVector(TrackLength, 6.f, 4.f), C.Muted);
	AddBlock(CubeMesh, TEXT("RailB"), FVector(TrackMidX, 70.f, -96.f), FVector(TrackLength, 6.f, 4.f), C.Muted);
	AddBlock(CubeMesh, TEXT("RailingBar"), FVector(PlatformEdgeX, 0.f, 105.f), FVector(6.f, FullWidth, 6.f), C.Help);
	for (int32 Index = 0; Index <= 8; ++Index)
	{
		const float Y = -HalfWidth + FullWidth * Index / 8.f;
		AddBlock(CylinderMesh, *FString::Printf(TEXT("RailingPost%d"), Index), FVector(PlatformEdgeX, Y, 52.f), FVector(8.f, 8.f, 104.f), C.Help);
	}

	// Pillars and benches.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Y = Side == 0 ? -420.f : 420.f;
		AddBlock(CylinderMesh, *FString::Printf(TEXT("Pillar%d"), Side), FVector(550.f, Y, WallHeight * 0.5f), FVector(60.f, 60.f, WallHeight), C.Surface);
		AddBlock(CubeMesh, *FString::Printf(TEXT("Bench%d"), Side), FVector(780.f, Y * 0.6f, 22.f), FVector(50.f, 180.f, 45.f), C.Brand);
	}

	// Ceiling light panels with a point light under each.
	int32 LightIndex = 0;
	for (float X : { 200.f, 600.f, 1000.f })
	{
		for (float Y : { -300.f, 300.f })
		{
			AddBlock(CubeMesh, *FString::Printf(TEXT("LightPanel%d"), LightIndex), FVector(X, Y, WallHeight - 2.f), FVector(120.f, 60.f, 4.f), C.White);
			AddLight(*FString::Printf(TEXT("Light%d"), LightIndex), FVector(X, Y, WallHeight - 40.f));
			++LightIndex;
		}
	}

	// Three weak shadowless fills (two from opposite sides, one up at the ceiling) so no wall, pillar or ceiling face is pure black.
	const FRotator FillAngles[3] = { FRotator(-50.f, 210.f, 0.f), FRotator(-35.f, 40.f, 0.f), FRotator(60.f, 120.f, 0.f) }; // the last one shines up onto the ceiling
	const float FillLux[3] = { 2.5f, 1.5f, 1.5f };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(this, *FString::Printf(TEXT("Fill%d"), Index));
		Fill->SetMobility(EComponentMobility::Movable);
		Fill->SetupAttachment(RootComponent);
		Fill->SetRelativeRotation(FillAngles[Index]);
		Fill->SetIntensity(FillLux[Index]);
		Fill->SetCastShadows(false);
		Fill->ForwardShadingPriority = 3 - Index; // one clear winner, no engine warning
		Fill->RegisterComponent();
	}

	BuildKiosk();
}

void AKioskEnvironment::BuildKiosk()
{
	const FKioskColors& C = KC();

	// Cabinet: front face at X = 0, standing against the wall.
	AddBlock(CubeMesh, TEXT("KioskCabinet"), FVector(-30.f, 0.f, 95.f), FVector(60.f, 120.f, 190.f), C.Ink);
	AddBlock(CubeMesh, TEXT("KioskSign"), FVector(-29.f, 0.f, 203.f), FVector(62.f, 120.f, 26.f), C.Brand);
	AddBlock(CubeMesh, TEXT("KioskCardReader"), FVector(2.f, -32.f, 88.f), FVector(4.f, 28.f, 12.f), C.Chip);
	AddBlock(CubeMesh, TEXT("KioskTray"), FVector(12.f, 28.f, 76.f), FVector(24.f, 50.f, 6.f), C.DeviceDark);

	// Screen face: the live kiosk UI, hosted in a world-space widget component sized to 1 m wide.
	ScreenHost = SNew(SBox);
	Screen = NewObject<UWidgetComponent>(this, TEXT("KioskScreen"));
	Screen->SetMobility(EComponentMobility::Movable);
	Screen->SetupAttachment(RootComponent);
	Screen->SetRelativeLocation(FVector(0.6f, 0.f, ScreenCentreZ));
	const float PixelToCm = ScreenWidthCm / SKioskRoot::CanvasWidth;
	Screen->SetRelativeScale3D(FVector(1.f, PixelToCm, PixelToCm));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetDrawSize(FVector2D(SKioskRoot::CanvasWidth, SKioskRoot::CanvasHeight));
	Screen->SetPivot(FVector2D(0.5f, 0.5f));
	Screen->SetBlendMode(EWidgetBlendMode::Opaque);
	Screen->SetTwoSided(false);
	Screen->SetCastShadow(false);
	Screen->RegisterComponent();
	Screen->SetSlateWidget(ScreenHost);
}

void AKioskEnvironment::SetScreenContent(const TSharedRef<SWidget>& Content)
{
	if (ScreenHost.IsValid())
	{
		ScreenHost->SetContent(Content);
	}
}

void AKioskEnvironment::ClearScreenContent()
{
	if (ScreenHost.IsValid())
	{
		ScreenHost->SetContent(SNullWidget::NullWidget);
	}
}

FVector AKioskEnvironment::GetScreenCenter() const
{
	return GetActorTransform().TransformPosition(FVector(0.6f, 0.f, ScreenCentreZ));
}

FVector AKioskEnvironment::CanvasToWorld(const FVector2D& Pixel, float SignU, float SignV) const
{
	if (!Screen)
	{
		return GetScreenCenter();
	}
	const FVector Local(0.f, SignU * (Pixel.X - SKioskRoot::CanvasWidth * 0.5f), SignV * (SKioskRoot::CanvasHeight * 0.5f - Pixel.Y));
	return Screen->GetComponentTransform().TransformPosition(Local);
}

bool AKioskEnvironment::ScreenContentIs(const TSharedRef<SWidget>& Widget) const
{
	return ScreenHost.IsValid() && &ScreenHost->GetChildren()->GetChildAt(0).Get() == &Widget.Get();
}

FTransform AKioskEnvironment::GetVisitorStart() const
{
	// Capsule centre is 93 cm up (3 cm of clearance so floor contact does not block sweeps); facing -X, towards the screen.
	return FTransform(FRotator(0.f, 180.f, 0.f), GetActorTransform().TransformPosition(FVector(300.f, 0.f, 93.f)));
}
