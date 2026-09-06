# Decal & Surface Editor — implementation audit

This document records the current implementation against the agreed authoring contract. It is intentionally split into what is verified in code and automation, what is present but needs an artist pass, and what remains out of scope for the first publish.

## Verified now

| Contract | State | Evidence |
|---|---|---|
| One Collection Actor owns placement records; derived render components are rebuildable | Implemented | `ADevKitDecalCollectionActor` / `UDevKitDecalCollectionComponent` |
| RVT plane decal and RVT visible-mesh backends | Implemented | RVT_TEST reload audit: 191 mesh records, 16 derived ISM batches; FloorBrick uses `MI_L1_Corridor_01a_FloorBrick_RVT_Test`, not a debug fallback |
| Mesh Decal and Static Mesh Overlay backends | Implemented | Unified asset backend grouping creates normal ISM batches without RVT binding |
| Deferred Decal backend | Implemented and map-verified | One real `UDecalComponent` per authored record; RVT_TEST includes a `M_Rune512_GroundPath_Poison_Decal` sample; no false ISM batching claim |
| Individual editing | Implemented | Native ISM typed-element W/E/R; Deferred ray-volume selection plus one transaction per gizmo operation |
| Surface brush placement | Implemented | Selected palette asset can be painted on collision surfaces at an artist-adjustable 25–1000cm world-space interval (100cm default); Deferred decals align to the surface normal and each brush stroke is one Undo transaction |
| Duplicate / Delete | Implemented | Mode commands duplicate/delete the selected record, then rebuild derived rendering |
| Quality-scaled source geometry | Implemented and regression-tested | RVT Plane always projects; QualityScaled visible objects retain source mesh at PC High/Epic and project on Mid/Low or handheld force-projection |
| Audit | Implemented | Per-Collection validity, record/batch counts, and scan of unowned Deferred and likely legacy mesh/RVT candidates |
| Explicit Deferred / compatible ISM adopt / restore | Implemented | User-selected source only; source component is hidden, never deleted; records retain actor/component provenance and Restore disables the replacement records |
| Mode UI | Implemented | Visible editor mode opens directly on 放置; it has management, placement, asset and audit sections, type filters, collection selector, asset cards, current-asset state and dedicated Apply/Cancel footer. |

## Important intentional boundaries

- ISM batching is only inside one compatible backend + Mesh + Material batch. Deferred decals remain individual projection proxies.
- Adopt never performs a whole-world replacement. It requires an explicitly selected source and a selected valid collection asset. ISM adopt additionally rejects multi-material, collision/navigation, shadow-state mismatch, and per-instance custom-data batches so that authored semantics cannot be silently flattened.
- Restoring an adopted source keeps its record for audit history but disables it, so source and replacement do not render together.
- Collection sessions are in-place editor sessions. `取消` restores the session snapshot; it does not reset global editor undo history or auto-save a map.
- QualityScaled assets with gameplay collision are kept visible in every quality tier. Collision never becomes a per-client visual-quality gameplay difference.

## Contract coverage at this checkpoint

| Authoring contract item | Coverage | Current interpretation |
|---|---:|---|
| Unified asset, collection and source/derived ownership | Complete | Records are canonical; every derived component is rebuildable from them. |
| RVT Plane / RVT visible mesh / mesh decal / overlay / deferred backends | Complete baseline | Deferred is deliberately a managed component projection, not an invented ISM batch. RVT_TEST has a persisted Deferred sample. |
| Per-instance select, W/E/R, duplicate, delete and material override | Complete | ISM uses native typed elements; Deferred has a projected-volume selection and a real transaction. |
| Hand placement and ground painting | Complete baseline | Center placement, drag/drop surface placement, and an adjustable-spacing brush are available. |
| Explicit legacy ownership migration | Complete baseline | Deferred and compatible ISM batches can Adopt/Restore with provenance; no whole-world automatic takeover. |
| Quality/platform source-model policy | Complete | PC High/Epic retains QualityScaled source geometry; Mid/Low and handheld force projection retain RVT output. |
| Artist-oriented mode UI | Complete baseline | 打开即进入放置工作区；美术可先筛选 RVT 平面/物件、网格贴花、地表物件或延迟投射，再选择、拖放、单点放置或刷地面，并在同一工作区直接查看当前实例、修改材质预览与 Bake。 |
| Lasso, density/reapply/fill, conversion/finalize and generic mesh adoption | Remaining | These are production-expansion tools, not silently substituted by the baseline brush. |
| PCG/Niagara/Spline adapters, recovery journal, Multi-User locks, cook/perf telemetry | Remaining | Require their own acceptance and lifecycle contracts before they can be called production-complete. |

## Remaining before full production scope

1. Lasso select, density/reapply/fill, generic non-ISM static-mesh adopt, and bulk Convert/Finalize workflows.
2. Explicit Convert between backends, source Finalize/Delete workflow, recovery journal, and Multi-User resource locks.
3. PCG/Niagara/Spline adapter interfaces and bake/stamp flow.
4. Cook validation, per-Collection budgets, and performance telemetry.
5. An artist visual pass of the four Mode pages (the RVT_TEST Deferred sample is now persisted and reload-audited).

## Current automated acceptance

`DevKit.Surface.DecalCollection` currently runs:

- `ActorDefaults`
- `RecordContract`
- `BackendContract`
- `QualityPolicyContract`

All four completed successfully in the source-built editor on 2026-08-17. The tests verify data contracts and policy behaviour; they do not replace the required interactive viewport acceptance pass.
