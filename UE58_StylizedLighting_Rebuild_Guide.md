# UE 5.8 风格化灯光与角色 Shading Model 重建指南

## 1. 目标

在 UE 5.8 源码引擎中完成以下能力：

1. 场景使用 Stylized Lumen Lighting：直接光和 Lumen 场景直接光具有可调的分层光照效果。
2. 角色使用独立的 `MSM_StylizedCharacterLit`，不再依赖 `Unlit + Emissive` 手算主光。
3. 角色可以接收 UE 灯光、阴影、投影与 Lumen 间接光，同时保留 Toon 明暗控制。
4. 美术不需要输入 Console Variable；通过项目设置调整全局风格化灯光。
5. 提供不可见的“自发光照明代理”工具。它本身没有游戏可见网格，只产生可分层的照明。
6. 保留原有 `CelesLight` 的 `Tex_LightInfo` 方案，作为局部特殊补光，而不再承担角色主照明。

## 2. 推荐目录

本次开发使用的目录：

| 用途 | 路径 |
| --- | --- |
| UE 5.8 源码引擎 | `D:\Dev-BuildEngine` |
| 项目 | `D:\Self\GItGame\Dev01` |
| 项目文件 | `D:\Self\GItGame\Dev01\DevKit.uproject` |
| 灯光插件 | `D:\Self\GItGame\Dev01\Plugins\CelesLight` |
| 角色主材质 | `/Game/Art/Material/CharacterMaterial/M_MasterCharacterMAT` |

建议另一台机器保持相同结构；若路径不同，所有命令替换为本机实际路径即可。

## 3. 先决条件

安装：

1. Visual Studio 2022 Build Tools 或 Visual Studio，包含 Desktop development with C++、MSVC、Windows SDK。
2. `.NET SDK 10`。UE 5.8 的 `UnrealBuildTool.exe` 在本次源码版本中要求 `Microsoft.NETCore.App 10.0`。
3. Git、Git LFS（若 UnrealEngine 仓库使用 LFS）。
4. 足够的磁盘空间。源码、依赖、Intermediate、Derived Data Cache 和 Editor 构建建议至少预留 250 GB。

检查 .NET：

```powershell
dotnet --list-runtimes
dotnet --list-sdks
```

如果缺少 10.x，可安装：

```powershell
winget install --id Microsoft.DotNet.SDK.10 --exact --accept-package-agreements --accept-source-agreements
```

## 4. 获取并初始化 UE 5.8 源码

需要已获得 Epic Games GitHub 访问权限。使用 Epic 账号关联 GitHub 后，克隆对应 UE 5.8 源码分支或团队已准备的分支。

```powershell
git clone <UE5.8-源码仓库地址> D:\Dev-BuildEngine
Set-Location D:\Dev-BuildEngine
.\Setup.bat
.\GenerateProjectFiles.bat
```

注意：

- `Setup.bat` 需要下载依赖，网络与磁盘都要稳定。
- 不要把引擎安装在带空格、中文或过深层级的目录中。
- 只在此引擎工作树修改 Renderer、Engine 和 Shaders；不要在 Launcher 安装的二进制 UE 中修改。

## 5. 编译并打开项目

首次编译项目编辑器目标：

```powershell
& "D:\Dev-BuildEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" `
  DevKitEditor Win64 Development `
  "-Project=D:\Self\GItGame\Dev01\DevKit.uproject" `
  -WaitMutex -NoHotReloadFromIDE
```

编译成功后使用源码引擎明确启动项目：

```powershell
& "D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe" `
  "D:\Self\GItGame\Dev01\DevKit.uproject"
```

不要只双击 `.uproject`，因为它可能根据 `EngineAssociation` 打开另一套 UE 5.8。

### 5.1 UHT 生成文件过期

如果长时间编译期间有人修改了带 `UCLASS`、`USTRUCT` 的头文件，可能看到以下特征：

- `UCLASS`、`GENERATED_BODY` 报 `C4430` 或 `C2143`。
- `.generated.h` 中的 `..._GENERATED_BODY` 行号与实际头文件中的 `GENERATED_BODY()` 行号不一致。

这通常不是类本身缺少基类。先确保源文件不再变化，再直接重新运行同一条 UBT 命令，让 UHT 重新生成代码。

## 6. 已有 Stylized Lumen Lighting

本引擎分支已有 Stylized Lumen 实现，核心文件为：

| 文件 | 作用 |
| --- | --- |
| `Engine/Source/Runtime/Renderer/Private/LightRendering.cpp` | 定义 `r.StylizedLumenLighting.*` CVar，并向渲染器传递参数。 |
| `Engine/Shaders/Private/StylizedLumenLighting.ush` | NdotL 分层、粗糙度影响、镜面高光控制。 |
| `Engine/Shaders/Private/DeferredLightingCommon.ush` | 延迟直接光应用分层。 |
| `Engine/Source/Runtime/Renderer/Private/Lumen/LumenSceneDirectLighting.cpp` | 将参数传入 Lumen Scene Direct Lighting。 |
| `Engine/Shaders/Private/LightData.ush` | 灯光数据参数结构。 |

原始全局参数：

```ini
r.StylizedLumenLighting.Enable 1
r.StylizedLumenLighting.DirectBlend 1
r.StylizedLumenLighting.IndirectBlend 0.35
r.StylizedLumenLighting.BandCount 5
r.StylizedLumenLighting.BandSoftness 0.18
r.StylizedLumenLighting.GlossInfluence 0.65
r.StylizedLumenLighting.SpecularIntensity 1.2
r.StylizedLumenLighting.SpecularOffset 0.0
```

参数含义：

| 参数 | 建议范围 | 美术含义 |
| --- | --- | --- |
| `Enable` | 0 / 1 | 开启全局场景分层灯光。 |
| `BandCount` | 1 - 8 | 漫反射分层数量；越高层次越多。 |
| `BandSoftness` | 0 - 1 | 分层边缘柔和度；低值更硬。 |
| `GlossInfluence` | 0 - 1 | 粗糙度对分层位置的影响。 |
| `DirectBlend` | 0 - 1 | 屏幕空间延迟直接光的风格化强度。 |
| `IndirectBlend` | 0 - 1 | Lumen 场景直接光写入的风格化强度。 |
| `SpecularIntensity` | 0 以上 | 全局镜面高光强度。 |
| `SpecularOffset` | 约 -1 到 1 | 镜面高光的 dot-space 偏移。 |

## 7. `MSM_StylizedCharacterLit` 设计

### 7.1 为什么需要独立 Shading Model

旧角色材质 `M_MasterCharacterMAT` 是：

```text
Surface + Opaque + Unlit
最终颜色通过 Emissive 输出
材质内部采样 Tex_LightInfo RT，循环计算局部灯光
```

该方案的问题：

- 每个像素按灯循环并多次采样 RT，角色材质成本随灯数量增加。
- 不能自然接收 UE 阴影、投影、SkyLight、反射和 Lumen GI。
- 无法把角色纳入统一的场景光照工作流。

新模型应采用传统 Deferred/GBuffer 路径：

```text
MSM_StylizedCharacterLit
  Base Color / Normal / Roughness / Specular / AO / Emissive
  + Light Threshold (Custom Data 0)
  + Light Softness (Custom Data 1)
  -> UE 直接光、阴影、投影、Lumen 间接光
```

### 7.2 材质输入约定

| 材质输入 | 用途 |
| --- | --- |
| `Base Color` | 角色固有色。 |
| `Normal` | 角色法线。 |
| `Roughness` | 高光宽度与反射粗糙度。 |
| `Specular` | 高光强度基础控制。 |
| `Ambient Occlusion` | 局部遮蔽。 |
| `Emissive Color` | 仅用于角色本身视觉发光，不作为主要受光结果。 |
| `Light Threshold` | Toon 明暗分界；`0` 偏亮，`1` 阴影面积更大。 |
| `Light Softness` | Toon 分界过渡宽度；`0` 接近硬边，`1` 较柔和。 |

初版建议只使用两个 Custom Data 槽，避免扩大 GBuffer。后续如果确实需要角色独有的 Rim、MatCap、Shadow Tint、Face SDF，再用 Material Function 或 Custom Primitive Data 扩展，不要先增加新的 GBuffer MRT。

### 7.3 GBuffer ID 约束

`SHADINGMODELID_*` 只使用 4 bit，最多 16 个 ID。当前引擎中：

```text
12 = SHADINGMODELID_SUBSTRATE
13 = SHADINGMODELID_SUBSTRATE_TOON
14 = SHADINGMODELID_STYLIZED_CHARACTER_LIT
15 = 预留
```

因此 C++ 枚举不要把新模型直接插入 `MSM_Strata` 后的默认序号 13；13 已被 Substrate Toon 占用。建议写为：

```cpp
MSM_Strata UMETA(DisplayName="Substrate", Hidden),
MSM_StylizedCharacterLit = 14 UMETA(DisplayName="Stylized Character Lit"),
MSM_NUM UMETA(Hidden),
```

## 8. 引擎修改清单

以下是实现 `MSM_StylizedCharacterLit` 的最小修改集合。文件相对 `D:\Dev-BuildEngine`。

### 8.1 C++ 材质系统

1. `Engine/Source/Runtime/Engine/Classes/Engine/EngineTypes.h`
   - 增加 `MSM_StylizedCharacterLit = 14`。

2. `Engine/Source/Runtime/Engine/Private/Materials/MaterialShader.cpp`
   - 在 `GetShadingModelString()` 中加入名称。

3. `Engine/Source/Runtime/Engine/Private/Materials/Material.cpp`
   - 在 Substrate legacy-conversion 映射中把该模型临时映射到 `SSM_DefaultLit`。
   - 让 `MP_CustomData0` 和 `MP_CustomData1` 对该模型激活。

4. `Engine/Source/Runtime/Engine/Private/Materials/MaterialAttributeDefinitionMap.cpp`
   - `MP_CustomData0` 显示名称：`Light Threshold`。
   - `MP_CustomData1` 显示名称：`Light Softness`。

5. `Engine/Source/Runtime/Engine/Private/Materials/HLSLMaterialTranslator.cpp`
   - 定义 `MATERIAL_SHADINGMODEL_STYLIZED_CHARACTER_LIT`。

6. `Engine/Source/Runtime/Engine/Private/Materials/MaterialIRToHLSLTranslator.cpp`
   - 将新枚举映射为 `MATERIAL_SHADINGMODEL_STYLIZED_CHARACTER_LIT`。

7. `Engine/Source/Runtime/Engine/Private/ShaderCompiler/ShaderGenerationUtil.cpp`
   - 读取该 define。
   - 为该模型使用 Standard GBuffer Slots，并启用 `GBS_CustomData`。

### 8.2 Shader 侧

1. `Engine/Shaders/Private/ShadingCommon.ush`
   - 定义：

```hlsl
#define SHADINGMODELID_STYLIZED_CHARACTER_LIT 14
#define SHADINGMODELID_NUM 15
```

   - 在 `GetShadingModelColor()` 中加入调试颜色。

2. `Engine/Shaders/Private/BasePassCommon.ush`
   - 在 `WRITES_CUSTOMDATA_TO_GBUFFER` 中加入 `MATERIAL_SHADINGMODEL_STYLIZED_CHARACTER_LIT`。

3. `Engine/Shaders/Private/ShadingModelsMaterial.ush`
   - 写入：

```hlsl
GBuffer.CustomData.x = saturate(GetMaterialCustomData0(PixelMaterialInputs));
GBuffer.CustomData.y = saturate(GetMaterialCustomData1(PixelMaterialInputs));
```

4. `Engine/Shaders/Private/ShadingModels.ush`
   - 新增 `StylizedCharacterLitBxDF()`。
   - 初版可先调用 `DefaultLitBxDF()`，然后以分层结果缩放漫反射：

```hlsl
FDirectLighting StylizedCharacterLitBxDF(
    FGBufferData GBuffer, half3 N, half3 V, FAreaLight AreaLight, FShadowTerms Shadow)
{
    FDirectLighting Lighting = DefaultLitBxDF(GBuffer, N, V, AreaLight, Shadow);

    const float RawNoL = saturate(dot(N, AreaLight.DiffuseL));
    const float Threshold = lerp(0.15f, 0.85f, saturate(GBuffer.CustomData.x));
    const float Softness = lerp(0.01f, 0.40f, saturate(GBuffer.CustomData.y));
    const float StylizedNoL = smoothstep(Threshold - Softness, Threshold + Softness, RawNoL);

    Lighting.Diffuse *= StylizedNoL / max(RawNoL, 0.001f);
    return Lighting;
}
```

   - 在 `IntegrateBxDF()` switch 中接入 `SHADINGMODELID_STYLIZED_CHARACTER_LIT`。

5. `Engine/Shaders/Private/DeferredLightingCommon.ush`
   - 全局 `StylizedDirectBlend` 对新角色模型跳过，避免角色被角色 BxDF 与全局场景分层重复处理：

```hlsl
if (StylizedDirectBlend > 0.0f &&
    GBuffer.ShadingModelID != SHADINGMODELID_STYLIZED_CHARACTER_LIT)
```

6. `Engine/Shaders/Private/Lumen/LumenMaterial.ush`
   - 在 `HasDefaultShading()` 中把新模型视为 Default Lit 类材质，使 Lumen 使用正常的简化材质路径。

## 9. CelesLight 美术工具

### 9.1 全局风格化灯光设置

在 `Plugins/CelesLight/Source/CelesLightRuntime` 新增：

```text
Public/StylizedLightingSettings.h
Private/StylizedLightingSettings.cpp
```

使用 `UDeveloperSettings`：

```cpp
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Stylized Lighting"))
class UStylizedLightingSettings : public UDeveloperSettings
```

暴露以下 `Config + EditAnywhere` 字段：

```text
bEnableStylizedLumenLighting
BandCount
BandSoftness
GlossInfluence
DirectBlend
IndirectBlend
SpecularIntensity
SpecularOffset
```

在 `ApplyToConsoleVariables()` 中通过 `IConsoleManager` 写入 `r.StylizedLumenLighting.*`。在 `PostEditChangeProperty()` 中调用该函数，实现编辑器内即时预览。

模块启动时调用：

```cpp
GetMutableDefault<UStylizedLightingSettings>()->ApplyToConsoleVariables();
```

`CelesLightRuntime.Build.cs` 需要添加：

```csharp
"DeveloperSettings"
```

### 9.2 不可见自发光照明代理

新增 Actor：

```text
Public/Actors/StylizedEmissiveLight.h
Private/Actors/StylizedEmissiveLight.cpp
```

类名：`AStylizedEmissiveLight`。

结构：

```text
AStylizedEmissiveLight
  SceneRoot
  UPointLightComponent LightComponent
  无 StaticMeshComponent
```

建议暴露字段：

```text
LightColor
Intensity
AttenuationRadius
SourceRadius
bUseInverseSquaredFalloff
bCastShadows
```

在 `OnConstruction()` 和 `RefreshLight()` 中同步到 `UPointLightComponent`：

```cpp
LightComponent->SetLightColor(LightColor);
LightComponent->SetIntensity(Intensity);
LightComponent->SetAttenuationRadius(AttenuationRadius);
LightComponent->SetSourceRadius(SourceRadius);
LightComponent->SetUseInverseSquaredFalloff(bUseInverseSquaredFalloff);
LightComponent->SetCastShadows(bCastShadows);
```

设计原则：

- 该 Actor 没有游戏可见网格，因此符合“模型不显示、只显示照明”。
- 它不是隐藏的 Emissive Mesh。隐藏网格通常也会离开 Lumen Surface Cache，不能稳定照亮周围物体。
- 光源本身受全局 Stylized Lumen 通道影响，因此照亮场景时有分层效果。
- 默认关闭阴影；只有需要投影时再开启，以控制性能。

### 9.3 编辑器入口

扩展 `CelesLightEditor`：

| 文件 | 修改 |
| --- | --- |
| `CelesLightEditor.Build.cs` | 添加 `Settings` 依赖。 |
| `CelesLightEditorLibrary.h/.cpp` | 新增 `CreateStylizedEmissiveLight()`。 |
| `CelesLightEditorModule.h/.cpp` | 在 `Tools > Celes Light` 中新增创建代理和打开设置页。 |

设置页打开方式：

```cpp
FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"))
    .ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("Stylized Lighting"));
```

编辑器使用流程：

1. 打开 `Project Settings > Plugins > Stylized Lighting`，调整全局分层参数。
2. 从 `Tools > Celes Light > Create Stylized Emissive Light` 创建照明代理。
3. 在 Actor Details 中调颜色、强度、半径、衰减和阴影。
4. 只有局部特效灯需要原始角色 RT 灯光数据时，使用 `ACelesPointLight` 和 Capture Box。

## 10. 角色材质迁移流程

对 `M_MasterCharacterMAT` 执行：

1. 复制原材质，保留一个可回退版本。
2. 在材质详情中将 `Shading Model` 设为 `Stylized Character Lit`。
3. 将原先纯 Emissive 主计算拆分：
   - 固有色接 `Base Color`。
   - 法线接 `Normal`。
   - 粗糙度接 `Roughness`。
   - 高光遮罩或强度接 `Specular`。
   - 真正需要自亮的部分才接 `Emissive Color`。
4. 新增标量参数：
   - `LightThreshold` 接 `Light Threshold`。
   - `LightSoftness` 接 `Light Softness`。
5. 第一轮先断开主光计算中对 `Tex_LightInfo` 的逐灯循环，仅保留为特殊补光或效果层。
6. 创建 Material Instance，交给美术调阈值、柔和度、粗糙度、镜面和 Emissive。

推荐初始值：

| 参数 | 起点 |
| --- | --- |
| `Light Threshold` | 0.5 |
| `Light Softness` | 0.08 - 0.18 |
| `Roughness` | 0.35 - 0.65 |
| `Specular` | 0.2 - 0.5 |
| `BandCount` | 5 |
| `BandSoftness` | 0.18 |
| `DirectBlend` | 1.0 |
| `IndirectBlend` | 0.35 |

## 11. 验证清单

### 引擎与编译

- [ ] `UnrealBuildTool` 能执行，不报 .NET 运行时缺失。
- [ ] `DevKitEditor Win64 Development` 编译完成。
- [ ] `UnrealEditor.exe <项目.uproject>` 能从 `D:\Dev-BuildEngine` 启动。
- [ ] Shader Compile 完成后没有 `SHADINGMODELID` 或 `MATERIAL_SHADINGMODEL` 报错。

### 角色

- [ ] 材质列表中显示 `Stylized Character Lit`。
- [ ] 角色受 Directional Light、Point Light、Spot Light 影响。
- [ ] 角色接收并投射阴影。
- [ ] `Light Threshold` 改变明暗边界。
- [ ] `Light Softness` 改变边界过渡。
- [ ] 角色没有因 `r.StylizedLumenLighting.DirectBlend` 出现双重分层。
- [ ] Lumen GI、SkyLight 和反射仍然可见。

### 场景与工具

- [ ] Project Settings 中能找到 `Plugins > Stylized Lighting`。
- [ ] 修改全局设置可即时改变场景分层效果。
- [ ] Tools 菜单可创建 `Stylized Emissive Light`。
- [ ] 代理 Actor 在游戏中不显示任何模型。
- [ ] 代理 Actor 能照亮场景，且随 Stylized Lumen 参数分层。
- [ ] 打开 `bCastShadows` 后只有预期的灯开启阴影。

## 12. 性能原则

1. 角色主照明交给 Deferred Lighting 与 Lumen，不在每个角色像素中循环采样 `Tex_LightInfo`。
2. `CelesLight` RT 仅服务于少量特殊灯、脸部补光、技能特效或不受标准灯光约束的表现。
3. 自发光照明代理默认关闭阴影，默认使用较少的局部灯数量。
4. 大范围持续光优先用 Directional Light、SkyLight 或少数 Rect/Point Light；不要把大量小型光源全部当作 Lumen Emissive Mesh。
5. 分层使用数学函数和两个 Custom Data，不额外增加 GBuffer 纹理。

## 13. 后续迭代建议

第一阶段完成后，按风险从低到高推进：

1. 在角色 Material Function 中增加 Rim、MatCap、Shadow Tint、Face SDF 控制。
2. 将角色 Shadow Tint 或 Ramp Texture 作为材质参数，不增加新的 Shading Model 输入。
3. 给 `AStylizedEmissiveLight` 增加 Spot/Rect 类型或以子类区分。
4. 为特定角色/头发建立独立模型或采用 Material Function；不要把头发全部逻辑立刻塞入角色基础模型。
5. 若需“真实 Emissive Mesh 参与 Lumen、但主视图不可见”，需要定制 Primitive Scene Proxy / Lumen Surface Cache 参与规则；这比照明代理复杂，应单独立项。

## 14. 本次实现状态

已写入的实现包括：

- `D:\Dev-BuildEngine` 中的 `MSM_StylizedCharacterLit` 引擎改动。
- `D:\Self\GItGame\Dev01\Plugins\CelesLight` 中的全局设置和自发光照明代理工具。
- `.NET SDK 10.0.301` 已安装。

首次完整构建未成功结束，原因是构建期间项目已有 UHT 头文件变化，造成生成代码行号过期；重新构建即可刷新。重建时确保无人编辑带反射宏的头文件。

