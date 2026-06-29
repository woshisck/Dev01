# Material Batch Audit Report

- Root: `/Game/Art`
- Map: `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
- MaxAssets: 500
- MaxActors: 120
- IncludeEngine: false
- Mode: report-only; no assets are modified or generated.

## Usage

```text
UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchAudit -Root=/Game/Art -MaxAssets=500
UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchAudit -Map=/Game/Maps/TestMap -MaxActors=200
```

- `-Root=/Game/Art`: package root to scan recursively.
- `-MaxAssets=500`: load and inspect at most this many StaticMesh assets. Use 0 for no limit.
- `-Map=/Game/Maps/TestMap`: load a map and scan placed StaticMeshComponent instances.
- `-MaxActors=200`: inspect at most this many actors when `-Map` is provided. Use 0 for no limit.
- `-IncludeEngine`: allow non-/Game roots for engine/plugin experiments.

## Summary

- StaticMesh components found: 65
- StaticMesh components inspected: 65
- Batch candidates: 36
- Rejected: 29

## Reject Reasons

| Reason | Count |
| --- | ---: |
| NotStaticMeshComponent | 29 |

## Candidate Detail

| Source | Asset/Map | Actor | Component | Materials | LODs | Tags | Status | Reason |
| --- | --- | --- | --- | ---: | ---: | --- | --- | --- |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube4` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube6` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube8` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube10` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube12` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube17` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube28` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube29` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box21` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube9` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube23` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube33` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube49` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube22` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube15` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box29` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box30` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube2` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube18` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube5` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube19` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube24` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box22` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube52` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box23` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box24` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube39` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube40` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube41` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube61` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube63` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube65` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box34` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box35` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube30` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube32` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube47` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube66` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube69` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube77` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube71` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box46` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box47` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box53` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box54` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box59` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box60` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube72` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube73` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box61` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box62` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box67` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box68` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box69` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box70` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box75` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box76` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Cube42` | `StaticMeshComponent0` | 1 | 1 | `` | Candidate | None |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box48` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box49` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box63` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box64` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box31` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box65` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |
| MapComponent | `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01` | `Box66` | `StaticMeshComponent0` | 0 | 0 | `` | Rejected | NotStaticMeshComponent |

## Next Steps

- Review multi-material candidates before enabling merge; one material slot remains the preferred production target.
- Use actor/Data Layer/tag scans to exclude dynamic, gameplay-critical, destructible, or interactive scene actors.
- This report does not create Texture2DArray, property textures, UV index data, or generated proxy meshes yet.