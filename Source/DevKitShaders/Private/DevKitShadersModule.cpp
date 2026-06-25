#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

class FDevKitShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const FString ShaderDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
		if (!AllShaderSourceDirectoryMappings().Contains(TEXT("/Project")))
		{
			AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDir);
		}
	}
};

IMPLEMENT_MODULE(FDevKitShadersModule, DevKitShaders)
