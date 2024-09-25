// Copyright 2021 Mickael Daniel. All Rights Reserved.

using UnrealBuildTool;

public class GASCompanionDeveloper : ModuleRules
{
	public GASCompanionDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{

			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"GASCompanion",
				"GameplayAbilities",
				"BlueprintGraph"
			}
		);
	}
}
