# UE5.8 Performance and Art Production Static Audit

- Time: 2026-06-27T15:04:30.3287107+08:00
- Repo: D:\Self\GItGame\Dev01
- Tier plan: D:\Self\GItGame\Dev01\Docs\04_开发实现与系统文档\性能\UE58_ArtPerformanceTieringAndBatching.md
- Comprehensive plan: D:\Self\GItGame\Dev01\Docs\04_开发实现与系统文档\性能\UE58_美术制作_性能分级_自动批处理完整计划.md
- Target sample material exists: True
- MCP project settings detected: True

## Renderer Defaults

| Key | Value |
| --- | --- |
| r.DynamicGlobalIlluminationMethod | 0 |
| r.ReflectionMethod | 2 |
| r.Shadow.Virtual.Enable | 0 |
| r.Nanite.ProjectEnabled | False |
| r.Nanite | 0 |
| r.GenerateMeshDistanceFields | True |
| r.AntiAliasingMethod | 2 |
| r.VirtualTextures | False |
| r.VirtualTexturedLightmaps | False |
| r.ForwardShading | False |

## Source-Controlled Tier Files

| File | Exists |
| --- | --- |
| Config/DefaultScalability.ini | True |
| Config/DefaultDeviceProfiles.ini | True |
| UE58 tier plan | True |
| UE58 comprehensive art/performance plan | True |
| UYogSettingsSave graphics fields | True |
| UYogPerformanceSettingsLibrary | True |
| Custom graphics settings builder | True |
| Frontend Options performance UI | True |
| Frontend Options detailed custom UI | True |
| Frontend preset state sync | True |
| Performance settings automation test | True |
| Graphics settings UMG base | True |
| Graphics settings frontend entry | True |
| Graphics settings setup commandlet | True |
| Graphics settings widget automation contracts | True |
| Graphics settings focus contract | True |
| Generated graphics settings widget asset | True |
| Graphics settings setup report | True |
| Graphics settings config/cook path | True |
| UE58 Material MCP audit report | True |
| UE58 Material MCP required batch params | True |
| UE58 Material MCP graph evidence | True |
| UE58 Batch Visual MCP audit report | True |
| UE58 Batch Visual MCP captures | True |
| UE58 Batch Visual MCP PNG files | True |
| UE58 Scene Parity MCP audit report | True |
| UE58 Scene Parity MCP PNG file | True |
| UE58 Scene Parity MCP ready | True |
| MaterialBatchAudit commandlet | True |
| MaterialBatchBuild dry-run commandlet | True |
| MaterialBatchMaterialAudit commandlet | True |
| Material batch candidate test | True |
| Material batch build plan test | True |
| Material batch mapping data asset test | True |
| Material batch mapping data asset type | True |
| MaterialBatchBuild ApplyMappingOnly | True |
| Generated mapping data asset | True |
| MaterialBatchBuild ApplyTextureArraysOnly | True |
| Generated Texture2DArray assets | True |
| MaterialBatchBuild ApplyPropertyTextureOnly | True |
| Generated property texture asset | True |
| MaterialBatchBuild ApplyProxyMeshOnly | True |
| Generated proxy mesh asset | True |
| MaterialBatchBuild ApplyBatchMaterialOnly | True |
| Generated batch material instance | True |
| MaterialBatchAudit report | True |
| MaterialBatchBuild report | True |
| MaterialBatchMaterialAudit report | True |
| MaterialBatchMaterialAudit target loaded | True |
| MaterialBatchMaterialAudit texture params | True |
| MaterialBatchMaterialAudit scalar params | True |
| MaterialBatchMaterialAudit vector params | True |
| MaterialBatchMaterialAudit static switches | True |
| MaterialBatchMaterialAudit Texture2DArray evidence | True |
| MaterialBatchMaterialAudit light info evidence | True |
| MaterialBatchBuild planned entries | True |
| MaterialBatchBuild JSON manifest | True |
| MaterialBatchBuild material rows | True |
| MaterialBatchBuild material sources | True |
| MaterialBatchBuild texture channel arrays | True |
| MaterialBatchBuild texture channel values | True |
| MaterialBatchBuild report texture array eligibility | True |
| MaterialBatchBuild JSON texture array eligibility values | True |
| MaterialBatchBuild report texture array slices | True |
| MaterialBatchBuild JSON texture arrays | True |
| MaterialBatchBuild JSON texture array slice values | True |
| MaterialBatchBuild report property rows | True |
| MaterialBatchBuild JSON property rows | True |
| MaterialBatchBuild report property texture layout | True |
| MaterialBatchBuild JSON property texture layout | True |
| MaterialBatchBuild parent material contract source | True |
| MaterialBatchBuild report parent material contract | True |
| MaterialBatchBuild JSON parent material contract | True |
| MaterialBatchParentMaterialSetup commandlet | True |
| M_Env_Building_Batch asset | True |
| MaterialBatchParentMaterialSetup report evidence | True |
| MaterialBatchBuild report geometry merge plan | True |
| MaterialBatchBuild report material slot remap | True |
| MaterialBatchBuild JSON geometry merge plan | True |
| MaterialBatchBuild JSON material slot remap | True |
| MaterialBatchBuild report mapping asset saved | False |
| MaterialBatchBuild report Texture2DArray assets saved | False |
| MaterialBatchBuild report property texture saved | False |
| MaterialBatchBuild report proxy mesh saved | False |
| MaterialBatchBuild report batch material saved | True |
| MaterialBatchBuild report batch material bindings | True |
| UE58ScenePerformanceAudit report | True |
| UE58ScenePerformanceAudit loaded map | True |
| UE58ScenePerformanceAudit StaticMesh evidence | True |
| UE58ScenePerformanceAudit light evidence | True |
| UE58RuntimeProfilingPlan commandlet | True |
| UE58RuntimeProfilingPlan automation test | True |
| UE58RuntimeProfilingPlan report | True |
| UE58RuntimeProfilingPlan scenario matrix | True |
| UE58RuntimeProfilingPlan capture commands | True |
| UE58RuntimeProfiling MCP smoke report | True |
| UE58RuntimeProfiling MCP smoke ready | True |
| UE58RuntimeProfiling capture script | True |
| UE58RuntimeProfiling capture report | True |
| UE58RuntimeProfiling capture ready | True |

## DevKit Device Profiles

- DevKit_High
- DevKit_Low
- DevKit_Medium
- DevKit_Ultra

## Scalability Section Count

- Detected quality sections: 32

## Content Counts

| Item | Count |
| --- | ---: |
| TotalContentAssets | 7380 |
| EnvMaterialAssets | 99 |
| ArtAssets | 1340 |
| StaticMeshNameCandidates | 807 |
| MaterialNameCandidates | 1292 |

## Material Batch Audit Latest Report

| Metric | Value |
| --- | --- |
| Root | `/Game/Art` |
| Map | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` |
| StaticMesh components found | 65 |
| StaticMesh components inspected | 65 |
| Batch candidates | 36 |
| Rejected | 29 |

## Material Batch Build Latest Report

| Metric | Value |
| --- | --- |
| Root | `/Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03` |
| Map | `(not set)` |
| DataLayer | `(not set)` |
| Cluster | `FloorBrick03_Probe` |
| Tier | `Medium` |
| OutputRoot | `/Game/Generated/MaterialBatch` |
| Mode | partial apply; supported generated assets may be written, while full apply and map replacement remain disabled. |
| Source kind | StaticMeshAsset |
| Source found | 24 |
| Source inspected | 24 |
| Batch candidates | 24 |
| Rejected | 0 |
| Planned entries | present |
| JSON manifest | present |
| JSON manifest schema | DevKit.MaterialBatchBuildPlan.v1 |
| JSON material rows | present |
| JSON material sources | present |
| JSON texture channel arrays | present |
| JSON texture channel values | present |
| Report texture array eligibility | present |
| JSON texture array eligibility values | present |
| Report texture array slices | present |
| JSON texture arrays | present |
| Report property texture rows | present |
| JSON property rows | present |
| Report property texture layout | present |
| JSON property texture layout | present |
| Report geometry merge plan | present |
| Report material slot remap | present |
| JSON geometry merge plan | present |
| JSON material slot remap | present |
| Mapping data asset type | present |
| ApplyMappingOnly commandlet path | present |
| Generated mapping asset | D:\Self\GItGame\Dev01\Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\DA_MaterialBatchMap_FloorBrick03_Probe.uasset |
| ApplyTextureArraysOnly commandlet path | present |
| Generated Texture2DArray asset count | 4 |
| ApplyPropertyTextureOnly commandlet path | present |
| Generated property texture | D:\Self\GItGame\Dev01\Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\T_PropTexture_FloorBrick03_Probe.uasset |
| ApplyProxyMeshOnly commandlet path | present |
| Generated proxy mesh | D:\Self\GItGame\Dev01\Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\SM_BatchProxy_FloorBrick03_Probe.uasset |
| ApplyBatchMaterialOnly commandlet path | present |
| Generated batch material instance | D:\Self\GItGame\Dev01\Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\MI_Env_Batch_FloorBrick03_Probe.uasset |
| Report batch material saved | present |
| Report batch material bindings | present |

## Target Material Audit Latest Report

| Metric | Value |
| --- | --- |
| Material | `/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building` |
| Loaded | Yes |
| Class | `Material` |
| BlendMode | BLEND_Masked |
| ShadingModel | MSM_DefaultLit |
| Masked | Yes |
| Texture parameters | present |
| Scalar parameters | present |
| Vector parameters | present |
| Static switches | present |
| Texture2DArray parameters | present |
| Material light info | present |

## UE5.8 Scene Performance Audit Latest Report

| Metric | Value |
| --- | --- |
| Map | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` |
| Loaded | Yes |
| Levels | 1 |
| Actors | 72 |
| StaticMesh components | 65 |
| StaticMesh components with mesh | 36 |
| StaticMesh material slot upper-bound | 36 |
| Movable StaticMesh components | 0 |
| Light components | 0 |
| Movable lights | 0 |
| Shadow-casting lights | 0 |

## UE5.8 Runtime Profiling Plan Latest Report

| Metric | Value |
| --- | --- |
| Map | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` |
| Cluster | `Prison_S_01` |
| Camera label | `WideShot_A` |
| Evidence status | NotMeasured |
| Scenario matrix | present |
| Capture commands | present |
| NotMeasured guard | present |

## Static Risks and Gaps

- Lumen is disabled in project defaults; Lumen Lite must be validated through an explicit profile or test cvar set.
- Nanite is disabled in project defaults; handheld batching should not depend on Nanite until a separate experiment proves value.

## Required Next Evidence

- Scene parity evidence is available; review the source/proxy side-by-side PNG before enabling production map replacement.
- Compile before upload or push.
