# UE5.8 Runtime Profiling MCP Smoke

- Time: 2026-06-26T20:43:41.7057770+08:00
- Repo: D:\Self\GItGame\Dev01
- MCP server: http://127.0.0.1:8765/mcp
- Status: Ready
- PIE viewport capture: Skipped
- Viewport PNG: (not captured)
- Viewport bytes: 0
- Limitation: this is a readiness smoke, not a GPU measurement. Final acceptance still requires stat/profilegpu captures from the runtime profiling plan.

## CVar Search Results

| CVar | Found | DetailLength | Error |
| --- | --- | ---: | --- |
| `sg.GlobalIlluminationQuality` | True | 375 |  |
| `r.ScreenPercentage` | True | 1467 |  |
| `r.Lumen.DiffuseIndirect.Allow` | True | 175 |  |
| `r.MeshDrawCommands.LogDynamicInstancingStats` | True | 127 |  |
| `t.MaxFPS` | True | 91 |  |

## Next Measurement Gate

- Run each scenario from `UE58RuntimeProfilingPlanReport.md` in the same loaded map/camera state.
- Collect `stat unit`, `stat rhi`, `stat scenerendering`, `stat gpu`, `r.MeshDrawCommands.LogDynamicInstancingStats 1`, and `profilegpu` output.
- Compare `Baseline_LumenOff_NoBatch`, `LumenLite_NoBatch`, `BatchProxy_LumenOff`, and `BatchProxy_LumenLite` before enabling handheld Lumen by default.
