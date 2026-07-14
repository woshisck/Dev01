# UE5.8 风格化角色、灯光、Lumen 与自发光系统完整指南

> 状态：current
>
> 引擎：UE 5.8 源码版
>
> 引擎目录：`D:\Dev-BuildEngine`
>
> 项目目录：`D:\Self\GItGame\Dev01`
> 目标：在另一台 Windows 电脑上从源码获取、编译、生成资产并继续开发同一套角色渲染系统。

## 1. 最终目标与冻结规范

这套系统不再让每个角色材质采样一张 LightInfo 纹理并循环所有灯光。角色改用引擎 Deferred Lighting 的原生灯光循环，材质只提交表面数据和两个风格化控制量。

最终角色贴图契约如下，后续不得重新解释通道：

| 贴图 | 通道 | 含义 | 导入设置 |
| --- | --- | --- | --- |
| `T_Color` | RGB | 美术原始颜色 | sRGB 开启 |
| `T_Color` | A | 不使用 | 不接任何输出 |
| `T_Normal` | RG | 切线空间法线 XY，范围 `[0,1]` | sRGB 关闭，Compression=`Masks`，禁止 BC5 |
| `T_Normal` | B | `Diffuse Bias`，范围 `[0,1]` | Shader 内映射到 `[-1,1]` |
| `T_MixMap` | R | Metallic | sRGB 关闭，Compression=`Masks` |
| `T_MixMap` | G | Glossiness | Shader 使用 `Roughness = 1 - G` |
| `T_MixMap` | B | MatCapMask | 暂时保留但不参与计算 |
| `T_MixMap` | A | 不使用 | 保留 |

材质使用标准 `BaseColor / Metallic / Roughness / Normal`。不提供 Specular 贴图、参数或材质输入。非金属 F0 使用 UE 标准固定值，金属反射颜色来自 BaseColor。

`Stylized Character Lighting Output` 只保留：

1. `Diffuse Bias`
2. `Lighting Profile`

`GI Normal WS` 不需要美术提供。引擎自动保存平滑顶点法线，并按 Profile 的 `GINormalBlend` 与像素法线混合。

## 2. 当前实现状态

| 模块 | 状态 | 说明 |
| --- | --- | --- |
| `MSM_StylizedCharacterLit` | 已实现 | 非 Substrate 自定义 Shading Model |
| 三张角色贴图契约 | 已实现 | 主材质可由 Commandlet 重建 |
| Curve Atlas 七区 Ramp | 已实现 | 完整复刻旧材质的七区权重算法 |
| Look Profile 与体积切换 | 已实现 | 相机进入体积后混合到目标 Profile |
| 多光源模式 0/1/2 | 已实现 | Wash / Toon / Rim |
| 标准 Metallic/Roughness GGX | 已实现 | 直接高光与间接反射保留 PBR 质感 |
| GI 平滑法线 | 已实现 | 自动顶点法线，不增加贴图输入 |
| 反射 Kuwahara | 已实现 | 仅过滤角色间接反射，可动态/分级关闭 |
| 半程向量自阴影 | 已实现 | 半分辨率 HZB 屏幕空间方案 |
| 隐藏 Lumen 自发光面源 | 已实现 | 游戏中隐藏，仍参与动态间接光 |
| 场景投影与原生角色自投影完全分图 | 已实现 | CSM/逐物体阴影下使用 caster 级模板排除，详见第 10 节 |
| 代表场景 GPU/画质验收 | 待美术联调 | 需要真实角色、灯光和目标截图 |

## 3. 从源码获取 UE 5.8

前置条件：GitHub 账号已与 Epic Games 账号关联，并能访问私有仓库 `EpicGames/UnrealEngine`。安装 Visual Studio Build Tools、Desktop development with C++、Windows SDK 和 Git。

PowerShell：

```powershell
git clone --branch 5.8 --filter=blob:none https://github.com/EpicGames/UnrealEngine.git D:\Dev-BuildEngine
Set-Location D:\Dev-BuildEngine
git switch -c dev-buildengine-5.8
& .\Setup.bat
& .\GenerateProjectFiles.bat
& .\Engine\Build\BatchFiles\Build.bat UnrealEditor Win64 Development -WaitMutex
```

本机基线：

```text
Branch: dev-buildengine-5.8
Base commit: 6673776aad735f49a5ce3bbed474ffcc701e7a8e
```

不要把改造直接做在本地 `5.8` 分支上。保留干净基线，使用独立开发分支，方便未来 rebase Epic 的 5.8 更新。

## 4. 获取项目并编译

项目放在：

```text
D:\Self\GItGame\Dev01\DevKit.uproject
```

生成项目文件和编译：

```powershell
Set-Location D:\Dev-BuildEngine
& .\GenerateProjectFiles.bat D:\Self\GItGame\Dev01\DevKit.uproject -Game -Engine
& .\Engine\Build\BatchFiles\Build.bat DevKitEditor Win64 Development D:\Self\GItGame\Dev01\DevKit.uproject -WaitMutex -NoHotReloadFromIDE
```

编辑器：

```text
D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe
```

首次启动前确认项目启用了 `CelesLight` 插件。项目不依赖 Substrate，保持 Substrate 关闭即可。

## 5. 自动生成角色主材质与工具材质

项目的 `/Content/Art` 被 `.gitignore` 忽略，因此另一台电脑不能依赖提交后的 `.uasset`。使用 Commandlet 重建：

```powershell
& D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe `
  D:\Self\GItGame\Dev01\DevKit.uproject `
  -run=StylizedCharacterMaterialSetup -Apply `
  -unattended -nop4 -nosplash -NullRHI -NoSound
```

它生成或覆盖：

```text
/Game/Art/Material/CharacterMaterial/M_StylizedCharacterMaster
/Game/Art/Material/CharacterMaterial/Utility/T_StylizedCharacter_DefaultNormalBias
/Game/Art/Material/CharacterMaterial/Utility/T_StylizedCharacter_DefaultMixMap
/Game/Art/Material/LightingTools/M_StylizedHiddenEmissive
```

主材质图固定为：

```text
T_Color.RGB ------------------------------> Base Color
T_Normal.RG -> *2-1 -> DeriveNormalZ ----> Normal
T_Normal.B -------------------------------> Diffuse Bias
T_MixMap.R -------------------------------> Metallic
1 - T_MixMap.G ---------------------------> Roughness
T_MixMap.B -------------------------------> 保留节点，不接输出
Lighting Profile -------------------------> Lighting Profile
```

角色材质实例只配置三张贴图和 `Lighting Profile`。七区颜色、Ramp、环境色影响、曝光、对比度和反射强度统一放在 Profile 中，避免每个材质实例维护一套重复参数。

## 6. Shading Model 数据流

`MSM_StylizedCharacterLit` 使用 CustomData：

```text
CustomData.r  = Diffuse Bias [0,1]
CustomData.gb = 平滑顶点法线的 Octahedral 编码
CustomData.a  = Lighting Profile 0..7 的归一化编码
```

平滑顶点法线直接取材质像素参数中始终存在的 `TangentToWorld[2]`，并乘 `TwoSidedSign` 处理双面朝向。不要使用可选的 `WorldVertexNormal_Center`：该字段只在启用 Normal Curvature to Roughness 的 Shader permutation 中生成，普通角色材质会因字段缺失而编译失败。

Diffuse Bias 的唯一公式：

```hlsl
float DiffuseBias = saturate(TextureB) * 2.0 - 1.0;
float Attenuation = dot(N, L) + DiffuseBias;
float CurveTime = saturate(Attenuation * 0.5 + 0.5);
```

因此 `T_Normal.B = 0.5` 近似无偏移，0 向暗面移动，1 向亮面移动。它同时承担旧方案中 Self Shadow Mask/Diffuse Bias 的用途，不再增加第二张语义重复的贴图。

## 7. Curve Atlas 与七区 Ramp

默认资产：

```text
/Game/Art/Material/CharacterMaterial/ProxCurve/Ramp/CA_Ramp
/Game/Art/Material/CharacterMaterial/ProxCurve/Ramp/CC_Ramp_01
```

每个 Profile 选择 Atlas 中的一条 Curve。Shader 用 `CurveTime` 采样 RGBA，然后完整执行旧角色材质的七区算法：

1. Shadow Fade
2. Shadow
3. Shallow Fade
4. Shallow
5. SSS
6. Front
7. Forward

Profile 暴露七个 Tint 和 `ShadowFadePower`。不要在新主材质中再搭一套 SmoothStep 或二分节点，否则会出现两次分层、暗部变脏和材质指令膨胀。

## 8. Profile、Look Volume 与颜色还原

编辑入口：

```text
Project Settings > Plugins > Stylized Lighting
```

每个 `FStylizedCharacterLightingProfile` 包含：

| 分类 | 参数 |
| --- | --- |
| Base Light | 七区 Tint、Curve、ShadowFadePower、DirectDiffuseIntensity |
| Specular | SpecularTint、SpecularIntensity |
| Environment | Direct/Indirect/Reflection Color Influence、GINormalBlend、Indirect/Reflection Intensity |
| Color Fidelity | CharacterExposure、CharacterContrast |

色彩还原原则：

1. `T_Color` 是固有色，不在材质中做额外 Gamma。
2. 暗部颜色主要来自七区 Tint，不直接乘成黑色。
3. `DirectLightColorInfluence=0` 只接受灯光亮度，`1` 完整接受 RGB 光色。
4. 环境光和反射分别使用独立 Influence，避免蓝色室内把角色整体染脏。
5. Scene 仍使用原生 ACES；角色曝光和对比度只调整该 Shading Model 的光照响应。

关卡中放置 `StylizedCharacterLookVolume`：

| 属性 | 用途 |
| --- | --- |
| Profile Index | 进入体积后使用的 Profile |
| Priority | 重叠体积选择优先级 |
| Blend Radius | 边界混合距离 |
| Blend Weight | 最大混合权重 |
| Unbound | 全局生效 |

当前子系统根据主玩家相机选择体积，适合单主镜头项目。分屏、多本地玩家或 SceneCapture 需要改成 per-view 参数。

## 9. 多光源模式

所有原生 `ULightComponent` 都新增 `Light > Stylized Character` 参数，`CelesPointLight` 也有相同入口：

| Mode | 目标 | 阴影 | 高光 |
| --- | --- | --- | --- |
| 0 Wash | 纯亮度晕染 | 不使用 | 不使用 |
| 1 Toon | Curve Ramp 二分/七区光 | 使用可调原生阴影 | 标准 GGX |
| 2 Rim | 仅边缘光 | 不使用 | 不使用 |

Mode 1 的 `StylizedCharacterShadowStrength`：

```text
0 = 角色忽略该灯的原生 Shadow Mask
1 = 完整使用该灯的原生 Shadow Mask
```

Mode 2 使用 `RimPower` 和 `RimIntensity`，颜色取灯光颜色与 Profile 的 ForwardTint。不要通过额外 LightInfo 纹理重复计算灯光。

## 10. 阴影分层与当前边界

角色阴影分为四类：

1. `NdotL + Diffuse Bias + Curve Atlas`：亮暗面分区，不是几何投影。
2. 半程向量自阴影：使用 HZB 屏幕空间追踪，方向在 LightDir 与 ViewDir 间混合。
3. 原生场景投影：方向光、点光、场景物体遮挡产生的 Shadow Mask。
4. 角色投影到环境：角色仍是标准 shadow caster。

半程向量自阴影 CVar：

```text
r.StylizedCharacter.SelfShadow.Enable 1
r.StylizedCharacter.SelfShadow.HalfViewBlend 0.5
r.StylizedCharacter.SelfShadow.Strength 1
r.StylizedCharacter.SelfShadow.MaxTraceDistance 200
```

当前实现是半分辨率 HZB，随后双边模糊并上采样到角色像素。它不需要面部专用阴影，不增加角色贴图。

### 10.1 CSM/逐物体阴影严格分离

材质和 Deferred Lighting 只能读取最终 `Shadow.SurfaceShadow`，无法知道 caster 身份。因此严格分离实现在 Shadow Setup/Projection 阶段，而不是材质阶段。

引擎为 `UPrimitiveComponent` 新增：

```text
Shadow Other Objects Only
Set Cast Shadow On Other Objects Only
```

开启后的流程：

1. Scene Proxy 自动强制 Dynamic Inset Shadow。
2. 角色 caster 从整场 CSM 中排除，避免同一角色在 CSM 和逐物体阴影中重复出现。
3. 每盏投影灯为角色建立逐物体 Shadow Depth。
4. 投影时先用光锥写模板，再把角色当前可见像素的模板清零。
5. 逐物体阴影只投到地面、墙体和其他对象；角色自身不接收这份原生投影。
6. 角色仍接收整场 CSM 中的场景遮挡，角色自身层次由 half-view HZB pass 提供。

最终同时成立：

```text
角色接收场景物体投影
角色拒绝自己的原生投影
角色仍向场景投影
```

角色蓝图推荐添加组件：

```text
Stylized Character Shadow Policy
```

默认配置会自动处理 Actor 上全部 Skinned Mesh Component，打开 `Shadow Other Objects Only`，并关闭不能做 caster/receiver 分离的 Contact Shadow 和 Capsule Direct Shadow。模块化角色若使用 Child Actor，请在每个拥有骨骼组件的 Actor 上添加该组件；也可填写 `Explicit Target Components` 精确指定。

也可以不加策略组件，直接在 Skeletal Mesh Component 的 `Lighting > Advanced` 勾选 `Shadow Other Objects Only`。运行时蓝图可调用 `Set Cast Shadow On Other Objects Only`。

当前边界：

- 项目固定 `r.Shadow.Virtual.Enable=0`，本实现针对 SM5/SM6 的 CSM、Stationary/Movable Local Light 逐物体投影。
- 必须保持 `r.Shadow.PerObject=1` 和 Shadow Stencil Culling 可用；桌面 Deferred 默认满足。
- 每个角色、每盏实际投影的 Mode 1 灯会增加一份逐物体 Shadow Depth。补光使用 Mode 0、轮廓灯使用 Mode 2，避免无意义的阴影开销。
- Hair Strands 的 sub-pixel shadow mask 是独立路径；当前严格排除以不透明/Masked 骨骼网格为验收对象。
- Translucent shadow、Mobile Deferred 和 VSM 不属于本轮严格分离验收范围。

UE5.8 自带 First Person Shadow 可作为未来 VSM 路径：把可见角色标为 `FirstPerson`，再用一份 Leader Pose 骨骼代理标为 `WorldSpaceRepresentation`。VSM 会让可见角色接收普通场景阴影、跳过代理对 First Person 像素的投影，同时让代理把角色阴影投到环境。该路径依赖额外 First Person VSM Clipmap；如果以后 PC Epic 档开启 VSM，应改用并验证这条路径，不要假定当前 CSM 模板排除自动覆盖 VSM。

## 11. GI、Lumen 与反射

角色 GI 使用：

```hlsl
IndirectNormal = normalize(lerp(PixelNormal, SmoothVertexNormal, GINormalBlend));
```

`GINormalBlend=1` 可抹平法线贴图在间接光中的细碎立体感；直接光仍使用像素法线，金属和粗糙度质感不会被全部抹掉。

场景 Stylized Lumen 默认参数：

```text
r.StylizedLumenLighting.Enable 1
r.StylizedLumenLighting.DirectBlend 1
r.StylizedLumenLighting.IndirectBlend 0.35
r.StylizedLumenLighting.BandCount 5
r.StylizedLumenLighting.BandSoftness 0.18
r.StylizedLumenLighting.GlossInfluence 0.65
r.StylizedLumenLighting.SpecularIntensity 1.2
```

这些参数已经出现在 `Project Settings > Plugins > Stylized Lighting`，日常不需要手输 CVar。

Kuwahara 只处理 `MSM_StylizedCharacterLit` 的间接反射，不处理 BaseColor、直接高光或整个 SceneColor：

```text
r.StylizedReflection.Kuwahara.Enable -1  ; Auto
r.StylizedReflection.Kuwahara.Enable 0   ; Off
r.StylizedReflection.Kuwahara.Enable 1   ; On
r.StylizedReflection.Kuwahara.Strength 1
```

Auto 分级：Low/Medium/High 关闭，Epic 使用轻量核，Cinematic 使用更高质量核。

## 12. PBR 质感

Ramp 只替换漫反射的连续 `NdotL` 梯度，GGX 高光仍使用 UE 标准 BRDF：

```text
Metallic = T_MixMap.R
Roughness = 1 - T_MixMap.G
Specular input = 不暴露
```

这样布料、漆面和金属仍能通过 Metallic/Roughness 区分。Kuwahara 只降低间接反射的高频信息，不把直接高光也做成同一个色块。

## 13. 隐藏 Lumen 自发光工具

编辑器菜单：

```text
Tools > Celes Light > Create Stylized Emissive Light
```

`StylizedEmissiveLight` 包含一个自发光 StaticMesh 和一个默认关闭的 PointLight 代理。

自发光 StaticMesh 的关键状态：

```text
Hidden In Game = true
Affect Dynamic Indirect Lighting = true
Affect Indirect Lighting While Hidden = true
Emissive Light Source = true
Cast Shadow = false
Render In Main Pass = true
```

`Render In Main Pass` 必须保持 true，因为 Lumen Card Capture 会检查该标记；真正避免主画面显示的是 `Hidden In Game`。`Affect Indirect Lighting While Hidden` 保证隐藏后 Scene Proxy 仍为间接光保留。

艺术参数：

| 参数 | 说明 |
| --- | --- |
| SourceMesh | 自发光面源形状，默认 Engine Sphere |
| EmissiveMaterial | 默认 `M_StylizedHiddenEmissive` |
| LightColor | 发光颜色 |
| EmissiveIntensity | Lumen HDR 强度 |
| UsePointLightProxy | 可选解析点光，默认 false |
| Intensity/Radius/Falloff | 只在 PointLight Proxy 开启时使用 |

纯自发光很小、很亮时可能噪点或闪烁。优先放大 SourceMesh 面积再降低强度；需要稳定主照明时才打开 PointLight Proxy。

## 14. 主要源码位置

引擎侧：

```text
Engine/Source/Runtime/Engine/Classes/Engine/EngineTypes.h
Engine/Source/Runtime/Engine/Public/SceneTypes.h
Engine/Source/Runtime/Engine/Public/Materials/MaterialExpressionStylizedCharacterLightingOutput.h
Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp
Engine/Source/Runtime/Engine/Private/Materials/HLSLMaterialTranslator.cpp
Engine/Source/Runtime/Engine/Classes/Components/LightComponent.h
Engine/Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h
Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h
Engine/Source/Runtime/Renderer/Public/StylizedCharacterLighting.h
Engine/Source/Runtime/Renderer/Private/LightRendering.cpp
Engine/Source/Runtime/Renderer/Private/IndirectLightRendering.cpp
Engine/Source/Runtime/Renderer/Private/ShadowSetup.cpp
Engine/Source/Runtime/Renderer/Private/ShadowRendering.cpp
Engine/Source/Runtime/Renderer/Private/Shadows/FirstPersonSelfShadow.cpp
Engine/Shaders/Private/StylizedCharacterLighting.ush
Engine/Shaders/Private/ShadingModelsMaterial.ush
Engine/Shaders/Private/ShadingModels.ush
Engine/Shaders/Private/DeferredLightingCommon.ush
Engine/Shaders/Private/DiffuseIndirectComposite.usf
Engine/Shaders/Private/FirstPersonSelfShadow.usf
Engine/Shaders/Private/StylizedLumenLighting.ush
Engine/Shaders/Private/Lumen/LumenSceneDirectLighting.usf
```

项目和插件侧：

```text
Plugins/CelesLight/Source/CelesLightRuntime/Public/StylizedLightingSettings.h
Plugins/CelesLight/Source/CelesLightRuntime/Private/StylizedLightingSettings.cpp
Plugins/CelesLight/Source/CelesLightRuntime/Public/Actors/CelesPointLight.h
Plugins/CelesLight/Source/CelesLightRuntime/Public/Actors/StylizedCharacterLookVolume.h
Plugins/CelesLight/Source/CelesLightRuntime/Public/Actors/StylizedEmissiveLight.h
Plugins/CelesLight/Source/CelesLightRuntime/Public/Components/StylizedCharacterShadowPolicyComponent.h
Plugins/CelesLight/Source/CelesLightRuntime/Public/Systems/StylizedCharacterLookSubsystem.h
Plugins/CelesLight/Source/CelesLightEditor/Public/CelesLightEditorLibrary.h
Source/DevKitEditor/MaterialBatch/StylizedCharacterMaterialSetupCommandlet.cpp
Config/DefaultGame.ini
Config/DefaultEngine.ini
```

## 15. 美术使用流程

1. 运行 Commandlet 生成主材质。
2. 从 `M_StylizedCharacterMaster` 创建材质实例。
3. 填写 `T_Color`、`T_Normal`、`T_MixMap`。
4. `T_Normal` 和 `T_MixMap` 关闭 sRGB，Compression 设为 Masks。
5. 设置材质实例的 `Lighting Profile`，默认 0。
6. 在 Project Settings 配置 Profile 0 的 Ramp 和七区 Tint。
7. 场景主灯设 Mode 1；补光设 Mode 0；轮廓灯设 Mode 2。
8. 在角色蓝图添加 `Stylized Character Shadow Policy`，默认自动处理全部骨骼网格。
9. 需要区域色调变化时放置 `StylizedCharacterLookVolume`。
10. 需要只照亮周围、不显示发光模型时放置 `StylizedEmissiveLight`。
11. 先调 Ramp 和 Tint，再调灯光颜色影响，最后调 Exposure/Contrast，避免多个参数同时补偿。

## 16. 验收矩阵

### 16.1 通道验收

| 操作 | 预期 |
| --- | --- |
| 修改 `T_Color.A` | 画面不变 |
| `T_MixMap.R` 从 0 到 1 | 非金属变金属 |
| `T_MixMap.G` 从 0 到 1 | 粗糙变光滑 |
| 修改 `T_MixMap.B` | 当前画面不变 |
| `T_Normal.B=0.5` | Diffuse Bias 近似中性 |
| `T_Normal.B=0/1` | Ramp 边界向相反方向移动 |

### 16.2 光照验收

| 场景 | 预期 |
| --- | --- |
| Mode 0 彩色补光 | 只按亮度晕染，不出现阴影和高光 |
| Mode 1 主灯 | 出现 Curve 分层、场景阴影和 GGX 高光 |
| Mode 2 轮廓灯 | 只增加视角边缘亮度 |
| DirectColorInfluence=0 | 彩灯不强烈染角色固有色 |
| DirectColorInfluence=1 | 角色完整接受灯光 RGB |
| GINormalBlend=1 | 间接光表面更平，直接高光仍有法线细节 |
| 角色站在场景物体阴影中 | 角色接收场景投影 |
| 角色自身遮挡自身 | 不出现原生自投影，改由 half-view 自阴影控制 |
| 角色站在地面上 | 地面仍出现角色投影 |

### 16.3 性能验收

在同一机位记录：

```text
stat gpu
profilegpu
viewmode shadercomplexity
```

对比四组：基础场景、角色系统、角色系统+Kuwahara、角色系统+多个隐藏自发光面源。记录分辨率、画质档、GPU 和灯光数量，不只记录 FPS。

## 17. 编译与 Shader 验证

增量 C++ 编译：

```powershell
& D:\Dev-BuildEngine\Engine\Build\BatchFiles\Build.bat `
  DevKitEditor Win64 Development `
  D:\Self\GItGame\Dev01\DevKit.uproject `
  -WaitMutex -NoHotReloadFromIDE
```

编辑器中执行：

```text
RecompileShaders Changed
```

命令行自动验证可以使用逗号分隔 ExecCmds：

```powershell
& D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe `
  D:\Self\GItGame\Dev01\DevKit.uproject `
  -log -nosplash -NoSound `
  '-ExecCmds=RecompileShaders Changed,Quit'
```

如果 Shader 报错，先修第一条真实 HLSL 错误。后面的 `Internal Error` 常是首条错误引发的级联，不要逐条处理。

## 18. 参考资料

- [角色渲染参考 1](https://zhuanlan.zhihu.com/p/20592939601)
- [色彩还原参考](https://zhuanlan.zhihu.com/p/2007870047671035236)
- [高光与 PBR 参考 1](https://zhuanlan.zhihu.com/p/2008887309844640748)
- [高光与 PBR 参考 2](https://zhuanlan.zhihu.com/p/2012135042818778255)

这些文章用于视觉方向参考。项目中的唯一实现真源是本文件的冻结通道规范、引擎源码和 `StylizedLightingSettings`。

## 19. 最终验证与跨机器复现包

本机最终验证环境：

```text
Engine root: D:\Dev-BuildEngine
Engine branch: dev-buildengine-5.8
Engine base commit: 6673776aad735f49a5ce3bbed474ffcc701e7a8e
Project root: D:\Self\GItGame\Dev01
Project branch: agent/stylized-character-rendering-system
Platform: Win64 Development Editor / D3D12 / SM6
```

验证结果：

1. 完整 `DevKitEditor` 构建：`2957/2957`，`Result: Succeeded`。
2. 最终 Renderer 增量构建：`6/6`，`Result: Succeeded`。
3. D3D12 冷启动执行 `RecompileShaders Changed`：`No Shader changes found`，无全局或材质 Shader 错误。
4. `StylizedCharacterMaterialSetup -Apply`：`Success - 0 error(s), 4 warning(s)`；四条均为项目既有插件/配置/Python 名称警告。
5. `git diff --check`：Engine 与项目相关文件均通过。

独立复现包位于：

```text
Docs/ReproductionPackages/UE58_StylizedCharacterRendering_2026-07-13
```

包内包含按原目录保存的 Engine 与 Project 文件、SHA-256 清单、安装/备份脚本、构建验证脚本、本完整指南副本以及验证报告。另一台编译机可以克隆 Dev01 的目标分支后只向 UE 源码树安装 Engine 文件，也可以把该目录单独下载后同时安装 Engine 与 Project 文件。

不要把整个本机 Engine 工作区复制过去。本机工作区还存在 Setup/构建工具造成的第三方脚本换行变化和 `uatobj/uatbin` 产物，它们与本系统无关，复现包已明确排除。
