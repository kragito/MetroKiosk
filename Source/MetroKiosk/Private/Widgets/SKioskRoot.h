// Root widget: fixed 1917x1105 canvas scaled to fit, hosting header / step bar / screen / footer.
#pragma once

#include "CoreMinimal.h"
#include "KioskSession.h"
#include "Widgets/SCompoundWidget.h"

class SKioskRoot : public SCompoundWidget
{
public:
	static constexpr float CanvasWidth = 1917.f;
	static constexpr float CanvasHeight = 1105.f;

	SLATE_BEGIN_ARGS(SKioskRoot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& InSession);
	virtual ~SKioskRoot() override;

	virtual bool SupportsKeyboardFocus() const override { return true; }
	TSharedRef<FKioskSession> GetSession() const { return Session.ToSharedRef(); }

private:
	void Rebuild();
	TSharedRef<SWidget> BuildCanvas();
	TSharedRef<SWidget> BuildCallPip();
	EActiveTimerReturnType OnFrame(double InCurrentTime, float InDeltaTime);

	TSharedPtr<FKioskSession> Session;
	FDelegateHandle ChangedHandle;
	bool bDirty = true;
};
