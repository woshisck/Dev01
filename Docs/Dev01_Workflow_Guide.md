# Dev01 开发、构建与数据工作流

维护日期：2026-09-20。版本号应从现场预检获取。

## 统一入口

- 主入口在编辑器中：`YogTool → 开发工作台`，或工具栏的“开发工作台”。这是可停靠的原生 Slate 面板。
- `YogTool → 构建、发布与数据整理` 打开同一 UE 面板的发布页，不再启动外部 GUI。
- 编辑器外的 `Dev01_Workflow.bat` 仅作必须关闭 UE 才能构建时的辅助入口，不是主工作台。
- 工具、制作顺序、输入/产出与本机路径统一在 `Build/Dev01WorkflowCatalog.json`。
- 现场报告：`Saved/WorkflowHub/Reports`；运行日志：`Saved/WorkflowHub/Runs`；归档：`Saved/WorkflowHub/Archive`。

## UE5 风格与使用方式

界面采用 UE5 Starship 默认配色：背景 `#151515`、面板 `#242424`、分区 `#2F2F2F`、选中/主操作 `#0070E0`。原生面板直接使用 `FAppStyle` / `FStyleColors`，随编辑器主题变化；独立工作台使用同一套默认深色视觉语义。保持扁平、少边框，不使用渐变和装饰阴影。

- 原生面板左侧按材质、地表、场景、玩法分组，支持关键词搜索；底部进入“发布与数据整理”。
- 选中制作步骤后，右侧分别说明准备内容和产出。顶部明确当前是源码引擎还是 Installed Build。
- 发布页提供“检查模块匹配”“发布只读预检”“扫描重复日志”和“建立本地资产分类集合”，结果直接显示在面板内，详情默认折叠。
- 检查异步执行，运行期间禁止重复点击；失败不会显示成完成。不自动提交或上传，构建和发布的关闭编辑器/人工验收边界会明确显示。
- 原生只读检查共用 `workflow_core.py`，本机默认 `C:/Python313/python.exe`；其他机器在 `Saved/WorkflowHub/settings.json` 配置 `pythonExecutable`。缺少环境会显示原因，不会悄悄调用其他 Python 或执行发布。
- 独立辅助窗口另提供 `Ctrl+F`、开发副本路径设置和关闭编辑器后的构建执行。它不替代 UE 内的制作工具。

## 两套引擎的二进制隔离

主工作区 `X:/Project/YogProject/Dev01` 保持 UGS 已发布引擎与对应 PCB。源码引擎只能编译独立开发副本，不能再直接编译主工作区，否则项目和插件 DLL 会覆盖 UGS 产物而造成 Missing Modules。

本机开发副本为 `X:/Dev-ProjectBuilds/Dev01-Workflow`。它不是新的 P4 workspace；源码、Shaders、配置、Binaries 和 Intermediate 独立复制，Content 通过 junction 引用原资产。项目即使没有自定义 Shader 文件，也必须保留 `Shaders` 目录供 DevKitShaders 注册。**二进制隔离不等于资产写隔离**：本轮验证不保存任何资产；需要修改资产仍应明确其 P4 归属。

工作台“连接与路径”设置独立副本后，源码构建和启动都使用该目录；主目录、主目录内子目录及指回主目录的 Binaries 会被拒绝。副本不会自动同步源码；更新前应审查文件白名单，避免覆盖副本的在途修改。`prepare_development_copy.ps1` 只允许创建全新目标，遇到现有目录立即停止。

开发副本的 `Dev01Development.json` 标记让原生面板的发布检查使用主工作区配置，不会误把开发副本当作 UGS 安装。新版 UE 内工作台只有在项目源码提交、云端编译和匹配 PCB 发布完成后才进入团队 UGS 版本；仅有本地开发版本不代表已经发布。

## UGS 报错的本次诊断（2026-09-20）

`Saved/Logs/DevKit.log` 记录了引擎 BuildId `792eec68-5f82-4e2f-852a-66474164b377` 与项目/插件 `4b62cf96-fa6d-4dad-a6dd-a57b299a0955` 不匹配。原因是同一主工作区被源码引擎编译过，不是 Engine 未安装。

现场 UGS 记录项目 / Code CL130、PCB #27（P4 CL131）。本轮已用 P4 摘要 `BA565187BC627FD5AAFC8C5090178811` 校验下载包，备份后恢复 49 个项目/插件二进制文件。原文件与前后 SHA-256 清单位于 `Saved/WorkflowHub/Recovery/Restore-20260920-211500/manifest.json`。恢复后 12 处 Engine / target / 项目 / 插件 BuildId 均为 `792eec68-5f82-4e2f-852a-66474164b377`。

恢复未修改 Source、Content、Engine 或 P4 提交状态。不能只改 `.modules` 的 BuildId。`restore_ugs_pcb.ps1` 默认仅校验，需显式 `-Apply` 才恢复，并保存原文件；再次恢复前仍须核对 opened / pending、目标 PCB、编辑器进程和发布摘要。模块匹配和自动化启动通过不能替代同事端完整 UGS 冷下载/效果验收。

## 制作流程

| 工作流 | 顺序 | 权威数据 |
| --- | --- | --- |
| 材质与灯光 | RVT 材质库 → 材质合规 → NoVT 检查 → Texture Collection → 风格化自发光 | 原材质/纹理资产与库配置 |
| 地表与贴花 | 地表物件库 → 贴花放置 → 关卡 RVT / 烘焙 | `Collection.Records`；派生组件不替代放置记录 |
| 场景搭建与优化 | 地图包 → 关卡数据 → 模型检查 → EnvBatch 分组 → 合批审查 | RoomData / CampaignData / Actor Tag / BatchedAsset |
| 角色、战斗与玩法 | 角色 → 武器 → 敌人 → 卡牌/BuffFlow → 数值 → 成长 → 剧情 → 调试 | 各系统原有 DataAsset 与配置 |

工具按需创建，切换步骤保留本次工具状态。步骤序号是制作导航，不是自动验收勾选；每一步以工具自身报告和实际效果为准。贴花步骤调用已有编辑模式，战斗日志保留已有独立窗口的对象生命周期。原工具页 ID 和入口保留在“高级 / 独立工具”，旧布局仍可打开。

这里合并的是编辑入口和制作顺序，未重写渲染算法，也未合并不同业务资产的数据结构。模型/材质/贴花仍由各自原工具执行具体操作。

Git 工作区中的角色 LookDev 是当前未完成的在途开发，本次未复制或修改。待该工具完成构建后，可使用相同的目录契约接入材质与灯光流程。

## 构建与发布

1. 点击“只读预检”：核对 P4 opened / pending / resolve、项目和引擎 head、固定 Engine CL、PCB、BuildId。
2. 在 `X:/Dev-BuildEngine` 中开发引擎，关闭 UE 后用“编译开发副本”编译设置中的独立项目目录。不会覆盖主工作区的 UGS 预编译模块。
3. 用“打开开发副本”确认材质、Shader、场景和模块加载。验收是人工步骤，工作台不会从“编译成功”推断效果合格。
4. 项目代码变更走独立项目 CL；不发布引擎。引擎变更先运行流水线自检，再生成 Installed Build。
5. Installed Build 启动器返回只是 `started`。它输出的 OutLog / ExitFile 才是后台构建结果；退出码 0 后继续引擎发布。
6. 引擎发布交接：核对 `X:/Dev-EnginePublishStage/UE_5.8_Dev01/Engine` 的 source-free 内容，提交 Engine CL，验证 BuildId，再更新 stream 的显式 `@EngineCL` 和 release marker。
7. 使用已配置 SSH 公钥的 Host，点击“读取云端状态”。云端发布统一走 `C:/BuildAgent/Dev01/dev01_auto_ugs_build.ps1`。点击“云端构建并发布 PCB”是明确的发布动作，会再次执行现场预检。
8. 核对 completed_build.json 的 ProjectCL / CodeCL / EngineCL / BuildId / 输出哈希，校验 PCB，再用 UGS 同步和启动验证。

源码构建与源码编辑器启动使用现有 `Global\Dev01BinaryReleasePipeline` 互斥锁，避免在 Installed Build 后台仍运行时修改/读取构建产物。工作台同一时间只执行一个操作；构建前也会检查普通编辑器和命令行编辑器进程。若预检发现待 resolve 文件，发布被阻止。

**旧引擎发布入口的适用边界：** `X:/Dev-BuildPipeline/Dev01/start_dev01_engine_publish.ps1` 对应的发布器依赖旧 `Prepared PCB`，`anchor_dev01_engine_release.ps1` 使用 `.json` marker，而当前项目现场使用 `.txt` marker。不能把旧入口串接到云端生产流程并称为“一键发布”。工作台将此阶段保留为明确的人工交接，须在下一次引擎更新任务中统一该契约后再开放自动执行。

发布脚本、实际计划任务和云端状态以 `Docs/Dev01_Cloud_Build_Automation_Runbook.md` 的最新维护记录和实时读取为准。仅配置 SSH Host，密码和票据不写进工作流配置。

## 数据整理与恢复

本次建立统一分类，不移动 `.uasset` / `.umap`，因此不会破坏软引用或地图依赖。模型、材质、贴图、贴花、角色、战斗、剧情的编辑入口已经归组；已有业务记录仍由原工具维护。

工作台中的“整理资产：建立本地分类集合”按 Asset Registry 读取 `/Game` 和项目插件，创建 `Dev01WF_Materials`、`Textures`、`Meshes`、`Effects`、`Levels`、`GameplayData`、`Other`、`Redirectors_Review` 本地集合。集合只保存资产引用，重复运行补充成员，不删除其他集合或移动资产。Redirector 只进入审查集合，不能仅凭该集合直接删除。

可自动整理的数据严格限定为 `Saved/Logs`、`Saved/Codex`、`Saved/Automation` 中的文本生成物：

- 扫描 SHA-256，保留同组最新副本。
- 只选取七天前的重复文件，跳过 latest/current、票据、凭据、锁文件、备份、恢复记录、链接和 junction。
- 归档前再次核对源文件和保留副本的哈希。
- 每次归档先写 manifest 再移动文件；保留原相对路径。
- “恢复归档”按 manifest 恢复，并拒绝覆盖现有文件。
- 归档仍占空间；本阶段整理目录，不永久删除数据。

Content、Plugins、Engine、Saved/Autosaves、SaveGames、Screenshots、正式构建输出、发布包、Docs/GeneratedReports 都保留。需要清理正式资产时，应另走 Asset Registry 依赖检查与编辑器迁移流程。

命令行只读入口：

```powershell
python BuildScripts/Workflow/workflow_core.py --project . inventory
python BuildScripts/Workflow/workflow_core.py --project . preflight
python BuildScripts/Workflow/workflow_core.py --project . buildids
python BuildScripts/Workflow/workflow_core.py --project . cleanup-preview
```

确认候选后，`archive-duplicates` 会重新扫描并归档符合规则的全部候选；`restore --manifest <manifest.json>` 恢复对应归档。GUI 可选择部分候选。

## 维护约定

新增工具先归属现有工作流；增加目录项、输入/产出和 widget factory，避免继续堆积工具栏按钮。业务数据模型和发布契约分别维护，工作流面板只负责组织、导航和调用已有能力。

本次整理不自动提交 P4/Git、不更新 stream、不上传引擎或 PCB。普通预检可以在有未提交工作的环境中运行；发布阶段必须先明确 opened/pending 的归属。

本机可直接用已打包的 `BuildScripts/Workflow/dist/Dev01Workflow.exe`。该 EXE 与 `build/`、`__pycache__/` 属于本机生成物，已加入忽略规则；并不自动随 P4 代码发布。其他机器可用 Python 3.13+ 运行入口，或在单独批准的工具发布任务中分发 EXE。重新打包执行 `BuildScripts/Workflow/build_exe.ps1`（需要 PyInstaller）。

回归验证：`python -m unittest -v test_workflow test_workflow_ui`（在 `BuildScripts/Workflow` 内运行）；UE 自动化过滤器为 `DevKit.Workflow` 和 `DevKitEditor.Toolbar`。

## 上传前本地验证记录（2026-09-20）

- 源码引擎下的独立副本编译通过；UI 增量构建使用 `-NoEngineChanges`，禁止覆盖现有引擎二进制。
- 工作流/安全/UI Python 测试 19 项通过，含“BuildId 相同但 DLL 缺失仍阻止启动”。
- UE 测试 4 项通过：目录、工具栏/旧入口、原生页面、原生面板实际调用主工作区模块检查。
- 原生页面 PNG 是 Slate 组件实际渲染，不是网页概念图：开发副本 `Saved/WorkflowHub/Reports/NativeWorkbench.png` 与 `NativeOperations.png`。组件验收在正常 D3D12 模式运行；本机 `-RenderOffscreen` 会触发既有 Wintab/NullApplication 初始化崩溃，因此未采用该启动模式，也未修改引擎输入/渲染实现。
- UGS 主工作区恢复后，Installed Build + 项目完成启动，工具栏测试 1 项通过：主工作区 `Saved/WorkflowHub/Reports/UGS-Restored.log`。没有执行新同事 workspace 冷下载验收。
- 开发副本测试记录：`Saved/WorkflowHub/Reports/NativeTests-Final/index.json`；主工作区保留 `NativeBuild-Final.log`。
- 以上是上传前的本地验收快照，当时团队 PCB #27 尚不包含工作台。正式发布状态须另外核对项目 CL、云端 `completed_build.json` 和匹配 PCB revision，不能从本地测试结果推断。
