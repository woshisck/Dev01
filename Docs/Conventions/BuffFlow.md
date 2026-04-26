# 编码规范：BuffFlow / FA

> **Claude 编码前必读**。设计新效果（buff / 符文 / 状态 / 命中反馈 / 暴击 / 升阶 / 衰退...）时遵循。
>
> 来源：原 memory `feedback_flow_first.md`（用户反馈，2026-04-04 起）。已转写到本规范。

## 核心原则

**所有效果逻辑应在 BuffFlow 节点图（FA = NotifyFlowAsset）里实现，不要单独写 GA / GE 蓝图。**

GA / GE 仍可作为底层执行单元（FA 节点的 `Apply Effect` 调用 GE，`Grant GA` 注入 GA），但**触发逻辑、条件判断、目标选择、跨系统通信全部走 Flow**。

## 为什么

策划要能在 Flow 编辑器里可视化地编排效果逻辑（触发条件 / 目标选择 / 效果施加），而不是去改 GA 蓝图。改 Flow Graph = 拖几个节点；改 GA = 重新编译，门槛差一个量级。

## 如何应用

| 场景 | ❌ 错误 | ✅ 正确 |
|---|---|---|
| 命中触发流血 | 写新 GA 在攻击命中时 ApplyEffect | FA：`OnDamageDealt → Probability(20%) → ApplyEffect(GE_Bleed)` |
| 暴击触发冻结 | GA 内部判 IsCrit 后 AddTag | FA：`OnCritHit → AddTag(Buff.Status.HitStop.Freeze)` |
| 升阶时清空热度 | PlayerCharacter cpp 里 OnPhaseUp 调 SetHeat(0) | FA：`OnPhaseUpReady → IncrementPhase → ApplyAttributeModifier(Heat = 0)` |
| 击杀回血 | GA 内 BindOnKill | FA：`OnKill → ApplyEffect(GE_HealOnKill)` |

## 何时仍需写 C++

- **新底层节点**：FA 缺少某种事件 / 操作（例如新增 `BFNode_OnPortalEnter`）。继承 `UBFNode_Base` 派生新节点，编辑器中即可拖用
- **新 ASC 接口**：FA 节点要读取 / 写入新字段（如 `CurrentActionPoiseBonus` / `PendingHitStopOverride`），需在 ASC 或 Character 上加属性
- **跨系统底层联动**：例如 LevelFlow 与 BuffFlow 节点互通（`LENode_WaitForLootSelected` → ASC `OnLootSelected` 委托）

## 节点目录速查

详细节点说明 → [BuffFlow_NodeUsageGuide](../Systems/Rune/BuffFlow_NodeUsageGuide.md) · [BuffFlow_NodeReference](../Systems/Rune/BuffFlow_NodeReference.md)

常用：
- 事件源：`OnDamageDealt` / `OnDamageReceived` / `OnCritHit` / `OnKill` / `OnDash` / `OnHealthChanged` / `OnPhaseUpReady` / `OnHeatReachedZero` / `OnPeriodic` / `OnBuffAdded` / `OnBuffRemoved`
- 控制流：`Probability` / `Delay` / `DoOnce` / `IfStatement` / `CompareFloat` / `CompareInt` / `CheckDistance` / `CheckTargetType`
- 副作用：`ApplyEffect` / `ApplyExecution` / `ApplyAttributeModifier` / `AddTag` / `RemoveTag` / `GrantGA` / `GrantTag` / `IncrementPhase` / `DecrementPhase` / `HitStop` / `SacrificeDecay` / `PlayMontage` / `PlayNiagara` / `SpawnGameplayCueOnActor` / `SpawnActorAtLocation` / `SendGameplayEvent` / `WaitGameplayEvent`

## 子图传参

详见 [BuffFlow_CustomInputDataPin](../Systems/Rune/BuffFlow_CustomInputDataPin.md)。模块化拆分 FA 时用 CustomInput DataPin 在父子图之间传值。

## 节点隔离

- BuffFlow 节点（继承 `BFNode_Base`，构造函数自动设 `AllowedAssetClasses = {UNotifyFlowAsset}`）—— 只在 FA 编辑器出现
- LevelFlow 节点（继承 `LENode_Base`）—— 只在 LevelFlow 编辑器出现

不要混用基类。
