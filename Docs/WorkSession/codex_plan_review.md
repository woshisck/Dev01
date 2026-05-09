## Codex 审查结果

### 发现的问题
- **首次命中链路不可靠**：方案要求“含本次命中”，但 BaseFlow 在命中后才施加 GE 并激活 `GA_FinisherCharge`，后续 `WaitGameplayEvent(Ability.Event.Attack.Hit)` 通常无法捕获已经发生的本次命中。
- **最后一层充能可能丢失效果**：`GA_FinisherCharge` 先移除 GE 层数、再派发 `ChargeConsumed`；最后一层被移除时，绑定 GE 生命周期的 FA 可能已停止，导致最后一次击退/印记不执行。
- **H3 触发终结技存在边界漏洞**：`AN_TriggerFinisherAbility` 放在 H3 伤害帧之后检查 `FinisherCharge`。如果 H3 命中消耗了最后一层，Tag 已消失，终结技不会触发。
- **重复激活阻断配置不完整**：`GA_FinisherCharge` 只写了 `AbilityTags` 和 `ActivationBlockedTags`，但缺少 `ActivationOwnedTags`；按项目 GAS 规范，`ActivationBlockedTags` 检查的是 ASC 当前持有 Tag，不能靠 `AbilityTags` 自阻断。
- **终结技 GA 可能取消充能 GA**：`GA_Player_FinisherAttack` 的 `CancelAbilitiesWithTag = PlayerState.AbilityCast` 可能匹配并取消 `PlayerState.AbilityCast.FinisherCharge`，从而提前清除 GE、印记或停止 `FA_Finisher_Detonate`。
- **印记缺少归属范围**：`DetonateMarks` 和清理方案按全场 `Buff.Status.FinisherMark` 扫描，未区分是哪名玩家、哪次窗口施加的印记，容易误引爆或误清除其他来源的印记。
- **印记清除链路未闭环**：方案声明通过 `Action.ClearFinisherMark` 清除印记，但未定义接收该事件并移除 `GE_FinisherMark` 的 GA/FA；引爆后也没有明确消费印记，可能重复引爆。
- **数值表与 FA 流程不一致**：`KnockbackDistance` 写入数值表，但 `FA_Finisher_ChargeHit` 和确认击退流程没有读取并传递该值；“所有伤害/效果逻辑在 FA 中完成”的目标也需要确保 B-1/B-2 不再保留 C++ 直接击退/伤害逻辑。

### 潜在风险
- **Category C FA 绑定 GE 生命周期是核心依赖**，但方案仍把它列为待确认；若项目 FA 系统不支持该机制，当前架构需要整体调整。
- **`BFC.LastEventContext` 语义风险较高**：多次事件同帧或连续目标事件可能覆盖上下文，导致 `LastDamageTarget` 指向错误目标。
- **全局时间膨胀会影响整个 World**：直接恢复到 `1.0` 可能覆盖其他慢动作、暂停、HitStop 或多人场景中的时间控制。
- **GE 持续时间与终结技时机耦合**：`FA_Finisher_Detonate` 绑定 `GE_FinisherCharge`，如果 GE 在终结技蒙太奇命中帧前过期或被取消，引爆事件会无人处理。
- **事件接收方依赖预授予 GA**：`Action.Knockback`、`Action.Slash`、`Action.ApplyFinisherMark`、`Action.ClearFinisherMark` 若目标未预授予对应 GA，会静默失败。
- **全场扫描成本与正确性风险**：频繁 `GetAllActorsOfClass` 扫描所有角色在敌人多时成本较高，也需要处理死亡、销毁、无 ASC、友军目标等过滤。

### 改进建议
- 将“本次命中”目标通过 `TriggerEventData` 或 BaseFlow 上下文显式传入，或者由 BaseFlow 对当前目标直接派发一次 `ChargeConsumed`。
- 调整消耗顺序：先派发并完成 `ChargeConsumed` 效果，再移除 GE 层数；或者让 FA 在最后一层事件处理完成后再停止。
- 为 `GA_FinisherCharge` 增加 `ActivationOwnedTags = PlayerState.AbilityCast.FinisherCharge`，并明确重复打出卡牌时是叠层、拒绝、刷新窗口还是合并剩余次数。
- 终结技启动时不要取消充能 GA，或把 `CancelAbilitiesWithTag` 收窄到普通攻击 GA；也可在终结技开始时显式冻结窗口并启动独立的 Detonate FA。
- 给 `GE_FinisherMark` 增加来源标识，例如 SourceASC、WindowId 或 SetByCaller 标记；引爆和清理只处理当前窗口的印记。
- 明确实现 `Action.ClearFinisherMark` 的接收方，并在引爆完成后消费或清除对应印记，避免重复引爆。
- 将 `KnockbackDistance`、确认击退距离、确认倍率等规则参数统一到 FA 数值表，或在方案中标明哪些是固定机制常量。
- 增加验收用例：首次命中、最后一层命中、H3 消耗最后一层、窗口超时、终结技取消、确认/未确认、重复打出卡牌、多目标印记、多玩家或多来源印记。

### 需要向用户确认的问题
- 充能窗口内再次打出终结技卡牌时，期望行为是增加层数、拒绝、重开窗口，还是只保留原窗口？
- H3 命中如果消耗最后一层，是否仍应触发终结技？
- 印记在引爆后是否必须立即移除，还是允许保留到 8 秒窗口结束？
- 项目是否只考虑单人场景？如果存在多人或召唤物，需要明确印记归属和全局慢动作策略。
- 项目 FA 系统是否已经支持 Category C 与 GE 同生共死，以及 `WaitGameplayEvent` 将 `EventData.Target` 写入 `LastEventContext` 的目标语义？
- `Action.ClearFinisherMark`、`Action.Slash`、`Action.Knockback` 的接收 GA 是否已在所有可能目标上预授予？