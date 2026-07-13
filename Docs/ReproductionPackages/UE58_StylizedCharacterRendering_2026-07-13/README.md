# UE5.8 风格化角色渲染系统复现包

该目录是 `MSM_StylizedCharacterLit`、风格化多光源、角色 Look Profile、Lumen/GI、Kuwahara 反射、严格阴影分离和隐藏自发光工具的可移植源码快照。

## 冻结规范

- `T_Color.RGB`：美术原始颜色；`T_Color.A` 不使用。
- `T_Normal.RG`：切线空间法线 XY；`T_Normal.B`：Diffuse Bias `[0,1]`，Shader 映射到 `[-1,1]`。
- `T_MixMap.R`：Metallic。
- `T_MixMap.G`：Glossiness，运行时 `Roughness = 1 - G`。
- `T_MixMap.B`：MatCapMask，仅保留，当前不参与计算。
- 不提供 Specular 贴图、参数或材质输入；非金属 F0 使用 UE 固定值，金属反射颜色来自 BaseColor。

完整需求、参考、方案、任务方向、架构和技术细节见 [CompleteGuide.md](Docs/CompleteGuide.md)。

## 目录

```text
Files/Engine/      UE 5.8 源码覆盖文件
Files/Project/     Dev01 项目覆盖文件
Docs/              完整指南与验证报告
Scripts/           安装、校验、构建和 Shader 验证脚本
FILE_MANIFEST.csv  文件来源、相对路径、大小和 SHA-256
```

## 推荐复现流程

1. 取得 EpicGames/UnrealEngine 访问权限。
2. 把 UE 5.8 源码放到目标目录，并切到基线 `6673776aad735f49a5ce3bbed474ffcc701e7a8e`。
3. 克隆 Dev01 的 `agent/stylized-character-rendering-system` 分支。
4. 以管理员权限非必需的普通 PowerShell 运行安装脚本。
5. 运行构建验证脚本；第一次公共头全量重编可能很久。
6. 启动编辑器，按完整指南第 15、16 节完成角色材质实例与场景画质验收。

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
- 不复制 `Binaries`、`Intermediate`、`DerivedDataCache`、开发者测试资产或本机 Engine 第三方脚本换行变化。
- `Content/Art` 下的主材质和默认工具贴图由 Commandlet 生成，不依赖提交二进制 `.uasset`。

## 完成标志

- `Build.bat` 输出 `Result: Succeeded`。
- Commandlet 输出 `Success - 0 error(s)`。
- D3D12 日志包含 `Cmd: RecompileShaders Changed` 和 `No Shader changes found`，且不包含 `Shader compiler errors`、`errors compiling global shaders` 或 `Fatal error`。
- 材质编辑器能选择 `MSM_StylizedCharacterLit`，并能搜索到 `Stylized Character Lighting Output`。
- 角色蓝图能添加 `Stylized Character Shadow Policy`。
