# UE5.8 Batch Visual MCP Audit

- Time: 2026-06-26T20:19:13.9605146+08:00
- Repo: D:\Self\GItGame\Dev01
- MCP server: http://127.0.0.1:8765/mcp
- Status: Captured
- Scope: asset thumbnail capture for source and generated material-batch artifacts.
- Limitation: this proves UE can load and render thumbnails through MCP; final visual parity still requires side-by-side review in the target scene.

## Captures

| Label | Asset | Status | PNG | Bytes | SHA256 |
| --- | --- | --- | --- | ---: | --- |
| Source floor mesh | `/Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03/SM_FloorBrick_03a1.SM_FloorBrick_03a1` | Captured | `Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\source_floor_mesh.png` | 61595 | `324879a4eab45ac9f9be079cbab829c02a5b6ae74ed779f0edc1415fe7e054fd` |
| Source floor material instance | `/Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03/MI_FloorBrick_03.MI_FloorBrick_03` | Captured | `Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\source_floor_material_instance.png` | 76464 | `fbfbdad2bb11f74b841dd17361dbeda1a71cb359598958abcd052fb7e4159c58` |
| Generated batch proxy mesh | `/Game/Generated/MaterialBatch/Medium/FloorBrick03_Probe/SM_BatchProxy_FloorBrick03_Probe.SM_BatchProxy_FloorBrick03_Probe` | Captured | `Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\generated_batch_proxy_mesh.png` | 54927 | `99550e27300ead01ae7e55b237b8b7811c4739275a5e8c234c55dacaf9cbfa22` |
| Generated batch material instance | `/Game/Generated/MaterialBatch/Medium/FloorBrick03_Probe/MI_Env_Batch_FloorBrick03_Probe.MI_Env_Batch_FloorBrick03_Probe` | Captured | `Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\generated_batch_material_instance.png` | 112822 | `8e0b8d89ea7a234d7f28a505421ed733708d5c2f8d62bc5183afca2778a2062b` |

## Review Gate

- Open the generated PNGs side by side before production replacement.
- In-scene parity must still be checked with the proxy visible and the matching source actors hidden or isolated.
- If the generated batch material thumbnail is blank, distorted, or untextured, review the M_Env_Building_Batch graph, TexCoord7.x material-index path, and _PropTexture slice rows before enabling map replacement.
