// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class DevKit : ModuleRules
{
	public DevKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;


        PublicIncludePaths.AddRange(
        new string[] {
            "DevKit",
            "DevKitEditor"
        }
        );

        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "CoreOnline",
                "CoreUObject", 
				"Engine", 
				"InputCore",
                "Niagara",
                "ModularGameplay",
                "ModularGameplayActors",
                "PhysicsCore",
                "GameplayTags",
                "GameplayTasks",
                "DataRegistry",
                "ReplicationGraph",
                "GameFeatures",
                "Hotfix",
                "PropertyPath",
                "CommonUI",
                "AIModule",
                "AnimGraphRuntime",

            });

		PrivateDependencyModuleNames.AddRange(new string[] {
                "InputCore",
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",
                "DeveloperSettings",
                "EnhancedInput",
                "NetCore",
                "RHI",
                "Projects",
                "Gauntlet",
                "UMG",
                "AIModule",
                "AnimGraphRuntime"
            });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
