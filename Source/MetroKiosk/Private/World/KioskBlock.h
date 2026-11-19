// A basic-shape mesh coloured by a KC() token name, plus the actor that places one block in the concourse level.
#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "KioskBlock.generated.h"

class UMaterialInstanceDynamic;

UENUM()
enum class EKioskShape : uint8
{
	Cube,
	Cylinder,
};

/** Unit-size (100 cm) engine basic shape. Size it with the component or actor scale; colour it with ColourToken (a FKioskColors name such as Muted, Ink, Brand). */
UCLASS(ClassGroup = (Kiosk), meta = (BlueprintSpawnableComponent))
class UKioskBlockComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UKioskBlockComponent();

	UPROPERTY(EditAnywhere, Category = "Kiosk")
	EKioskShape Shape = EKioskShape::Cube;

	/** Name of a colour token in FKioskColors (KioskStyle.h). */
	UPROPERTY(EditAnywhere, Category = "Kiosk")
	FName ColourToken = TEXT("Muted");

	/** Colour the token stands for (magenta for an unknown token). */
	FLinearColor GetBaseColour() const;
	/** Temporary colour (hover highlight, guide light); ClearColourOverride goes back to the token. */
	void SetColourOverride(const FLinearColor& Colour);
	void ClearColourOverride();

	virtual void OnRegister() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyLook();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
};

/** One placed block of the concourse (floor, wall, pillar, bench, rail ...): move, scale and recolour it in the editor. */
UCLASS()
class AKioskBlock : public AActor
{
	GENERATED_BODY()

public:
	AKioskBlock();

	UPROPERTY(VisibleAnywhere, Category = "Kiosk")
	TObjectPtr<UKioskBlockComponent> Block;
};
