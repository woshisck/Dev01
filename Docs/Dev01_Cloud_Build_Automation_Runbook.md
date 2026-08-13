# Dev01 云端构建、UGS 与同步运行手册

## 目标

将云服务器 `124.223.187.156` 作为 Dev01 的构建与发布节点：

1. P4 `//Dev01/main` 是完整项目和艺术资产的权威来源。
2. GitHub 只同步项目代码；绝不提交或同步 UE 引擎源码。
3. 云端在项目变更后编译 `DevKitEditor`，生成 UGS 可用的预编译编辑器包，并提交到 `//Dev01Binaries`。
4. 同事通过 UGS/P4 同步项目和对应编辑器包后可以直接打开 `DevKit.uproject`。

## 固定边界

- 不把 `Engine/Source`、本地 UE 源码 Git 历史或本地完整引擎工作区提交到 GitHub/P4。
- 引擎发布内容来自本地已经编译的 UE5.8 引擎；云端仅保留为项目代码编译所需的构建支持文件。
- 不修改同事的本地 workspace、P4 stream 映射或已提交项目内容，除非这是明确的修复提交。
- 不删除 `C:\Project\Dev01-P4`、P4 depot 内容、已发布 UGS zip 或 GitHub 工作区。只能清理已校验部署完成的下载/上传缓存。

## 云端重要位置

| 用途 | 路径/标识 |
| --- | --- |
| 项目 P4 workspace | `C:\Project\Dev01-P4` |
| 项目文件 | `C:\Project\Dev01-P4\DevKit.uproject` |
| 发布引擎 | `C:\Project\Dev01-P4\Engine` |
| 构建脚本/日志 | `C:\BuildAgent\Dev01` |
| 云端 GitHub 工作区 | `C:\Project\Dev01-GitHub` |
| 项目 stream | `//Dev01/main` |
| 引擎 stream | `//Dev01Engine/main` |
| UGS 二进制 depot | `//Dev01Binaries/UGS` |
| UGS 自更新 depot | `//Dev01Binaries/Tools/UnrealGameSync` |

## P4 构建身份

- 服务地址：`ssl:localhost:1666`
- 构建用户：`Admin`
- 构建 client：`build_10_0_0_10_Dev01_main`
- ticket/trust：`C:\BuildAgent\Dev01\.p4tickets`、`C:\BuildAgent\Dev01\p4trust.txt`

不要把密码写入日志、脚本、Markdown、GitHub 或 P4。

## 当前构建策略

服务器为 8 核、32GB 内存。UE 项目构建必须使用低并发：

```text
-MaxParallelActions=2 -NoUBA -UsePrecompiled
```

原因：高并发 UBA 曾将内存推到约 40GB 并触发编译器/UBA 失败。低并发构建较慢，但已验证稳定。

构建入口：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File C:\BuildAgent\Dev01\dev01_ci_project_build.ps1 -Force
```

主要日志：

```text
C:\BuildAgent\Dev01\logs\ci_build_after_support.out.log
C:\BuildAgent\Dev01\logs\ci_build_after_support.err.log
C:\BuildAgent\Dev01\logs\ci_build_after_support.exit.txt
```

成功条件：

1. 进程退出码为 `0`。
2. `C:\Project\Dev01-P4\Binaries\Win64\DevKitEditor.target` 存在且是本次构建产生。
3. 不存在 UBT error/fatal 编译错误。

## 构建支持文件

以下内容只保留在服务器项目引擎目录，用于云端编译；它们不是 P4/GitHub 发布内容：

- `Engine\Source` 的必要规则/头文件支持。
- `Engine\Plugins\...\Source` 的必要构建规则/头文件支持。
- `Engine\Intermediate\Build\Win64\x64\UnrealEditor\Development`。
- `Engine\Intermediate\Build\Win64\UnrealEditor\Inc`。
- `Engine\Shaders`。

部署成功标记：

```text
C:\BuildAgent\Dev01\build_support_deployed.ok
C:\BuildAgent\Dev01\precompiled_intermediate_deployed.ok
```

若空间紧张，只清理 `C:\BuildAgent\Dev01\downloads` 中已经通过校验、并且存在上述部署标记的压缩包/分卷；禁止删除项目 `Engine` 目录中的已部署支持文件。

## UGS 发布流程

只有当项目构建成功后才能发布：

1. 获取当前 `//Dev01/main/...` 最新 submitted changelist，记为 `<ProjectCL>`。
2. 从 `C:\Project\Dev01-P4` 收集本次的项目 `Binaries\Win64` 以及项目插件所需的 `Binaries\Win64`，保持相对目录结构。
3. 打包为：

   ```text
   //Dev01Binaries/UGS/++Dev01+main-Editor.zip
   ```

4. 使用内部发布 client `Dev01BuildAgentRelease` 提交 zip。
5. 提交描述必须含有：

   ```text
   [CL <ProjectCL>] Cloud-built DevKitEditor
   ```

   UGS 依靠这个 CL 标签把项目同步版本与编辑器包对应起来。
6. 验证 P4 中 zip 已更新，并用 `p4 verify -q` 验证。
7. 验证 `//Dev01/main/Build/UnrealGameSync.ini` 仍指向：

   ```ini
   ZippedBinariesPath=//Dev01Binaries/UGS/++Dev01+main-Editor.zip
   ```

不要把工程包上传至 GitHub。

## GitHub 与 P4 双向同步

GitHub 是代码协作入口，P4 是完整游戏内容入口。同步前必须检查两边状态与变更范围。

- GitHub -> P4：同步代码、配置、插件源码；不要删除 P4 的艺术资产或 UGS 配置。
- P4 -> GitHub：只同步代码/配置白名单；禁止包含 `Engine/`、`Binaries/`、`Intermediate/`、`Saved/`、内容资产与 UGS zip。
- 默认先 dry-run；只有工作区干净、没有冲突且用户明确要求时才提交/推送。
- 每次同步后，如果 `Source/`、`Config/`、`.uproject` 或插件源码发生变更，则触发云端 `DevKitEditor` 构建和 UGS 发布。

## Codex CLI 自动化职责

Codex CLI 在服务器上只负责：

1. 读取本手册和构建日志。
2. 监控当前构建直至成功或出现明确的项目代码错误。
3. 构建成功后执行 UGS 打包、P4 提交与验证。
4. 仅在构建支持缺失、磁盘不足、P4/Git 认证失效、项目代码本身编译失败时停止并报告确切原因。
5. 每次修改脚本前先备份；不改变 P4/GitHub 的发布范围。

Codex CLI 不得：

- 上传 UE 源码到 GitHub/P4。
- 删除 P4/UGS 已发布内容。
- 改写同事 workspace。
- 因 SSH 临时限制而重复上传已经校验成功的大文件。

## 立即执行指令

从当前状态开始持续执行，直到“最终验收”全部满足：

1. 监控正在运行的低并发 `DevKitEditor` 构建。不要启动第二个并行构建。
2. 如构建失败，先读取日志中的第一处真正错误；仅修复云端构建环境缺失或明确的项目代码错误。不要重新上传已经部署的构建支持包。
3. 构建成功后，打包项目编辑器输出并按“UGS 发布流程”提交对应项目 CL 的 archive。
4. 用 P4 校验 archive、确认 UGS 配置仍正确，并留下简短的最终状态报告到 `C:\BuildAgent\Dev01\logs\codex_automation_final.md`。
5. 若存在必须由人决定的 P4/GitHub 冲突、认证失效或项目代码设计问题，停止自动修改并写明阻塞原因与精确日志位置。

## 最终验收

任务完成必须同时满足：

- 云端 `DevKitEditor` 构建成功。
- 当前项目 CL 对应的 UGS zip 已提交并通过 P4 校验。
- UGS 客户端同步该 CL 后可找到 `DevKitEditor.target`，无需本地编译即可启动编辑器。
- GitHub/P4 同步脚本为可重复执行、默认安全检查的状态。
- 云端下载缓存已清理，C 盘保留至少 80GB 可用空间。
