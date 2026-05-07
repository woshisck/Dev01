# 512 Rune Card Batch Report
- Mode: Apply
- Generated DA root: `/Game/Docs/BuffDocs/V2-RuneCard/512Generated`
- Generated Flow root: `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow`
- Generated EffectProfile root: `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile`
- Generated sacrifice root: `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice`

## Poison GameplayEffect assets
- Configured `/Game/Code/GAS/Abilities/Shared/GE_Poison`: removed old modifiers, added GEExec_PoisonDamage, duration=5.0s period=1.0s executeOnApply=false stackLimit=20 grantedTag=Buff.Status.Poisoned.
- Configured `/Game/Docs/BuffDocs/Playtest_GA/DeathPoison/GE_PoisonSplash`: removed old modifiers, added GEExec_PoisonDamage, duration=3.0s period=1.0s executeOnApply=true stackLimit=10 grantedTag=Buff.Status.Poisoned.

## Card `DA_Rune512_Burn`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Burn_Base`.
- Configured `FA_Rune512_Burn_Base`: Play Niagara node `Rune.Burn.ApplyNiagara` -> `/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor`, Socket=spine_03, Offset=X=0.000 Y=0.000 Z=6.000, Scale=X=0.280 Y=0.280 Z=0.280, lifetime=3.2, burnDOT=1, cleared Flipbook nodes=0 (updated existing node).
- Verified `FA_Rune512_Burn_Base`: Apply Attribute Modifier has no inline VFX fields; checked nodes=1.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Burn`.
- Updated RuneInfo -> CombatCard.
- Manual check: 确认燃烧 GE 的持续时间、周围伤害半径和护甲减半逻辑。

## Card `DA_Rune512_Poison`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Poison_Base`.
- Configured `FA_Rune512_Poison_Base`: Play Niagara node `Rune.Poison.ApplyNiagara` -> `/Game/Art/EnvironmentAsset/VFX/Niagara/Smoke/NS_Smoke_7_acid`, Socket=spine_02, Offset=X=0.000 Y=0.000 Z=8.000, Scale=X=0.320 Y=0.320 Z=0.320, lifetime=1.2, burnDOT=0, cleared Flipbook nodes=1 (updated existing node).
- Verified `FA_Rune512_Poison_Base`: Apply Attribute Modifier has no inline VFX fields; checked nodes=1.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Poison`.
- Updated RuneInfo -> CombatCard.
- Manual check: 补毒液路径碰撞体与 3 秒爆发 Execution；命中表现使用小尺寸 Play Niagara 节点。

## Card `DA_Rune512_Bleed`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Bleed_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Bleed`.
- Updated RuneInfo -> CombatCard.
- Manual check: 护甲目标不触发流血：如 FA_Effect_Bleed 内未做判断，需要在触发前增加 HasTag(Buff.Status.Armored) 阻断。

## Card `DA_Rune512_Rend`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Rend_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Rend`.
- Updated RuneInfo -> CombatCard.
- Manual check: 护甲目标触发概率下降 25%：基础 GA_Rend 已处理移动掉血和静止消失，概率门槛应在 FA 触发前配置。

## Card `DA_Rune512_Wound`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Wound_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Wound`.
- Updated RuneInfo -> CombatCard.
- Manual check: 护甲目标触发概率下降 25%；伤口额外伤害由 GA_Wound 监听 Ability.Event.Damaged。

## Card `DA_Rune512_Knockback`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Knockback_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Knockback`.
- Updated RuneInfo -> CombatCard.
- Manual check: 已有 GA_KnockbackDebuff 处理 15% 额外护甲伤害；护甲时击退距离减少需要在 GA_Knockback 或专用节点中参数化。

## Card `DA_Rune512_Fear`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Fear_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Fear`.
- Updated RuneInfo -> CombatCard.
- Manual check: 玩家不应获得恐惧：确认 FA_Effect_Fear 的目标筛选为敌人，或在卡牌触发层排除玩家。

## Card `DA_Rune512_Freeze`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Freeze_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Freeze`.
- Updated RuneInfo -> CombatCard.
- Manual check: 确认 FrozenStunEffect 已授予 Buff.Status.Frozen 和 Character.State.Stunned。

## Card `DA_Rune512_Stun`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Stun_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Stun`.
- Updated RuneInfo -> CombatCard.
- Manual check: 需要重点验收霸体交互：霸体免疫眩晕，获得霸体时驱散已有眩晕。

## Card `DA_Rune512_Curse`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Curse_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Curse`.
- Updated RuneInfo -> CombatCard.
- Manual check: 当前 Generic Curse 需要确认是否为“每个负面效果 -7% MaxHealth”；若仍是死亡诅咒版本，需要补负面状态计数逻辑。

## Card `DA_Rune512_Splash`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Splash_Base`.
- Configured `FA_Rune512_Splash_Base`: OnDamageDealt -> MathFloat(*0.2) -> ApplyGEInRadius radius=300, exclude main target.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Splash`.
- Updated RuneInfo -> CombatCard.
- Manual check: Melee weapon reward/deck pools should use Splash instead of Split. Splash flow uses OnDamageDealt -> MathFloat(0.2) -> ApplyGEInRadius.

## Card `DA_Rune512_Split`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Split_Base`.
- Configured `FA_Rune512_Split_Base`: Start -> Spawn Ranged Projectiles, YawOffsets=-8/+8, shared AttackInstanceGuid, removed 0 legacy nodes, no moonblade.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split`.
- Updated RuneInfo -> CombatCard.
- Manual check: Ranged weapon reward/deck pools should use Split. Split base flow only spawns extra musket projectiles; Moonlight split exists only as Moonlight reversed LinkFlow.

## Card `DA_Rune512_Shield`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Shield_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Shield`.
- Updated RuneInfo -> CombatCard.
- Manual check: 确认护甲属性字段、反弹比例和护盾破碎反馈。

## Card `DA_Rune512_Pierce`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Pierce_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Pierce`.
- Updated RuneInfo -> CombatCard.
- Manual check: 在月刃节点中启用 BonusArmorDamageMultiplier / bDestroyOnWorldStaticHit / bForcePureDamage。

## Card `DA_Rune512_Attack`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Attack_Base`.
- Configured `FA_Rune512_Attack_Base`: Apply Attribute Modifier nodes use Combat Card Effect Multiplier; nodes=1.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack`.
- Configured Combo Scaling: bUse=true, ComboScalarPerIndex=0.25, MaxComboScalar=0.5.
- Updated RuneInfo -> CombatCard.
- Manual check: 确认加成数值与战斗属性快照恢复逻辑。

## Card `DA_Rune512_ReduceDamage`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_ReduceDamage_Base`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_ReduceDamage`.
- Updated RuneInfo -> CombatCard.
- Manual check: 补临时减伤 GE 与敌方攻速/移速削弱 GE。

## Card `DA_Rune512_Moonlight_Forward`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Base`.
- Configured `FA_Rune512_Moonlight_Base`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Configured `FA_Rune512_Moonlight_Base`: Combo1=1 projectile, Combo2=2 projectiles, Combo3+=3 projectiles.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Base`.
- Updated EffectProfile `EP_Rune512_Moonlight_Base` from `FA_Rune512_Moonlight_Base` (copied node parameters).
- `FA_Rune512_Moonlight_Base` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Base`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Burn`.
- Configured `FA_Rune512_Moonlight_Forward_Burn`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes, HitEvent=`Action.Rune.MoonlightBurnHit`.
- Configured `FA_Rune512_Moonlight_Forward_Burn`: SpawnSlashWave -> Wait `Action.Rune.MoonlightBurnHit` -> Play Niagara `/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor` -> persistent UGE_RuneBurn DOT; cleared Flipbook nodes=2.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Burn`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Burn` from `FA_Rune512_Moonlight_Forward_Burn` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Burn` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Burn`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Poison`.
- Configured `FA_Rune512_Moonlight_Forward_Poison`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes, HitEvent=`Action.Rune.MoonlightPoisonHit`.
- Configured `FA_Rune512_Moonlight_Forward_Poison`: SpawnSlashWave -> Wait `Action.Rune.MoonlightPoisonHit` -> Play Niagara hit -> Apply GE_Poison x3 -> Play Niagara spread -> ApplyGEInRadius radius=300 max=3; cleared Flipbook nodes=2.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Poison`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Poison` from `FA_Rune512_Moonlight_Forward_Poison` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Poison` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Poison`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Shield`.
- Configured `FA_Rune512_Moonlight_Forward_Shield`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Shield`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Shield` from `FA_Rune512_Moonlight_Forward_Shield` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Shield` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Shield`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Pierce`.
- Configured `FA_Rune512_Moonlight_Forward_Pierce`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Pierce`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Pierce` from `FA_Rune512_Moonlight_Forward_Pierce` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Pierce` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Pierce`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Attack`.
- Configured `FA_Rune512_Moonlight_Forward_Attack`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Attack`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Attack` from `FA_Rune512_Moonlight_Forward_Attack` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Attack` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Attack`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_ReduceDamage`.
- Configured `FA_Rune512_Moonlight_Forward_ReduceDamage`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_ReduceDamage`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_ReduceDamage` from `FA_Rune512_Moonlight_Forward_ReduceDamage` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_ReduceDamage` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_ReduceDamage`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Burn`.
- `FA_Rune512_Moonlight_Reversed_Burn` uses Rune Ground Path; slash-wave projectile configuration is bypassed.
- Configured `FA_Rune512_Moonlight_Reversed_Burn`: Start -> Calc Rune Ground Path Transform -> Spawn Rune Ground Path Effect, Facing=ToLastDamageTarget, Policy=EnemiesOnly, Shape=Fan, Effect=GE_RuneBurn, Length=520, Width=230, Duration=4.0, Tick=0.5, Damage=6.0, DecalPlaneRot=0, LocationPin=1, RotationPin=1; legacy slash-wave disconnected.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Burn`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Burn` from `FA_Rune512_Moonlight_Reversed_Burn` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Burn` now executes Spawn Rune Area Profile `EP_Rune512_Moonlight_Reversed_Burn`; LocationPin=1 RotationPin=1, legacy ground-path node retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Poison`.
- `FA_Rune512_Moonlight_Reversed_Poison` uses Rune Ground Path; slash-wave projectile configuration is bypassed.
- Configured `FA_Rune512_Moonlight_Reversed_Poison`: Start -> Calc Rune Ground Path Transform -> Spawn Rune Ground Path Effect, Facing=ToLastDamageTarget, Policy=EnemiesOnly, Shape=Rectangle, Effect=GE_Poison_C, Length=560, Width=210, Duration=4.5, Tick=1.0, Damage=0.0, DecalPlaneRot=0, LocationPin=1, RotationPin=1; legacy slash-wave disconnected.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Poison`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Poison` from `FA_Rune512_Moonlight_Reversed_Poison` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Poison` now executes Spawn Rune Area Profile `EP_Rune512_Moonlight_Reversed_Poison`; LocationPin=1 RotationPin=1, legacy ground-path node retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Shield`.
- Configured `FA_Rune512_Moonlight_Reversed_Shield`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Shield`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Shield` from `FA_Rune512_Moonlight_Reversed_Shield` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Shield` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Shield`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Pierce`.
- Configured `FA_Rune512_Moonlight_Reversed_Pierce`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Pierce`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Pierce` from `FA_Rune512_Moonlight_Reversed_Pierce` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Pierce` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Pierce`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Attack`.
- Configured `FA_Rune512_Moonlight_Reversed_Attack`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Attack`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Attack` from `FA_Rune512_Moonlight_Reversed_Attack` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Attack` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Attack`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_ReduceDamage`.
- Configured `FA_Rune512_Moonlight_Reversed_ReduceDamage`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_ReduceDamage`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_ReduceDamage` from `FA_Rune512_Moonlight_Reversed_ReduceDamage` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_ReduceDamage` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_ReduceDamage`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Split`.
- Configured `FA_Rune512_Moonlight_Reversed_Split`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Split`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Split` from `FA_Rune512_Moonlight_Reversed_Split` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Split` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Split`; legacy slash-wave node is retained but disconnected.
- Updated RuneInfo -> CombatCard.
- Manual check: Forward/Reversed 配方 Flow 已按模板复制，复杂节点连接与表现节点参数需要按配置文档检查。

## Card `DA_Rune512_Moonlight_Reversed`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Base`.
- Configured `FA_Rune512_Moonlight_Base`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Configured `FA_Rune512_Moonlight_Base`: Combo1=1 projectile, Combo2=2 projectiles, Combo3+=3 projectiles.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Base`.
- Updated EffectProfile `EP_Rune512_Moonlight_Base` from `FA_Rune512_Moonlight_Base` (copied node parameters).
- `FA_Rune512_Moonlight_Base` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Base`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Reversed`.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Burn`.
- Configured `FA_Rune512_Moonlight_Forward_Burn`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes, HitEvent=`Action.Rune.MoonlightBurnHit`.
- Configured `FA_Rune512_Moonlight_Forward_Burn`: SpawnSlashWave -> Wait `Action.Rune.MoonlightBurnHit` -> Play Niagara `/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor` -> persistent UGE_RuneBurn DOT; cleared Flipbook nodes=2.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Burn`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Burn` from `FA_Rune512_Moonlight_Forward_Burn` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Burn` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Burn`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Poison`.
- Configured `FA_Rune512_Moonlight_Forward_Poison`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes, HitEvent=`Action.Rune.MoonlightPoisonHit`.
- Configured `FA_Rune512_Moonlight_Forward_Poison`: SpawnSlashWave -> Wait `Action.Rune.MoonlightPoisonHit` -> Play Niagara hit -> Apply GE_Poison x3 -> Play Niagara spread -> ApplyGEInRadius radius=300 max=3; cleared Flipbook nodes=2.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Poison`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Poison` from `FA_Rune512_Moonlight_Forward_Poison` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Poison` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Poison`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Shield`.
- Configured `FA_Rune512_Moonlight_Forward_Shield`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Shield`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Shield` from `FA_Rune512_Moonlight_Forward_Shield` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Shield` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Shield`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Pierce`.
- Configured `FA_Rune512_Moonlight_Forward_Pierce`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Pierce`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Pierce` from `FA_Rune512_Moonlight_Forward_Pierce` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Pierce` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Pierce`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Attack`.
- Configured `FA_Rune512_Moonlight_Forward_Attack`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Attack`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_Attack` from `FA_Rune512_Moonlight_Forward_Attack` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_Attack` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_Attack`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_ReduceDamage`.
- Configured `FA_Rune512_Moonlight_Forward_ReduceDamage`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_ReduceDamage`.
- Updated EffectProfile `EP_Rune512_Moonlight_Forward_ReduceDamage` from `FA_Rune512_Moonlight_Forward_ReduceDamage` (copied node parameters).
- `FA_Rune512_Moonlight_Forward_ReduceDamage` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Forward_ReduceDamage`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Burn`.
- `FA_Rune512_Moonlight_Reversed_Burn` uses Rune Ground Path; slash-wave projectile configuration is bypassed.
- Configured `FA_Rune512_Moonlight_Reversed_Burn`: Start -> Calc Rune Ground Path Transform -> Spawn Rune Ground Path Effect, Facing=ToLastDamageTarget, Policy=EnemiesOnly, Shape=Fan, Effect=GE_RuneBurn, Length=520, Width=230, Duration=4.0, Tick=0.5, Damage=6.0, DecalPlaneRot=0, LocationPin=1, RotationPin=1; legacy slash-wave disconnected.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Burn`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Burn` from `FA_Rune512_Moonlight_Reversed_Burn` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Burn` now executes Spawn Rune Area Profile `EP_Rune512_Moonlight_Reversed_Burn`; LocationPin=1 RotationPin=1, legacy ground-path node retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Poison`.
- `FA_Rune512_Moonlight_Reversed_Poison` uses Rune Ground Path; slash-wave projectile configuration is bypassed.
- Configured `FA_Rune512_Moonlight_Reversed_Poison`: Start -> Calc Rune Ground Path Transform -> Spawn Rune Ground Path Effect, Facing=ToLastDamageTarget, Policy=EnemiesOnly, Shape=Rectangle, Effect=GE_Poison_C, Length=560, Width=210, Duration=4.5, Tick=1.0, Damage=0.0, DecalPlaneRot=0, LocationPin=1, RotationPin=1; legacy slash-wave disconnected.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Poison`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Poison` from `FA_Rune512_Moonlight_Reversed_Poison` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Poison` now executes Spawn Rune Area Profile `EP_Rune512_Moonlight_Reversed_Poison`; LocationPin=1 RotationPin=1, legacy ground-path node retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Shield`.
- Configured `FA_Rune512_Moonlight_Reversed_Shield`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Shield`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Shield` from `FA_Rune512_Moonlight_Reversed_Shield` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Shield` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Shield`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Pierce`.
- Configured `FA_Rune512_Moonlight_Reversed_Pierce`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Pierce`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Pierce` from `FA_Rune512_Moonlight_Reversed_Pierce` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Pierce` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Pierce`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Attack`.
- Configured `FA_Rune512_Moonlight_Reversed_Attack`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Attack`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Attack` from `FA_Rune512_Moonlight_Reversed_Attack` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Attack` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Attack`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_ReduceDamage`.
- Configured `FA_Rune512_Moonlight_Reversed_ReduceDamage`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_ReduceDamage`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_ReduceDamage` from `FA_Rune512_Moonlight_Reversed_ReduceDamage` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_ReduceDamage` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_ReduceDamage`; legacy slash-wave node is retained but disconnected.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Split`.
- Configured `FA_Rune512_Moonlight_Reversed_Split`: Moonlight slash-wave nodes=1, projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes.
- Found existing EffectProfile `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Reversed_Split`.
- Updated EffectProfile `EP_Rune512_Moonlight_Reversed_Split` from `FA_Rune512_Moonlight_Reversed_Split` (copied node parameters).
- `FA_Rune512_Moonlight_Reversed_Split` now executes Spawn Rune Projectile Profile `EP_Rune512_Moonlight_Reversed_Split`; legacy slash-wave node is retained but disconnected.
- Updated RuneInfo -> CombatCard.
- Manual check: Forward/Reversed 配方 Flow 已按模板复制，复杂节点连接与表现节点参数需要按配置文档检查。

## Sacrifice passive `DA_Rune512_Sacrifice_MoonlightShadow`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/Flow/FA_Rune512_Sacrifice_MoonlightShadow`.
- Configured `FA_Rune512_Sacrifice_MoonlightShadow`: hidden passive grant node `MoonlightShadow`, removed 0 template nodes.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_MoonlightShadow`.
- Updated sacrifice RuneInfo: empty Shape, passive TriggerType, non-combat-card.

## Sacrifice passive `DA_Rune512_Sacrifice_ShadowMark`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/Flow/FA_Rune512_Sacrifice_ShadowMark`.
- Configured `FA_Rune512_Sacrifice_ShadowMark`: hidden passive grant node `ShadowMark`, removed 0 template nodes.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_ShadowMark`.
- Updated sacrifice RuneInfo: empty Shape, passive TriggerType, non-combat-card.

## Sacrifice passive `DA_Rune512_Sacrifice_GiantSwing`
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/Flow/FA_Rune512_Sacrifice_GiantSwing`.
- Configured `FA_Rune512_Sacrifice_GiantSwing`: hidden passive grant node `GiantSwing`, removed 0 template nodes.
- Found existing asset `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_GiantSwing`.
- Updated sacrifice RuneInfo: empty Shape, passive TriggerType, non-combat-card.

## Production weapon combo config
- Assigned `/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword` -> GameplayAbilityComboGraph `/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test`.
- Assigned `/Game/Code/Weapon/GreatSword/DA_WPN_RedSword` -> GameplayAbilityComboGraph `/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test`.
## Production weapon combat card pool config
- Updated `/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword`: replace melee Split -> `DA_Rune512_Splash` in InitialCombatDeck/InitialRunes.
- Updated `/Game/Code/Weapon/GreatSword/DA_WPN_RedSword`: replace melee Split -> `DA_Rune512_Splash` in InitialCombatDeck/InitialRunes.
- Updated `/Game/Code/Weapon/Harquebus/DA_WPN_Harquebus`: replace ranged Splash -> `DA_Rune512_Split` in InitialCombatDeck/InitialRunes.

## Generic Rune status card integration
- Bleed, Rend, Wound, Knockback, Fear, Freeze, Stun, and Curse are generated as Normal combat cards.
- These cards reuse Playtest_GA/RuneBaseEffect FA templates. Tune the underlying Generic Rune GA/GE when the status rule itself needs to change.
- Bloodvine remains design-only; no dedicated 512 card is generated in this pass.

## FA VFX todos
- Card VFX is configured in each BaseFlow/LinkFlow, not on CombatCard data.
- Moonlight generated slash-wave nodes clear inline Niagara fields and use BP/default projectile visuals; hit/status visuals should be independent atomic VFX nodes.
- Moonlight base and forward LinkFlows use combo projectiles as sequential same-path shots: Cone=0, Sequential=true, Interval=0.12, MaxBonus=2.
- Moonlight projectile/area tuning is mirrored into EffectProfile assets; new profile nodes execute the copied profile while legacy nodes are retained for parameter comparison.
- Moonlight forward poison LinkFlow is configured as atomic nodes: projectile event -> Wait Gameplay Event -> Play Niagara -> ApplyEffect -> Play Niagara -> ApplyGEInRadius.
- Moonlight forward burn LinkFlow is configured as atomic nodes: projectile event -> Wait Gameplay Event -> Play Niagara attached to enemy socket -> persistent UGE_RuneBurn.
- Moonlight reversed poison/burn LinkFlows use Spawn Rune Ground Path Effect and disconnect legacy slash-wave execution.
- Burn/Poison base VFX use compact Play Niagara nodes; stale Flipbook nodes are cleared when found.
- Projectile inline Niagara fields remain empty. Link/status VFX must live in independent FA visual nodes.
- Projectile visuals stay on projectile-spawn nodes; hit/status visuals should use separate visual nodes.
- Splash/Split are one weapon-adaptive card family: both variants share Card.ID.SplashSplit and Card.Effect.SplashSplit; melee uses Splash BaseFlow, ranged uses Split BaseFlow; Moonlight Split only exists as reversed LinkFlow.
- Any Flow copied from a template must be opened once and checked against the 512 design doc before gameplay signoff.