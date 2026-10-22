// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class DevKit : ModuleRules
{
	public DevKit(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        string enginePath = Path.GetFullPath(Target.RelativeEnginePath);


        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;


        PublicIncludePaths.AddRange(
        new string[] {
            //"DevKit",
            //"DevKitEditor"
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
                "DeveloperSettings",
                "ReplicationGraph",
                "GameFeatures",
                "Hotfix",
                "PropertyPath",
                "CommonUI",
                "CommonInput",
                "NavigationSystem",
                "AIModule",
                "AnimGraphRuntime",
                "GameplayAbilities",
                "Flow",
                "GenericGraphRuntime",
                "MotionWarping",
                "StateTreeModule",
                "GameplayStateTreeModule",
                "CelesLightRuntime",
                // Public because AYogRoomDirector implements IUnLuaInterface in its header
                "UnLua",

            });

		PrivateDependencyModuleNames.AddRange(new string[] {
                "Core",
                "InputCore",
                "Json",
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",
                "EnhancedInput",
                "NetCore",
                "RHI",
                "Projects",
                "Gauntlet",
                "UMG",
                "AIModule",
                "AnimGraphRuntime",
                // Lua is built as a DLL, so any module calling the lua_* C API directly must link it
                // explicitly rather than relying on UnLua's public dependency.
                "Lua"
            });

        // Must match the DevKitLuaRules "Enabled" flag in DevKit.uproject: linking against a disabled
        // plugin's DLL makes UnrealEditor-DevKit.dll fail to load (missing import, error 126).
        bool bWithLuaRules = false;
        if (bWithLuaRules)
        {
            PrivateDependencyModuleNames.Add("DevKitLuaRules");
        }
        PrivateDefinitions.Add("WITH_DEVKIT_LUA_RULES=" + (bWithLuaRules ? "1" : "0"));

        // Versioned rule bundles are staged inside the game's pak, not loaded from authoring spreadsheets.
        RuntimeDependencies.Add("$(ProjectDir)/Content/ActionRules/...", StagedFileType.UFS);

        // Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
