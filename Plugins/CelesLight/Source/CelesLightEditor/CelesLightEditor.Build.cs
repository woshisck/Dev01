using UnrealBuildTool;

public class CelesLightEditor : ModuleRules
{
	public CelesLightEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"CelesLightRuntime",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"AssetRegistry",
				"ContentBrowser",
				"LevelEditor",
				"PropertyEditor",
				"ToolMenus",
				"Slate",
				"SlateCore",
			}
		);
	}
}
