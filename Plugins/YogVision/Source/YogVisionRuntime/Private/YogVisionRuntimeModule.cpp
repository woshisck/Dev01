#include "Modules/ModuleManager.h"
#include "YogAirFogRenderBridge.h"
#include "YogNinjaFogMaskCache.h"

class FYogVisionRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override { YogAirFogRenderBridge::Startup();YogNinjaFogMaskCache::Startup(); }
    virtual void ShutdownModule() override { YogNinjaFogMaskCache::Shutdown();YogAirFogRenderBridge::Shutdown(); }
};

IMPLEMENT_MODULE(FYogVisionRuntimeModule, YogVisionRuntime)
