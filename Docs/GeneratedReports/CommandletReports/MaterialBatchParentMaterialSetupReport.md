# Material Batch Parent Material Setup

- Target: `/Game/Art/Material/EnvMaterial/Main/M_Env_Building_Batch.M_Env_Building_Batch`
- Apply: Yes
- Force: Yes
- Contract: `TexCoord7.x -> batchMaterialIndex -> _PropTexture row -> Texture2DArray slice`.
- Created `/Game/Art/Material/EnvMaterial/Main/M_Env_Building_Batch.M_Env_Building_Batch`.
- Saved dirty material packages.

## Contract Evidence

| Evidence | Present |
| --- | --- |
| TextureCoordinate index 7 | Yes |
| Texture object parameter `T_Array_A` | Yes |
| Texture object parameter `T_Array_N` | Yes |
| Texture object parameter `T_Array_M` | Yes |
| Texture object parameter `_PropTexture` | Yes |
| Scalar parameter `BatchRowCount` | Yes |
| Scalar parameter `PropertyColumnCount` | Yes |
| Custom HLSL `Texture2DArraySample` | Yes |
| Custom HLSL `_PropTexture` sample | Yes |