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
| Engine 快照 | 48 个文件 |
| Project 快照 | 22 个文件 |
| `FILE_MANIFEST.csv` | 70 条，路径、大小与 SHA-256 完整 |
| `Verify-Package.ps1` | `Package verification succeeded: 70 file(s).` |
| PowerShell 静态解析 | 3 个脚本，0 个语法错误 |
| 安装冒烟测试 | 同时跳过 Engine/Project 覆盖，0 错误、0 个写入文件 |

快照保持源文件原始字节，不会为通过格式检查而改写 UE 上游已有空白。真实 Engine 修改、Project 修改和包内脚本/文档均单独通过 `git diff --check`。

## 2026-07-14 材质排列修正

修正 `ShadingModelsMaterial.ush` 对可选字段 `WorldVertexNormal_Center` 的无条件访问。GI 平滑法线改为使用所有普通 BasePass permutation 都具备的 `TangentToWorld[2]`，并应用 `TwoSidedSign`。该改动不增加插值器，覆盖 Local Vertex Factory 和 GPU Skin Vertex Factory。

使用 `MaterialEditingLibrary.recompile_material` 直接重新编译截图对应资产 `/Game/Developers/g/L1_CommonLevel_Test/Material/NewMaterial2`：返回 0 个材质错误，日志包含 `STYLIZED_MATERIAL_PERMUTATION_VALIDATION_OK`，且 `WorldVertexNormal_Center`、`Shader compiler errors`、`LogMaterial: Error` 均为 0 次。验证日志位于 `Saved/Logs/NewMaterial2-PermutationFix.log`。

同日修正复现包在 Windows `core.autocrlf=true` 检出后的哈希漂移：包内 `.gitattributes` 将 `Files/**` 标记为 `binary`（包含 `-text`），70 个源码快照重新按源文件字节冻结并生成清单。该属性只作用于复现包快照，不改变 Dev01 或 UE 源码文件的换行策略。

Windows 干净检出还需要 `core.longpaths=true` 或足够短的仓库目录。未启用时，深层 Material/Plugin 快照会被 Git 拒绝创建；复现 README 已把长路径配置放到克隆前步骤。

## 尚需美术验收

代码、构建、Shader 和生成资产已验证。最终视觉仍需在代表场景中按完整指南第 16 节检查：暗部颜色保真、三种 Light Mode、金属/粗糙度、场景投影接收、角色只向环境投影、Kuwahara 分级以及隐藏自发光面源噪声。
