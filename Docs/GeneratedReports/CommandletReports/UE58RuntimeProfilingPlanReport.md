# UE5.8 Runtime Profiling Plan

- Map: `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
- Cluster: `Prison_S_01`
- Camera label: `WideShot_A`
- Evidence status: NotMeasured

## Rules

- Run every scenario from the same camera position and scene state.
- Record at least a five-second stable sample for `stat unit`, `stat rhi`, and `stat scenerendering`.
- Run `profilegpu` once per scenario and store the generated CSV path.
- Batch proxy scenarios require the generated proxy to be visible and matching source actors to be hidden or isolated in a test layer.
- This report is a measurement checklist; it does not contain measured GPU values.

## Scenario Matrix

| Scenario | Tier | Requires Batch Proxy | Description |
| --- | --- | --- | --- |
| `Baseline_LumenOff_NoBatch` | Medium | No | Current project renderer baseline: Lumen off, no generated batch proxy. |
| `LumenLite_NoBatch` | Handheld15W | No | Handheld 15W candidate with Lumen Lite enabled before proxy batching. |
| `BatchProxy_LumenOff` | Medium | Yes | Generated geometry-merge proxy path with Lumen off. |
| `BatchProxy_LumenLite` | Handheld15W | Yes | Combined handheld candidate: generated proxy plus Lumen Lite. |
| `Handheld5W_LumenOff_Aggressive` | Handheld5W | Yes | Low-power safety profile; Lumen remains off and the batch/proxy path should be preferred. |
| `PCUltra_LumenHigh` | PCUltra | No | PC upper-bound quality profile for comparison against handheld cuts. |

## Scenario: Baseline_LumenOff_NoBatch

- Tier: `Medium`
- Requires batch proxy: No

### Setup CVars

```text
r.SetRes 1280x720
sg.GlobalIlluminationQuality 0
sg.ReflectionQuality 1
sg.ShadowQuality 1
r.ScreenPercentage 70
r.Lumen.DiffuseIndirect.Allow 0
t.MaxFPS 60
```

### Capture Commands

```text
stat unit
stat rhi
stat scenerendering
stat gpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
profilegpu
```

### Result Template

| Metric | Value |
| --- | --- |
| FPS | NotMeasured |
| Game ms | NotMeasured |
| Draw ms | NotMeasured |
| GPU ms | NotMeasured |
| RHI ms | NotMeasured |
| Mesh draw calls | NotMeasured |
| Total draw calls | NotMeasured |
| Visible dynamic primitives | NotMeasured |
| Highest GPU pass | NotMeasured |
| profilegpu CSV | NotMeasured |
| Visual notes | NotMeasured |

## Scenario: LumenLite_NoBatch

- Tier: `Handheld15W`
- Requires batch proxy: No

### Setup CVars

```text
r.SetRes 1280x720
sg.GlobalIlluminationQuality 1
sg.ReflectionQuality 1
sg.ShadowQuality 1
r.ScreenPercentage 70
r.Lumen.DiffuseIndirect.Allow 1
r.Lumen.FinalGatherMethod 0
r.Lumen.TraceMeshSDFs.Allow 0
r.Lumen.HardwareRayTracing.HitLighting.Allowed 0
t.MaxFPS 60
```

### Capture Commands

```text
stat unit
stat rhi
stat scenerendering
stat gpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
profilegpu
```

### Result Template

| Metric | Value |
| --- | --- |
| FPS | NotMeasured |
| Game ms | NotMeasured |
| Draw ms | NotMeasured |
| GPU ms | NotMeasured |
| RHI ms | NotMeasured |
| Mesh draw calls | NotMeasured |
| Total draw calls | NotMeasured |
| Visible dynamic primitives | NotMeasured |
| Highest GPU pass | NotMeasured |
| profilegpu CSV | NotMeasured |
| Visual notes | NotMeasured |

## Scenario: BatchProxy_LumenOff

- Tier: `Medium`
- Requires batch proxy: Yes

### Setup CVars

```text
r.SetRes 1280x720
sg.GlobalIlluminationQuality 0
sg.ReflectionQuality 1
sg.ShadowQuality 1
r.ScreenPercentage 70
r.Lumen.DiffuseIndirect.Allow 0
t.MaxFPS 60
```

### Capture Commands

```text
stat unit
stat rhi
stat scenerendering
stat gpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
profilegpu
```

### Result Template

| Metric | Value |
| --- | --- |
| FPS | NotMeasured |
| Game ms | NotMeasured |
| Draw ms | NotMeasured |
| GPU ms | NotMeasured |
| RHI ms | NotMeasured |
| Mesh draw calls | NotMeasured |
| Total draw calls | NotMeasured |
| Visible dynamic primitives | NotMeasured |
| Highest GPU pass | NotMeasured |
| profilegpu CSV | NotMeasured |
| Visual notes | NotMeasured |

## Scenario: BatchProxy_LumenLite

- Tier: `Handheld15W`
- Requires batch proxy: Yes

### Setup CVars

```text
r.SetRes 1280x720
sg.GlobalIlluminationQuality 1
sg.ReflectionQuality 1
sg.ShadowQuality 1
r.ScreenPercentage 70
r.Lumen.DiffuseIndirect.Allow 1
r.Lumen.FinalGatherMethod 0
r.Lumen.TraceMeshSDFs.Allow 0
r.Lumen.HardwareRayTracing.HitLighting.Allowed 0
t.MaxFPS 60
```

### Capture Commands

```text
stat unit
stat rhi
stat scenerendering
stat gpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
profilegpu
```

### Result Template

| Metric | Value |
| --- | --- |
| FPS | NotMeasured |
| Game ms | NotMeasured |
| Draw ms | NotMeasured |
| GPU ms | NotMeasured |
| RHI ms | NotMeasured |
| Mesh draw calls | NotMeasured |
| Total draw calls | NotMeasured |
| Visible dynamic primitives | NotMeasured |
| Highest GPU pass | NotMeasured |
| profilegpu CSV | NotMeasured |
| Visual notes | NotMeasured |

## Scenario: Handheld5W_LumenOff_Aggressive

- Tier: `Handheld5W`
- Requires batch proxy: Yes

### Setup CVars

```text
r.SetRes 1280x720
sg.ViewDistanceQuality 0
sg.ShadowQuality 0
sg.GlobalIlluminationQuality 0
sg.ReflectionQuality 0
sg.PostProcessQuality 0
sg.TextureQuality 1
sg.EffectsQuality 0
sg.FoliageQuality 0
r.ScreenPercentage 55
r.Lumen.DiffuseIndirect.Allow 0
t.MaxFPS 30
```

### Capture Commands

```text
stat unit
stat rhi
stat scenerendering
stat gpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
profilegpu
```

### Result Template

| Metric | Value |
| --- | --- |
| FPS | NotMeasured |
| Game ms | NotMeasured |
| Draw ms | NotMeasured |
| GPU ms | NotMeasured |
| RHI ms | NotMeasured |
| Mesh draw calls | NotMeasured |
| Total draw calls | NotMeasured |
| Visible dynamic primitives | NotMeasured |
| Highest GPU pass | NotMeasured |
| profilegpu CSV | NotMeasured |
| Visual notes | NotMeasured |

## Scenario: PCUltra_LumenHigh

- Tier: `PCUltra`
- Requires batch proxy: No

### Setup CVars

```text
r.SetRes 1920x1080
sg.ViewDistanceQuality 3
sg.ShadowQuality 3
sg.GlobalIlluminationQuality 3
sg.ReflectionQuality 3
sg.PostProcessQuality 3
sg.TextureQuality 3
sg.EffectsQuality 3
sg.FoliageQuality 3
r.ScreenPercentage 100
r.Lumen.DiffuseIndirect.Allow 1
t.MaxFPS 0
```

### Capture Commands

```text
stat unit
stat rhi
stat scenerendering
stat gpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
profilegpu
```

### Result Template

| Metric | Value |
| --- | --- |
| FPS | NotMeasured |
| Game ms | NotMeasured |
| Draw ms | NotMeasured |
| GPU ms | NotMeasured |
| RHI ms | NotMeasured |
| Mesh draw calls | NotMeasured |
| Total draw calls | NotMeasured |
| Visible dynamic primitives | NotMeasured |
| Highest GPU pass | NotMeasured |
| profilegpu CSV | NotMeasured |
| Visual notes | NotMeasured |

## Acceptance Checks

- Handheld15W passes only if `BatchProxy_LumenLite` is stable at the target frame budget or has a documented fallback to `BatchProxy_LumenOff`.
- Handheld5W passes only if `Handheld5W_LumenOff_Aggressive` is stable at 30 FPS with acceptable visual loss.
- Batch proxy is considered useful only if mesh draw calls drop enough to offset any extra material shader cost.
- Lumen Lite is optional for handheld if Lumen passes consume more frame time than the visual gain justifies.