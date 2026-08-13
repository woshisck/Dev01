# Dev01 角色 Substance Painter 工具

这套工具对应当前 `MSM_StylizedCharacterLit` 贴图契约。Painter 负责贴图制作和通道验收，不承载 Unreal 的 Lighting Profile、曝光、对比度或最终场景后处理。

## 中性预览原则

`Dev01_StylizedCharacter.glsl` 不采样 Painter 自带环境球。Painter 环境图、环境旋转和环境曝光不会改变角色制作用光。

Shader Settings 只开放以下项目参数：

- `Key azimuth`：水平旋转主光方向。
- `Key elevation`：调整主光高度。
- `Viewport output`：检查 Base Color、打包通道、二分区域和高光余量。

方向参数只旋转同一盏固定能量的中性白光，不改变灯光颜色、色温、强度和曝光。预览不加入冷暖色环境，因此 Base Color 不会被校准灯染成青色或暖色。

材质响应遵循“写实质感、风格化光影”：

- Base Color、Metallic、Roughness、介电/金属 F0 和 GGX 使用写实 PBR 逻辑。
- 风格化只作用于直接光的半兰伯特明暗二分；二分是在同一照明预算内重分配，不额外叠加亮度。
- 漫反射使用 `BaseColor / PI` 并扣除镜面占用的能量；金属不再保留不合理的漫反射。
- 固定中性半球光仅用于看清暗面材质，不是可调的角色最低亮度参数。
- Painter 7.4 兼容预览不采样或开放独立 AO；Unreal 中的间接遮蔽仍只保留 35% 影响，用于接缝和接触细节，不允许整块背光面被压成黑色。
- 非金属暗部保留中性漫反射信息；金属暗部不增加伪漫反射，只依靠中性环境反射显示颜色和粗糙度。
- 高光使用有限面积光近似，避免小粗糙度出现针状过曝。
- 不叠加项目的 `CharacterExposure=1` 和 `CharacterContrast=1.5`；这些属于 Unreal 最终画面，不属于贴图制作用光。

这个预览用于稳定判断 Base Color、明暗分区、粗糙度和高光控制，不是 Unreal 的逐像素 Lumen、多灯、阴影或后处理替代。最终效果仍必须在 Unreal 代表场景中验收。

## 唯一贴图契约

| 输出 | 通道 | Painter 来源 | UE 导入 |
| --- | --- | --- | --- |
| `T_<TextureSet>_Color.png` | RGB | Base Color | sRGB 开；Compression Default |
| `T_<TextureSet>_Normal.png` | RG | DirectX Normal XY | sRGB 关；Compression Masks；禁止 BC5/Normalmap |
| `T_<TextureSet>_Normal.png` | B | User0：`Diffuse Bias` | 0.5 中性；0 偏暗；1 偏亮 |
| `T_<TextureSet>_MixMap.png` | R | Specular Level | 有符号 GGX 控制；0.5 中性 |
| `T_<TextureSet>_MixMap.png` | G | Roughness | 直接使用，不反相 |
| `T_<TextureSet>_MixMap.png` | B | Metallic | 0 非金属；1 金属 |

三张输出均不使用 Alpha。当前主材质没有独立 AO、Emissive、Opacity、MatCap 或 Face SDF 输入；不要沿用旧 `Dev01_Export.spexp` 的 MRA/Displacement 语义。

## 安装

关闭 Painter 后，右键或在 PowerShell 中运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Install-Dev01CharacterTools.ps1
```

默认安装到：

```text
%USERPROFILE%\Documents\Adobe\Adobe Substance 3D Painter\assets\shaders\Dev01_StylizedCharacter.glsl
%USERPROFILE%\Documents\Adobe\Adobe Substance 3D Painter\python\plugins\dev01_character_tools.py
```

重启 Painter，在 `Python > dev01_character_tools` 启用一次插件。之后每个项目执行：

1. 在 `Shader Settings` 选择 `Dev01_StylizedCharacter`。
2. 运行 `Dev01 Character > Apply Neutral Character Preview Display`，统一使用 ACES 显示映射。
3. 运行 `Prepare Character Channels` 补齐缺失通道。
4. 制作 Base Color、Normal、Roughness、Metallic、Specular Level、Diffuse Bias。
5. 用 `Key azimuth` / `Key elevation` 从正面、侧面和背面检查材质；它们只改变方向。用 `Viewport output` 检查各通道；`Linear Highlight Headroom` 中绿色表示未超过线性 1，红色表示局部 HDR 高光超过 1。局部红点可以由 ACES 压缩，大面积红色需要检查 Base Color、Roughness 和 Specular Control。
6. 运行 `Validate 3-Map Export`，再运行 `Export T_Color / T_Normal / T_MixMap`。
7. UE 中把 `T_Normal` 和 `T_MixMap` 设为非 sRGB Masks；`T_Normal` 不能使用 BC5，否则会丢失 B 通道的 Diffuse Bias。

未绘制的 Specular Level 和 Diffuse Bias 按中性 0.5 处理。正式资产仍建议在底层 Fill Layer 显式填入 0.5，方便交接检查。

### 整体紫色的处理

整体紫色表示 Painter 的自定义 GLSL Shader 编译失败，和贴图、模型或通道无关。当前截图中的 Painter 是 7.4.3；请使用本目录的兼容版工具包（支持 Painter 7.4+），不要继续使用先前仅在 Painter 9.x 验证的旧 `.glsl`。

1. 完全关闭 Painter。
2. 用新版工具包再次运行 `Install-Dev01CharacterTools.ps1`，允许覆盖旧文件。
3. 重启 Painter，在 `Shader Settings` 重新选择 `Dev01_StylizedCharacter`。
4. 如果仍是紫色，打开 `Window > Views > Log`，把第一条 `Shader` / `GLSL` 编译错误发给维护者；它会直接指出无法兼容的函数或库名。

本版不依赖 7.4.3 缺失的 `lib-bent-normal.glsl` 或新版 `lib-pbr.glsl` 辅助函数；预览保持 Base Color、法线、Diffuse Bias 二分和 MixMap 高光控制，但不把 Painter 环境光当作 Unreal 的 Lumen。

为避免依赖 Painter 7.4 不同补丁版本中的全局相机参数和辅助库组合，兼容版以稳定的物体空间视线近似镜面视线。这只会使预览中的高光落点与 Painter 新版略有差异，不影响三图打包、导出或 Unreal 内的最终效果。

## 给同事分发

维护者运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Package-Dev01CharacterTools.ps1
```

生成 `Dist\Dev01CharacterPainterTools.zip`。同事解压后只需运行压缩包内的安装脚本，不需要引擎源码、P4 Engine workspace 或本地编译。

更新工具时，重新生成压缩包并覆盖团队共享位置。建议在 UGS/P4 项目侧只分发本目录与压缩包；不要把 `X:\Dev-BuildEngine` 的完整引擎源码带入项目 GitHub 或工具包。

## Unreal 边界

以下内容只能在 Unreal 代表场景中最终确认：

- 场景投影、自投影、局部多灯和 Lumen 间接光；
- Box Reflection Capture、SSR/Lumen Reflection 与场景雾；
- Lighting Profile / Look Volume、角色曝光、对比度、ACES 和项目后处理；
- Outline、头发、眼睛、透明材质和特效材质。

源码依据：

- `X:\Project\Dev01\Source\DevKitEditor\MaterialBatch\StylizedCharacterMaterialSetupCommandlet.cpp`
- `X:\Dev-BuildEngine\Engine\Shaders\Private\StylizedCharacterLighting.ush`
- `X:\Dev-BuildEngine\Engine\Shaders\Private\ShadingModels.ush`
- `X:\Dev-BuildEngine\Engine\Shaders\Private\DeferredLightingCommon.ush`
- `X:\Project\Dev01\Config\DefaultGame.ini`
