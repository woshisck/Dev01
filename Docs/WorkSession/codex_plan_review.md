## Codex 审查结果

### 发现的问题
- 方案把“卡牌匹配奖励”定义为 `bActionMatched + bTriggeredMatchedFlow`，但 `FDamageBreakdown` 只计划新增 `bActionMatched`，没有新增 `bTriggeredMatchedFlow`。这会把普通动作匹配误判为匹配奖励，导致颜色、过滤和统计失真。
- `CombatDeckComponent` 的 `FCombatCardResolveResult` 没有 `bConsumedCard` 字段，当前更接近“已消耗卡牌”的字段是 `bHadCard` / `ConsumedCard`。方案中的字段映射不完整。
- 方案要求 GE Execution 写入伤害行并携带 `CardId`，但当前 `DamageExecution.cpp` 构造 `FDamageBreakdown` 时没有卡牌上下文来源；方案缺少从 `ResolveAttackCard` 结果传递到 GE Execution 的数据通道。
- “卡牌额外 / 终结技 / 连携”伤害统计缺少可计算依据。若卡牌事件行 `FinalDamage=0`，仅靠现有伤害行无法拆分基础伤害与卡牌增益伤害。
- 方案声称伤害行与卡牌事件行会相邻展示，但若在 `ResolveAttackCard` 后立即 `PushEntry`，OnHit 流程中卡牌事件可能出现在 GE 伤害行之前；OnCommit 流程还可能早于命中，甚至未命中也会记录卡牌事件。
- `SignatureTrait` 的来源没有定义。当前可见结构中有 `CardIdTag`、`CardEffectTags`、`ReasonText`、`ExecutedFlows` 等信息，但方案未说明 12 种标志性特征如何映射到单个 `FName`。
- `CombatLogStatics` 和 `CombatLogWidget` 各自有过滤、格式化、颜色和统计逻辑，方案同时修改两处，容易产生行为分叉。
- 计划决定移除 `DamageBreakdownWidget.h/cpp`，但“涉及文件”没有列出删除这两个文件，也没有明确清理 `OnDamageBreakdown` 注释、蓝图父类、Content 资产引用和构建残留。

### 潜在风险
- 新增 `ECombatLogFilter` 枚举值若插入到旧枚举中间，可能导致已保存蓝图枚举值错位；应追加到末尾。
- 使用 `◆ / ▲ / ⬡ / ── / → / ×` 等特殊字符，可能在 UE 源码编码、Slate 字体或日志输出中出现乱码或缺字。
- 每次攻击拆成“伤害行 + 卡牌事件行”后，多目标命中、投射物命中、未命中、OnCommit 卡牌都会让日志语义变复杂。
- `MaxEntries` 扩容到 1000 只是缓解容量问题，不解决事件倍增后的阅读密度和性能问题。
- 删除 `DamageBreakdownWidget` 可能破坏仍以它为父类的 Widget Blueprint，尤其是二进制 Content 引用不容易通过文本搜索完整发现。
- 仅编译验证不足，过滤、颜色优先级、摘要统计、空字段格式化和卡牌事件顺序都容易出现逻辑回归。

### 改进建议
- 明确新增字段：至少补充 `bTriggeredMatchedFlow`，并定义 `bConsumedCard = Result.bHadCard && Result.ConsumedCard.IsValidCard()` 的映射规则。
- 增加关联字段，例如 `AttackInstanceGuid`、`CardDisplayName`、`CardIdTag`、`bIsCardEventOnly`，用于排序、分组和区分零伤害状态行。
- 不建议在 `CombatDeckComponent` 底层直接写日志；更适合在拥有攻击上下文和目标上下文的调用层组装日志，或先缓存卡牌结果，再由伤害日志消费。
- 把格式化、过滤、颜色和统计逻辑集中到 `CombatLogStatics`，`CombatLogWidget` 只负责展示，避免两套规则维护。
- 先定义统计口径：卡牌统计只计次数，还是要估算增益伤害；若要统计增益伤害，需要在伤害结算前后保留 baseline。
- 为 `DamageBreakdownWidget` 移除增加独立清理步骤：C++ 文件删除、include 清理、委托注释更新、蓝图父类检查、Content 引用检查。
- 增加自动化测试或最小验证用例：过滤器匹配、颜色优先级、`NAME_None` 空字段、摘要统计、卡牌事件与伤害行顺序、多目标命中。

### 需要向用户确认的问题
- 卡牌事件是记录“消耗发生”还是只记录“命中后产生效果”？未命中时是否要显示？
- 多目标命中时，一张卡牌应显示一条从属事件，还是每个目标伤害行下都显示一条？
- `SignatureTrait` 是否必须支持多特征？若 12 种特征可能同次触发，单个 `FName` 会丢信息。
- “卡牌额外伤害”是否必须精确统计？如果必须，需要确认基准伤害计算规则。
- 是否确定本期删除 `DamageBreakdownWidget`，而不是先废弃但保留一版兼容过渡？