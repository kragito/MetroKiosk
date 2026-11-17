// Runs the optional -Kiosk3D mode: spawns the concourse, switches between walking and the full-screen kiosk view, shows the hints.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "InputCoreTypes.h"
#include "KioskTypes.h"

class AKioskEnvironment;
class AKioskVisitorPawn;
class FKioskSession;
class IInputProcessor;
class SKioskRoot;
class SWidget;
class UGameViewportClient;
class UWorld;

class FKioskWorld
{
public:
	/** True when the app was launched with -Kiosk3D. */
	static bool IsRequested();

	FKioskWorld(UWorld* InWorld, UGameViewportClient* InViewport, const TSharedRef<SKioskRoot>& InRoot, const TSharedRef<FKioskSession>& InSession);
	~FKioskWorld();

	/** Takes over from the 2D viewport hosting: the kiosk root is shown on the 3D screen instead of the viewport. */
	void Start();
	void Shutdown();

	void EnterKioskView();
	void ExitKioskView();
	bool IsInKioskView() const { return bKioskView; }
	bool IsNearKiosk() const;

	/** Called by the input processor. Returns true when the key was used. */
	bool HandleKey(const FKey& Key);

private:
	bool Tick(float DeltaSeconds);
	/** -Kiosk3DSelfTest: walks, clicks the screen, presses E and Esc and bumps into the kiosk, logging one line per step. */
	bool TickSelfTest(float DeltaSeconds);
	void SelfTestOk(const FString& Step, const FString& Detail);
	void SelfTestFail(const FString& Step, const FString& Detail);
	bool FindButtonPixel(const FString& Name, FVector2D& OutPixel) const;
	void SetWalkingInput();
	TSharedRef<SWidget> BuildHud();

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	TSharedPtr<SKioskRoot> Root;
	TSharedPtr<FKioskSession> Session;
	TWeakObjectPtr<AKioskEnvironment> Environment;
	TWeakObjectPtr<AKioskVisitorPawn> Pawn;
	TSharedPtr<IInputProcessor> InputProcessor;
	TSharedPtr<SWidget> Hud;
	FTSTicker::FDelegateHandle TickHandle;

	bool bStarted = false;
	bool bKioskView = false;
	FString ShotPath;
	float ShotTimer = -1.f;
	float QuitTimer = -1.f;

	// Self-test state.
	bool bSelfTest = false;
	int32 TestStep = 0;
	int32 TestFrames = 0;
	int32 TestTrial = 0;
	float TestTime = 0.f;
	float TestTotal = 0.f;
	FVector2D TestTarget = FVector2D::ZeroVector;
	EKioskScreen ScreenBeforeClick = EKioskScreen::Home;
};
