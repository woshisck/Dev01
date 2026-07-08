# YogArt_Material

Shared material content plugin for project-wide art materials.

## Asset Boundary

- Put stable master materials, material functions, shared meshes, default textures, and reusable template material instances in this plugin.
- Put level-specific or style-specific material instances under the main project content, usually in the level folder such as `/Game/Art/Map/Map_Data/<LevelName>/LevelMaterial`.
- Do not put one-off room tuning materials in this plugin unless they become shared standards.

## Suggested Content Layout

- `Content/MasterMaterial`: project-wide master materials, such as RVT mesh decal and ground reader materials.
- `Content/MaterialFunction`: shared material functions.
- `Content/MaterialInstance/RVTMeshDecal`: reusable RVT mesh decal template instances.
- `Content/MaterialInstance/Ground`: reusable ground template instances.
- `Content/Mesh`: shared helper meshes such as a default decal plane.
- `Content/Texture`: shared default masks, normals, and debug textures.
- `Content/Data`: shared material preset data assets when editor tools need them.

## Recommended RVT Mesh Decal Rule

- The master material lives in this plugin.
- Common template instances can live in this plugin.
- Per-level art instances should live in `/Game/Art/Map/Map_Data/<LevelName>/LevelMaterial`.
- Generated RVT assets and bake outputs should stay in the level's `BakeInfo` folder.
