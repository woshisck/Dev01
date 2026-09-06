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

## Scene Master Materials

The building and prop scene master-material contracts are generated from the
JSON/HLSL sources in `Config/MaterialCode` and `/Project/YogArt/Env`:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=YogArtCodeMaterialSetup
UnrealEditor-Cmd.exe DevKit.uproject -run=YogArtCodeMaterialSetup -Apply -Target=All -nop4 -AllowCommandletRendering
```

The first command is a read-only validation pass. Before the apply command,
open the selected generated assets plus `M_Yog_Ground_RVT_Source` and
`RVT_Default_WorldHeight` in the intended P4 changelist. Apply refuses to run
without `-AllowCommandletRendering`, because every generated quality/RVT
permutation must compile before save. Use `-Force` only when intentionally
rebuilding a generated material whose fingerprint is already current.

`YogArtMaterialMasterSetup` is retained for report compatibility only. Its
apply mode is blocked because the legacy graph generator targeted the same
building and prop assets and could overwrite the code-authored materials.

- `/YogArt_Material/MasterMaterial/Env/M_Yog_Ground_RVT_Source`: ground authoring material. It blends source layers with vertex color and writes the result to RVT Output. Its `T_Ground_*` source textures must be normal Texture2D assets with `VirtualTextureStreaming=false`.
- `/YogArt_Material/MasterMaterial/Env/M_Yog_Building_Source`: two-layer MRAH building material with height blend, quality-gated POM, two dirt overlays, bindless Texture Collections, per-instance data, and optional ground RVT contact.
- `/YogArt_Material/MasterMaterial/Env/M_Yog_Prop_Source`: compact MRAH prop material with bindless Texture Collections, per-instance data, emissive control, and optional ground RVT contact.
- `/YogArt_Material/Data/RVT_Default_Base` and `/YogArt_Material/Data/RVT_Default_WorldHeight`: default surface and height RVTs for master-material compilation. Level-specific RVT assets should still live in the level `BakeInfo` folder.

The artist-facing tiling library uses flat category folders under
`/Game/Art/Texture/CommonTex/<Category>`. Do not create a separate folder for
each texture set. Every set keeps its BaseColor, MRAH, NormalLight, and ready-to-use
Material Layer instance together in the same category folder. Each instance is
parented to `/YogArt_Material/MaterialInstance/RVT_Ground/BaseMI/MLI_BasicMat_Base`
and overrides only `T BaseColor`, `T MRAH`, and `T Normal`, so artists can select
the processed set directly in an RVT ground Material Layers stack while keeping
the existing projection and height-blend controls.

The three runtime `TC_Env_*` Texture Collections and the CommonBrick_01 fallback
triplet intentionally remain under `/Game/Environment/CommonTexture`. Generated
Building/Prop materials reference those stable object paths; the collections in
turn reference the categorized artist textures. Moving the collections is a
separate atomic migration that requires regenerating both code materials.

Environment tiling texture packing is:

- BaseColor: RGB, sRGB on.
- MRAH: R=Metallic, G=Roughness, B=Ambient Occlusion, A=Height; sRGB off.
- NormalLight: RG=Tangent-space normal XY, B=MaterialLightMask, A=reserved;
  sRGB off and `TC_BC7`. Never use
  `TC_Normalmap`/BC5 because it discards the authored B channel. Normal Z is
  reconstructed from RG in HLSL.

The legacy packed R channel drove the removed Color Tint/variation path and is
discarded. During conversion the packed texture maps to MRAH as
`(Metallic=set metadata, Roughness=old G, AO=1, Height=old B)`. Metallic is zero
for the non-metal sets and one for CommonBrass_01. The old normal texture keeps
RG as tangent-space normal XY and B as MaterialLightMask; CommonBrass_01 receives
a repaired flat RG normal because its legacy normal was an invalid white
placeholder. The conversion report records channel ranges and flags uniform
BaseColor or repaired inputs for visual review.

Texture Collection indices are zero-based and aligned across BaseColor, MRAH,
and NormalLight. Runtime parameters and PerInstanceCustomData store `0..21`.
Indices `0..11` preserve the existing CommonBrick/WallBrick/TrimBrick order;
the additional sets are appended. If an art-facing UI displays `1..22`, convert
it to zero-based before writing CustomData; the shader does not apply an
implicit `-1`. Legacy Texture2DArray assets are not migration sources because
their recorded members are incomplete or misaligned.

HDRI and Matcap assets are not tiling PBR sets and never enter the three Texture
Collections. They are organized separately under `/Game/Art/Texture/HDRI` and
`/Game/Art/Texture/Matcap`.

Default static switches keep expensive optional paths off:

- `UseGroundRVT = false`
- `UseTextureCollection = true`

Current texture strategy:

- Ground RVT writer source textures stay in the NoVT path. UE does not allow virtual texture samples during Runtime Virtual Texture Output.
- Ordinary scene model, building, and prop textures default to NoVT Texture2D. Most project textures are 512 or 1K, so enabling VT globally is not useful.
- Dedicated VTC/VT/BakeInfo or special large textures must be reviewed separately before enabling VT.

## Recommended RVT Mesh Decal Rule

- The master material lives in this plugin.
- Common template instances can live in this plugin.
- Per-level art instances should live in `/Game/Art/Map/Map_Data/<LevelName>/LevelMaterial`.
- Generated RVT assets and bake outputs should stay in the level's `BakeInfo` folder.
