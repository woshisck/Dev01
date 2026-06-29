# UE5.8 Scene Source/Proxy Parity MCP Audit

- Time: 2026-06-26T22:00:11.8486196+08:00
- Repo: D:\Self\GItGame\Dev01
- MCP server: http://127.0.0.1:8765/mcp
- Map: /Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01
- Status: Captured
- Capture status: Captured
- Cleanup status: RemovedScratchActors
- Existing source tagged actors: 0
- Existing proxy tagged actors: 0
- Scratch source asset: /Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03/SM_FloorBrick_03a1
- Scratch proxy asset: /Game/Generated/MaterialBatch/Medium/FloorBrick03_Probe/SM_BatchProxy_FloorBrick03_Probe
- Scratch isolation X: 200000
- Scratch actor scale: 5
- Viewport PNG: Docs\GeneratedReports\UE58PerformanceAutomation\SceneParityAudit\scene_parity_side_by_side.png
- Viewport bytes: 924089
- Viewport SHA256: ed43f294ab9399df475be6273077999e992e373ef7f43e3f6868a952c5141c6a
- Labeled actors in capture: 0

## Bounds

| Actor | Bounds |
| --- | --- |
| Scratch source | min=(199739.2579460144, -611.56705856323242, 114.1883659362793); max=(200260.74213027954, -88.432979583740234, 184.27953243255615); valid=True |
| Scratch proxy | min=(199730.85420608521, 80.92926025390625, 114.18830871582031); max=(200269.14575576782, 619.07073974609375, 200.23158073425293); valid=True |

## Camera

| Field | Value |
| --- | --- |
| Location | 198800, 0, 850 |
| Rotation | -28.000000000000021, 0, 0 |

## Interpretation

- This validates that the source mesh and generated proxy mesh can render side by side in the target UE5.8 level lighting context through MCP viewport capture.
- Existing production EnvBatch.Source.* / EnvBatch.Proxy.* tagged actor pairs were not required for this scratch capture.
- The script removes the scratch actors after capture and does not save the level.
- This is visual evidence for generated proxy review; it does not enable full production map replacement by itself.
