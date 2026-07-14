# 验证报告

## 构建

| 项目 | 结果 |
| --- | --- |
| DevKitEditor 全量构建 | `2957/2957`, `Result: Succeeded` |
| Renderer 最终增量构建 | `6/6`, `Result: Succeeded` |
| 输出程序 | `D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe` |

全量构建包含 Engine、Renderer、项目和所有启用的编辑器插件。最终模板写掩码修正后又单独重编了 `ShadowSetup.cpp`、`ShadowRendering.cpp` 并重新链接 Renderer。

## Shader

使用真实 D3D12 编辑器启动：

```text
-d3d12 -unattended -ExecCmds="RecompileShaders Changed,Quit"
```

日志结果：

```text
Cmd: RecompileShaders Changed
No Shader changes found.
Cmd: Quit
```

未发现全局 Shader、材质 Shader、Fatal 或 Ensure 错误。项目插件在收到 `Quit` 后仍保持编辑器进程，验证脚本会在确认上述结果后关闭该专用进程。

## 资产生成

```text
StylizedCharacterMaterialSetup rebuilt
/Game/Art/Material/CharacterMaterial/M_StylizedCharacterMaster
/Game/Art/Material/LightingTools/M_StylizedHiddenEmissive
Success - 0 error(s), 4 warning(s)
```

四条 warning 来自项目既有 Flow 配置、VSM 项目优先级、MCP EULA 提示和 Python PlayerState 重名，不是本渲染系统错误。

## 复现包

| 检查 | 结果 |
| --- | --- |
| Engine 快照 | 52 个文件 |
| Project 快照 | 31 个文件 |
| `FILE_MANIFEST.csv` | 83 条，路径、大小与 SHA-256 完整 |
| `Verify-Package.ps1` | `Package verification succeeded: 83 file(s).` |
| PowerShell 静态解析 | 3 个脚本，0 个语法错误 |
| 安装冒烟测试 | 同时跳过 Engine/Project 覆盖，0 错误、0 个写入文件 |

快照保持源文件原始字节，不会为通过格式检查而改写 UE 上游已有空白。真实 Engine 修改、Project 修改和包内脚本/文档均单独通过 `git diff --check`。

## 2026-07-14 材质排列修正

修正 `ShadingModelsMaterial.ush` 对可选字段 `WorldVertexNormal_Center` 的无条件访问。GI 平滑法线改为使用所有普通 BasePass permutation 都具备的 `TangentToWorld[2]`，并应用 `TwoSidedSign`。该改动不增加插值器，覆盖 Local Vertex Factory 和 GPU Skin Vertex Factory。

使用 `MaterialEditingLibrary.recompile_material` 直接重新编译截图对应资产 `/Game/Developers/g/L1_CommonLevel_Test/Material/NewMaterial2`：返回 0 个材质错误，日志包含 `STYLIZED_MATERIAL_PERMUTATION_VALIDATION_OK`，且 `WorldVertexNormal_Center`、`Shader compiler errors`、`LogMaterial: Error` 均为 0 次。验证日志位于 `Saved/Logs/NewMaterial2-PermutationFix.log`。

同日修正复现包在 Windows `core.autocrlf=true` 检出后的哈希漂移：包内 `.gitattributes` 将 `Files/**` 标记为 `binary`（包含 `-text`），当前 83 个源码快照均按源文件字节冻结并生成清单。该属性只作用于复现包快照，不改变 Dev01 或 UE 源码文件的换行策略。

Windows 干净检出还需要 `core.longpaths=true` 或足够短的仓库目录。未启用时，深层 Material/Plugin 快照会被 Git 拒绝创建；复现 README 已把长路径配置放到克隆前步骤。

## 2026-07-14 Profile 与 Attenuation 更新

本轮只更新非阴影功能：Profile 选择语义、原材质 Attenuation 公式、全局/分 Profile 色彩影响倍率，以及 `ShadowFadePower=0.0` 的原始默认值。角色自投影、角色接收场景投影和角色向环境投影的实现代码均未调整。

增量构建重新编译 `StylizedLightingSettings.cpp`、CelesLight 模块、`LightRendering.cpp` 和 Renderer，共 `16/16` 个 Action，结果为 `Result: Succeeded`，耗时 32.43 秒。

对截图对应资产 `/Game/Developers/g/L1_CommonLevel_Test/Material/NewMaterial2` 执行 `MaterialEditingLibrary.recompile_material`：Commandlet 退出码为 0，日志包含 `STYLIZED_CHARACTER_PROFILE_UPDATE_VALIDATION_OK`；`Shader compiler errors`、`errors compiling global shaders`、`LogMaterial: Error`、`Fatal error` 和 `WorldVertexNormal_Center` 均为 0 次。日志位于 `Saved/Logs/StylizedCharacter-ProfileUpdate.log`。

随后重新执行角色主材质与隐藏自发光材质生成脚本：`Success - 0 error(s), 4 warning(s)`。四条 warning 均为项目既有配置提示，日志位于 `Saved/Logs/StylizedCharacterMaterialSetup-ProfileUpdate.log`。

## 2026-07-14 角色自阴影默认关闭

将独立 Half-View HZB 角色自阴影的 Engine CVar 默认值、Developer Settings 默认值和项目配置统一改为关闭。`Shadow Other Objects Only`、Contact Shadow/Capsule Direct Shadow 的排除，以及角色向环境投影的路径保持不变。

重新构建 `DevKitEditor` 共 `9/9` 个 Action，包含 `StylizedLightingSettings.cpp`、CelesLightRuntime 和 `FirstPersonSelfShadow.cpp`，结果为 `Result: Succeeded`。

无窗口 Commandlet 验证日志包含：

```text
STYLIZED_SELF_SHADOW_SETTING=0
r.StylizedCharacter.SelfShadow.Enable = "0" LastSetBy: ProjectSetting
STYLIZED_SELF_SHADOW_DISABLED_VALIDATION_OK
```

Commandlet 退出码为 0，无 Python、材质、Shader 或 Fatal 错误。日志位于 `Saved/Logs/StylizedCharacter-SelfShadowDisabled.log`。

## 2026-07-14 角色基类 Shadow Policy 组件

`AYogCharacterBase` 现在通过 `CreateDefaultSubobject` 自动创建 `UStylizedCharacterShadowPolicyComponent`。DevKit 模块增加对 `CelesLightRuntime` 的公开依赖，因此玩家和敌人派生蓝图不再需要手工添加组件。

由于模块依赖发生变化，`DevKitEditor` 重新执行 `133/133` 个 Action，结果为 `Result: Succeeded`。

无窗口 Commandlet 读取 `AYogCharacterBase` CDO，确认：

```text
STYLIZED_SHADOW_POLICY_COMPONENT=StylizedCharacterShadowPolicyComponent
STYLIZED_SHADOW_POLICY_ENVIRONMENT_ONLY=1
STYLIZED_SHADOW_POLICY_AUTO_FIND=1
STYLIZED_SHADOW_POLICY_DISABLE_CONTACT=1
STYLIZED_SHADOW_POLICY_DISABLE_CAPSULE=1
STYLIZED_SHADOW_POLICY_SUBOBJECT_VALIDATION_OK
```

验证退出码为 0，无 Python 或 Fatal 错误。日志位于 `Saved/Logs/StylizedCharacter-ShadowPolicySubobject.log`。

## 尚需美术验收

代码、构建、Shader 和生成资产已验证。最终视觉仍需在代表场景中按完整指南第 16 节检查：暗部颜色保真、三种 Light Mode、金属/粗糙度、场景投影接收、角色只向环境投影、Kuwahara 分级以及隐藏自发光面源噪声。

## 2026-07-14 动态灯光崩溃与双语设置界面

崩溃调用栈通过 PDB 符号化定位到 `LumenSceneDirectLighting.cpp:2153`：Lumen 批量灯光使用全零 `FDeferredLightUniformStruct` 创建占位 Uniform，而新增的 `StylizedCharacterRampAtlas` 纹理资源不能为空。修复增加 `GetDummyDeferredLightParameters()`，并让 Lumen 与四条 Heterogeneous Volumes 占位路径统一绑定白色 Ramp 和有效 Sampler。

`DevKitEditor` 首轮重新编译 Renderer 共执行 63 个 Action；修复一个本地化头文件依赖后，增量执行 7 个 Action，最终 `Result: Succeeded`。输出程序仍为 `D:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe`。

使用真实 D3D12 编辑器执行 `Build/Verification/VerifyStylizedPointLight.py`，动态创建原生 Point Light 并等待 8 秒。日志包含：

```text
STYLIZED_POINT_LIGHT_TEST: spawned
STYLIZED_POINT_LIGHT_TEST: passed
```

进程正常退出，日志中没有 `Null resource entry`、`Assertion failed` 或 `Fatal error`。验证日志位于 `Saved/Logs/StylizedPointLightValidation.log`。

`Stylized Lighting` 项目设置增加 `Auto / English / 简体中文` 界面切换。自定义 Details 会即时刷新所有顶层参数、Profile 内部参数、分类、Kuwahara 枚举选项和悬停说明；语言设置不参与运行时渲染计算。

## 2026-07-14 SpecMask 与间接遮蔽修正

根据旧 `M_MasterCharacterMAT` 的导出材质图，确认 `T_MixMap.R` 并非 Metallic，而是旧风格化 GGX/Toon 高光链的 `SpecMask`。新主材质已改为 `Specular = saturate(T_MixMap.R * SpecIntensity)`，并新增默认值为 `0` 的独立 `Metallic` 参数。Lighting Profile 继续提供整套 Look 共用的 `SpecularTint` 与 `SpecularIntensity`。

Engine 已移除 `MSM_StylizedCharacterLit` 对 `GBuffer.Specular=0.5` 的固定覆盖，因此材质实例的 SpecMask 与 SpecIntensity 可以逐像素生效。项目当前 Profile 的高光强度由 `0` 恢复为 `1`。

Detail Lighting 中仍可见的凹槽暗化来自 SSAO/Lumen 短距离间接遮蔽，不是已经关闭的原生角色自投影。Profile 新增 `IndirectOcclusionStrength`：`0` 完全抑制这类局部遮蔽，`1` 保留 UE 原始结果；当前项目默认 `0.25`。该参数不会关闭场景投影，也不会阻止角色向环境投影。

本轮重新生成 `/Game/Art/Material/CharacterMaterial/M_StylizedCharacterMaster`，Commandlet 结果为 `Success - 0 error(s), 4 warning(s)`。随后执行真实 D3D12 验证，日志包含 `Cmd: RecompileShaders Changed`、`No Shader changes found` 和 `Cmd: Quit`，Shader/Material/Fatal/Assertion 错误关键字均为 0。最终 UBT 增量构建输出 `Target is up to date` 与 `Result: Succeeded`。验证日志位于 `Saved/Logs/StylizedSpecOcclusionValidation-Editor2.log`。

## 2026-07-14 启动自动刷新修复

启动时仅在 Runtime 模块 `StartupModule()` 上传 Ramp Atlas 会早于纹理 RHI 资源就绪，表现为必须打开 Project Settings 才恢复角色分层效果。Runtime 模块现在保留早期 CVar 初始化，并在 `FCoreDelegates::GetOnPostEngineInit()` 再上传一次完整 Profile 与 Ramp Atlas。

`DevKit Win64 Development` 游戏目标完成首次全量构建，输出 `D:\Self\GItGame\Dev01\Binaries\Win64\DevKit.exe`，结果为 `Result: Succeeded`。该目标不加载 `CelesLightEditor` 设置界面，确认修复属于可用于 Standalone/打包游戏的 Runtime 路径。关闭编辑器后又完成 `DevKitEditor` 增量构建，执行 4 个 Action 并成功链接 `UnrealEditor-CelesLightRuntime.dll`，结果同样为 `Result: Succeeded`。

## 2026-07-14 角色底色补光

Lighting Profile 增加 `CharacterBaseFill`，使用现有 Profile `ForwardTint.a` 传递，不增加 GBuffer、Profile Uniform 数量或灯光数量。非 Substrate 间接光合成在 Lumen/AO 结果后执行 `max(LitDiffuse, GBuffer.BaseColor * CharacterBaseFill * View.PreExposure)`，从而保留暗场景中的美术底色，并避免 Metallic 将 `DiffuseColor` 压到零后补光失效。

`DevKitEditor` 增量构建执行 14 个 Action，重新编译 Settings、Details 与 `LightRendering.cpp`，并成功链接 CelesLightRuntime、CelesLightEditor 和 Renderer，结果为 `Result: Succeeded`。真实 D3D12 日志包含 `Cmd: RecompileShaders Changed`、`No Shader changes found` 与 `Cmd: Quit`，Shader/Material/Fatal/Assertion 错误关键字均为 0。日志位于 `Saved/Logs/StylizedCharacterBaseFillValidation.log`。

随后 `DevKit Win64 Development` 游戏目标执行 7 个 Action，成功重新链接 `D:\Self\GItGame\Dev01\Binaries\Win64\DevKit.exe`，结果为 `Result: Succeeded`。UBA 在高提交内存状态下自动重试了部分编译进程，但没有产生编译或链接错误。
