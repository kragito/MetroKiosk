// Game mode that hosts the kiosk UI in the game viewport (UI-only input, visible cursor).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KioskGameMode.generated.h"

class FKioskQARunner;
class FKioskSession;
class SKioskRoot;

UCLASS()
class METROKIOSK_API AKioskGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AKioskGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RemoveKioskWidget();

	TSharedPtr<FKioskSession> Session;
	TSharedPtr<SKioskRoot> Root;
	TSharedPtr<FKioskQARunner> QARunner;
	TWeakObjectPtr<UGameViewportClient> HostViewport;
};
