using UnrealBuildTool;

public class YogComboGraphEditor : ModuleRules
{
	public YogComboGraphEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"YogComboGraph",
			"GenericGraphRuntime",
			"GenericGraphEditor",
			"GraphEditor",
			"AssetTools",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd",
			"EditorFramework",
			"ToolMenus",
			"GameplayTags",
		});
	}
}
