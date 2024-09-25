// Copyright 2021 Mickael Daniel. All Rights Reserved.

using UnrealBuildTool;

public class GASCompanionEditor : ModuleRules
{
	public GASCompanionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayAbilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"GASCompanion",
				"GameplayAbilitiesEditor",
				"GameProjectGeneration",
				"ContentBrowser",
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"MainFrame",
				"AppFramework",
				"DesktopPlatform",
				"EngineSettings",
				"Engine",
				"AssetTools",
				"BlueprintGraph",
				"WorkspaceMenuStructure",
				"PropertyEditor",
				"HTTP",
				"AssetRegistry",
				"NavigationSystem",
				"GameplayTags",
				"GameplayTagsEditor",
				"UATHelper"
			}
		);
	}
}
