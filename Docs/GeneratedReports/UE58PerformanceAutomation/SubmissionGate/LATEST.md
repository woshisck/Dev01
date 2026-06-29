# UE5.8 Submission Gate

- Time: 2026-06-29T13:02:47.6592005+08:00
- Repo: D:\Self\GItGame\Dev01
- Can commit Phase 1 scope: True
- Can upload final main: True
- Latest UE build: Result: Succeeded
- Working tree dirty: True
- Local branch behind origin/main: False
- MCP online in latest heartbeat: True
- Static audit ready: True
- Batch visual MCP ready: True
- Scene parity MCP ready: True
- Runtime profiling MCP smoke ready: True
- Artifact contract ready: True
- Runtime profiling capture ready: True
- Remote delta audit ready: True
- Phase 1 pathspec: Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\Phase1Candidate.paths.txt
- Evidence pathspec: Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\EvidenceOnly.paths.txt
- Manual-review pathspec: Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\NeedsManualReview.paths.txt

## Test Logs

| Suite | Passed | Log |
| --- | --- | --- |
| DevKit.Performance.Settings | True | `Saved\Logs\Codex_PerformanceSettings_Focus_Tests.log` |
| DevKitEditor.UI | True | `Saved\Logs\Codex_UI_Focus_Tests.log` |
| DevKitEditor.MaterialBatch | True | `Saved\Logs\Codex_MaterialBatchTests.log` |
| DevKitEditor.UE58Performance | True | `Saved\Logs\Codex_UE58Performance_Tests.log` |

## Remote Delta

```text
(no remote delta)
```

## Git Status

```text
## main...origin/main
A  BuildScripts/Automation/Analyze-UE58PerformanceInputs.ps1
A  BuildScripts/Automation/Invoke-UE58BatchVisualMcpAudit.ps1
A  BuildScripts/Automation/Invoke-UE58FinalUpload.ps1
A  BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1
A  BuildScripts/Automation/Invoke-UE58McpTool.ps1
A  BuildScripts/Automation/Invoke-UE58RemoteDeltaAudit.ps1
A  BuildScripts/Automation/Invoke-UE58RequiredTests.ps1
A  BuildScripts/Automation/Invoke-UE58ReviewedStaging.ps1
A  BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1
A  BuildScripts/Automation/Invoke-UE58RuntimeProfilingMcpSmoke.ps1
A  BuildScripts/Automation/Invoke-UE58SceneParityMcpAudit.ps1
A  BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1
A  BuildScripts/Automation/Test-UE58PerformanceArtifactContract.ps1
A  BuildScripts/Automation/Write-UE58ApprovalPacket.ps1
A  BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1
A  Config/DefaultDeviceProfiles.ini
M  Config/DefaultGame.ini
A  Config/DefaultScalability.ini
M  Content/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_03_ZS.umap
A  Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/MI_Env_Batch_FloorBrick03_Probe.uasset
A  Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/SM_BatchProxy_FloorBrick03_Probe.uasset
A  Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T2DA_BaseColor_FloorBrick03_Probe.uasset
A  Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T2DA_Normal_FloorBrick03_Probe.uasset
A  Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T2DA_ORM_FloorBrick03_Probe.uasset
A  Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T_PropTexture_FloorBrick03_Probe.uasset
M  Content/UI/BP_YogHUD.uasset
A  Content/UI/Frontend/WBP_GraphicsSettingsWidget.uasset
M  Content/UI/Playtest_UI/HUD/WBP_HUDRoot.uasset
M  Content/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud.uasset
M  Content/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.uasset
M  Content/UI/Playtest_UI/UI_Tex/HUD/T_MaterialQuestionIcon.uasset
A  Docs/04_开发实现与系统文档/性能/GPU_Profiling_Guide.md
A  Docs/04_开发实现与系统文档/性能/README.md
A  Docs/04_开发实现与系统文档/性能/UE58_ArtPerformanceTieringAndBatching.md
A  Docs/04_开发实现与系统文档/性能/UE58_美术制作_性能分级_自动批处理完整计划.md
A  Docs/04_开发实现与系统文档/性能/UE58_美术制作与性能分级综合计划.md
A  Docs/Design/VFX/BatchMaterial_ArtistGuide.md
A  Docs/GeneratedReports/CommandletReports/GraphicsSettingsWidgetSetupReport.md
M  Docs/GeneratedReports/CommandletReports/MainUISetupReport.md
A  Docs/GeneratedReports/CommandletReports/MaterialBatchAuditReport.md
A  Docs/GeneratedReports/CommandletReports/MaterialBatchBuildManifest.json
A  Docs/GeneratedReports/CommandletReports/MaterialBatchBuildReport.md
A  Docs/GeneratedReports/CommandletReports/MaterialBatchMaterialAuditReport.md
A  Docs/GeneratedReports/CommandletReports/MaterialBatchParentMaterialSetupReport.md
A  Docs/GeneratedReports/CommandletReports/UE58RuntimeProfilingPlanReport.md
A  Docs/GeneratedReports/CommandletReports/UE58ScenePerformanceAuditReport.md
A  Docs/GeneratedReports/UE58PerformanceAudit/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58_Progress_Questions_Response.html
A  Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UserResponses_2026-06-29.md
AM Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_material_instance.png
A  Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_proxy_mesh.png
A  Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_material_instance.png
A  Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_mesh.png
A  Docs/GeneratedReports/UE58PerformanceAutomation/LATEST.md
AM Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/LATEST.md
AM Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/scene_parity_side_by_side.png
A  Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/EvidenceOnly.paths.txt
A  Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/LATEST.md
A  Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/NeedsManualReview.paths.txt
A  Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/Phase1Candidate.paths.txt
A  Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/LATEST.md
A  Docs/Tags/Tag_Namespaces.md
M  Source/DevKit/Private/SaveGame/YogSaveSubsystem.cpp
A  Source/DevKit/Private/System/MaterialBatchMappingDataAsset.cpp
M  Source/DevKit/Private/System/YogGameInstanceBase.cpp
A  Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp
A  Source/DevKit/Private/Tests/PerformanceSettingsTests.cpp
A  Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp
M  Source/DevKit/Public/SaveGame/YogSettingsSave.h
A  Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h
M  Source/DevKit/Public/System/YogGameInstanceBase.h
A  Source/DevKit/Public/System/YogPerformanceSettingsLibrary.h
A  Source/DevKit/Public/UI/YogGraphicsSettingsWidgetBase.h
M  Source/DevKitEditor/DevKitEditor.Build.cs
A  Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py
A  Source/DevKitEditor/MaterialBatch/MaterialBatchAuditCommandlet.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchAuditCommandlet.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchAuditHelpers.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchAuditHelpers.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchCandidateRules.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchCandidateRules.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAudit.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAudit.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAuditCommandlet.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAuditCommandlet.h
A  Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.cpp
A  Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.h
A  Source/DevKitEditor/Performance/UE58RuntimeProfilingPlan.cpp
A  Source/DevKitEditor/Performance/UE58RuntimeProfilingPlan.h
A  Source/DevKitEditor/Performance/UE58RuntimeProfilingPlanCommandlet.cpp
A  Source/DevKitEditor/Performance/UE58RuntimeProfilingPlanCommandlet.h
A  Source/DevKitEditor/Performance/UE58ScenePerformanceAudit.cpp
A  Source/DevKitEditor/Performance/UE58ScenePerformanceAudit.h
A  Source/DevKitEditor/Performance/UE58ScenePerformanceAuditCommandlet.cpp
A  Source/DevKitEditor/Performance/UE58ScenePerformanceAuditCommandlet.h
A  Source/DevKitEditor/Private/Tests/MaterialBatchCandidateRulesTests.cpp
A  Source/DevKitEditor/Private/Tests/UE58ScenePerformanceAuditTests.cpp
A  Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp
A  Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.h
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-16-54.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-16-55.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-18-25.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-22-45.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-23-10.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-28-54.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-29-35.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-32-36.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-32-39.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-33-43.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-39-30.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-39-33.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-52-19.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-52-56.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-53-58.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-57-33.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-57-36.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-07-36.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-08-04.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-08-27.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-08-41.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-10-16.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-13-45.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-14-25.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-15-15.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-18-18.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-18-31.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-18-48.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-21-33.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-21-46.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-22-05.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-26-36.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-26-43.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-30-14.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-30-21.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-33-39.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-33-46.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-41-23.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-41-25.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-50-49.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-51-03.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-55-34.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-56-17.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-00-56.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-02-33.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-02-46.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-17-03.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-17-41.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-19-37.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-25-43.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-27-37.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-34-38.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-35-17.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-44-59.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-45-53.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-54-44.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-55-42.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-57-12.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-57-40.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_15-07-05.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_15-13-20.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_15-14-12.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-23-27.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-24-57.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-26-03.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-39-18.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-40-16.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-49-09.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-49-30.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-49-40.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-52-17.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-57-14.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-57-52.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-04-59.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-05-13.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-06-31.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-19-21.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-19-29.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-20-49.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-36-11.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-37-18.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-46-28.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-47-52.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-57-36.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-58-03.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-58-35.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-59-57.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-06-09.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-06-51.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-10-28.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-13-05.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-21-03.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-44-26.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-45-38.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-49-50.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-53-51.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-56-40.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-58-15.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-00-23.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-02-08.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-13-02.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-13-25.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-13-57.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-14-26.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-34-15.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-34-46.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-02-02.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-02-37.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-06-06.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-09-39.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-13-30.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_00-03-33.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_00-04-16.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_05-02-28.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_10-02-43.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_15-03-37.md
?? Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_15-04-29.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-27_15-04-06.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-33-22.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-43-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-44-07.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-58-54.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-59-10.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-03-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-04-01.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-05-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-11-47.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-12-20.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-15-31.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-22-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-27-32.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-28-56.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-38-34.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-39-59.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-01-51.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-02-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-03-11.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-05-23.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-08-09.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-11-38.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-12-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-13-51.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-15-14.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-16-48.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-18-33.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-19-36.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-21-59.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-23-29.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-27-15.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-38-24.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-54-18.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-56-51.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-57-26.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-58-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_13-00-53.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_13-01-19.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_13-02-41.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/UE58BatchVisualMcpAudit_2026-06-26_20-15-37.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/UE58BatchVisualMcpAudit_2026-06-26_20-16-07.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/UE58BatchVisualMcpAudit_2026-06-26_20-18-05.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_material_instance.png.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_proxy_mesh.png.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_material_instance.png.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_mesh.png.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-26_22-13-01.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_00-02-01.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_00-03-09.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_05-01-59.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_10-02-14.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_15-03-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_11-33-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_11-43-55.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-05-21.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-28-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-38-23.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-54-19.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-56-50.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-57-25.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-58-38.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_13-00-53.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_13-01-18.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_13-02-41.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-26_22-08-55.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-27_05-01-59.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-27_10-02-14.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-27_15-03-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_11-33-11.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_12-38-47.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_12-54-18.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_12-59-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_13-01-43.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-15-02/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-21-26/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-22-57/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-25-42/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-30-25/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-32-31/
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-13-25.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-15-02.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-21-26.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-22-57.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-25-42.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-30-25.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-32-31.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/CaptureViewport.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/GetCameraTransform.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/IsPIERunning.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/SearchCVars.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/StartPIE.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/StopPIE.args.json
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/UE58RuntimeProfilingMcpSmoke_2026-06-26_20-22-05.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/UE58RuntimeProfilingMcpSmoke_2026-06-26_20-42-20.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-44-17.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-45-11.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-50-16.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-55-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_21-05-16.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_21-14-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_21-34-24.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-02-13.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-05-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-09-17.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-13-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_00-03-15.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_05-02-05.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_10-02-20.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_15-03-19.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_11-58-54.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_11-59-11.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-03-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-04-01.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-05-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-11-48.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-15-31.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-22-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-27-41.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-39-59.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-55-13.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-56-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-57-27.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-58-20.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-58-41.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_13-00-42.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_13-00-55.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_13-01-21.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-49-09.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-49-51.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-53-01.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-53-18.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-53-52.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-55-50.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-56-13.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-56-41.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-57-57.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-58-16.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-59-37.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-59-56.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-00-24.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-01-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-02-09.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-05-10.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-05-25.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-13-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-13-57.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-14-27.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-34-19.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-34-47.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-02-07.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-02-38.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-05-23.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-05-34.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-06-07.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-09-00.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-09-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-09-40.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-13-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-13-31.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-03-09.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-03-34.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-03-58.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-04-17.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_05-02-05.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_05-02-29.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_10-02-20.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_10-02-44.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-03-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-03-38.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-04-05.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-04-30.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-33-22.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-43-02.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-43-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-43-57.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-44-07.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-58-54.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-03-35.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-04-00.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-05-23.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-05-34.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-11-47.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-15-31.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-22-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-27-32.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-28-56.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-38-34.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-39-59.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-55-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-56-31.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-56-52.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-57-26.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-58-16.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-58-39.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_13-00-36.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_13-00-54.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_13-01-20.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_2026-06-26_19-28-44.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_2026-06-26_19-35-08.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_LATEST.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-02-19.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-05-48.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-09-21.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-13-12.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_00-03-15.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_00-03-58.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_05-02-10.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_10-02-25.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_15-03-19.md
?? Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_15-04-11.md
```

## Worktree Scope Summary

| Category | Count |
| --- | ---: |
| Codex automation scripts | 15 |
| Design and production docs | 7 |
| Editor automation code | 30 |
| Generated material-batch assets | 6 |
| Generated reports | 376 |
| Map assets | 1 |
| Project config | 3 |
| Runtime code | 11 |
| UI assets | 6 |

## Staging Recommendation Summary

| Recommendation | Count | Meaning |
| --- | ---: | --- |
| EvidenceOnly | 31 | Evidence/report artifact; keep only if the review wants generated proof in the commit. |
| GeneratedHistoryIgnored | 345 | Historical generated report/log artifact; not staged by the reviewed upload path. |
| NeedsManualReview | 6 | Binary or broad asset change; review explicitly before staging. |
| Phase1Candidate | 73 | Likely belongs to the UE5.8 Phase 1 implementation scope. |

## Worktree Scope Files

| Recommendation | Category | Status | Path |
| --- | --- | --- | --- |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Analyze-UE58PerformanceInputs.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58BatchVisualMcpAudit.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58FinalUpload.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58McpTool.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58RemoteDeltaAudit.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58RequiredTests.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58ReviewedStaging.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58RuntimeProfilingMcpSmoke.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Invoke-UE58SceneParityMcpAudit.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Test-UE58PerformanceArtifactContract.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Write-UE58ApprovalPacket.ps1` |
| Phase1Candidate | Codex automation scripts | A | `BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1` |
| Phase1Candidate | Design and production docs | A | `Docs/04_开发实现与系统文档/性能/GPU_Profiling_Guide.md` |
| Phase1Candidate | Design and production docs | A | `Docs/04_开发实现与系统文档/性能/README.md` |
| Phase1Candidate | Design and production docs | A | `Docs/04_开发实现与系统文档/性能/UE58_ArtPerformanceTieringAndBatching.md` |
| Phase1Candidate | Design and production docs | A | `Docs/04_开发实现与系统文档/性能/UE58_美术制作_性能分级_自动批处理完整计划.md` |
| Phase1Candidate | Design and production docs | A | `Docs/04_开发实现与系统文档/性能/UE58_美术制作与性能分级综合计划.md` |
| Phase1Candidate | Design and production docs | A | `Docs/Design/VFX/BatchMaterial_ArtistGuide.md` |
| Phase1Candidate | Design and production docs | A | `Docs/Tags/Tag_Namespaces.md` |
| Phase1Candidate | Editor automation code | M | `Source/DevKitEditor/DevKitEditor.Build.cs` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchAuditCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchAuditCommandlet.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchAuditHelpers.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchAuditHelpers.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchCandidateRules.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchCandidateRules.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAudit.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAudit.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAuditCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAuditCommandlet.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58RuntimeProfilingPlan.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58RuntimeProfilingPlan.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58RuntimeProfilingPlanCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58RuntimeProfilingPlanCommandlet.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58ScenePerformanceAudit.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58ScenePerformanceAudit.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58ScenePerformanceAuditCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Performance/UE58ScenePerformanceAuditCommandlet.h` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Private/Tests/MaterialBatchCandidateRulesTests.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/Private/Tests/UE58ScenePerformanceAuditTests.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp` |
| Phase1Candidate | Editor automation code | A | `Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.h` |
| Phase1Candidate | Generated material-batch assets | A | `Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/MI_Env_Batch_FloorBrick03_Probe.uasset` |
| Phase1Candidate | Generated material-batch assets | A | `Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/SM_BatchProxy_FloorBrick03_Probe.uasset` |
| Phase1Candidate | Generated material-batch assets | A | `Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T_PropTexture_FloorBrick03_Probe.uasset` |
| Phase1Candidate | Generated material-batch assets | A | `Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T2DA_BaseColor_FloorBrick03_Probe.uasset` |
| Phase1Candidate | Generated material-batch assets | A | `Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T2DA_Normal_FloorBrick03_Probe.uasset` |
| Phase1Candidate | Generated material-batch assets | A | `Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe/T2DA_ORM_FloorBrick03_Probe.uasset` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/GraphicsSettingsWidgetSetupReport.md` |
| EvidenceOnly | Generated reports | M | `Docs/GeneratedReports/CommandletReports/MainUISetupReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/MaterialBatchAuditReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/MaterialBatchBuildManifest.json` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/MaterialBatchBuildReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/MaterialBatchMaterialAuditReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/MaterialBatchParentMaterialSetupReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/UE58RuntimeProfilingPlanReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/CommandletReports/UE58ScenePerformanceAuditReport.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAudit/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-16-54.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-16-55.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-18-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-22-45.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-23-10.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-28-54.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-29-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-32-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-32-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-33-43.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-39-30.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-39-33.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-52-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-52-56.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-53-58.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-57-33.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_12-57-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-07-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-08-04.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-08-27.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-08-41.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-10-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-13-45.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-14-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-15-15.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-18-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-18-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-18-48.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-21-33.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-21-46.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-22-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-26-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-26-43.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-30-14.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-30-21.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-33-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-33-46.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-41-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-41-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-50-49.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-51-03.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-55-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_13-56-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-00-56.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-02-33.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-02-46.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-17-03.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-17-41.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-19-37.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-25-43.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-27-37.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-34-38.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-35-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-44-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-45-53.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-54-44.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-55-42.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-57-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_14-57-40.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_15-07-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_15-13-20.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_15-14-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-23-27.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-24-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-26-03.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-39-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-40-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-49-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-49-30.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-49-40.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-52-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-57-14.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_18-57-52.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-04-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-05-13.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-06-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-19-21.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-19-29.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-20-49.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-36-11.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-37-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-46-28.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-47-52.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-57-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-58-03.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-58-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_19-59-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-06-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-06-51.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-10-28.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-13-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-21-03.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-44-26.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-45-38.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-49-50.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-53-51.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-56-40.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_20-58-15.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-00-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-02-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-13-02.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-13-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-13-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-14-26.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-34-15.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_21-34-46.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-02-02.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-02-37.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-06-06.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-09-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-26_22-13-30.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_00-03-33.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_00-04-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_05-02-28.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_10-02-43.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_15-03-37.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAudit/UE58PerformanceAudit_2026-06-27_15-04-29.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/LATEST.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58_Progress_Questions_Response.html` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-27_15-04-06.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-33-22.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-43-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-44-07.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-58-54.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_11-59-10.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-03-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-04-01.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-05-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-11-47.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-12-20.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-15-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-22-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-27-32.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-28-56.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-38-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58ApprovalPacket_2026-06-29_12-39-59.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UserResponses_2026-06-29.md` |
| EvidenceOnly | Generated reports | AM | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-01-51.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-02-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-03-11.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-05-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-08-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-11-38.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-12-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-13-51.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-15-14.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-16-48.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-18-33.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-19-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-21-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-23-29.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-27-15.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-38-24.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-54-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-56-51.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-57-26.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_12-58-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_13-00-53.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_13-01-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/UE58ArtifactContract_2026-06-29_13-02-41.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_material_instance.png` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_material_instance.png.args.json` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_proxy_mesh.png` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/generated_batch_proxy_mesh.png.args.json` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/LATEST.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_material_instance.png` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_material_instance.png.args.json` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_mesh.png` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/source_floor_mesh.png.args.json` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/UE58BatchVisualMcpAudit_2026-06-26_20-15-37.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/UE58BatchVisualMcpAudit_2026-06-26_20-16-07.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/UE58BatchVisualMcpAudit_2026-06-26_20-18-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-26_22-05-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-26_22-09-00.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_11-42-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_11-43-53.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_12-05-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_12-56-47.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_12-57-22.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_12-58-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_13-00-49.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/UE58FinalUpload_2026-06-29_13-01-14.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/LATEST.md` |
| EvidenceOnly | Generated reports | AM | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-26_22-13-01.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_00-02-01.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_00-03-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_05-01-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_10-02-14.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-27_15-03-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_11-33-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_11-43-55.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-05-21.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-28-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-38-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-54-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-56-50.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-57-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_12-58-38.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_13-00-53.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_13-01-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RemoteDeltaAudit/UE58RemoteDeltaAudit_2026-06-29_13-02-41.md` |
| EvidenceOnly | Generated reports | AM | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-26_22-08-55.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-27_05-01-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-27_10-02-14.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-27_15-03-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_11-33-11.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_12-38-47.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_12-54-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_12-59-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RequiredTests/UE58RequiredTests_2026-06-29_13-01-43.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-15-02/Baseline_LumenOff_NoBatch/Baseline_LumenOff_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-15-02/LumenLite_NoBatch/LumenLite_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-21-26/Baseline_LumenOff_NoBatch/Baseline_LumenOff_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-21-26/LumenLite_NoBatch/LumenLite_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-22-57/Baseline_LumenOff_NoBatch/Baseline_LumenOff_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-22-57/LumenLite_NoBatch/LumenLite_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-25-42/Baseline_LumenOff_NoBatch/Baseline_LumenOff_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-25-42/LumenLite_NoBatch/LumenLite_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-30-25/Baseline_LumenOff_NoBatch/Baseline_LumenOff_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-30-25/LumenLite_NoBatch/LumenLite_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-32-31/Baseline_LumenOff_NoBatch/Baseline_LumenOff_NoBatch.log` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/2026-06-26_21-32-31/LumenLite_NoBatch/LumenLite_NoBatch.log` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-13-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-15-02.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-21-26.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-22-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-25-42.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-30-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/UE58RuntimeProfilingCapture_2026-06-26_21-32-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/CaptureViewport.args.json` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/GetCameraTransform.args.json` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/IsPIERunning.args.json` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/SearchCVars.args.json` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/StartPIE.args.json` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/StopPIE.args.json` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/UE58RuntimeProfilingMcpSmoke_2026-06-26_20-22-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/UE58RuntimeProfilingMcpSmoke_2026-06-26_20-42-20.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/LATEST.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/scene_parity_side_by_side.png` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-44-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-45-11.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-50-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/UE58SceneParityMcpAudit_2026-06-26_21-55-39.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/EvidenceOnly.paths.txt` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/LATEST.md` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/NeedsManualReview.paths.txt` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/Phase1Candidate.paths.txt` |
| EvidenceOnly | Generated reports | A | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_21-05-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_21-14-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_21-34-24.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-02-13.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-05-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-09-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-26_22-13-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_00-03-15.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_05-02-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_10-02-20.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-27_15-03-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_11-58-54.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_11-59-11.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-03-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-04-01.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-05-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-11-48.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-15-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-22-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-27-41.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-39-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-55-13.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-56-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-57-27.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-58-20.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_12-58-41.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_13-00-42.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_13-00-55.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/StagingReview/UE58ReviewedStaging_2026-06-29_13-01-21.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-49-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-49-51.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-53-01.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-53-18.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-53-52.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-55-50.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-56-13.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-56-41.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-57-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-58-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-59-37.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_20-59-56.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-00-24.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-01-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-02-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-05-10.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-05-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-13-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-13-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-14-27.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-34-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_21-34-47.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-02-07.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-02-38.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-05-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-05-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-06-07.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-09-00.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-09-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-09-40.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-13-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-26_22-13-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-03-09.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-03-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-03-58.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_00-04-17.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_05-02-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_05-02-29.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_10-02-20.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_10-02-44.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-03-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-03-38.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-04-05.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-27_15-04-30.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-33-22.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-43-02.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-43-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-43-57.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-44-07.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_11-58-54.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-03-35.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-04-00.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-05-23.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-05-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-11-47.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-15-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-22-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-27-32.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-28-56.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-38-34.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-39-59.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-55-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-56-31.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-56-52.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-57-26.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-58-16.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_12-58-39.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_13-00-36.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_13-00-54.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/UE58SubmissionGate_2026-06-29_13-01-20.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_2026-06-26_19-28-44.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_2026-06-26_19-35-08.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_LATEST.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-02-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-05-48.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-09-21.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-26_22-13-12.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_00-03-15.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_00-03-58.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_05-02-10.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_10-02-25.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_15-03-19.md` |
| GeneratedHistoryIgnored | Generated reports | ?? | `Docs/GeneratedReports/UE58PerformanceAutomation/UE58PerformanceGoalHeartbeat_2026-06-27_15-04-11.md` |
| NeedsManualReview | Map assets | M | `Content/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_03_ZS.umap` |
| Phase1Candidate | Project config | A | `Config/DefaultDeviceProfiles.ini` |
| Phase1Candidate | Project config | M | `Config/DefaultGame.ini` |
| Phase1Candidate | Project config | A | `Config/DefaultScalability.ini` |
| Phase1Candidate | Runtime code | M | `Source/DevKit/Private/SaveGame/YogSaveSubsystem.cpp` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Private/System/MaterialBatchMappingDataAsset.cpp` |
| Phase1Candidate | Runtime code | M | `Source/DevKit/Private/System/YogGameInstanceBase.cpp` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Private/Tests/PerformanceSettingsTests.cpp` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp` |
| Phase1Candidate | Runtime code | M | `Source/DevKit/Public/SaveGame/YogSettingsSave.h` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h` |
| Phase1Candidate | Runtime code | M | `Source/DevKit/Public/System/YogGameInstanceBase.h` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Public/System/YogPerformanceSettingsLibrary.h` |
| Phase1Candidate | Runtime code | A | `Source/DevKit/Public/UI/YogGraphicsSettingsWidgetBase.h` |
| NeedsManualReview | UI assets | M | `Content/UI/BP_YogHUD.uasset` |
| Phase1Candidate | UI assets | A | `Content/UI/Frontend/WBP_GraphicsSettingsWidget.uasset` |
| NeedsManualReview | UI assets | M | `Content/UI/Playtest_UI/HUD/WBP_HUDRoot.uasset` |
| NeedsManualReview | UI assets | M | `Content/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud.uasset` |
| NeedsManualReview | UI assets | M | `Content/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.uasset` |
| NeedsManualReview | UI assets | M | `Content/UI/Playtest_UI/UI_Tex/HUD/T_MaterialQuestionIcon.uasset` |

## Remote Overlap Files

- None.

## Hard Blocks

- Working tree is dirty; review and intentionally stage the UE5.8 scope before commit.

## Evidence Gaps


## Required Final Sequence

1. Resolve remote/main delta without reverting local UE5.8 work.
2. Run the required automation tests again after merge/rebase.
3. Compile immediately before upload.
4. Commit only the reviewed UE5.8 scope.
5. Push main only after the user approves the final scope.
