// First-person visitor pawn: capsule, camera, WASD movement, mouse look and a pointer that clicks the kiosk screen.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KioskVisitorPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UWidgetInteractionComponent;

UCLASS()
class AKioskVisitorPawn : public APawn
{
	GENERATED_BODY()

public:
	AKioskVisitorPawn();

	virtual void Tick(float DeltaSeconds) override;

	/** False while the full-screen kiosk view is open: no walking, no looking, no pointer clicks. */
	void SetWalkingEnabled(bool bEnabled);
	bool IsWalkingEnabled() const { return bWalking; }

	/** Test hooks: extra movement input that goes through the same code as the keys, and a scriptable pointer. */
	void SetInjectedMove(float Forward, float Right) { InjectedForward = Forward; InjectedRight = Right; }
	void AimPointerAt(const FVector& WorldPosition);
	void ResetPointerAim();
	void InjectPointer(bool bDown);
	/** True when the pointer is over a widget component; Out2D is the hit in widget pixels. */
	bool GetPointerHit(FVector2D& Out2D) const;
	void SetFieldOfView(float Degrees);

	FVector GetEyeLocation() const;
	FVector GetEyeForward() const;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetInteractionComponent> Pointer;

	bool bWalking = true;
	float InjectedForward = 0.f;
	float InjectedRight = 0.f;
};
