using UnrealBuildTool;

public class DevKitShaders : ModuleRules
{
	public DevKitShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"RenderCore"
		});
	}
}
