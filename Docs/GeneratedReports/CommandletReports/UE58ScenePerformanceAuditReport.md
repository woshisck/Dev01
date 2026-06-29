# UE5.8 Scene Performance Audit

- Map: `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
- Loaded: Yes
- Levels: 1
- Actors: 72

## Static Meshes

- StaticMesh components: 65
- StaticMesh components with mesh: 36
- StaticMesh material slot upper-bound: 36
- Movable StaticMesh components: 0

## Lights

- Light components: 0
- Directional lights: 0
- Point lights: 0
- Spot lights: 0
- Rect lights: 0
- Movable lights: 0
- Shadow-casting lights: 0

## Interpretation

- Material slot count is a draw-call pressure proxy before instancing, HLOD, and mesh draw command merging.
- Movable StaticMesh components should not enter offline geometry merge without explicit gameplay review.
- Movable and shadow-casting lights are the first lighting budget checks for handheld Lumen Lite tests.

## Usage

```text
UnrealEditor-Cmd.exe DevKit.uproject -run=UE58ScenePerformanceAudit -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01
```

- Mode: report-only; no map or asset packages are saved.