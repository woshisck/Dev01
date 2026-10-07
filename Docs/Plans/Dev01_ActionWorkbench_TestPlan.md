# Dev01 原生动作工作台：测试与验收方案

日期：2026-09-20  
适用版本：第一版 UE 原生 Slate 工作台  
执行状态：**本文是可执行验收方案，不是已经通过的测试报告。** 编译、自动化与人工结果须填写文末记录；未执行的项目保持 `NOT RUN`。

### P4 / UGS 部署补充（2026-09-20）

本文第 7 节是 Git 开发副本的初始结果。P4 集成候选在独立开发副本 `X:\Dev-ProjectBuilds\Dev01-Workflow` 重新编译通过，未覆盖 UGS 主工作区二进制。`Saved/ActionWorkbenchValidation/P4CandidateReport/index.json` 记录工作台 10 项、工作流目录 1 项、旧工具栏兼容 1 项，共 12 项成功、0 失败；其中一个用例收到既有 `ABT_Base` 两条未连接转场条件警告，没有更改该资产。

团队验证使用自己的 UGS 主 workspace（本机 `X:\Project\YogProject\Dev01`）和其导入的 Installed Build；必须等待云端为本次代码 CL 发布对应 PCB，不能复制上述开发副本 DLL。发布后的准确 CL、PCB revision、客户端加载与测试结果记录在本次发布报告中。本段不是云端发布成功证明。

## 1. 验收对象与不包含的能力

入口：编辑器 `Designer → 动作工作台`；控制台命令 `DevKit.ActionWorkbench.Open`；标签页标识 `DevKitActionWorkbench`。

本版复用 `UStateConflictDataAsset`，不创建第二份运行时规则表。验收范围是：动作/Tag 目录、有向 Block/Cancel 矩阵、来源说明、草稿校验、带事务的 Apply、用户显式 Save、源资产并发修改保护，以及 PIE 的只读 ASC 诊断。

以下项目不能因工作台界面出现而被宣布已经完成：

- 不自动修改现有冲刺、普攻、战技或移动行为；冲刺需求仍为待确认的玩法变更。
- 不新增 Owner/Token 锁账本、动作准入框架、输入排队、资源共存裁决或动画窗口条带。
- 不把 `Priority` 显示为已接入的运行时优先级裁决。当前 StateConflict 消费路径按 ActiveTag 查表执行 Block/Cancel，不靠此字段选择胜者。
- 不把矩阵中的“本表无阻断”称为“保证能激活”。GA 自带条件、成本、冷却、目标、对话、死亡等检查仍然存在。
- 不执行 Tag 自动重命名、`.Attack.1` 到 `.Attack.Combo1` 的隐式迁移、P4/GitHub 提交或 UGS 发布。

### 必须固定的方向

矩阵**行 = 新请求，列 = 当前动作**。同一格中的两个控制不是互斥选项，也不是同一个字段：

| 单元格 | 控制 | 实际数据写入 |
| --- | --- | --- |
| 行 Attack，列 Dash | 当前动作阻止此请求 | `Rules[ActiveTag=Dash].BlockTags` 包含 `Attack` |
| 行 Dash，列 Attack | 新请求出现时取消当前动作 | `Rules[ActiveTag=Dash].CancelTags` 包含 `Attack` |
| 行 Attack，列 Dash | 新请求出现时取消当前动作 | `Rules[ActiveTag=Attack].CancelTags` 包含 `Dash` |

取消规则只有请求状态真正出现才可能执行；被 Block 拒绝的请求不会因此神奇地先取消当前动作。不可取消的能力也不能由界面承诺一定结束。

## 2. 测试前保护现场

1. 关闭同项目的重复编辑器进程，确认没有 Live Coding/编译正在改写模块。
2. 在 `X:\Project\Dev01` 记录 Git 状态；如果使用 P4 工作区，则先记录 `p4 opened`、pending CL 和 `p4 resolve -n`。不要 revert 或整理与本次测试无关的文件。
3. 记录当前项目、测试模块、引擎路径、BuildId、Git HEAD，以及工作台选中的源资产完整 Object Path。不要把文档中的旧 CL 当作当前值。
4. 正式全局冲突资产默认只读查看。自动化优先使用 `GetTransientPackage()` 下的临时对象；需要 Save 验收时，明确创建全新的 `/Game/__ActionWorkbenchQA/<RunId>/` 测试资产，不修改正式资产。
5. 确认没有将测试资产设置到 DevAssetManager 全局配置、正式角色 Blueprint、正式地图或武器/技能默认数据上。
6. 对正式 `Saved/SaveGames` 中实际存在的文件记录绝对路径、长度和 SHA-256。完成后重复检查，要求文件集合及哈希一致。不存在的目录记录 `ABSENT`，不要为了测试创建正式存档目录。
7. 自动化不得启动正式 GameInstance 或调用正式保存/读档流程。人工 PIE 必须先验证测试宿主不会接入正式存档：使用专用测试 GameInstance/测试存储，以及独立的临时测试世界；仅新建一个空关卡、备份旧存档或改 GameMode **不等于**存档隔离。没有已验证隔离宿主时，PIE 用例记录 `BLOCKED`，不在正式地图强行运行。
8. 本次不创建维护 CL、不运行 reconcile、不开启云构建/PCB/Installed Build 发布；临时 QA 资产不要提交。测试后保留失败现场；清理仅限逐项确认的本次临时资产，不删除整个 Content 或 Saved。

建议本次证据根目录：

```text
X:\Project\Dev01\Saved\ActionWorkbenchValidation\<RunId>\
  Environment.md
  Before\                 Git/P4 状态、正式存档哈希、源资产快照
  Build.log
  Automation\             自动化报告、测试日志
  Screenshots\            用例编号 + 分辨率 + DPI
  Manual\                 每条用例的动作、观察结果、相关资产路径
  After\                  状态和哈希对比
  Result.md
```

`<RunId>` 用执行时间，例如 `20260920_153000`。截图不得包含 P4 Ticket、密码或其他凭据。证据输出不构成资产发布。

## 3. 本地编译与自动化入口

以下为 PowerShell 命令模板。只选本次实际实现所在的项目副本，不在另一工作区盲目替换 DLL。运行前按第 2 节保护现场，并确认控制台里没有正在使用同一项目的编辑器。

```powershell
$QaProject = 'X:\Project\Dev01\DevKit.uproject'
$QaEngine = 'X:\Dev-BuildEngine\Engine'
$QaRunId = Get-Date -Format 'yyyyMMdd_HHmmss'
$QaEvidence = Join-Path 'X:\Project\Dev01\Saved\ActionWorkbenchValidation' $QaRunId
New-Item -ItemType Directory -Path $QaEvidence -Force | Out-Null

& "$QaEngine\Build\BatchFiles\Build.bat" DevKitEditor Win64 Development "-Project=$QaProject" -WaitMutex 2>&1 |
    Tee-Object -FilePath (Join-Path $QaEvidence 'Build.log')
if ($LASTEXITCODE -ne 0) { throw 'Compile failed. Do not run stale binaries as the new workbench.' }

& "$QaEngine\Binaries\Win64\UnrealEditor-Cmd.exe" $QaProject /Engine/Maps/Entry -unattended -nop4 -nosplash -NullRHI `
    '-ExecCmds=Automation RunTests DevKit.ActionWorkbench' `
    '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$QaEvidence\Automation" "-abslog=$QaEvidence\Automation.log"
```

自动化前缀是 `DevKit.ActionWorkbench`。执行前确认当前二进制确实注册该前缀：**零测试、未加载测试模块、进程启动失败不算通过**。报告必须记录具体测试名、数量、成功/失败/跳过，不能只看进程退出码。若实际调用因引擎/环境阻塞，记录命令、退出码及最后错误，不替换成旧测试结果。

`-NullRHI` 仅适合数据/模型测试，不证明 Slate 布局、字体、DPI、真实 PIE 或动画表现正确。UI 验收要正常启动图形编辑器；入口命令只能打开工作台，不能作为截图已经合格的证据。

原生 Slate 渲染证据可独立生成：去掉 `-NullRHI`，加入 `-ActionWorkbenchCapture`，仍运行同一测试前缀，使用正常图形应用环境。`ConfiguredSourceReadOnly` 会通过 UE `FWidgetRenderer` 输出三张实际控件渲染图到 `Saved/ActionWorkbenchValidation/SlateCaptures/Final`（1600×900 / 100%、1024×720 / 100%、1920×1080 / 150% 渲染比例）。这不是网页仿制图，也不是桌面截图；不能替代操作系统 DPI、窗口停靠、键盘与真实点击验收。重复运行前另存上次截图，避免覆盖历史证据。不要在本机加入 `-RenderOffscreen`：它触发已安装 StylusInputWintab 插件的 FNullApplication 启动崩溃，尚未进入工作台测试，证据保留在 `Capture.log`；本任务没有修改或禁用该插件。

### 自动化应覆盖的断言

具体注册名称以实现与本次报告为准，下列是覆盖要求而非虚构的通过记录。

| 编号 | 应覆盖内容 | 通过标准 |
| --- | --- | --- |
| A01 | 空表、默认目录、缺失源资产 | 无崩溃；空态清楚；缺源时 Apply/Save 不可用；不自动创建全局资产 |
| A02 | 双向矩阵映射 | Block 修改当前动作规则，Cancel 修改新请求规则；反向单元格保持不变 |
| A03 | Block/Cancel 独立性 | 只改一个控制不会误清另一个；组合态含义准确，不宣称必然取消 |
| A04 | 父子 Tag 匹配 | 目标 Tag 层级匹配与 ActiveTag 精确索引分开处理；不伪造不存在的父级触发规则 |
| A05 | 多条来源规则 | 一个来源移除后，其他有效来源仍显示；继承/间接来源不能显示为已被精确项删除 |
| A06 | 非法输入 | 无效/未知 ActiveTag、重复 ActiveTag 等阻止 Apply；指出具体规则，不静默后者覆盖 |
| A07 | 无关数据保留 | 修改矩阵不丢失其他规则、Priority、BlockCategoryMap 或未展示的 Tag |
| A08 | 草稿与事务 | 草稿不直接改源；Apply 后源变脏；Undo/Redo 恢复正确；未执行 Save 不声称已落盘 |
| A09 | 并发源变更 | 加载后源 Rules/分类等发生变化，旧草稿 Apply 被拒；重新载入不静默丢弃草稿 |
| A10 | 生命周期与只读 | 源对象失效、PIE 开始、目标 ASC 销毁时安全退出/失效；不对运行 ASC 写入或调用热切 API |

对于无法通过纯模型测试覆盖的保存失败、P4 占用、标签页关闭和 UI 状态，必须执行下面对应人工用例；不要用 A01–A10 推定通过。

## 4. 人工快速验收：先做这 8 项

第一次给策划/美术验收，按以下顺序操作，预计 15–25 分钟，不包含编译等待：

1. 打开工作台，确认 UE5 原生深色扁平风格，四周无网页壳/浏览器控件；中文、搜索、三段区域和数据来源可读。
2. 只读浏览正式规则，确认动作名称旁能查到真实 Tag；`Attack.Combo1` 没被工具重命名为 `Attack.1`。
3. 切换到全新 QA 冲突资产；在“行 Attack、列 Dash”修改 Block，确认旁边自然语言和 Rules 差异指向 `Dash.BlockTags += Attack`。
4. 在“行 Dash、列 Attack”修改 Cancel，确认是 `Dash.CancelTags += Attack`。反向格不被自动镜像。
5. 不按 Apply，检查源资产无改动；按 Apply，确认只影响 QA 资产并变脏；Undo/Redo 各一次。
6. 显式 Save，关闭并重开资产/工作台，确认内容可重读；保存失败时不能显示“已保存”。
7. 另一个 Details 面板修改同一个 QA 资产，再对旧草稿 Apply，确认检测冲突并要求重新加载/人工处理。
8. 完成环境/哈希对比，确认正式规则、正式地图、存档、引擎导入和 UGS 发布状态没有因测试发生变化。

## 5. 完整人工用例

### UI 与编辑体验

| 编号 | 操作步骤 | 预期结果 | 失败证据 |
| --- | --- | --- | --- |
| U01 | 用菜单和控制台各打开一次；把标签页停靠、拆出、重开 | 单一工作台标签页；无重复注册/重复窗口；来源与草稿状态始终可见 | `Screenshots/U01-*`、`Manual/U01.md`、Editor 日志 |
| U02 | 在 1920×1080/100%、2560×1440/150%、3840×2160/200% 检查；再将面板缩到约 1000×650 | 文本、按钮、警告不互相盖住；空间不足有滚动/合理最小尺寸；Apply/Save 可达；矩阵行列不会错位 | 每种组合一张总览及溢出细节截图 |
| U03 | 在 UE 默认深色主题和浅色主题分别查看；选择、悬停、禁用、错误、警告、键盘焦点各观察一次 | 使用 UE 风格与主题色；无写死黑底导致浅色失读；不能只靠红绿区分 Block/Cancel/来源 | `Screenshots/U03-*` |
| U04 | 搜索中文、英文、完整 Tag；输入长中文动作名；切换目录筛选，再选择矩阵格 | 无乱码/方块字；搜索结果与清空后列表一致；截断项有完整提示；被筛掉项不残留为另一动作 | `Manual/U04.md` 与搜索前后截图 |
| U05 | 让矩阵有足够多动作，水平/垂直滚动；对中部单元格修改 | 行列身份可辨；不会改到相邻或已回收的行；滚动后保留选中项与说明对应关系 | `Screenshots/U05-*`；QA 资产前后差异 |
| U06 | Tab/Shift+Tab 导航，Enter/Space 切换受控选项；尝试 Esc、Ctrl+W、标签页 X 和关闭编辑器 | 焦点清楚；不会误 Apply；未 Apply 草稿丢失风险有明确提示/取消路径。若某关闭路径暂不受拦截，必须明确报告限制，不能通过该项 | 各关闭方式的短视频/截图和日志 |
| U07 | 有草稿时切换源资产、重新加载、刷新、关闭重开 | 丢弃/取消的语义一致；未确认不覆盖草稿；无草稿时不弹无意义警告 | `Manual/U07.md`，标明每个按钮实际行为 |

### 方向、继承与规则安全

所有操作仅在 QA 资产进行；正式冲刺规则不改。

| 编号 | 操作步骤 | 预期结果 | 失败证据 |
| --- | --- | --- | --- |
| R01 | 设置行 Attack、列 Dash 的 Block；查看源差异；再查看反向格 | 只有 Dash 规则的 BlockTags 增加 Attack；反向格不自动勾选 | `Manual/R01.md`，记录完整 Tag 与差异 |
| R02 | 设置行 Dash、列 Attack 的 Cancel；分别打开该格 Block 与反向格 Cancel | 只有 Dash 规则的 CancelTags 增加 Attack；三个控制彼此独立 | `Manual/R02.md` |
| R03 | 同一个格同时具有阻止请求和请求取消当前的关系 | UI 解释请求被阻止时不能据此取消；不误导为优先取消、先清锁或保证打断成功 | `Screenshots/R03-*` |
| R04 | 某规则 BlockTags 已包含 `Character.State.Skill`，检查 Attack 及 Combo1；尝试移除精确 Attack 项 | 父 Tag 匹配来源仍清楚；精确项取消不显示“全部允许”；不为了去掉子项偷偷删除父项及其其他影响 | `Manual/R04.md` 与来源说明截图 |
| R05 | ActiveTag 有父级和子级两条规则，分别观察显式状态 Tag；比较普通目标 Tag 的父子匹配 | 诊断尊重当前 `ConflictMap.Find(Tag)` 的精确触发索引，不仅因 owned child 匹配 parent 就宣称父规则已经触发 | `Manual/R05.md` 与 ASC Tag 快照 |
| R06 | 在两条不同 ActiveTag 规则中配置同一个 Block 目标；分别移除一条来源 | 剩余来源仍可见；没有“第一个来源解除就全解锁”的描述；不得调用全局 ClearAllLocks | `Manual/R06.md` |
| R07 | 用 Details 在 QA 源中制造重复 ActiveTag、无效 ActiveTag；重新加载并校验；未知 Tag 通过旧引用/受控测试构造 | 明确指出具体规则并阻止 Apply；不静默覆盖或将未知 Tag 解释成空白允许；字典不被工具自动修改 | `Manual/R07.md` 与校验错误截图 |
| R08 | 修改普通动作关系，前后比较死亡、对话限制、BlockCategoryMap、未展示规则及 Priority | 没有顺带删除/改写；工作台的无阻断结论不豁免这些约束。受保护规则有只读/禁止操作提示；若通用编辑模式允许改安全规则，应判风险并先停止正式使用 | `Manual/R08.md`，完整差异 |
| R09 | 源里含一个目录未识别的动作和已有兼容 Tag；只编辑一个已支持动作 | 未识别项保留且可定位；不自动登记新 Tag、新 GA 或新动作模板；无隐式迁移 | `Manual/R09.md` |

### Apply、Save、并发及撤销

| 编号 | 操作步骤 | 预期结果 | 失败证据 |
| --- | --- | --- | --- |
| S01 | 记录 QA 源数据和磁盘哈希；编辑草稿，但不 Apply；切到源 Details | 源对象与磁盘都未改变；界面明确“草稿未应用” | `Manual/S01.md`，对象/磁盘前后对比 |
| S02 | 校验通过后 Apply；观察资产 dirty 状态和磁盘哈希 | 只修改该源对象；事务可撤销；磁盘未因 Apply 自动保存；“已应用未保存”可辨认 | `Manual/S02.md` |
| S03 | Apply 后 Undo，再 Redo；更换选中格重复一次；重新载入 | 源对象正确还原/重做；工作台刷新或拒绝基于旧快照的操作，不覆盖撤销结果 | `Manual/S03.md` 与事务日志/截图 |
| S04 | Apply 后显式 Save；关掉工作台，卸载或重开编辑器，再打开 QA 源 | 只有目标资产保存；校验、dirty 状态与落盘结果一致；内容重读一致 | `Manual/S04.md`，哈希与重开截图 |
| S05 | 在独立测试资产上模拟文件只读、P4 未 checkout/被其他用户锁定；点击 Save，并选择取消 checkout | 不自动强制覆盖、不提交 CL；Save 失败/取消不能显示成功；源仍保持可重试的 dirty 状态；草稿/已应用状态不丢 | `Manual/S05.md`，保存错误与 P4 状态；不要更改正式文件权限 |
| S06 | 在另一个 Details 面板改变同一源的 Rules、Priority 或 BlockCategoryMap；回旧草稿 Apply | 检测变化并拒绝陈旧 Apply，指出需重新加载；不执行静默最后写入覆盖 | `Manual/S06.md`，两个面板差异 |
| S07 | 有草稿时从 Content Browser 重命名/删除 QA 源，或让源引用失效；再点击校验/Apply | 清楚报失效；不会写到同名的另一个资产或崩溃；源重选需显式确认草稿处理 | `Manual/S07.md`，对象路径、Editor 日志 |
| S08 | 同一编辑器内打开两个工作台入口并尝试前后修改；若只能一个标签页则改用 Details | 单标签页策略明确；另一编辑入口的变化仍被快照保护；无数据争抢 | `Manual/S08.md` |

### PIE 只读诊断与热切保护

执行门槛：第 2 节的测试存储隔离已经有证据。没有隔离宿主时，下列用例保持 `BLOCKED`；不以正式地图和正式存档冒险补齐报告。

| 编号 | 操作步骤 | 预期结果 | 失败证据 |
| --- | --- | --- | --- |
| P01 | 启动隔离 PIE，选测试 Actor/ASC；打开诊断；选择没有 ASC 的 Actor | 显示正确实例/世界；无 ASC 时为空态；不会悄悄替换成默认玩家 | `Manual/P01.md`，实例路径与截图 |
| P02 | 切换两个使用不同 ConflictTable 的测试 ASC，再切换玩家/敌人 | 每次显示该实例的显式 owned Tags、GAS blocked ability Tags、active abilities 资产 Tag；所选源与实际 ASC 表不同有醒目警告 | `Manual/P02.md`，两个实例快照 |
| P03 | 一条限制来自本表，一条来自 GA/其他机制；观察诊断语句 | 只说“本表匹配/可能来源”；不伪造最近请求、失败原因历史、Owner/Token、精确剩余秒数；本表无命中不等于能够激活 | `Manual/P03.md`，真实输入与UI文本 |
| P04 | PIE 前保留草稿；PIE 中尝试 Apply、Save、重新绑定表；退出 PIE 后再操作 | PIE 期间拒绝所有写入和热切；不调用 SetConflictTable/InitConflictTable/CanActivateAbility；退出后重新校验源快照才能 Apply | `Manual/P04.md`，前后数据与日志 |
| P05 | PIE 暂停/继续，销毁所选 Actor，停止并重新启动 PIE | 不保留失效 ASC 指针，不跨 World 读旧实例，不崩溃；诊断显式清空/刷新 | `Manual/P05.md`，崩溃报告如有 |
| P06 | 多 PIE 实例下选择不同世界的 ASC；检查服务器/客户端身份 | 实例身份可辨；不把另一个世界的诊断当当前结果；不因刷新改变运行 Tags/能力数量 | `Manual/P06.md`，世界/网络身份及前后计数 |

## 6. 发布前的回归与停止条件

### 必須停止，不进入正式资产制作的情况

- 矩阵方向反了、反向自动镜像、只改 Block 却更改 Cancel，或父级来源被静默删除。
- 不点 Apply 就改了源、不点 Save 就写了磁盘、保存失败却显示成功。
- 陈旧草稿覆盖了其他编辑入口的改动，或 Undo/Redo 后工作台继续覆盖撤销结果。
- PIE 诊断会激活/取消能力、改变 Tag、热切冲突表，或展示另一个 ASC 的数据而不提示。
- 重复/未知规则被静默“修正”、死亡/对话限制被默认豁免，或本表允许被描述成全局通行。
- 正式存档、地图、全局规则、现有 Dash 数据或 UGS/P4 发布状态发生非预期变化。

### 第一版完成的判定

1. 编译成功，自动化测试非零且全部通过；失败与未覆盖项有明确记录。
2. U01–U07、R01–R09、S01–S08 通过或有用户明确接受的限制；关键安全项不能通过“已知问题”直接放行。
3. P01–P06 在安全宿主验证；若仍 BLOCKED，交付必须明确“PIE 诊断未实测”，不能声称完整验收。
4. 前后差异只包含本次代码/文档、批准的临时 QA 资产及本地构建产物；现有用户修改保持原样；正式存档哈希相同。
5. 策划/美术能够在不编辑 Tag 字典、不改 C++ 的前提下，对既有动作关系完成“查来源 → 改草稿 → 校验 → Apply → 显式 Save”。这不等于已支持提案中的新动作向导、动画条带或输入排队。

UGS/同事端验收是独立发布步骤：只有项目编辑器模块变更时通常走项目 PCB，不据此默认重发完整引擎。本方案的本地通过不能替代 P4 submitted CL、云端构建、BuildId 门禁、PCB verify 与新工作区启动证据。

## 7. 本轮结果记录（执行后填写）

| 项目 | 状态 | 必须附带的证据 |
| --- | --- | --- |
| 源码/引擎/项目身份 | VERIFIED | 项目 `X:\Project\Dev01`；HEAD `7cc93081ee4895358c98e83cc40bfb2a0df00bae` 加本地变更；引擎 `X:\Dev-BuildEngine`；本地模块 BuildId `4b62cf96-fa6d-4dad-a6dd-a57b299a0955`（不是 UGS Installed Build 的 BuildId） |
| 编译 | PASS | `Saved/ActionWorkbenchValidation/build_delivery.log`，Result: Succeeded，退出码 0；源引擎 UE5.8 DevKitEditor Win64 Development |
| 自动化 | PASS：10/10，0 warning，0 failed，0 notRun | 最终真实 RHI 编辑器报告 `Saved/ActionWorkbenchValidation/AutomationReport_Validated/index.json` 与 `Validated.log`；此前结果亦保留。计数仅指本测试组，不宣称项目启动日志没有其他警告 |
| UI/DPI/主题/键盘 | 默认打开及三尺寸原生渲染 PASS；实际交互未完整验收 | `SlateCaptures/Final/Matrix_1600x900_100.png`、`Matrix_1024x720_100.png`、`Matrix_1920x1080_150.png` 已逐张检查。矩阵按宽度裁切滚动，侧栏内容可滚动；文字、工具栏和选择状态清晰。Windows 截图接口失败，真实键盘、停靠、系统 DPI 与浅色主题仍 NOT RUN |
| 矩阵与规则保留 | 自动化 PASS；人工未完整执行 | 方向、父目标匹配、精确触发、目录、无关数据保留断言通过；不等于真实战斗通过 |
| Apply/Save/并发/撤销 | 模型自动化 PASS；交互式 Save/P4 锁未完整执行 | 内存/磁盘冲突、只读、独立保存确认、Undo 已测；Save 取消、外部 Save All UI 尚需人工 |
| PIE 只读 | 写保护 PASS；实际角色诊断 BLOCKED | 临时未初始化 WorldContext 通过；未启动正式 GameInstance；未验证安全宿主，P01–P06 不运行 |
| 正式数据保护 | PASS（自动化后） | 4 个正式存档 SHA-256、默认 StateConflict 资产 SHA-256 保持相同；P4 仍仅原有 Lighting BuiltData opened |
| 云端/UGS 发布 | NOT REQUESTED | 本地工作台实现不等于已经发布 |

每条失败记录最少包含：用例编号、准确步骤、期望与实际结果、实际源资产路径、Tag/规则索引、世界/ASC 身份（若有）、截图或日志路径、复现频率。修复后保留旧证据，并在同一用例下追加复测结果，不覆盖失败历史。

### 2026-09-20 自动化实际执行

以下 10 个注册测试已在独立 `UnrealEditor-Cmd.exe` 进程通过；并非上文全部人工验收均已通过：

1. `DevKit.ActionWorkbench.ApplyIsExplicitAndUndoable`
2. `DevKit.ActionWorkbench.DeterministicSnapshotAndCatalog`
3. `DevKit.ActionWorkbench.DraftIsolationAndDirection`
4. `DevKit.ActionWorkbench.ExactTriggersAndInheritedTargets`
5. `DevKit.ActionWorkbench.IsolatedDiskSaveAndConflictGuards`
6. `DevKit.ActionWorkbench.PIETransitionRejectsWrites`
7. `DevKit.ActionWorkbench.SourceConflictAndDirtyProtection`
8. `DevKit.ActionWorkbench.ValidationFailsClosed`
9. `DevKit.ActionWorkbench.WidgetConstruction`
10. `DevKit.ActionWorkbench.ConfiguredSourceReadOnly`

WidgetConstruction 使用 30 个注册 Tag 的隔离对象构建 Slate，并在 1.0/1.5 比例做预布局；这不替代真实 DPI 截图。PIETransition 只创建未初始化的临时 WorldContext，明确无 GameInstance、无 BeginPlay；这验证写保护，不等于真实战斗/多客户端诊断通过。

磁盘测试使用 `Saved/Automation/ActionWorkbench/<GUID>/Rules.uasset` 独占文件，验证只读拒绝、真实落盘确认、外部磁盘变化及无法通过 Reset 绕过。测试文件与 mount 保留到测试编辑器退出，文件作为 QA 证据保留；不进入 Content，不覆盖已有文件。测试专用 Save 作用域暂时禁止编辑器异步整资产/本地化校验排队（随后自动恢复），不改变生产 Save 路径或工作台的规则校验。

失败历史已保留：首次编译的两处 C++ 声明问题在 `build.log`，修复后编译通过；首轮 `AutomationReport/index.json` 为 8 通过、1 失败，原因是临时 mount/文件过早卸载，后台 AssetSearch/Localization 校验仍在访问。修复隔离夹具生命周期后，全量重跑得到 9/9 通过。没有删除或覆盖失败报告。

真实图形窗口暴露了菜单默认打开路径的错误，日志保留为 `UI.log` / `UI_Retry.log`：Slate `InitialAsset` 参数没有默认初始化，显式传参的测试未覆盖它。修复为默认 nullptr，并把与菜单一致的无参数构建加入 `ConfiguredSourceReadOnly`；草稿同时收窄为强引用源、仅复制 Rules/BlockCategoryMap 的原生 transient 对象。崩溃报告进程短暂占用 DLL/PDB 的两次链接失败也保留，退出本测试报告进程后重建成功。未修改引擎或正式资产来绕过失败。

最终原生渲染捕获等待布局、字体与 SVG 资源稳定后取样；校正测试渲染目标的色彩转换，三个尺寸均实测 `Panel expected=242424FF captured=242424FF`。没有后期修改图片像素，也没有为截图更改工作台主题。早期首帧、双重 gamma 和线性未转换样图只作为调试记录，验收以 `SlateCaptures/Final` 为准。小窗口的复选框文案已缩短，矩阵状态改为单行；未发生正式规则编辑。
