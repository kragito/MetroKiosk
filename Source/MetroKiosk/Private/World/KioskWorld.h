// Runs the default 3D mode: spawns the concourse, lets the visitor walk up to the kiosk (E), lock the camera on it and use its screen, parts and wallet like the real thing.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "InputCoreTypes.h"
#include "KioskTypes.h"
#include "KioskWallet.h"

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
	/** True unless the app was launched with -Kiosk2D (the flat kiosk that journeys and captures use). -Kiosk3D is still accepted. */
	static bool IsRequested();

	FKioskWorld(UWorld* InWorld, UGameViewportClient* InViewport, const TSharedRef<SKioskRoot>& InRoot, const TSharedRef<FKioskSession>& InSession);
	~FKioskWorld();

	/** Takes over from the 2D viewport hosting: the kiosk root is shown on the 3D screen instead of the viewport. */
	void Start();
	void Shutdown();

	/** Step up to the kiosk: the camera locks on its front, the cursor is shown and moves over the screen and the parts. */
	void EnterKioskView();
	/** Step back: the camera returns to the visitor's eye and walking resumes. */
	void ExitKioskView();
	bool IsInKioskView() const { return bKioskView; }
	bool IsNearKiosk() const;

	/** Called by the input processor. Returns true when the key was used. */
	bool HandleKey(const FKey& Key);

	/** Chooses the wallet item the next kiosk-part click uses (INDEX_NONE: empty hands). */
	void SelectWalletItem(int32 Index);

private:
	bool Tick(float DeltaSeconds);
	/** Cursor, hover, clicks on parts and screen, lights, tray contents. */
	void TickKiosk(float DeltaSeconds);
	void OnPartClicked(EKioskPart Part);
	void PickUpFromTray();
	void ShowToast(const FText& Text);
	bool TrayHasContents() const { return bTrayTicket || bTrayReceipt || TrayChangeCents > 0; }
	void SyncTray();

	void SetWalkingInput();
	TSharedRef<SWidget> BuildHud();
	TSharedRef<SWidget> BuildWallet();

	// --- self-test ---------------------------------------------------------------------------
	/** -Kiosk3DSelfTest: walks up, locks on, buys a ticket with cash and then with a card tap through the real 3D parts, logging one line per step. */
	bool TickSelfTest(float DeltaSeconds);
	void BuildSelfTestPlan();
	/** Runs the current plan step. 1 = finished, 0 = still running, -1 = failed (TestDetail says why). */
	int32 StepSelfTest();
	int32 StepClickPart(EKioskPart Part);
	void SelfTestOk(const FString& Step, const FString& Detail);
	void SelfTestFail(const FString& Step, const FString& Detail);
	bool FindButtonPixel(const FString& Name, FVector2D& OutPixel) const;
	void PressKeyThroughProcessor(const FKey& Key);
	void SetSyntheticCursor(const FVector& WorldTarget);
	void ClearSyntheticCursor() { bSyntheticCursor = false; }
	void QueueSyntheticClick() { SyntheticClickFrames = 4; }
	void RequestShot(const FString& FilePath);

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	TSharedPtr<SKioskRoot> Root;
	TSharedPtr<FKioskSession> Session;
	TWeakObjectPtr<AKioskEnvironment> Environment;
	TWeakObjectPtr<AKioskVisitorPawn> Pawn;
	TSharedPtr<IInputProcessor> InputProcessor;
	TSharedPtr<SWidget> Hud;
	TSharedPtr<SWidget> WalletHud;
	FTSTicker::FDelegateHandle TickHandle;

	bool bStarted = false;
	bool bKioskView = false;
	bool bHardwareDemo = false; // -Kiosk3DShotView=kiosk: the hardware frame shows a card, lit parts and a full tray
	FString ShotPath;
	float ShotTimer = -1.f;
	float QuitTimer = -1.f;

	// Wallet, cursor and parts.
	int32 SelectedItem = INDEX_NONE;
	EKioskPart CursorPart = EKioskPart::None;
	bool bScreenPressed = false;
	bool bSyntheticCursor = false;
	FVector SyntheticTarget = FVector::ZeroVector;
	int32 SyntheticClickFrames = 0;
	FText ToastText;
	double ToastUntil = 0.0;
	bool bLastUseAccepted = false;
	int32 UseCount = 0;

	// Collection tray and card pad.
	EKioskScreen LastScreen = EKioskScreen::Home;
	bool bTrayTicket = false;
	bool bTrayReceipt = false;
	int32 TrayChangeCents = 0;
	bool bCardOnPad = false;

	// Self-test state.
	enum class ETestAct : uint8
	{
		Spawn, Walk, LockOn, Unlock, Collide,
		Click,          // click a button on the 3D screen by name
		Part,           // click a kiosk part with the current wallet item
		Select,         // choose a wallet item with its number key (A = index, -1 = none)
		ExpectScreen,   // wait for a screen (A = EKioskScreen)
		ExpectUse,      // the last part click was accepted (A = 1) or refused (A = 0)
		CashUntilPaid,  // note slot clicks with the chosen item until the cash screen is left
		ExpectTray,     // tray holds a ticket / is empty (A = 1 / 0)
		Shot,           // screenshot (only when -Kiosk3DShotDir= is given)
	};
	struct FTestAct
	{
		ETestAct Type = ETestAct::Spawn;
		FString Name;
		int32 A = 0;
		EKioskPart PartArg = EKioskPart::None;
	};
	TArray<FTestAct> TestPlan;
	FString ShotDir;
	bool bSelfTest = false;
	int32 TestStep = 0;
	int32 TestFrames = 0;
	int32 TestPhase = 0;
	int32 TestSub = 0;
	int32 SubFrames = 0;
	int32 TestTrial = 0;
	int32 CashClicks = 0;
	bool bSignsKnown = false;
	float SignU = 1.f;
	float SignV = 1.f;
	float TestTime = 0.f;
	float TestTotal = 0.f;
	uint32 SerialBefore = 0;
	FVector2D TestTarget = FVector2D::ZeroVector;
	FString TestDetail;
};
