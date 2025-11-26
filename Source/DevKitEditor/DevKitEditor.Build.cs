// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DevKitEditor : ModuleRules
{
    public DevKitEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"DevKitEditor",
                "PropertyEditor"
            }
		);

		PrivateIncludePaths.AddRange(
			new string[] {
                "DevKit"
                //"AssetRegistry",
                //"AssetTools",
                //"ContentBrowser",
                //"AnimGraph",
                //"AdvancedPreviewScene",
                //"SlateCore",
                //"SequencerWidgets",
                //"SlateNullRenderer",
                //"EditorWidgets",
                //"NiagaraEditor",

            }
		);

		PublicDependencyModuleNames.AddRange(
            new string[] {
                "DevKit",
                "Core",
                "CoreUObject",
				"TimeManagement",
                "Engine",
                "EditorFramework",
                "AnimGraph",
                "Engine",
                "BlueprintGraph",
                "UnrealEd",
				"PhysicsCore",
				"GameplayTagsEditor",
				"GameplayTasksEditor",
				"GameplayAbilities",
				"GameplayAbilitiesEditor"
				//"StudioTelemetry"


            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "TimeManagement",
                "InputCore",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"EditorStyle",
				"DataValidation",
				"MessageLog",
				"Projects",
				"DeveloperToolSettings",
				"CollectionManager",
				"SourceControl",
				"Chaos",
                "AIModule" // Add this line
				
			}
        );

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
			}
		);

		// Generate compile errors if using DrawDebug functions in test/shipping builds.
		PublicDefinitions.Add("SHIPPING_DRAW_DEBUG_ERROR=1");
    }
}
