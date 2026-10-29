#include "KioskStyle.h"
#include "Modules/ModuleManager.h"

/** Primary game module; releases the shared style (fonts, brushes) before Slate shuts down. */
class FMetroKioskModule : public FDefaultGameModuleImpl
{
public:
	virtual void ShutdownModule() override
	{
		FKioskStyle::Shutdown();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FMetroKioskModule, MetroKiosk, "MetroKiosk");
