# Dev01 动作规则：表格与可挂载脚本框架

状态：保留的前期设计记录。2026-10-02 已按用户确认的 Lua 运行时方案实现第一版，实际接口、范围与验收见 [实现与使用说明](Dev01_ActionRules_Implementation.md)。本文中的拟定 API 和更广的锁治理目标不代表全部已经实现。

已确认：Python 用于作者侧转换，Lua 在动作请求时执行只读条件，C++ 负责 GAS 接入。现有编辑器 Python 不用于打包游戏的动作判断。

## 1. 目标与边界

- 常规动作关系由 Excel/CSV 管理，复杂条件由脚本表达，不要求在大矩阵逐格编辑。
- 表格、脚本和旧 StateConflict 资产都通过统一的规则协议接入。执行器不依赖编辑器 UI。
- 动作工作台降为规则源管理、差异预览和运行诊断；可关闭，不影响战斗。
- 动作身份沿用项目正式 GameplayTag 字典，不自动发明或注册标签；状态身份与 GA 的 AbilityTags 映射必须显式区分。
- 新的动作关系限制只走新规则入口。现有 GAS/CDO、输入、蒙太奇和移动/AI 限制必须盘点迁移，不可宣称一次增加接口就已全部统一。
- 不改变既有正式资产、存档和冲刺默认关系；例子只放隔离测试数据。项目模块即可实现核心框架，无需修改引擎 Renderer 或重新发布完整引擎。

## 2. 已核实的缺口

1. `YogAbilitySystemComponent.cpp::InitConflictTable/SetConflictTable` 仅重建索引，没有释放旧版本已施加的 block 计数。直接接一个“重载文件”按钮并不安全。
2. `ProcessStateConflict` 通过 `bProcessingConflict` 跳过嵌套处理。取消能力若同步移除另一个状态，其解锁处理可能被跳过。需要队列和自身贡献账本，而非简单防重入 return。
3. 本地 GAS 的 `BlockAbilitiesWithTags/UnBlockAbilitiesWithTags` 只更新 BlockedAbilityTags 计数，并不触发 owned tag 的 `OnTagUpdated`；原代码相关注释不能作为生命周期依据。
4. `OnTagUpdated` 另有直接 Movement/AI 控制；`IsPlayerActionMontageLocked` 有独立动作窗口判断。GA CDO 的 ActivationBlockedTags/CancelAbilitiesWithTag 等也是限制来源。
5. 当前工作台只有草稿 JSON 导出，没有表格导入或脚本挂载。现有草稿/保存保护可以复用。
6. 项目已启用 PythonScriptPlugin，但实际执行模块为 UncookedOnly；WeaveLanguage 为 Editor 蓝图生成工具。未发现项目 Lua/UnLua/SLua 运行时插件。

这些是代码层风险和接入点，不等于已在正式战斗中复现了所有残锁。

## 3. 三层架构

### 规则作者层

- Excel：面向策划的稀疏规则列表，不建立随动作数平方增长的全量矩阵。
- CSV/JSON：可读文本产物，可 diff、审查和版本管理。
- Python：批量生成、校验、转换表格和复杂条件组合；不默认让 Python 进程进入每帧战斗路径。
- 可选运行时 Lua：通过同一 Provider 接口提供动态判断，不直接操作 ASC、存档或世界对象。

表格建议字段：`RuleId, Enabled, Scope, CurrentStateTag, RequestActionTag, Effect, ConditionId, Reason`。

- `RuleId` 是稳定标识，不能用行号代替。
- `Effect` 区分 BlockRequest 与 CancelCurrent；先验证请求准入，再执行批准的取消计划。
- 两个方向分别写规则，不隐式镜像。
- 允许/白名单只解除明确指定的本规则域限制，不是跳过死亡、成本、冷却或其他所有者锁的通行证。
- 条件树支持 All/Any/Not、Tag 匹配、阶段与注册上下文值比较；未知条件、未知版本、重复 RuleId、无效 Tag 一律拒绝。

### C++ 核心层

拟定接口契约（以下是待实现 API，不是现有函数）：

```text
IActionRuleSource.LoadCandidate() -> RuleSetCandidate
ActionRuleService.Validate(Candidate) -> Diagnostics
ActionRuleService.Diff(CurrentVersion, Candidate) -> RuleDiff
ActionRuleService.CommitSnapshot(ExpectedRevision, Candidate) -> NewRevision
IActionRuleProvider.Evaluate(ReadOnlyContext) -> RuleDecision
ActionGate.EvaluateRequest(Request) -> DecisionWithProvenance
ActionGate.ExecuteApprovedRequest(Decision) -> ExecutionResult
ConstraintLedger.Acquire(Owner, Resource, Reason, Lifetime) -> Handle
ConstraintLedger.Release(Handle) -> ReleasedOrAlreadyReleased
ConstraintLedger.ReleaseOwner(Owner) -> ReleaseResult
```

`ReadOnlyContext` 包括请求身份、当前动作与阶段、owned Tags、只读属性快照、角色/武器上下文和规则版本。它不是可以任意修改 UObject 的脚本代理。

`RuleDecision` 包括命中规则 ID、来源路径/版本、拒绝原因、拟取消的动作实例和资源限制。无匹配规则与最终允许必须区分。

查询不能激活或取消能力，也不能修改 Tags。执行前复核版本和动作实例仍有效；GAS 成本/冷却/权限继续生效。取消与启动若不能保证原子性，必须返回真实部分执行结果，不能伪称已回滚被取消的动作。

### 工作台层

顶部改为“规则来源、路径、版本、载入候选、校验、预览差异、显式启用”。

- 文件源是唯一作者源时，矩阵只读，避免同时改 Excel 和资产形成双主源。
- 仅查看并不调用能力准入/激活，也不清锁。
- 运行诊断显示角色/世界、规则版本、请求结果、来源、规则 ID、资源限制所有者和句柄。
- 现有草稿 stale/dirty/只读/显式保存/Undo 保护保留。导入不自动应用、保存或提交。

## 4. 脚本挂载的两种交付范围

### A：运行时脚本

脚本 `evaluate(context)` 在动作请求或指定状态事件上返回结果，支持“体力低于阈值”“持特定武器”“恢复阶段且窗口开启”等条件。原则上按事件调用，而非每个 Actor 每帧遍历全部表。

需要新增独立 Lua 适配层，核心服务不直接依赖 Lua API。必须明确并测试：

- 仅主线程、安全边界、执行指令预算、内存预算、错误处理。
- 不默认开放文件/网络/OS、任意 UE 对象写入、存档访问或直接 Tag 清零。
- 限制随机数、墙钟和隐式全局状态；多人时权威方决策及客户端预测使用匹配版本。
- 开发期可显式重载；正式包采用 cook/stage 的锁定版本，不任意监听外部文件。
- 脚本返回无效数据或报错时拒绝受影响的请求并报告原因；不得默认放行或删除其他来源的锁。
- Python 可以负责生成/管理脚本，但不会因启用了编辑器 Python 就声称打包可用。

### B：脚本生成规则

Python/表格生成版本化 JSON，条件树由 C++ 运行时执行。修改已支持的规则和条件无需重编 C++，但不能执行任意 Python/Lua 函数。

不需新增战斗 VM，打包后不依赖 Python/Excel。遇到未支持的新条件类型，仍需扩展 C++ 或切换到 A 的动态 Provider。

两种范围共用 RuleSet、Provider、限制账本和诊断协议；不能把 B 包装成已经完成 A。

## 5. 残锁与重载契约

- 每个限制由 Owner + 唯一句柄持有，并记录动作实例、来源规则和版本。重复释放无副作用，只释放自身贡献，禁止为了修复锁而清零全局 Tag 计数。
- 同一资源存在多个所有者时，只有最后一个有效限制释放后才恢复。
- 能力结束/取消、角色销毁、关卡退出、来源卸载都走确定的释放路径。死亡/对话等系统仍拥有自身限制，其他脚本不能代为解除。
- Tag/能力回调先入队，完成当前变更后排空，避免嵌套取消造成解锁遗漏。检测循环和异常事件量，输出来源链。
- 热重载先完整解析、校验并构造不可变候选快照，再在安全边界切换；错误保留旧版本。
- 不能先移除全部旧锁再等待脚本加载。旧版本贡献与新版本贡献要基于同一状态快照计算差异，只更新本规则域贡献。
- 初版可以仅在角色初始化/退出 PIE 后启用新版本；若用户要求运行中重载，必须先通过活跃动作、多来源、失败回滚测试，不能直接调用旧 SetConflictTable。

## 6. 迁移顺序

1. 建立版本化 RuleSet/Context/Decision/Provider 协议和独立纯逻辑测试。
2. 引入 LegacyStateConflictAdapter 和贡献账本；保持旧规则语义，验证移除/取消/重入/换表行为。
3. 接入文件源、Python 工具；根据范围选择是否实现 Lua 运行时适配器。
4. 先在隔离角色中做对照模式，只报告新旧判断差异，不双重执行 Block/Cancel。
5. 逐项迁移动作关系：GAS 关系、输入缓冲/蒙太奇窗口、Movement、AI；记录未迁移来源，不一次删除全部旧检查。
6. 同一规则域切到新系统后才移除重复旧贡献；成本、冷却、网络权限等属于硬约束，保留而非强行改成表格关系。
7. 工作台以稀疏列表和来源诊断为主，矩阵仅作为可选预览。
8. 编译、隔离测试、打包运行时验证之后，再另行授权 P4/UGS 发布；不在此设计阶段改变已发布版本。

## 7. 验收清单

- CSV/JSON/Python 生成同等规则得到相同规范化版本；中文/BOM/引号/空单元格及重复身份处理明确。
- 无效候选、脚本异常、预算超限不替换有效规则、不默认放行。
- Block/Cancel 有向关系、父子 Tag 与状态/AbilityTags 身份映射正确；未命中不等于最终允许。
- 一个动作取消另一个动作时，嵌套结束回调不会遗留锁；两个来源锁同一动作，释放一个不会提前解锁。
- 重复结束、取消、GC/销毁、关卡切换、规则卸载能释放自己的贡献，不能影响外部贡献。
- 有活跃限制时重载、重载失败、版本冲突和重复载入无计数泄漏，无未授权解锁窗口。
- 同一个规则函数重复查询不产生副作用；不存在为 UI 诊断而偷偷执行取消的行为。
- 旧资产兼容、现有默认规则和正式存档哈希保持不变；不把冲刺问题的政策修改夹带进框架迁移。
- 若采用 A，必须测试真实打包运行时、Lua 资源预算、脚本错误和权限边界；编辑器测试通过不代表 Shipping 可用。
- 若采用 B，必须明确测试的是声明式条件执行，不声称运行时 Lua/Python 已实现。

## 本轮结果

2026-09-26 本文最初只完成审查与设计；之后用户批准实现。当前交付与测试证据统一记录在 [实现与使用说明](Dev01_ActionRules_Implementation.md)，请勿将本历史设计清单视为完整实现承诺或发布记录。
