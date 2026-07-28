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
				"AssetTools",
				"AssetRegistry",
				"ContentBrowser",
				"AdvancedPreviewScene",
				"InputCore",
				"LevelEditor",
				"PropertyEditor",
				"ToolMenus",
				"Slate",
				"SlateCore",
				"Settings",
			}
		);
	}
}
