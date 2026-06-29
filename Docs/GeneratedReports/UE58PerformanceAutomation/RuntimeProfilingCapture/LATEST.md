# UE5.8 Runtime Profiling Capture

- Time: 2026-06-26T21:34:02.0621536+08:00
- Repo: D:\Self\GItGame\Dev01
- Engine root: D:\UE\UE_5.8
- Editor executable: D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
- Map: /Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01
- Run requested: True
- NoQuit mode: True
- Status: ParsedLogCaptured
- Selected scenarios: Baseline_LumenOff_NoBatch, LumenLite_NoBatch
- Timeout per scenario: 45 seconds

## Scenario Results

| Scenario | Tier | Requires Batch Proxy | Status | ExitCode | TimedOut | Frame Time ms | Root Incl ms | Root Draws | Root Dispatches | Root Prims | Root Verts | Top Incl Event | Top Incl ms | ProfileGPU Report | MeshDraw Mentioned | Log | Artifacts |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | --- | --- | --- | ---: |
| `Baseline_LumenOff_NoBatch` | Medium | False | ParsedLogCaptured |  | True | 7.01 | 6.983 | 1393 | 58 | 1275833 | 1220549 | Frame 1 | 3.984 | True | True | `Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture\2026-06-26_21-32-31\Baseline_LumenOff_NoBatch\Baseline_LumenOff_NoBatch.log` | 0 |
| `LumenLite_NoBatch` | Handheld15W | False | ParsedLogCaptured |  | True | 8.15 | 8.14 | 1408 | 172 | 1275841 | 1220565 | Frame 1 | 4.427 | True | True | `Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture\2026-06-26_21-32-31\LumenLite_NoBatch\LumenLite_NoBatch.log` | 0 |

## Executed Commands

### Baseline_LumenOff_NoBatch

```text
r.SetRes 1280x720, sg.GlobalIlluminationQuality 0, sg.ReflectionQuality 1, sg.ShadowQuality 1, r.ScreenPercentage 70, r.Lumen.DiffuseIndirect.Allow 0, t.MaxFPS 60, r.ProfileGPU.ShowUI 0, r.ProfileGPU.UnicodeOutput 0, r.ProfileGPU.TableFormatting 1, stat unit, stat rhi, stat scenerendering, stat gpu, r.MeshDrawCommands.LogDynamicInstancingStats 1, profilegpu
```

### LumenLite_NoBatch

```text
r.SetRes 1280x720, sg.GlobalIlluminationQuality 1, sg.ReflectionQuality 1, sg.ShadowQuality 1, r.ScreenPercentage 70, r.Lumen.DiffuseIndirect.Allow 1, r.Lumen.FinalGatherMethod 0, r.Lumen.TraceMeshSDFs.Allow 0, r.Lumen.HardwareRayTracing.HitLighting.Allowed 0, t.MaxFPS 60, r.ProfileGPU.ShowUI 0, r.ProfileGPU.UnicodeOutput 0, r.ProfileGPU.TableFormatting 1, stat unit, stat rhi, stat scenerendering, stat gpu, r.MeshDrawCommands.LogDynamicInstancingStats 1, profilegpu
```

## Acceptance Notes

- This script is the Codex automation entry point for runtime profiling capture.
- `Status: Prepared` only proves the command harness is ready; it is not performance evidence.
- `Status: LogCaptured` means the UE process exited and logs show the profiling commands were invoked, but no ProfileGPU artifact was found.
- `Status: ParsedLogCaptured` means the UE process produced a `GPU Profile for Frame` log table and this report parsed frame/root/top-pass metrics, but no separate artifact file.
- `Status: Captured` requires at least one generated profiling artifact in addition to a successful UE process exit.
- Final handheld decisions still require reviewing the captured logs/artifacts and comparing at least baseline versus Lumen Lite scenarios.
