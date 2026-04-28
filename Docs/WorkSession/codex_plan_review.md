## Codex 审查结果

### 发现的问题
- `YogTargetType_Melee` 不能“保持不变”：当前逻辑优先从 `EventData.OptionalObject` cast `UAN_MeleeDamage` 并调用 `BuildActionData()`；方案删除 `BuildActionData()` 后会编译失败或命中数据取不到。
- `OnHitEventTags` 保留在 AN，但方案又删除 `PendingOnHitEventTags`，未说明新传递路径；当前广播逻辑依赖角色暂存数组，删除后镜头抖动、音效等命中事件会丢失。
- `HitStopMode` 移到 DA 后没有完整执行链路；当前 `BFNode_HitStop` 读取的是 `Owner->PendingHitStopOverride`，该值由 AN 写入，方案删除 AN 字段后 HitStop 配置会变成未消费数据。
- `StatAfterATKEffect` 未被完整纳入新数据模型；现有 `EndAbility` 使用 `LastFiredDamageNotify` 或 `CachedDamageNotify` 读取攻击后摇数值，方案移除 notify 数值后需要明确使用最后一次 `FMeleeHitConfig` 或首段 fallback。
- `GetHit()` 设计存在空数组风险：说明写“越界返回 `Hits[0]` 避免 crash”，但 `Hits` 为空时仍会崩溃，需要默认配置或显式失败处理。
- `ActivateAbility` 当前只监听硬编码 `GameplayEffect.DamageType.GeneralAttack`，方案保留 `AN_MeleeDamage.EventTag` 但不再扫描 notify；如果 AN 配了其他 `EventTag`，GA 不会收到事件。
- `CurrentHitConfig` 只在 `OnEventReceived` 开头赋值，但方案未要求在激活时初始化为 `GetHit(0)`；无命中、fallback 路径、攻击后摇或调试读取时可能拿到默认构造值而非配置值。
- `Ability.Event.Attack.Hit` payload 目前只携带 `Instigator` 和 `Target`，方案没有补充 `HitIndex`、攻击 Tag、配置引用或本次伤害上下文；符文想按“某次攻击/某段命中”触发时信息不足。

### 潜在风险
- 直接删除 AN 旧字段会让现有蒙太奇中的序列化配置不可逆丢失，不利于批量迁移和回滚。
- `AttackConfig` 放在 GA Blueprint Class Defaults，而蒙太奇仍从 `CharacterData.AbilityData` 按 AbilityTag 获取，可能出现“GA 配置”和“蒙太奇配置”不一致。
- 每次命中 remove/apply `StatBeforeATKEffect` 通常可接受，但如果 GE 有触发器、Cue、复制或依赖堆叠规则，可能产生额外副作用。
- 将符文效果改成监听 `Ability.Event.Attack.Hit` 不自动保证 GAS 生命周期正确；grant GA 仍需要明确结束条件，Duration GE 也要确认施加对象和移除策略。
- 存量 `NotifyRuneDataAsset` / `NotifyFlowAsset` 是专为 `AdditionalRuneEffects` 设计的资产类型，方案没有说明保留、废弃或迁移到普通 Rune/BuffFlow 的兼容策略。
- 多段攻击如果多个 AN 使用相同 `HitIndex` 或 DA `Hits` 数量少于 AN 数量，方案目前只 fallback，不会暴露配置错误，容易产生静默错配。

### 改进建议
- 明确修改 `YogTargetType_Melee::GetActionData`：当 OptionalObject 是 `UAN_MeleeDamage` 时只读取 `HitIndex`，再从当前 `UGA_MeleeAttack` 获取对应 `FMeleeHitConfig`。
- 在 `OnEventReceived` 中维护 `CurrentHitConfig`、`LastHitConfig` 和可选 `FirstHitConfig`，分别服务命中检测、攻击后摇和无命中 fallback。
- 将 `OnHitEventTags` 改为直接从 `FiredNotify->OnHitEventTags` 广播，或通过事件 payload 携带，避免继续依赖角色级暂存数组。
- 为 HitStop 增加明确路径：`OnEventReceived` 读取 `CurrentHitConfig.HitStopMode` 后写入 `PendingHitStopOverride`，或重构 `BFNode_HitStop` 直接读取当前 GA/事件上下文。
- `GetHit()` 建议返回指针/optional，或在 DA 中提供 `DefaultHitConfig`；空 `Hits` 应打印错误并终止本次攻击或使用显式默认值。
- 给 `Ability.Event.Attack.Hit` payload 增加 `EventMagnitude` / `OptionalObject` / `OptionalObject2` 或自定义上下文，至少包含 `HitIndex`、攻击 AbilityTag、SourceAbility、Target。
- 迁移期保留旧字段为 deprecated，并提供一次性迁移工具或编辑器检查；确认全部资产迁移后再删除字段。
- 增加自动化/半自动化校验：检查每个近战 GA 是否配置 `AttackConfig`、每个蒙太奇 AN 的 `HitIndex` 是否越界、每个 DA 是否至少有一个 Hit。

### 需要向用户确认的问题
- 是否接受迁移期保留 AN 旧字段一版，用于资产迁移和回滚？
- `AttackConfig` 应配置在 GA Blueprint 上，还是放回 `AbilityData` 与蒙太奇同源管理？
- 符文触发是否需要区分具体 `HitIndex`、攻击 AbilityTag、目标类型或伤害数值？
- `OnHitEventTags` 只用于表现事件，还是也存在 gameplay 逻辑依赖？
- `StatAfterATKEffect` 应使用最后一次命中的 Hit 配置，还是固定使用第一段/整套攻击的专用配置？
- 敌人攻击、玩家攻击、派生攻击类是否都要统一迁移到同一套 `UMeleeAttackConfigDA`？