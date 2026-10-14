// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DevKitEditor : ModuleRules
{
    public DevKitEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

		PublicIncludePaths.AddRange(
			new string[] {
				"DevKitEditor"
            }
		);

		PrivateIncludePaths.AddRange(
			new string[] {
                "DevKit",
                "DevKitEditor/Private"
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
				"GameplayTags",
				"GameplayAbilities",
				"GameplayAbilitiesEditor",
				"GenericGraphRuntime",
				"GenericGraphEditor",
				"Flow",
				"FlowEditor",
				"Niagara",
				"StateTreeModule",
				"StateTreeEditorModule",
				"GameplayStateTreeModule",
				"PropertyBindingUtils"
				//"StudioTelemetry"


            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
				"RHI",
				"RenderCore", // Offscreen Slate component validation.
				"Renderer", // Verify live character settings reach the renderer profile table.
                "TimeManagement",
                "InputCore",
				"Slate",
				"SlateCore",
				"LevelEditor",
				"ToolMenus",
				"EditorStyle",
				"DataValidation",
				"MessageLog",
				"Projects",
				"DeveloperToolSettings",
				"DeveloperSettings", // Live global-environment settings notifications.
				"CollectionManager",
				"SourceControl",
				"CommonUI",
				"Chaos",
                "AIModule",
				// 符文管理器工具依赖
				"PropertyEditor",
				"AssetRegistry",
				"AssetTools",
				"ContentBrowser",
				"ContentBrowserData",
				"Blutility",
				"UMG",
				"UMGEditor",
				"GraphEditor",
				"AnimationBlueprintLibrary",
				// 数值编辑工具依赖：FPlatformApplicationMisc::ClipboardCopy
				"ApplicationCore",
				"AppFramework", // Shared workbench UE color picker, with live preview and cancel restore.
				"EnhancedInput",
				// 文件对话框（CSV 导入）
				"DesktopPlatform",
				"Json",
				"JsonUtilities",
				"MeshDescription",
				"StaticMeshDescription",
				"AdvancedPreviewScene",
				"ClassViewer",
				"Foliage",
				"FoliageEdit",
				"VirtualTexturingEditor",
				"CelesLightEditor",
				"YogLookDevEditor", // Lighting comparison workbench and editor-only review maps.
				"CelesLightRuntime", // LookDev uses the production character shadow policy.
				"TypedElementFramework",
				"StateTreeModule",
				"StateTreeEditorModule",
				"GameplayStateTreeModule",
				"PropertyBindingUtils",
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
