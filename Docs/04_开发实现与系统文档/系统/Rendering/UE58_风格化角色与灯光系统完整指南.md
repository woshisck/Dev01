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
| `T_MixMap` | R | SpecMask | sRGB 关闭，Compression=`Masks`；白色允许高光，黑色关闭高光 |
| `T_MixMap` | G | Glossiness | Shader 使用 `Roughness = 1 - G` |
| `T_MixMap` | B | MatCapMask | 暂时保留但不参与计算 |
| `T_MixMap` | A | 不使用 | 保留 |

旧主材质的 R 通道本质是 `SpecMask`，不是金属度。新材质使用标准 `BaseColor / Roughness / Normal / Specular` 数据路径：`Specular = saturate(T_MixMap.R * SpecIntensity)`。`Metallic` 改为独立标量且默认 0，只有确实采用金属工作流的材质实例才开启。高光颜色和 Profile 总强度由 `SpecularTint / SpecularIntensity` 控制。

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
| SpecMask/Roughness GGX | 已实现 | R 通道遮罩直接高光，G 通道控制粗糙度；颜色和总强度由 Profile 控制 |
| 角色间接光遮蔽控制 | 已实现 | 独立调节 SSAO/Lumen short-range occlusion，不影响投影 |
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

`CelesLightRuntime` 会在模块加载时先应用 CVar，并在 `OnPostEngineInit` 再次上传角色 Lighting Profile 与 Ramp Atlas。第二次上传发生在渲染资源初始化之后，因此编辑器、PIE、Standalone 和打包游戏都不需要通过打开 Project Settings 手工刷新效果。若修改了 Runtime 插件源码，必须关闭编辑器后重新构建，才能替换被编辑器占用的插件 DLL。

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
T_MixMap.R -> * SpecIntensity -> Saturate -> Specular
1 - T_MixMap.G ---------------------------> Roughness
T_MixMap.B -------------------------------> 保留节点，不接输出
Metallic (Scalar，默认 0) ----------------> Metallic
Lighting Profile -------------------------> Lighting Profile
```

角色材质实例只配置三张贴图和 `Lighting Profile`。七区颜色、Ramp、环境色影响、曝光、对比度和反射强度统一放在 Profile 中，避免每个材质实例维护一套重复参数。材质实例中的 `Lighting Profile=0..7` 选择的是一整套配置，不是七区中的某一个颜色；每套 Profile 都包含完整七区 Tint、Ramp 和颜色还原参数。

## 6. Shading Model 数据流

`MSM_StylizedCharacterLit` 使用 CustomData：

```text
CustomData.r  = Diffuse Bias [0,1]
CustomData.gb = 平滑顶点法线的 Octahedral 编码
CustomData.a  = Lighting Profile 0..7 的归一化编码
```

这些是 Shading Model 写入 GBuffer 的内部数据，不是美术需要搜索或手工连接的 `CustomData` 节点。材质只需要把 `T_Normal.B` 接到 `Stylized Character Lighting Output.Diffuse Bias`，把一个 `0..7` 的标量参数接到 `Lighting Profile`；`CustomData.gb` 由引擎自动生成。

平滑顶点法线直接取材质像素参数中始终存在的 `TangentToWorld[2]`，并乘 `TwoSidedSign` 处理双面朝向。不要使用可选的 `WorldVertexNormal_Center`：该字段只在启用 Normal Curvature to Roughness 的 Shader permutation 中生成，普通角色材质会因字段缺失而编译失败。

Diffuse Bias 的唯一公式：

```hlsl
float DiffuseBias = saturate(TextureB) * 2.0 - 1.0;
float BaseAttenuation = dot(N, L) + DiffuseBias;
float CurveTime = saturate((BaseAttenuation + 1.0) * 0.5);
```

第二行对应原材质中的 `BaseAttenuation`，第三行严格对应 `ConstantBiasScale(Bias=1, Scale=0.5)`。因此 `T_Normal.B = 0.5` 近似无偏移，0 向暗面移动，1 向亮面移动。它同时承担旧方案中 Self Shadow Mask/Diffuse Bias 的用途，不再增加第二张语义重复的贴图。局部灯光的距离衰减、Radius/Falloff、Light Function 和原生 Shadow Mask 仍由 UE 原生路径叠加，未被 Ramp 替换。

## 7. Curve Atlas 与七区 Ramp

默认资产：

```text
/Game/Art/Material/CharacterMaterial/ProxCurve/Ramp/CA_Ramp
/Game/Art/Material/CharacterMaterial/ProxCurve/Ramp/CC_Ramp_01
```

每个 Profile 选择 Atlas 中的一条 Curve，并保存完整的七区配色。Shader 用 `CurveTime` 采样 RGBA，然后完整执行旧角色材质的七区算法：

1. Shadow Fade
2. Shadow
3. Shallow Fade
4. Shallow
5. SSS
6. Front
7. Forward

Profile 暴露七个 Tint 和 `ShadowFadePower`。`ShadowFadePower=0` 与原 `M_MasterCharacterMAT` 的默认参数一致；提高它会收紧 Shadow Fade 区域，但不改变 `BaseAttenuation` 公式。不要在新主材质中再搭一套 SmoothStep 或二分节点，否则会出现两次分层、暗部变脏和材质指令膨胀。

Profile 索引规则：

1. `Lighting Profile=0` 选择数组第 0 项，`1` 选择第 1 项，以此类推。
2. 只有 Mode 1 Toon 灯执行整套七区 Ramp；Mode 0 Wash 和 Mode 2 Rim 会绕过七区计算。
3. 多盏 Mode 1 灯会各自计算同一角色当前选中的完整 Profile。
4. 索引超过当前已配置数量时会限制到最后一个有效 Profile；要使用 `0..7`，必须在 Project Settings 中建立对应数组项。
5. `StylizedCharacterLookVolume` 覆盖和混合的是整套 Profile，不是单个七区颜色。

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
| Environment | Direct/Indirect/Reflection Color Influence、GINormalBlend、Indirect/Reflection Intensity、IndirectOcclusionStrength、CharacterBaseFill |
| Color Fidelity | CharacterExposure、CharacterContrast |

`Character Lighting > Global Multipliers` 中的三个顶层参数是全局主倍率，最终值为：

```text
最终 Direct RGB 影响    = Direct Light Color Influence Master    * Profile.DirectLightColorInfluence
最终 Indirect RGB 影响  = Indirect Light Color Influence Master  * Profile.IndirectLightColorInfluence
最终 Reflection RGB 影响= Reflection Color Influence Master      * Profile.ReflectionColorInfluence
```

三个 Master 默认均为 `1`，表示完全保留 Profile 的设置；设为 `0` 会让对应光照只保留亮度信息。若希望角色完整接受直接灯光 RGB，需要同时将 Direct Master 和当前 Profile 的 `DirectLightColorInfluence` 设为 `1`。

`CharacterExposure` 以曝光档为单位，只作用于该 Shading Model 的直接光、间接光和反射：`+1` 为两倍、`+0.5` 约为 1.414 倍、`-1` 为一半。它不是 Unlit 底色，在完全没有直接光和间接光时不会凭空产生亮度。

色彩还原原则：

1. `T_Color` 是固有色，不在材质中做额外 Gamma。
2. 暗部颜色主要来自七区 Tint，不直接乘成黑色。
3. `DirectLightColorInfluence=0` 只接受灯光亮度，`1` 完整接受 RGB 光色。
4. 环境光和反射分别使用独立 Influence，避免蓝色室内把角色整体染脏。
5. Scene 仍使用原生 ACES；角色曝光和对比度只调整该 Shading Model 的光照响应。
6. `IndirectOcclusionStrength` 控制细节光照中常见的接缝和凹陷压暗：`1` 为 UE 原生遮蔽，`0` 完全移除角色 SSAO/Lumen 短距离遮蔽。它不影响角色接收场景投影，也不影响角色向地面投影。
7. `CharacterBaseFill` 在 Lumen/AO 合成后直接使用 `GBuffer.BaseColor` 提供最低亮度：`0` 完全关闭，`0.2` 是推荐起点，`0.5` 明显接近 Unlit 保色。它不依赖 `DiffuseColor`，因此不会被 Metallic 压到零；同时不写入 Emissive、不产生 Bloom，也不会照亮环境。

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

Half-View HZB 是独立的角色自阴影层，不属于原生场景 Shadow Map。当前默认关闭，避免它与 `Shadow Other Objects Only` 叠加后重新在角色表面产生头发、披风等部件的自投影：

```text
r.StylizedCharacter.SelfShadow.Enable 0
r.StylizedCharacter.SelfShadow.HalfViewBlend 0.5
r.StylizedCharacter.SelfShadow.Strength 1
r.StylizedCharacter.SelfShadow.MaxTraceDistance 200
```

需要恢复风格化自阴影时，可在 `Project Settings > Plugins > Stylized Lighting > Character Self Shadow` 打开 `Enable Character Half-View Self Shadow`。该实现是半分辨率 HZB，随后双边模糊并上采样到角色像素；关闭时不会调度这套角色自阴影 Pass。

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

项目角色 C++ 基类 `AYogCharacterBase` 已自动创建组件：

```text
Stylized Character Shadow Policy
```

玩家和敌人蓝图继承 `AYogCharacterBase` 后会直接获得该组件，不需要在每个角色蓝图中手工添加。默认配置会自动处理 Actor 上全部 Skinned Mesh Component，打开 `Shadow Other Objects Only`，并关闭不能做 caster/receiver 分离的 Contact Shadow 和 Capsule Direct Shadow。蓝图可以展开继承组件查看或覆盖参数。

C++ 接入位置：

```cpp
StylizedCharacterShadowPolicyComponent =
    CreateDefaultSubobject<UStylizedCharacterShadowPolicyComponent>(
        TEXT("StylizedCharacterShadowPolicyComponent"));
```

模块化角色若使用独立 Child Actor，且该 Child Actor 不继承 `AYogCharacterBase`，仍需在 Child Actor 类中创建同类组件；也可填写 `Explicit Target Components` 精确指定。

也可以不加策略组件，直接在 Skeletal Mesh Component 的 `Lighting > Advanced` 勾选 `Shadow Other Objects Only`。运行时蓝图可调用 `Set Cast Shadow On Other Objects Only`。

该策略组件不执行 Tick，也不会复制骨骼或增加一次蒙皮；额外成本来自它强制创建的逐物体 Inset Shadow 和投影模板处理。暂时不需要严格区分原生自投影时，可以移除策略组件并关闭 `Shadow Other Objects Only`，角色会恢复原生自阴影。若同时不需要 half-view 自阴影，应在 Project Settings 关闭 `Enable Character Half View Self Shadow`，避免两套自阴影叠加；保持 `Cast Shadow=true` 仍会向环境投影，关闭它则成本最低但角色不再投影。

### 10.2 完全取消角色对自身的投影

要得到“角色接收场景投影、角色不接收角色自身投影、角色仍向环境投影”的组合，必须同时满足：

1. Project Settings 中关闭 `Enable Character Half-View Self Shadow`。这会移除独立的 HZB 角色自阴影。
2. 角色继承 `AYogCharacterBase`，使用基类自动提供的 `Stylized Character Shadow Policy`，保持 `Environment Only Native Shadow=true`。
3. 角色 Skeletal Mesh 保持 `Cast Shadow=true`，并由策略组件启用 `Shadow Other Objects Only`。
4. 不继承 `AYogCharacterBase` 的模块化 Child Actor 需要自己的策略组件，或由父级策略的 `Explicit Target Components` 明确覆盖；否则漏掉的头发、披风、配件仍会写入原生 Shadow Map 并投到角色上。
5. Contact Shadow 与 Capsule Direct Shadow 保持关闭，因为它们没有 caster/receiver 身份分离能力。

当前实现按整个骨骼网格组件排除自投影，不读取 UV。参考项目的 `UV0.y < 0` 是进一步的局部 caster/receiver Mask 方案，适合只屏蔽刘海或面部；它需要额外的 Shadow Pass 顶点约定，并非“完全关闭角色自阴影”的必要成本，因此暂不加入当前默认路径。

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
Specular = saturate(T_MixMap.R * SpecIntensity)
Metallic = 独立材质实例标量，默认 0
Roughness = 1 - T_MixMap.G
SpecularTint / Profile SpecularIntensity = 当前 Lighting Profile
```

这样布料、皮肤和漆面可以复用旧贴图的 SpecMask，同时仍由 Roughness 保留不同质感。真正金属需要显式设置独立 Metallic 参数或后续增加专用金属遮罩。Kuwahara 只降低间接反射的高频信息，不把直接高光也做成同一个色块。

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
Engine/Source/Runtime/Renderer/Private/LightRendering.h
Engine/Source/Runtime/Renderer/Private/IndirectLightRendering.cpp
Engine/Source/Runtime/Renderer/Private/Lumen/LumenSceneDirectLighting.cpp
Engine/Source/Runtime/Renderer/Private/HeterogeneousVolumes/HeterogeneousVolumesHardwareRayTracing.cpp
Engine/Source/Runtime/Renderer/Private/HeterogeneousVolumes/HeterogeneousVolumesLiveShadingPipeline.cpp
Engine/Source/Runtime/Renderer/Private/HeterogeneousVolumes/HeterogeneousVolumesPreshadingPipeline.cpp
Engine/Source/Runtime/Renderer/Private/HeterogeneousVolumes/HeterogeneousVolumesVoxelGridPipeline.cpp
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
Plugins/CelesLight/Source/CelesLightEditor/Private/CelesLightEditorModule.cpp
Plugins/CelesLight/Source/CelesLightEditor/Private/StylizedLightingSettingsDetails.h
Plugins/CelesLight/Source/CelesLightEditor/Private/StylizedLightingSettingsDetails.cpp
Source/DevKitEditor/MaterialBatch/StylizedCharacterMaterialSetupCommandlet.cpp
Build/Verification/VerifyStylizedPointLight.py
Config/DefaultGame.ini
Config/DefaultEngine.ini
```

## 15. 美术使用流程

1. 运行 Commandlet 生成主材质。
2. 从 `M_StylizedCharacterMaster` 创建材质实例。
3. 填写 `T_Color`、`T_Normal`、`T_MixMap`。
4. `T_Normal` 和 `T_MixMap` 关闭 sRGB，Compression 设为 Masks。
5. 设置材质实例的 `Lighting Profile`，默认 0；该数值选择一整套 Profile。
6. 在 Project Settings 配置对应 Profile 的 Ramp、七区 Tint、环境光影响和 Character Exposure；`ShadowFadePower=0` 是原材质基准。
7. 场景主灯设 Mode 1；补光设 Mode 0；轮廓灯设 Mode 2。
8. 确认角色继承 `AYogCharacterBase`。基类已经创建 `Stylized Character Shadow Policy`，默认自动处理该 Actor 的全部骨骼网格。
9. 需要区域色调变化时放置 `StylizedCharacterLookVolume`。
10. 需要只照亮周围、不显示发光模型时放置 `StylizedEmissiveLight`。
11. 先调 Ramp 和 Tint，再调灯光颜色影响，最后调 Exposure/Contrast，避免多个参数同时补偿。

## 16. 验收矩阵

### 16.1 通道验收

| 操作 | 预期 |
| --- | --- |
| 修改 `T_Color.A` | 画面不变 |
| `T_MixMap.R` 从 0 到 1 | 直接与间接高光从关闭变为允许；底色不会因此变成金属 |
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
| Direct Master 或 Profile DirectColorInfluence=0 | 彩灯只提供亮度，不染角色固有色 |
| Direct Master=1 且 Profile DirectColorInfluence=1 | 角色完整接受灯光 RGB |
| CharacterExposure=1 | 角色直接光、间接光和反射约变为两倍 |
| GINormalBlend=1 | 间接光表面更平，直接高光仍有法线细节 |
| IndirectOcclusionStrength=0/1 | 0 去除角色局部 SSAO/Lumen 自遮蔽压暗；1 恢复 UE 原生遮蔽 |
| CharacterBaseFill=0/0.2/0.5 | 0 使用纯场景光照；0.2 保证暗场景可读性；0.5 用于验证接近 Unlit 的底色保真 |
| 角色站在场景物体阴影中 | 角色接收场景投影 |
| 角色自身遮挡自身 | 不出现原生自投影；如需额外风格化自阴影，再显式开启 half-view 自阴影 |
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

### 17.1 Stylized Lighting 中英文界面

打开路径：

```text
Edit > Project Settings > Plugins > Stylized Lighting / 风格化灯光
```

页面顶部的 `Interface Language / 界面语言` 有三种模式：

| 模式 | 行为 |
| --- | --- |
| Auto / 自动 | 跟随 Unreal Editor 当前语言 |
| English | 强制显示英文参数名、分类和说明 |
| 简体中文 | 强制显示中文参数名、分类和说明 |

切换后当前 Details 页面立即刷新。所有顶层设置和 `Character Lighting Profiles` 数组内部参数均有本地化名称与悬停说明；`Reflection Kuwahara Mode` 的枚举选项也会随界面语言切换。语言只影响编辑器显示，不会改变 CVar、Shader 数据或运行时画面。

### 17.2 新增灯光崩溃修复与回归测试

`FDeferredLightUniformStruct` 增加角色 Ramp 纹理后，Lumen 批量灯光路径原先仍用全零 Uniform 作为占位资源。动态创建 Point Light 时，RenderCore 会因 `Resources[0]` 为空触发断言。当前实现通过 `GetDummyDeferredLightParameters()` 为所有占位和关闭灯光路径绑定白色 Ramp 与有效 Sampler，并覆盖 Lumen 与 Heterogeneous Volumes 的默认 Uniform 初始化。

真实 D3D12 回归脚本：

```powershell
& D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe `
  D:\Self\GItGame\Dev01\DevKit.uproject `
  -d3d12 -unattended -nosplash -NoSound `
  -ExecutePythonScript="D:\Self\GItGame\Dev01\Build\Verification\VerifyStylizedPointLight.py"
```

成功日志必须依次包含：

```text
STYLIZED_POINT_LIGHT_TEST: spawned
STYLIZED_POINT_LIGHT_TEST: passed
```

同时不得包含 `Null resource entry`、`Assertion failed` 或 `Fatal error`。

## 18. 待做：弱光角色保真与角色专用最终调色

> 状态：`TODO / 待做`。本节是后续实现规范，不代表当前版本已经具备这些能力。

当前已实现的 `CharacterBaseFill` 只在间接光合成阶段为 `GBuffer.BaseColor` 提供最低亮度。它不是最终画面曝光，也不能单独解决错误材质通道、直接光过暗、投影收敛过强或色调映射压暗等问题。下一阶段采用“材质数据校验 + Shading Model 光照整形 + 可选角色后处理”三层方案，不使用固定三盏角色灯作为基础依赖。

### 18.1 第一阶段：先验证数据路径

1. 确认角色所有材质实例均使用 `MSM_StylizedCharacterLit`，不存在旧 Unlit、Default Lit 或遗漏材质槽。
2. 确认 `T_MixMap.R` 接入 `Specular`，Metallic 使用独立参数且默认 0；避免 Metallic 将 `DiffuseColor` 压到零。
3. 增加角色专用 GBuffer 调试视图，逐项显示 BaseColor、ShadingModelID、LightingProfile、DiffuseBias、Metallic、Specular、直接光、间接光和 `CharacterBaseFill`。
4. 用 `CharacterBaseFill=0/0.2/0.5` 做固定曝光 A/B 截图。若画面无变化，先修复数据或合成路径，不继续叠加参数。

### 18.2 第二阶段：Shading Model 弱光整形

下列参数加入 `Character Lighting Profile`，由 Look Volume 切换和混合：

| 待做参数 | 作用 | 建议默认值 |
| --- | --- | --- |
| `Minimum Direct Luminance` | 为角色直接漫反射提供最低亮度，避免无主光时完全变黑 | `0.05` |
| `Weak Light Flattening` | 光照越弱，越把暗区和中间区向贴图本色收敛 | `0.35` |
| `Role Main Light Color` | 为角色主光提供独立艺术色，不要求场景灯改色 | 白色 |
| `Role Light Color Influence` | 控制角色吸收场景直接光颜色的比例 | `0.25` |
| `Role GI Tint` | 角色暗部环境色 | 白色 |
| `Role GI Floor` | GI 最低贡献，和 BaseFill 分工：GI Floor 保留环境色，BaseFill 保留底色 | `0.05` |
| `Shadow Convergence` | 将投影向 Profile 的 `ShadowTint` 收敛，而不是直接乘黑 | `0.5` |

核心原则：亮暗分界仍由半兰伯特、`DiffuseBias` 和 Curve Atlas Ramp 决定；弱光整形只改变各区最终权重与最低亮度，不破坏 Ramp 的色块边界。原生场景投影继续存在，但投影颜色通过 `ShadowTint` 风格化收敛。Additional Lights 继续走原生多光源循环并设置数量/重要度上限，不建立固定三盏灯工作流。

### 18.3 第三阶段：可选角色专用后处理

Shading Model 负责光照关系，后处理只负责最终镜头级调色。角色通过 Custom Depth/Stencil 生成遮罩，建议为角色保留可配置 Stencil 值（初始建议 `240`，接入项目前先检查冲突）。后处理材质只修改角色像素，并提供：

| 待做参数 | 作用 |
| --- | --- |
| `Final Exposure` | 对角色最终颜色整体提亮或压暗，不只改变亮部 |
| `Final Contrast` | 调整角色最终明暗对比 |
| `Final Saturation` | 恢复弱光下的颜色纯度 |
| `Final Tint` | 镜头或区域级角色综合色调 |
| `Shadow Lift` | 只抬暗部，保护亮部不过曝 |
| `Highlight Clamp` | 限制高光和轮廓光溢出 |
| `Blend` | 与未处理角色画面混合，便于调试和体积过渡 |

这些参数由 MPC 或运行时 Subsystem 驱动，并由 `Stylized Character Look Volume` 按相机位置混合。该层必须可在画质分级中关闭；关闭后角色仍应通过 Shading Model 保持可读，不能把基本可见性依赖在后处理上。

### 18.4 实施与验收顺序

1. 完成材质/GBuffer 审计并定位 `CharacterBaseFill` 当前无视觉变化的原因。
2. 实现 `Minimum Direct Luminance` 与 `Weak Light Flattening`，验证暗场不黑、亮场不过曝。
3. 实现角色主光色、GI Floor/Tint 与 `Shadow Convergence`。
4. 增加角色 Stencil Component，自动设置并检测 Stencil 冲突。
5. 实现角色专用 Post Process Material、MPC/Subsystem 和 Look Volume 混合。
6. 在白天、室内弱光、强色光、纯阴影和多光源场景做固定机位截图；同时测试 Low/Medium/High/Epic 分级和 GPU 时间。

验收标准：暗场角色底色可辨且不会变成黑剪影；亮部仍响应场景光强和少量光色；暗部只接受受控环境色；Ramp 分层稳定；金属、粗糙度和 SpecMask 仍保留材质质感；不放置专用三点灯也能得到可用基础效果。

## 19. 参考资料

- [角色渲染参考 1](https://zhuanlan.zhihu.com/p/20592939601)
- [色彩还原参考](https://zhuanlan.zhihu.com/p/2007870047671035236)
- [高光与 PBR 参考 1](https://zhuanlan.zhihu.com/p/2008887309844640748)
- [高光与 PBR 参考 2](https://zhuanlan.zhihu.com/p/2012135042818778255)

这些文章用于视觉方向参考。项目中的唯一实现真源是本文件的冻结通道规范、引擎源码和 `StylizedLightingSettings`。

## 20. 最终验证与跨机器复现包

本机最终验证环境：

```text
Engine root: D:\Dev-BuildEngine
Engine branch: dev-buildengine-5.8
Engine base commit: 6673776aad735f49a5ce3bbed474ffcc701e7a8e
Project root: D:\Self\GItGame\Dev01
Project branch: main
Platform: Win64 Development Editor / D3D12 / SM6
```

验证结果：

1. 完整 `DevKitEditor` 构建：`2957/2957`，`Result: Succeeded`。
2. 最终 Renderer 增量构建：`6/6`，`Result: Succeeded`。
3. D3D12 冷启动执行 `RecompileShaders Changed`：`No Shader changes found`，无全局或材质 Shader 错误。
4. `StylizedCharacterMaterialSetup -Apply`：`Success - 0 error(s), 4 warning(s)`；四条均为项目既有插件/配置/Python 名称警告。
5. `git diff --check`：Engine 与项目相关文件均通过。
6. 真实 D3D12 编辑器动态创建原生 `PointLight` 并等待 Lumen 收集 8 秒：测试通过，无空 Uniform 资源断言。
7. `Stylized Lighting` 自定义 Details 模块成功编译；支持自动、英文和简体中文三种显示模式，并覆盖全部参数说明。

独立复现包位于：

```text
Docs/ReproductionPackages/UE58_StylizedCharacterRendering_2026-07-13
```

包内包含按原目录保存的 Engine 与 Project 文件、SHA-256 清单、安装/备份脚本、构建验证脚本、本完整指南副本以及验证报告。另一台编译机可以克隆 Dev01 的目标分支后只向 UE 源码树安装 Engine 文件，也可以把该目录单独下载后同时安装 Engine 与 Project 文件。

复现包根目录的 `.gitattributes` 对 `Files/**` 设置了 `binary`（包含 `-text`）。这些文件是字节级安装载荷，必须禁止 Git 的 CRLF/LF 自动转换，否则 SHA-256 会在 Windows 检出后失效；迁移或拆分该目录时必须一并保留此文件。

Windows 在克隆前执行 `git config --global core.longpaths true`，并优先使用 `D:\Dev01` 这类短项目路径。复现包保留了 UE 原始相对路径，默认 260 字符限制可能导致深层 Engine/Plugin 文件检出失败。

不要把整个本机 Engine 工作区复制过去。本机工作区还存在 Setup/构建工具造成的第三方脚本换行变化和 `uatobj/uatbin` 产物，它们与本系统无关，复现包已明确排除。
