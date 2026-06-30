# UE5.8 Art Production, Batching, and Performance Tier Plan

> Scope: PC, Nintendo Switch 2 candidate, Steam Deck 15W, and Steam Deck 5W.
> Current project baseline: Lumen GI/reflections on by default, Nanite off, VSM off, environment lighting uses directional light, dynamic lights, and Celes/material lighting.

## 1. Goals

- Build one production rule set for environment art, materials, lighting, scene layout, and performance tiers.
- Keep PC as the highest visual quality target.
- Treat Switch 2 as a Steam Deck 15W-equivalent candidate until real hardware profiling exists.
- Use UE5.8 Lumen Lite as a test path for handheld 15W and Switch 2 candidate profiles.
- Reduce static environment draw calls through offline Geometry Merge plus a batch material system.
- Keep all generated batch assets separate from source assets so artists can iterate safely.

## 2. Platform Tiers

| Tier | Target | GI | Reflections | Geometry | Materials | Lighting |
| --- | --- | --- | --- | --- | --- | --- |
| PCUltra | PC high quality | Lumen High/Epic | Lumen or high SSR | Original mesh detail, conservative HLOD, Nanite optional experiment | Highest material lighting samples, high texture resolution | Higher dynamic light budget |
| Handheld15W | Steam Deck 15W, Switch 2 candidate | Lumen Lite, `sg.GlobalIlluminationQuality=1` | SSR, `sg.ReflectionQuality=1` | Batch proxy, HLOD, tighter LOD/cull distances | Reduced samples, Texture2DArray batch preferred | Strict dynamic light/shadow budget |
| Handheld5W | Steam Deck low power | Lumen off | SSR low/off as needed | Aggressive batch proxy and HLOD | Low samples, lower texture resolution | Minimal dynamic lights, short shadow distance |
| FallbackLow | Safety/debug floor | Lumen off | Off/low SSR | Max culling, simple proxies | Lowest material switches | Static/unshadowed fallback where acceptable |

Acceptance starts from measured frame budgets, not feature availability. If Handheld15W cannot hold 60 FPS with Lumen Lite, keep the profile but add a 40/30 FPS downgrade recommendation in the generated report.

Source-controlled starting points:

- `Config/DefaultScalability.ini` defines the project quality groups for GI, reflections, shadows, post process, textures, effects, foliage, and view distance.
- `Config/DefaultDeviceProfiles.ini` defines explicit test profiles: `DevKit_PCUltra`, `DevKit_Handheld15W`, `DevKit_Switch2Candidate`, `DevKit_Handheld5W`, and `DevKit_FallbackLow`.
- These profiles are testable through command-line/editor profile selection. Runtime player profile switching starts in `UYogPerformanceSettingsLibrary` and the current native main-menu Options screen.

## 3. Material Production Rules

- Environment assets that should batch must share the same batchable master material family.
- Batchable materials must match blend mode, shading model, static switches, material layer stack, and required texture channels.
- Avoid making many unique MI/MID variants for scalar and color changes. Put batch-time parameters into a property texture or Custom Primitive Data.
- BaseColor, Normal, ORM, Emissive, Mask, and similar texture channels can be packed into Texture2DArray assets when dimensions, format, compression, and mip rules are compatible.
- Unique high-resolution textures that cause excessive resize waste should not be forced into Texture2DArray. Route them to atlas/HLOD bake or VT/SVT as a memory/streaming backend.
- Material light sample count, emissive contribution, rim/detail switches, and cel/material-light plugin parameters must expose per-tier controls.

Current `M_Env_Building` audit evidence:

- `MaterialBatchMaterialAudit` loaded `/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building` through UE5.8 editor APIs and writes `Docs/GeneratedReports/CommandletReports/MaterialBatchMaterialAuditReport.md`.
- The source material is `BLEND_Masked` and `MSM_DefaultLit`; masked environment materials should batch only with compatible masked materials, not opaque-only batches.
- Texture parameters include unique Texture2D inputs for color, mask, MRA, and normal, existing Texture2DArray inputs `T_Array_A`, `T_Array_M`, and `T_Array_N`, and a material-light `Tex_LightInfo` CanvasRenderTarget2D input.
- Scalar/vector parameters include array index/UV controls, roughness and color tint controls, stain controls, tri-planar controls, and `LightInfoCount=4`; these are property-texture row candidates or per-tier material-light controls.
- Static switches include wall/floor/decal, unique mask/normal, stain, tri-planar, camera culling, global mask, and fake light info options. Different switch combinations still split batches unless the batch material converts the needed differences into runtime parameters.

## 4. Texture Backend Rules

### Texture2DArray

- Preferred for static environment batches with standardized texture dimensions.
- Each slice in one array must have the same width, height, format, compression, and mip policy.
- Missing textures use default slices: white BaseColor, flat Normal, default ORM, black Emissive.
- If a group exceeds the platform slice limit or the configured max slice count, split it into multiple batches.
- Use this backend when draw-call reduction and material binding consistency are the priority.

### Atlas or HLOD Bake

- Preferred for far-distance proxy assets and high-variance unique textures.
- Use when resizing many textures into one Texture2DArray would waste too much memory.
- Accepts more visual baking and lower editability, so keep it mainly for distant or proxy geometry.

### VT/SVT

- Use for memory and streaming pressure from many unique high-resolution textures.
- Do not count VT/SVT alone as draw-call batching.
- Combine with Geometry Merge/HLOD if draw-call reduction is also required.

## 5. Geometry Merge Batch System

The project batch system should be implemented as an offline generated proxy pipeline:

1. Scan selected maps, folders, Data Layers, or Actor tags.
2. Exclude dynamic, interactive, destructible, animated, or gameplay-critical actors by default.
3. Group static environment actors by spatial cluster, material compatibility, texture backend, and target tier.
4. Merge compatible StaticMesh sections into one generated StaticMesh.
5. Collapse the generated proxy to one material slot using a batch material.
6. Write `BatchMaterialIndex` per triangle through a configured mesh channel.
7. Generate Texture2DArray or fallback texture backend assets.
8. Generate a property texture and material-index map.
9. Emit a report for included, excluded, split, and failed assets.

Default index storage:

- Use `UV7.x` for `BatchMaterialIndex`.
- Reserve `UV7.y` for variant, cluster, or future flags.
- If source meshes already use UV7, the commandlet must read a rule asset and use another configured UV channel.
- Do not use `VertexColor.A` as the long-term default because precision and existing art usage are more fragile.

Material sampling:

- Batch material reads `UV7.x`, rounds/floors it to an integer index.
- The index points to a row in `_PropTexture`.
- `_PropTexture` stores texture slice ids and packed scalar/vector parameters.
- Texture channels sample Texture2DArray slices using those slice ids.
- Property texture must use point/nearest sampling and no filtering between material rows.

## 6. Model Production Rules

- Prefer one material slot per mesh where possible.
- Split models only when needed for gameplay interaction, animation, visibility, collision, or authoring clarity.
- Repeated identical meshes should use ISM/HISM before generic merge.
- Do not merge an entire level into one mesh. Cluster by room, street block, visibility cell, or HLOD region.
- Near-view static modules can use local merge when occlusion and LOD loss are acceptable.
- Far-view geometry should use HLOD/proxy generation.
- Record for each production mesh: triangle count, LOD count, material slots, collision complexity, shadow casting, distance field setting, batch eligibility, and interaction status.

## 7. Lighting and Rendering Rules

- PCUltra may use Lumen High/Epic and higher reflection/shadow quality.
- Handheld15W and Switch 2 candidate test Lumen Lite plus SSR and no Nanite/VSM dependency.
- Handheld5W defaults to Lumen off.
- Dynamic local lights need per-tier budgets for count, radius, shadow casting, and visibility distance.
- Directional light and skylight settings must be documented per tier.
- Material lights from the cel/material lighting plugin must expose per-tier sample count and feature switches.
- Distance Field settings must be profiled because Lumen software tracing and distance-field AO paths can make dense static scenes expensive.

## 8. Player Graphics Settings

Expose these settings through the normal player settings UI and save them through the existing settings save system:

- Preset: PCUltra, Handheld15W, Switch2Candidate, Handheld5W, FallbackLow.
- Resolution mode and render scale.
- GI mode: Off, Lumen Lite, Lumen High.
- Reflection mode: Off/Low SSR/High SSR/Lumen where supported.
- Shadow quality and shadow distance.
- Dynamic light quality.
- Material light quality.
- Model quality, HLOD distance, and foliage/detail distance.
- VSync and frame limit.

Runtime setting changes should adjust cvars and quality parameters only. Batch proxy assets remain offline-generated.

Current implementation bridge:

- `UYogSettingsSave::GraphicsSettings` stores the selected preset and detailed graphics values.
- `UYogPerformanceSettingsLibrary` exposes Blueprint-callable profile creation, save/load, and apply functions.
- `UYogGameInstanceBase::HandleOptionsClicked` now opens a native graphics screen from the main menu Options button with presets, resolution scale, frame limit, Lumen Lite, batch proxy preference, and key quality controls.
- Final Blueprint settings UI should call the library functions instead of duplicating cvar/scalability logic.

## 9. Automation Pipeline

Codex automation:

- The recurring continuation must use a Codex heartbeat automation attached to this thread, not Windows Task Scheduler.
- Automation id: `ue5-8`.
- Each run should read `guide.md`, the latest heartbeat report, the latest static audit, and this tier plan before modifying files.
- Each run should continue from the latest unfinished implementation item and must not repeat completed edits or revert unrelated changes.

Create a `MaterialBatchBuild` commandlet or editor utility with these inputs:

- Source maps, folders, Data Layers, or selected actors.
- Include/exclude gameplay tags or Actor tags:
  - `EnvBatch.Include`
  - `EnvBatch.Exclude`
  - `EnvBatch.Group.<Name>`
  - `EnvBatch.Cluster.<Name>`
- Rule asset for texture backend choice, standard sizes, max slices, output folder, UV channel, and tier-specific limits.

Expected generated assets:

- `SM_BatchProxy_<Cluster>`
- `MI_Env_Batch_<Cluster>`
- `T2DA_BaseColor_<Cluster>`
- `T2DA_Normal_<Cluster>`
- `T2DA_ORM_<Cluster>`
- `T2DA_Emissive_<Cluster>`
- `T2DA_Mask_<Cluster>`
- `T_PropTexture_<Cluster>`
- `DA_MaterialBatchMap_<Cluster>`
- `MaterialBatchBuildReport.md`
- `MaterialBatchBuildManifest.json`

Generated assets should live under `Content/Generated/MaterialBatch/...`. Source assets and source maps must not be overwritten.

Current implementation state:

- `MaterialBatchAudit` commandlet exists as a report-only first stage.
- Run example:

```text
UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchAudit -Root=/Game/Art -MaxAssets=500
```

- Output report:
  - `Saved/MaterialBatchAuditReport.md`
  - `Docs/GeneratedReports/CommandletReports/MaterialBatchAuditReport.md`
- It currently scans StaticMesh assets, records material slot count and LOD count, and classifies basic candidates through `FMaterialBatchCandidateRules`.
- It does not yet generate merged geometry, UV7 material index data, or replacement Data Layers.
- Next build stage should add Actor/Data Layer/tag scanning before any asset generation.
- `MaterialBatchBuild` now exists as a dry-run build-plan commandlet. It scans map or asset candidates, reports found/inspected/candidate/rejected counts, assigns planned `BatchMaterialIndex` ranges per candidate entry, and generates deterministic output package names for the future proxy mesh, batch material instance, Texture2DArray assets, property texture, and mapping data asset without modifying or generating assets.
- The commandlet also writes `MaterialBatchBuildManifest.json` using schema `DevKit.MaterialBatchBuildPlan.v1`. This machine-readable manifest records the planned generated packages, candidate summary, entries, reject reasons, per-entry `FirstBatchMaterialIndex` / `BatchMaterialIndexCount`, and one `materialRows` record per candidate material slot. Each material row contains `batchMaterialIndex`, `sourceEntryIndex`, `materialSlotIndex`, `materialSlotName`, and the resolved source `material` path so the later Texture2DArray, property texture, and geometry merge generator stages have a stable input contract.
- Each `materialRows` record now also includes a `textureChannels` array. The dry-run stage enumerates texture parameters from the resolved source material or material instance through UE5.8 material APIs, classifies parameter names into BaseColor, Normal, ORM, Emissive, Mask, LightInfo, or Unknown, and records the resolved texture path/class when one is available.
- `MaterialBatchBuildManifest.json` now includes `textureArrays`, with one slice table each for BaseColor, Normal, ORM, Emissive, and Mask. Each slice row records `sliceIndex`, source texture path, and texture class. This is the source-to-slice table that the later Texture2DArray writer must consume.
- Texture channel records also expose `width`, `height`, `arrayBuildEligible`, and `arrayBuildReason`. The generator must only allocate slices for eligible Texture2D inputs. Existing Texture2DArray inputs, CanvasRenderTarget2D material-light inputs, missing textures, unsupported channels, and textures with unknown dimensions must be preserved or rejected with their recorded reason instead of being silently packed.
- `MaterialBatchBuildManifest.json` now also includes `propertyRows`. Each property row is keyed by `batchMaterialIndex` and records the planned BaseColor, Normal, ORM, Emissive, and Mask slice index for the future property texture row. `LightInfo` is kept as a separate texture reference rather than forced into ordinary Texture2DArray slice allocation.
- `MaterialBatchBuildManifest.json` now includes `propertyTextureLayout`. This declares the stable property texture column protocol: row key is `batchMaterialIndex`, x is `propertyIndex`, y is the material row, and the initial integer columns are BaseColorSlice, NormalSlice, ORMSlice, EmissiveSlice, and MaskSlice with `-1` as the missing-slice value.
- `MaterialBatchBuildManifest.json` now includes `geometryMergePlan`. This declares the proxy mesh target, source coordinate space, `TexCoord7.x` material index stream, source component transforms, and `materialSlotRemap` rows from each source material slot to its assigned `batchMaterialIndex`.
- `UMaterialBatchMappingDataAsset` now exists as a runtime-readable generated mapping asset type. `MaterialBatchBuild -ApplyMappingOnly` writes `/Game/Generated/MaterialBatch/<Tier>/<Cluster>/DA_MaterialBatchMap_<Cluster>` with material rows, Texture2DArray slice plans, property texture layout/rows, geometry merge sources, and material slot remaps.
- `MaterialBatchBuild -ApplyTextureArraysOnly` now writes Texture2DArray assets under `/Game/Generated/MaterialBatch/<Tier>/<Cluster>/T2DA_*_<Cluster>` from the build-plan slice payloads. This stage still needs validation on a real art cluster with eligible source Texture2D inputs so dimensions, formats, and useful slice counts are proven.
- `MaterialBatchBuild -ApplyPropertyTextureOnly` now writes `/Game/Generated/MaterialBatch/<Tier>/<Cluster>/T_PropTexture_<Cluster>` as a linear RGBA16F texture. The first implemented columns store BaseColor, Normal, ORM, Emissive, and Mask slice indices in the R channel; missing slices remain `-1`.
- `MaterialBatchBuild -ApplyProxyMeshOnly` now writes `/Game/Generated/MaterialBatch/<Tier>/<Cluster>/SM_BatchProxy_<Cluster>` as a conservative LOD0 merged StaticMesh. It remaps the output mesh to one `MaterialBatch` material slot and writes each source polygon's `batchMaterialIndex` into `TexCoord7.x`.
- `MaterialBatchBuild -ApplyBatchMaterialOnly` now writes `/Game/Generated/MaterialBatch/<Tier>/<Cluster>/MI_Env_Batch_<Cluster>`, uses the currently available `/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building` parent, binds generated `T2DA_BaseColor`, `T2DA_Normal`, and `T2DA_ORM` assets to the audited `T_Array_A`, `T_Array_N`, and `T_Array_M` parameters, and assigns the material instance to proxy mesh slot 0 when the proxy mesh exists.
- This is the current asset-binding bridge, not the final runtime batch shader. A future `M_Env_Building_Batch` parent still needs to read `TexCoord7.x`, fetch rows from `T_PropTexture_<Cluster>`, and use the row values as Texture2DArray slice indices before visual-correct material-row variation is complete. Full `-Apply` and map actor replacement remain disabled until generated proxies and the final batch parent are reviewed.
- `MaterialBatchMaterialAudit` now exists as a report-only target-material commandlet for `M_Env_Building`. It captures texture parameters, scalar parameters, vector parameters, static switches, blend mode, shading model, and material-light evidence without modifying assets.
- `UE58ScenePerformanceAudit` now exists as a report-only representative-map commandlet. It loads the requested map, flushes streaming levels, and records actor count, StaticMesh component count, StaticMesh material slot upper-bound, movable StaticMesh components, light counts, movable lights, and shadow-casting lights. This gives static scene pressure evidence for tier planning; it does not replace runtime GPU/draw-call profiling.
- `UE58RuntimeProfilingPlan` now exists as a report-only profiling checklist commandlet. It writes the required Baseline, Lumen Lite, Batch Proxy, Batch Proxy + Lumen Lite, Handheld5W, and PCUltra scenario matrix with setup CVars, capture commands, and `NotMeasured` result templates. It is intentionally not a profiler and must not be treated as measured evidence.
- Run example:

```text
UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchBuild -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 -Root=/Game/Art -Cluster=Prison_S_01 -Tier=Handheld15W -DryRun
UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchBuild -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 -Root=/Game/Art -Cluster=Prison_S_01 -Tier=Handheld15W -ApplyMappingOnly
UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchBuild -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 -Root=/Game/Art -Cluster=Prison_S_01 -Tier=Medium -ApplyMappingOnly -ApplyTextureArraysOnly -ApplyPropertyTextureOnly -ApplyProxyMeshOnly -ApplyBatchMaterialOnly
UnrealEditor-Cmd.exe DevKit.uproject -run=UE58ScenePerformanceAudit -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01
UnrealEditor-Cmd.exe DevKit.uproject -run=UE58RuntimeProfilingPlan -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 -Cluster=Prison_S_01 -Camera=WideShot_A
```

- Output report:
  - `Saved/MaterialBatchBuildReport.md`
  - `Docs/GeneratedReports/CommandletReports/MaterialBatchBuildReport.md`
- Output manifest:
  - `Saved/MaterialBatchBuildManifest.json`
  - `Docs/GeneratedReports/CommandletReports/MaterialBatchBuildManifest.json`
- Runtime profiling checklist output:
  - `Saved/UE58RuntimeProfilingPlanReport.md`
  - `Docs/GeneratedReports/CommandletReports/UE58RuntimeProfilingPlanReport.md`
- Current verified map sample reports 65 StaticMesh components inspected, 36 batch candidates, 29 rejected components, and a planned batch-entry table with `FirstBatchMaterialIndex` / `BatchMaterialIndexCount` for `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`.
- Current verified scene-performance sample for the same map reports 72 actors, 65 StaticMesh components, 36 StaticMesh components with mesh, 36 StaticMesh material slots as the pre-merge draw-call pressure proxy, 0 movable StaticMesh components, and 0 light components in that loaded map package.

## 10. MCP and Profiling Workflow

Use UE5.8 MCP to inspect and validate:

- `M_Env_Building` expression graph, parameter groups, static switches, texture samples, and material output paths.
- Scene lights, post-process volumes, and environment actors in representative maps.
- StaticMesh material slot counts, repeated meshes, actor folders, and batch candidates.
- Console stats and profile captures.

Current local connection state:

- UE5.8 MCP is enabled in `DevKit.uproject` and `Config/DefaultEditorPerProjectUserSettings.ini` on `http://127.0.0.1:8765/mcp`.
- `BuildScripts/Automation/Invoke-UE58McpTool.ps1` wraps the UE MCP HTTP/SSE protocol for this Codex session. Use it to list toolsets, describe toolsets, or call short tool names through `call_tool`.
- When an editor instance is already serving MCP on port 8765, commandlet runs should use a temporary MCP port, for example `-ModelContextProtocolPort=8766`, to avoid listener bind failures.

Required profiling commands:

- `stat unit`
- `stat rhi`
- `stat scenerendering`
- `stat gpu`
- `profilegpu`
- `r.MeshDrawCommands.LogDynamicInstancingStats 1` when testing instancing behavior.

## 11. Validation Matrix

For each representative scene, compare:

1. Baseline: explicit Lumen-off comparison, no batch proxy.
2. Lumen Lite only.
3. Geometry Merge plus Texture2DArray batch.
4. Geometry Merge plus Atlas/HLOD bake.
5. Geometry Merge plus VT/SVT backend.
6. PC-only Nanite experiment if the target content supports it.

Record:

- FPS, Game ms, GPU ms, RHI ms.
- Mesh draw calls and total draw calls.
- Dynamic light count and shadow cost.
- Lumen pass cost.
- Texture memory, streaming pool pressure, VT page misses when VT is enabled.
- Visual differences and known artifacts.
- Batch build success rate and reasons for skipped assets.

## 12. Rollout

Phase 1:

- Document the rules.
- Audit `M_Env_Building`.
- Run `MaterialBatchAudit` for `/Game/Art` and review static mesh candidate shape.
- Build one small manual batch sample.
- Measure baseline vs sample.

Phase 2:

- Extend `MaterialBatchAudit` into a separate `MaterialBatchBuild` commandlet after candidate rules are validated.
- Generate reports without replacing live scenes.
- Add artist-facing include/exclude tags and rule assets.

Phase 3:

- Integrate generated batch proxies into test maps or Data Layers.
- Extend the native preset-only graphics screen into the final Blueprint player settings UI.
- Profile PCUltra, Handheld15W, Handheld5W, and FallbackLow.

Phase 4:

- Connect the commandlet to packaging automation.
- Require compile before upload or push.
- Keep generated reports in `Docs/90_自动生成报告` or `Docs/GeneratedReports`.

## 13. Hard Constraints

- Do not edit binary assets manually.
- Do not overwrite source meshes, source materials, or source maps during batch generation.
- Do not include gameplay-critical or interactive actors without explicit tags.
- Do not treat VT/SVT as draw-call batching.
- Do not claim Switch 2 validation without target hardware.
- Do not upload or push without compiling first.
