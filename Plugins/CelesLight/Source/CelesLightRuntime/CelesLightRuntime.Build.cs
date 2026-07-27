using UnrealBuildTool;

public class CelesLightRuntime : ModuleRules
{
	public CelesLightRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				// Runtime profile uploads call exported renderer APIs directly.
				"Renderer",
				"DeveloperSettings",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"RHI",
			}
		);
	}
}
