using UnrealBuildTool;

public class WeaveLanguage : ModuleRules
{
	public WeaveLanguage(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"EditorStyle",
				"BlueprintGraph",
				"Kismet",
				"Json",
				"JsonUtilities",
				"ToolMenus",
				"ApplicationCore"
			}
		);
	}
}
