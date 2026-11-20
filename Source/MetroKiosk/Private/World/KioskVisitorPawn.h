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

	/** False while stepped up to the kiosk: no walking, no looking, no pointer clicks from the pawn (the world drives the cursor). */
	void SetWalkingEnabled(bool bEnabled);
	bool IsWalkingEnabled() const { return bWalking; }

	/** Steps up to the kiosk: the camera blends to View (world transform) with the given field of view and stays there. */
	void LockView(const FTransform& View, float FieldOfViewDegrees);
	/** Steps back: the camera blends to the walking eye position and look direction, then walking can resume. */
	void ReleaseView();
	/** True once the camera has arrived at the locked view (clicks are accepted only then). */
	bool IsViewSettled() const { return ViewPhase == EViewPhase::Locked; }
	/** True when the camera is back at the eye (not locked, not blending). */
	bool IsViewFree() const { return ViewPhase == EViewPhase::Free; }
	/** Points the widget pointer away from the screen (the cursor is not over it). */
	void AimPointerAway();
	/** Mouse wheel over the screen. */
	void ScrollPointer(float Delta);

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

	/** Degrees turned per mouse count while walking (was a fixed 0.12). Does not affect the cursor at the locked kiosk view. */
	UPROPERTY(EditAnywhere, Category = "Kiosk|Look", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float LookSensitivity = 0.24f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetInteractionComponent> Pointer;

	enum class EViewPhase : uint8 { Free, BlendIn, Locked, BlendOut };
	void TickView(float DeltaSeconds);
	FVector WalkEyeLocation() const;

	EViewPhase ViewPhase = EViewPhase::Free;
	FTransform LockedView;
	float LockedFov = 90.f;
	float WalkFov = 80.f;
	float BlendAlpha = 0.f;

	bool bWalking = true;
	float InjectedForward = 0.f;
	float InjectedRight = 0.f;
};
