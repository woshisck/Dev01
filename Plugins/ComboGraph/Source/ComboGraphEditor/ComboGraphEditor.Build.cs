// Copyright 2022 Mickael Daniel. All Rights Reserved.

using UnrealBuildTool;

public class ComboGraphEditor : ModuleRules
{
	public ComboGraphEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"AssetTools",
				"GameplayAbilitiesEditor",
				"GraphEditor",
				"ComboGraph",
				"EnhancedInput",
				"InputCore",
				"Persona",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"GameplayAbilities",
				"BlueprintGraph",
				"Kismet",
				"EditorStyle",
				"AnimGraphRuntime",
				"AnimGraph",
				"Projects",
				"MessageLog",
                "ApplicationCore",
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
