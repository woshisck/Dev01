# UE5.8 风格化角色渲染系统复现包

该目录是 `MSM_StylizedCharacterLit`、风格化多光源、角色 Look Profile、Lumen/GI、Kuwahara 反射、严格阴影分离和隐藏自发光工具的可移植源码快照。

## 冻结规范

- `T_Color.RGB`：美术原始颜色；`T_Color.A` 不使用。
- `T_Normal.RG`：切线空间法线 XY；`T_Normal.B`：Diffuse Bias `[0,1]`，Shader 映射到 `[-1,1]`。
- `T_MixMap.R`：SpecMask（风格化高光遮罩），接入材质 `Specular`；可用材质实例参数 `SpecIntensity` 二次缩放。
- `T_MixMap.G`：Glossiness，运行时 `Roughness = 1 - G`。
- `T_MixMap.B`：MatCapMask，仅保留，当前不参与计算。
- `Metallic` 是独立材质实例参数，默认 `0`，不再占用 MixMap 通道。
- 高光颜色与整套 Look 的高光强度由所选 `Lighting Profile 0..7` 的 `SpecularTint` 和 `SpecularIntensity` 控制。

## 2026-07-14 Spec 与间接遮蔽更新

- 恢复旧角色材质的 SpecMask 语义：旧实现是风格化 GGX/Toon 高光乘 `T_MixMap.R`、`SpecIntensity` 与 `SpecColorTint`；新实现保留同一遮罩数据，改由延迟多光源着色模型计算。
- `M_StylizedCharacterMaster` 新增材质实例参数 `SpecIntensity` 与独立 `Metallic`。默认接线为 `Specular = saturate(T_MixMap.R * SpecIntensity)`。
- 移除 `MSM_StylizedCharacterLit` 对 `GBuffer.Specular=0.5` 的强制覆盖，材质 Specular 输入现在会真实影响逐像素高光。
- `IndirectOcclusionStrength` 独立控制角色的 SSAO/Lumen 短距离遮蔽。`0` 可去除 Detail Lighting 中凹槽和贴近表面的局部自遮蔽，`1` 保持 UE 原始遮蔽；它不关闭场景物体投到角色上的投影，也不影响角色向环境投影。
- 当前项目基线：`SpecularIntensity=1.0`、`IndirectOcclusionStrength=0.25`。

## 2026-07-14 启动自动刷新修复

- Runtime 模块在加载时应用 CVar，并在 `OnPostEngineInit` 再次上传角色 Lighting Profile 与 Ramp Atlas，避免首次上传早于纹理 RHI 初始化。
- 编辑器、PIE、Standalone 和打包游戏均不需要打开 Project Settings 才能刷新效果。
- 修改 Runtime 插件源码后必须关闭编辑器再构建，否则 Windows 会锁定 `UnrealEditor-CelesLightRuntime.dll`，新代码不会进入当前编辑器进程。

## 2026-07-14 角色底色补光

- Lighting Profile 新增 `Character Base Fill / 角色底色补光`，默认 `0.20`。
- Shader 在 Lumen 与 AO 合成后直接使用 `GBuffer.BaseColor` 保证最低底色亮度，不受 Metallic 将 `DiffuseColor` 压低的影响，解决场景光照不足时角色接近黑色的问题。
- 该功能不使用专用三点灯，也不写入 Emissive，因此不产生 Bloom、不参与 Lumen 发光且不会照亮环境。
- 推荐用 `0 / 0.2 / 0.5` 三档对照；曝光继续调整已有光照，Base Fill 专门恢复接近零的暗部。

## 待做：弱光角色保真与角色专用后处理

- 当前 `CharacterBaseFill` 已实现，但“材质数据审计、最低直接光、弱光分层展平、投影向 ShadowTint 收敛、角色专用最终调色”仍标记为 `TODO`。
- 后续采用 Shading Model 与可选 Custom Stencil 后处理的两层方案；基础效果不依赖固定三盏角色灯。
- 后处理计划提供角色最终 Exposure、Contrast、Saturation、Tint、Shadow Lift、Highlight Clamp 和 Blend，并支持 Look Volume 与画质分级关闭。
- 完整参数、实施顺序和验收标准见 `CompleteGuide.md` 第 18 节。

## 2026-07-14 Profile 与 Attenuation 更新

- `Lighting Profile 0..7` 选择的是一整套角色 Look：Ramp、七区颜色、色彩影响、曝光、对比度和相关材质参数；它不是七区中的单独一区。
- Toon 灯会针对材质选择的同一个 Profile 执行七区 Ramp。多盏 Toon 灯各自计算光照，再按 UE 延迟光照流程累加。
- 三个顶层 Character Color Influence 参数是全局 Master；最终值为 `Global Master * Profile Influence`，默认均为 `1.0`。
- `T_Normal.B` 先从 `[0,1]` 映射到 `[-1,1]`，再严格按 `saturate((dot(N,L) + DiffuseBias + 1) * 0.5)` 生成 Curve Atlas 采样坐标。
- `ShadowFadePower` 默认恢复为原材质基线 `0.0`。本次更新没有修改角色自投影、场景投影或阴影策略代码。

## 2026-07-14 自阴影默认策略

- 独立的 Half-View HZB 角色自阴影默认关闭，运行时 `r.StylizedCharacter.SelfShadow.Enable=0`。
- `Stylized Character Shadow Policy` 继续负责原生投影分离：角色接收场景投影、拒绝自身原生投影，同时仍向环境投影。
- 不要通过关闭 Skeletal Mesh 的 `Cast Shadow` 消除自投影，否则角色对地面和墙体的投影也会消失。
- 模块化角色的每个 Child Actor 都需要应用 Shadow Policy，避免遗漏的头发、披风或配件继续产生角色自投影。

## 2026-07-14 角色 C++ 基类接入

- `AYogCharacterBase` 构造函数已创建 `StylizedCharacterShadowPolicyComponent` 默认子对象。
- 玩家和敌人角色蓝图继承该基类后自动获得 Shadow Policy，无需逐个蓝图添加。
- 继承组件在蓝图中可见，可按角色覆盖参数。
- 不继承 `AYogCharacterBase` 的独立 Child Actor 仍需自行创建组件或加入父组件的显式目标列表。

## 2026-07-14 动态灯光稳定性与双语界面

- 修复动态创建 Point Light 时 `FDeferredLightUniformStruct.Resources[0]` 为空导致的 RenderCore 断言。
- Lumen 批量灯光与 Heterogeneous Volumes 占位路径现在统一使用资源完整的默认灯光 Uniform。
- `Stylized Lighting` 项目设置支持 `Auto / English / 简体中文`，切换后立即刷新界面。
- 每个场景灯光、角色 Profile、反射和可选自阴影参数都有中英文名称与悬停说明。
- `Build/Verification/VerifyStylizedPointLight.py` 可在真实 D3D12 编辑器中自动创建点光源并验证崩溃修复。

完整需求、参考、方案、任务方向、架构和技术细节见 [CompleteGuide.md](Docs/CompleteGuide.md)。

## 目录

```text
Files/Engine/      UE 5.8 源码覆盖文件
Files/Project/     Dev01 项目覆盖文件
Docs/              完整指南与验证报告
Scripts/           安装、校验、构建和 Shader 验证脚本
FILE_MANIFEST.csv  文件来源、相对路径、大小和 SHA-256
.gitattributes     禁止 Files/** 自动换行转换，保证哈希稳定
```

## 推荐复现流程

1. 取得 EpicGames/UnrealEngine 访问权限。
2. Windows 执行 `git config --global core.longpaths true`，并把项目克隆到较短路径，例如 `D:\Dev01`。
3. 把 UE 5.8 源码放到目标目录，并切到基线 `6673776aad735f49a5ce3bbed474ffcc701e7a8e`。
4. 克隆 Dev01 的 `main` 分支；`agent/stylized-character-rendering-system` 保留为开发历史分支。
5. 以管理员权限非必需的普通 PowerShell 运行安装脚本。
6. 运行构建验证脚本；第一次公共头全量重编可能很久。
7. 启动编辑器，按完整指南第 15、16 节完成角色材质实例与场景画质验收。

示例：

```powershell
$Package = 'D:\Self\GItGame\Dev01\Docs\ReproductionPackages\UE58_StylizedCharacterRendering_2026-07-13'

& "$Package\Scripts\Verify-Package.ps1"

& "$Package\Scripts\Install.ps1" `
  -EngineRoot 'D:\Dev-BuildEngine' `
  -ProjectRoot 'D:\Self\GItGame\Dev01' `
  -SkipProjectFiles

& "$Package\Scripts\Build-And-Validate.ps1" `
  -EngineRoot 'D:\Dev-BuildEngine' `
  -ProjectRoot 'D:\Self\GItGame\Dev01'
```

克隆目标 Git 分支后，Project 文件已经存在，因此推荐 `-SkipProjectFiles`，只把包内 Engine 文件安装到 UE 源码树。如果只下载了本复现包，则去掉该参数，同时安装两侧文件。

## 安全规则

- 安装脚本默认要求 Engine `HEAD` 等于冻结基线；需要自行承担合并风险时才使用 `-AllowDifferentEngineCommit`。
- 覆盖前会备份所有已存在的目标文件到 `Saved/StylizedCharacterRenderingBackups/<timestamp>`。
- 脚本只触碰 `FILE_MANIFEST.csv` 中列出的文件，不删除任何项目或引擎文件。
- 必须保留包根目录的 `.gitattributes`；`Files/**` 被标记为 `binary` 字节快照，Git 的 CRLF/LF 自动转换会使 SHA-256 失效。
- Windows 必须启用 Git 长路径并尽量使用短克隆目录，否则深层 Engine/Plugin 快照可能报 `Filename too long`。
- 不复制 `Binaries`、`Intermediate`、`DerivedDataCache`、开发者测试资产或本机 Engine 第三方脚本换行变化。
- `Content/Art` 下的主材质和默认工具贴图由 Commandlet 生成，不依赖提交二进制 `.uasset`。

## 完成标志

- `Build.bat` 输出 `Result: Succeeded`。
- Commandlet 输出 `Success - 0 error(s)`。
- D3D12 日志包含 `Cmd: RecompileShaders Changed` 和 `No Shader changes found`，且不包含 `Shader compiler errors`、`errors compiling global shaders` 或 `Fatal error`。
- 材质编辑器能选择 `MSM_StylizedCharacterLit`，并能搜索到 `Stylized Character Lighting Output`。
- 角色蓝图能添加 `Stylized Character Shadow Policy`。
- `Stylized Lighting` 设置页可切换英文和简体中文，展开 Profile 后每个参数都有说明。
- 点光源回归日志包含 `STYLIZED_POINT_LIGHT_TEST: passed`，且不包含 `Null resource entry`、`Assertion failed` 或 `Fatal error`。
