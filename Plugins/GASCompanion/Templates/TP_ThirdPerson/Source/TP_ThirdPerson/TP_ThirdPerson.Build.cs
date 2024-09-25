// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class TP_ThirdPerson : ModuleRules
{
	public TP_ThirdPerson(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks",
				"GASCompanion",
				"InputCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
