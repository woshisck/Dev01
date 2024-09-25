// Copyright 2021 Mickael Daniel. All Rights Reserved.

using UnrealBuildTool;

public class GASCompanion : ModuleRules
{
	public GASCompanion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// ... add other public dependencies that you statically link with here ...
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"GameplayAbilities",
			"GameFeatures",
			"EnhancedInput"
		});

		// ... add private dependencies that you statically link with here ...
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayTasks",
			"GameplayTags",
			"AIModule",
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"ModularGameplay",
			"NetCore" // For OSx
		});

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Settings"
				}
			);
		}
	}
}
