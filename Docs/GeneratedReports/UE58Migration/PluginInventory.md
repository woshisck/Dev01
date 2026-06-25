# UE 5.8 Plugin Inventory

Generated during the UE 5.8 migration branch `codex/ue58-migration`.

## Summary

- Project file: `DevKit.uproject`
- Target engine: `D:\UE\UE_5.8`
- Engine association updated from `5.4` to `5.8`.
- `MassEntity` was removed from `DevKit.uproject` plugin entries because UE 5.8 provides it as `Engine\Source\Runtime\MassEntity`, not as an enableable plugin descriptor.
- `Plugins/CelesLight/CelesLight.uplugin` was rewritten as valid JSON; the original descriptor had a malformed `Description` value.

## Enabled Project Plugins

| Plugin | Descriptor | Modules | UE 5.8 action |
| --- | --- | --- | --- |
| CelesLight | `Plugins/CelesLight/CelesLight.uplugin` | `CelesLightRuntime`, `CelesLightEditor` | Descriptor JSON fixed; compile plugin modules. |
| CountDownTime | `Plugins/GameFeatures/CountDownTime/CountDownTime.uplugin` | `CountDownTimeRuntime` | Compile as GameFeature plugin. |
| ElectronicNodes | `Plugins/ElectronicNodes/ElectronicNodes.uplugin` | `ElectronicNodes` | Compile editor-only plugin. |
| WeaveLanguage | `Plugins/WeaveLanguage/WeaveLanguage.uplugin` | `WeaveLanguage` | Compile editor plugin. |
| YogAnimSource | `Plugins/YogAnimSource/YogAnimSource.uplugin` | content-only | Validate content load after editor startup. |
| YogComboGraph | `Plugins/YogComboGraph/YogComboGraph.uplugin` | `YogComboGraph`, `YogComboGraphEditor` | Compile custom runtime/editor graph plugin. |
| YogSFX | `Plugins/YogSFX/YogSFX.uplugin` | content-only | Validate content load after editor startup. |

## Enabled Engine Plugins Resolved In UE 5.8

| Plugin | UE 5.8 descriptor |
| --- | --- |
| AbilitySystemGameFeatureActions | `Engine/Plugins/Experimental/AbilitySystemGameFeatureActions/AbilitySystemGameFeatureActions.uplugin` |
| ActorPalette | `Engine/Plugins/Experimental/ActorPalette/ActorPalette.uplugin` |
| AnimationLocomotionLibrary | `Engine/Plugins/Animation/AnimationLocomotionLibrary/AnimationLocomotionLibrary.uplugin` |
| AnimationWarping | `Engine/Plugins/Animation/AnimationWarping/AnimationWarping.uplugin` |
| CommonUI | `Engine/Plugins/Runtime/CommonUI/CommonUI.uplugin` |
| EditorScriptingUtilities | `Engine/Plugins/Editor/EditorScriptingUtilities/EditorScriptingUtilities.uplugin` |
| GameFeatures | `Engine/Plugins/Runtime/GameFeatures/GameFeatures.uplugin` |
| GameplayAbilities | `Engine/Plugins/Runtime/GameplayAbilities/GameplayAbilities.uplugin` |
| GameplayGraph | `Engine/Plugins/Experimental/GameplayGraph/GameplayGraph.uplugin` |
| GameplayStateTree | `Engine/Plugins/Runtime/GameplayStateTree/GameplayStateTree.uplugin` |
| GeometryScripting | `Engine/Plugins/Runtime/GeometryScripting/GeometryScripting.uplugin` |
| ModelingToolsEditorMode | `Engine/Plugins/Editor/ModelingToolsEditorMode/ModelingToolsEditorMode.uplugin` |
| ModularGameplay | `Engine/Plugins/Runtime/ModularGameplay/ModularGameplay.uplugin` |
| MotionWarping | `Engine/Plugins/Animation/MotionWarping/MotionWarping.uplugin` |
| PerformanceMonitor | `Engine/Plugins/Performance/PerformanceMonitor/PerformanceMonitor.uplugin` |
| PythonScriptPlugin | `Engine/Plugins/Experimental/PythonScriptPlugin/PythonScriptPlugin.uplugin` |
| StateTree | `Engine/Plugins/Runtime/StateTree/StateTree.uplugin` |
| TargetingSystem | `Engine/Plugins/Experimental/GameplayTargetingSystem/TargetingSystem.uplugin` |

## Disabled Or Non-Referenced Project Plugins Present On Disk

These descriptors exist in `Plugins` but are not enabled by `DevKit.uproject`; they are still part of the repository and should not be deleted during the UE 5.8 migration.

| Plugin | Descriptor | Notes |
| --- | --- | --- |
| CommonLoadingScreen | `Plugins/CommonLoadingScreen/CommonLoadingScreen.uplugin` | Not enabled in project file. |
| Flow | `Plugins/FlowGraph-2.1-5.4/Flow.uplugin` | Runtime/editor Flow modules; referenced by `DevKit.Build.cs` and `DevKitEditor.Build.cs`, so compile coverage is required even though the descriptor is not explicitly listed in `DevKit.uproject`. |
| GenericGraph | `Plugins/GenericGraph-master/GenericGraph.uplugin` | Referenced by `YogComboGraph` and project modules; compile coverage is required. |
| ImGui | `Plugins/UnrealImGui/ImGui.uplugin` | Not enabled in project file. |
| Market | `Plugins/Market/Market.uplugin` | Not enabled in project file. |
| ModularGameplayActors | `Plugins/ModularGameplayActors/ModularGameplayActors.uplugin` | Project-local descriptor; `DevKit.uproject` also enables `ModularGameplayActors`, so compile coverage is required. |
| SpawnActor | `Plugins/GameFeatures/SpawnActor/SpawnActor.uplugin` | Not enabled in project file. |
| YogArt | `Plugins/YogArt/YogArt.uplugin` | Not enabled in project file. |
| YogFrontLib | `Plugins/YogFrontLib/YogFrontLib.uplugin` | Not enabled in project file. |
| YogTopDownCore | `Plugins/GameFeatures/YogTopDownCore/YogTopDownCore.uplugin` | Not enabled in project file. |

## Disabled Missing Plugin Entries

| Plugin | Enabled | Action |
| --- | --- | --- |
| ComboGraph | false | Leave disabled. `guide.md` says ComboGraph is no longer in the player runtime path; `YogComboGraph` remains for compatibility. |
| Empty | false | Leave disabled. Missing disabled placeholder does not block UE 5.8 load. |

## Verification Status

- UE 5.8 project-file generation completed successfully.
- `DevKitEditor Win64 Development` build completed successfully.
- Controlled content resave completed for the validated package subset; see `Docs/GeneratedReports/UE58Migration/ContentValidation.md` for exclusions and residual warnings.
