// Actor that builds the 3D metro concourse and the kiosk cabinet at runtime from engine basic shapes only.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KioskEnvironment.generated.h"

class SBox;
class SWidget;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class AKioskEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AKioskEnvironment();

	/** Creates every mesh, light and the screen component. Call once after spawning. */
	void Build();

	/** Shows a Slate widget on the kiosk screen face (the widget may only have one parent at a time). */
	void SetScreenContent(const TSharedRef<SWidget>& Content);
	void ClearScreenContent();

	FVector GetScreenCenter() const;
	/** World position of a pixel on the 1917x1105 kiosk canvas. SignU / SignV flip the axes (used by the self-test to find the mapping). */
	FVector CanvasToWorld(const FVector2D& Pixel, float SignU = 1.f, float SignV = 1.f) const;
	/** True while Widget is the content shown on the 3D screen. */
	bool ScreenContentIs(const TSharedRef<SWidget>& Widget) const;
	/** Where the visitor starts: a few metres in front of the kiosk, facing it. */
	FTransform GetVisitorStart() const;

private:
	UStaticMeshComponent* AddBlock(UStaticMesh* Mesh, const FName& Name, const FVector& Centre, const FVector& SizeCm, const FLinearColor& Colour);
	void AddLight(const FName& Name, const FVector& Location);
	void BuildKiosk();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> Screen;

	TSharedPtr<SBox> ScreenHost;
};
