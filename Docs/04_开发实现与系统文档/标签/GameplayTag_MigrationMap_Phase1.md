# GameplayTag 清理迁移表 Phase 1

> 日期：2026-06-23
> 状态：第一批低风险清理 + 第二批运行时双读兼容。已新增正式 Tag，旧 Tag 暂不删除。
> 依据：[GameplayTag_ReorgPlan_LOTF.md](GameplayTag_ReorgPlan_LOTF.md)

## 一、当前执行策略

本阶段只做低风险清理：

1. 新增正式 Tag，让新资产可以开始使用新命名。
2. 关键运行时代码开始双读新旧 Tag，让新资产迁移到正式命名后不会立即失效。
3. 保留旧 Tag，避免当前 C++、Blueprint、DataAsset、存档引用立即失效。
4. 暂不写入最终 Redirect。原因是旧 Tag 仍然定义在配置中，并且资产尚未统一迁移；Redirect 应在“旧定义删除/资产迁移”阶段和删除动作一起提交。

后续正式重命名时，顺序应为：

```text
新增新 Tag -> 代码兼容新旧 Tag -> 资产迁移 -> 删除旧 Tag 定义 -> 添加 GameplayTagRedirects -> 静态扫描确认旧引用归零
```

## 二、本阶段新增的配置文件

| 文件 | 内容 |
| --- | --- |
| `Config/Tags/CharacterTag.ini` | `Character.State.*` 正式角色动作/运行时状态 |
| `Config/Tags/RuneTag.ini` | `Rune.Type/Rarity/Element/Activation/Binding/Trigger/FlowRole/Effect/ID.*` |
| `Config/Tags/DirectorTag.ini` | `Director.*` 导演系统调度 Tag |
| `Config/Tags/GameStateFlags.ini` | `GameState.Flags.*` 存档/教程/任务 Flag 根 |

## 三、核心迁移映射

### 3.1 PlayerState.AbilityCast -> Character.State

| 旧 Tag | 新 Tag |
| --- | --- |
| `PlayerState.AbilityCast.Attack` | `Character.State.Skill.Attack` |
| `PlayerState.AbilityCast.Attack.Combo1` | `Character.State.Skill.Attack.Combo1` |
| `PlayerState.AbilityCast.Attack.Combo2` | `Character.State.Skill.Attack.Combo2` |
| `PlayerState.AbilityCast.Attack.Combo3` | `Character.State.Skill.Attack.Combo3` |
| `PlayerState.AbilityCast.Attack.Combo4` | `Character.State.Skill.Attack.Combo4` |
| `PlayerState.AbilityCast.WeaponSkill` | `Character.State.Skill.WeaponSkill` |
| `PlayerState.AbilityCast.WeaponSkill.Combo1` | `Character.State.Skill.WeaponSkill.Combo1` |
| `PlayerState.AbilityCast.WeaponSkill.Combo2` | `Character.State.Skill.WeaponSkill.Combo2` |
| `PlayerState.AbilityCast.WeaponSkill.Combo3` | `Character.State.Skill.WeaponSkill.Combo3` |
| `PlayerState.AbilityCast.WeaponSkill.Combo4` | `Character.State.Skill.WeaponSkill.Combo4` |
| `PlayerState.AbilityCast.Skill` | `Character.State.Skill.Active` |
| `PlayerState.AbilityCast.Skill.Skill1` | `Character.State.Skill.Active.Skill1` |
| `PlayerState.AbilityCast.Skill.Skill2` | `Character.State.Skill.Active.Skill2` |
| `PlayerState.AbilityCast.Dash` | `Character.State.Movement.Dash` |
| `PlayerState.AbilityCast.Dash.Combo1` | `Character.State.Movement.Dash.Combo1` |
| `PlayerState.AbilityCast.Dash.Combo2` | `Character.State.Movement.Dash.Combo2` |
| `PlayerState.AbilityCast.Dash.Combo3` | `Character.State.Movement.Dash.Combo3` |
| `PlayerState.AbilityCast.Dash.Combo4` | `Character.State.Movement.Dash.Combo4` |
| `PlayerState.AbilityCast.Reload` | `Character.State.Skill.Reload` |
| `PlayerState.AbilityCast.CanCombo` | `Character.State.Window.CanCombo` |
| `PlayerState.AbilityCast.PostAttackRecovery` | `Character.State.Window.PostAttackRecovery` |
| `PlayerState.AbilityCast.SwitchWeapon` | `Character.State.Equipment.SwitchWeapon` |
| `PlayerState.Block.Idle` | `Character.State.Block.Idle` |
| `PlayerState.Block.Start` | `Character.State.Block.Start` |

Deprecated compatibility tags such as `PlayerState.AbilityCast.LightAtk.*`、`HeavyAtk.*`、`Special*`、`Finisher*` should remain legacy-only until their assets are fully retired.

### 3.2 Card / Combo -> Rune

| 旧 Tag | 新 Tag |
| --- | --- |
| `Card.ID.*` | `Rune.ID.*` |
| `Card.Effect.*` | `Rune.Effect.*` |
| `Combo.CombatDeck.ActionSlot.Attack` | `Rune.Binding.Action.Attack` |
| `Combo.CombatDeck.ActionSlot.Skill` | `Rune.Binding.Action.Skill` |
| `Combo.CombatDeck.ActionSlot.WeaponSkill` | `Rune.Binding.Action.WeaponSkill` |
| `Combo.CombatDeck.ActionSlot.Dash` | `Rune.Binding.Action.Dash` |
| `Combo.CombatDeck.FlowRole.Starter` | `Rune.FlowRole.Starter` |
| `Combo.CombatDeck.FlowRole.Catalyst` | `Rune.FlowRole.Catalyst` |
| `Combo.CombatDeck.FlowRole.Finisher` | `Rune.FlowRole.Finisher` |
| `Combo.TriggerTiming.OnCommit` | `Rune.Trigger.OnCommit` |
| `Combo.TriggerTiming.OnHit` | `Rune.Trigger.OnHit` |

`Rune.FlowRole.Finisher` 只表示普通组合终结位，不表示旧 QTE Finisher 系统。

正式命名不使用 `Rune.Deck.*`。Deck/CombatDeck 只表示旧运行时容器或组件名；GameplayTag 用 `Rune.Binding.*`、`Rune.Activation.*`、`Rune.Trigger.*`、`Rune.FlowRole.*`、`Rune.Effect.*` 表达符文语义。

### 3.3 Buff.Rune -> Rune

| 旧 Tag | 新 Tag |
| --- | --- |
| `Buff.Rune.Type.*` | `Rune.Type.*` |
| `Buff.Rune.Rarity.*` | `Rune.Rarity.*` |
| `Buff.Rune.Element.*` | `Rune.Element.*` |

### 3.4 Character.State sustained status -> Buff.Status

| Old Tag | New Tag |
| --- | --- |
| `Character.State.Feared` | `Buff.Status.Feared` |
| `Character.State.Frozen` | `Buff.Status.Frozen` |
| `Character.State.Stunned` | `Buff.Status.Stunned` |
| `Character.State.SuperArmor` | `Buff.Status.SuperArmor` |

These are sustained buff/debuff/status-effect semantics, not momentary character action states. Keep the old tags only for compatibility until assets are migrated and resaved.

## 四、盘点结论

当前 Config 根节点数量最多的旧根：

| 根 | 数量 | 说明 |
| --- | ---: | --- |
| `PlayerState` | 76 | 玩家动作状态和旧兼容 Tag 混在一起，需要分阶段迁移 |
| `Card` | 41 | 已并入 Rune，后续应迁移为 `Rune.ID.*` / `Rune.Effect.*` |
| `Combo` | 9 | 原 CombatDeck 槽位/角色/触发时机，应迁移到 Rune 语义 |
| `Buff.Rune` | 14 | 符文身份信息误放在 Buff 下，应迁移到 Rune |

现有重复定义主要来自旧项目已经同时在 `DefaultGameplayTags.ini` 和 `Config/Tags/*.ini` 中声明相同 Tag。本阶段新增的正式 Tag 没有新增这些重复。

## 五、下一阶段

已落地的代码兼容层：

| 范围 | 处理 |
| --- | --- |
| 核心输入链 | Attack / WeaponSkill / Dash / Reload / SwitchWeapon 同时激活新旧 Tag |
| 核心 GA | Attack / WeaponSkill / Dash / Musket 攻击/换弹 使用正式 `Character.State.*` ActivationOwnedTags；旧 `PlayerState.AbilityCast.*` 仅保留为 AbilityTags / fallback aliases |
| AbilityData 默认键/查询 | 新建/加载 AbilityData 时预置 `Character.State.*` 正式键，同时保留旧键；查询蒙太奇/配置时支持 `Character.State.*` 和旧 `PlayerState.AbilityCast.*` fallback |
| Combo index 解析 | BuffFlow 先识别 `Character.State.*.ComboN`，再 fallback 到旧 `PlayerState.AbilityCast.*.ComboN` |
| Combo/Recovery 窗口 | AnimNotify 同时写入 `Character.State.Window.*` 和旧 `PlayerState.AbilityCast.*` 窗口 Tag |
| 近战 CanCombo 监听 | `GA_MeleeAttack` 同时监听/清理 `Character.State.Window.CanCombo` 和旧 `PlayerState.AbilityCast.CanCombo`，避免新窗口 Tag 资产无法触发缓冲取消 |
| 通用蒙太奇 CanCombo 监听 | `GA_PlayMontage` 同时监听/清理 `Character.State.Window.CanCombo` 和旧 `PlayerState.AbilityCast.CanCombo`，缓冲激活优先新 `Character.State.*` 再 fallback 旧名 |
| 近战动作槽/Combo 解析 | `GA_MeleeAttack` 的 CombatDeck 上下文、缓冲激活和 PreferredAbilityTags 优先识别 `Character.State.*`，再 fallback 到旧 `PlayerState.AbilityCast.*` |
| 伤害日志/拆分 | `DamageExecution` 的玩家动作名识别优先读取 `Character.State.*`，兼容旧 `PlayerState.AbilityCast.*` |
| 武器动作槽 UI | 先识别 `Character.State.*`，再 fallback 到旧 `PlayerState.AbilityCast.*` |
| SwitchWeapon | GA 同时挂 `Character.State.Equipment.SwitchWeapon` 和旧 `PlayerState.AbilityCast.SwitchWeapon` |
| Dash 充能键 | `SkillChargeComponent` 支持 alias；Dash 正式注册/查询键迁移到 `Character.State.Movement.Dash`，旧 `PlayerState.AbilityCast.Dash` 查询和 `OnChargeChanged` 广播仍映射同一充能状态 |
| ActiveSkill | ShieldBurst 能力身份使用 `Character.State.Skill.Active`，持续增益状态使用 `Buff.Status.ActiveSkill.ShieldBurst` |
| Rune/Card 查询 | 重击、月光、新手教程、物品火焰判定支持 `Rune.*` 和旧 `Card.*` 双读 |
| BuffFlow/LinkRecipe 编辑器筛选与匹配 | CombatCardContext 节点与 RuneDataAsset LinkRecipe 的 ID/Effect 选择器改为 `Rune.ID` / `Rune.Effect`，运行时匹配支持 `Rune.*` 和旧 `Card.*` 等价判断 |
| 策划配置文档 Rune 命名 | 非终结技符文制作手册中的 `Card.ID.*` / `Card.Effect.*` 示例已迁移为 `Rune.ID.*` / `Rune.Effect.*`；`CardIdTag` 等字段名暂保留资产序列化兼容 |
| 文档/注释默认示例 | `GameplayTag_MasterGuide`、`Tag_SituationalGuide`、`GA_TagFields_Guide`、`StateConflict_TagBlock`、编码规范和关键 GA 头文件示例已改为 `Character.State.*` / `Rune.*`；旧名只作为兼容说明出现 |
| 自动化回归 | `DevKit.GameplayTags.AbilityData*Fallback*` 覆盖 `Character.State.*` 与旧 `PlayerState.AbilityCast.*` 的 AbilityData 蒙太奇查询互通 |

后续清理顺序：

1. 迁移 Blueprint/DataAsset 中的 `Card.*`、`Combo.*`、`PlayerState.AbilityCast.*` 到正式 Tag。
2. 扫描确认旧引用只剩兼容路径/归档文档。
3. 删除旧 Tag 定义。
4. 添加最终 `GameplayTagRedirects`。
5. 重新保存资产并做编辑器内验证。

## 2026-06-23 追加记录

- `Config/DefaultGameplayTags.ini` 中的 `Card.ID.*` / `Card.Effect.*` 已统一标注为 deprecated compatibility，正式写法使用 `Rune.ID.*` / `Rune.Effect.*`。旧标签暂不删除，保留给旧资产、旧存档和兼容测试加载。
- `Config/Tags/BuffTag.ini` 中的 `Buff.Rune.Type.*` / `Buff.Rune.Rarity.*` / `Buff.Rune.Element.*` 已统一标注为 deprecated compatibility，正式写法使用 `Rune.Type.*` / `Rune.Rarity.*` / `Rune.Element.*`。旧标签暂不删除，等待资产迁移和重保存后再配合 Redirect 清理。
- `UGA_ActiveSkill_ShieldBurst` 的能力身份使用 `Character.State.Skill.Active` / `Character.State.Skill.Active.ShieldBurst`；持续 60 秒的增益状态只使用 `Buff.Status.ActiveSkill.ShieldBurst`。不要把持续 Buff 期标成 `Character.State.*`，否则会和角色动作状态/阻断规则混淆。
- `Config/Tags/PlayerGameplayTag.ini` 和 `Config/DefaultGameplayTags.ini` 中仍保留的 `PlayerState.AbilityCast.*` / `PlayerState.Block.*` 已统一标注为 deprecated compatibility，正式动作/窗口/格挡状态使用 `Character.State.*`。
- `Config/DefaultGameplayTags.ini` 中的 `Rune.Card.*` 已标注为 deprecated compatibility。正式 Rune 语义不要继续新增 `Rune.Card.*`，应根据用途拆到 `Rune.ID.*`、`Rune.Effect.*`、`Rune.Type.*`、`Rune.Binding.*` 或 `Rune.FlowRole.*`。

## 2026-06-23 Runtime cleanup note

- `UGA_WeaponSkill` now takes the `Character.State.Skill.WeaponSkill.Combo1-4` slots as the primary combo tag source and derives the legacy `PlayerState.AbilityCast.WeaponSkill.Combo1-4` tags only for compatibility.
- Blocking runtime state is dual-written by `UGA_PlayMontage`: `Character.State.Block.Start/Idle` are the formal state tags, while `PlayerState.Block.Start/Idle` remain compatibility tags.
- Physical damage block detection now reads both `Character.State.Block.*` and legacy `PlayerState.Block.*`, so newly authored assets can rely on the formal state tags without breaking old assets.
- Automation coverage now includes formal `Character.State.*` and `Rune.*` tag existence checks, while legacy `PlayerState.AbilityCast.*` and `Card.*` test references remain compatibility coverage rather than the only tested path.
- Editor generation tools have been moved to formal tags: `PlayerAbilityMontageDataSetup` now writes `Character.State.*` montage keys, `RuneCardBatchGenerator` now writes `Rune.ID.*` / `Rune.Effect.*`, and the Rune editor UI hints point authors to `Rune.*`.
- `CombatMontageSync` accepts new `Rune.Binding.*` / `Rune.FlowRole.*` / `Rune.Trigger.*` tags while still reading old `Combo.*` graph tags. It writes `Character.State.Window.CanCombo` and removes old `PlayerState.AbilityCast.CanCombo` notifies during sync.
- Static Content asset scan report: `Docs/GeneratedReports/GameplayTagContentAssetScan.md`. Current byte scan found 79 assets with old tag strings, mostly AbilityData, old Light/Heavy GA templates, TwoHandedSword combat cards, and generated Rune/Profile assets. These assets must be migrated and resaved before deleting old tag definitions or adding final redirects.
- Musket attack/weapon-skill/reload GAs now hold formal `Character.State.Skill.Attack`, `Character.State.Skill.WeaponSkill`, or `Character.State.Skill.Reload` in `ActivationOwnedTags` while keeping legacy `PlayerState.AbilityCast.*` tags for compatibility. This lets HUD, StateConflict, debug tools, and AI queries read the formal state during ranged actions.
- Native player and Musket combat GAs keep legacy `PlayerState.AbilityCast.*` only as `AbilityTags` / fallback activation aliases. They should not add those legacy tags to `ActivationOwnedTags`; current-state consumers such as HUD buff/state display, StateConflict block rules, AI queries, and debug UI should read the formal `Character.State.*` tags.
- Player input activation now tries the formal action tag first (`Character.State.Skill.Attack`, `Character.State.Skill.WeaponSkill`, `Character.State.Movement.Dash`, `Character.State.Skill.Reload`, `Character.State.Equipment.SwitchWeapon`) and falls back to the matching legacy `PlayerState.AbilityCast.*` tag only if no formal-tag ability activates. New assets no longer need to carry legacy input tags to be triggerable.
- Dash and SwitchWeapon cancellation now include formal action-state ranges (`Character.State.Skill`, `Character.State.Movement.Dash`, `Character.State.Equipment.SwitchWeapon`) in addition to the legacy `PlayerState.AbilityCast` root. This keeps action interruption behavior intact after abilities stop relying on the old root.
- AbilityData default key seeding now creates only formal `Character.State.*` action keys. Legacy `PlayerState.AbilityCast.*`, `LightAtk`, `HeavyAtk`, and old Dash aliases remain readable through `GetMontage` / `HasAbility` fallback, but new assets should not receive legacy empty montage slots.
- Runtime heavy-card normalization now adds only `Rune.Effect.Attack`; legacy `Card.Effect.Attack` remains readable through equivalent-tag matching but is no longer written as a new runtime effect tag.
- CombatDeck link recipe neighbor-ID matching normalizes legacy `Card.ID.*` card asset values to formal `Rune.ID.*` before building the runtime match container. Recipe requirements still accept either spelling through equivalent-tag matching, but runtime-authored temporary containers should prefer the formal Rune tag.
- Removed duplicate formal `Rune.ID.Burn`, `Rune.ID.Moonlight`, and `Rune.ID.Poison` declarations from `Config/DefaultGameplayTags.ini`; their authoritative definitions now live in `Config/Tags/RuneTag.ini`. Older compatibility duplicates are left in place until asset migration/resave is complete.
- Config tag dictionary ownership has been narrowed: duplicated `Action.*`, `Ability.Event.*`, `Buff.*`, and legacy `PlayerState.*` declarations were removed from `Config/DefaultGameplayTags.ini` when the same tag already existed in `Config/Tags/PlayerGameplayTag.ini` or `Config/Tags/BuffTag.ini`. The tags still exist for compatibility; they now have a single config owner.
- Remaining compatibility tag ownership was moved out of `Config/DefaultGameplayTags.ini`: legacy `PlayerState.AbilityCast.HeavyAtk/LightAtk/Reload` now live in `Config/Tags/PlayerGameplayTag.ini`; legacy `Card.ID.*`, `Card.Effect.*`, and `Rune.Card.*` now live in `Config/Tags/RuneTag.ini`; legacy sustained `Character.State.Feared/Frozen/Stunned/SuperArmor` now live in `Config/Tags/BuffTag.ini` as aliases to `Buff.Status.*`. `DefaultGameplayTags.ini` should not be used as the ongoing bucket for these system-specific compatibility roots.

## 2026-06-23 Asset migration commandlet

- Added `GameplayTagAssetMigrationCommandlet` for Blueprint/DataAsset migration. Do not hand-edit binary `.uasset` files.
- Default mode is dry-run. It scans `/Game`, skips `_Deprecated` packages unless `-IncludeDeprecated` is passed, and writes `Docs/GeneratedReports/CommandletReports/GameplayTagAssetMigrationReport.md`.
- Apply mode is explicit: add `-Apply` only after reviewing the report.
- Auto-migrated tag families:
  - `PlayerState.AbilityCast.Attack/WeaponSkill/Skill/Dash/Reload/SwitchWeapon/CanCombo/PostAttackRecovery` -> formal `Character.State.*`.
  - legacy `LightAtk.Combo1-4` -> `Character.State.Skill.Attack.Combo1-4`.
  - legacy `HeavyAtk.Combo1-4` -> `Character.State.Skill.WeaponSkill.Combo1-4`.
  - `PlayerState.Block.Idle/Start` -> `Character.State.Block.Idle/Start`.
  - `Card.ID.*` -> `Rune.ID.*`.
  - `Card.Effect.*` -> `Rune.Effect.*`.
  - `Buff.Rune.Type/Rarity/Element.*` -> `Rune.Type/Rarity/Element.*`.
  - `Character.State.Feared/Frozen/Stunned/SuperArmor` -> `Buff.Status.Feared/Frozen/Stunned/SuperArmor`.
  - `Combo.CombatDeck.ActionSlot.*`, `Combo.CombatDeck.FlowRole.*`, `Combo.TriggerTiming.*` -> `Rune.Binding.*`, `Rune.FlowRole.*`, `Rune.Trigger.*`.
- Report-only families:
  - `PlayerState.AbilityCast.Special*` and `PlayerState.AbilityCast.Finisher*`, because they are deprecated gameplay paths and should not be accidentally re-enabled.
  - bare `PlayerState.AbilityCast`, because it is only the legacy action-state root and has no single formal leaf target.
  - bare legacy `PlayerState.AbilityCast.LightAtk*` / `HeavyAtk*` outside the exact `Combo1-4` mappings, plus `Cooldown/Cooling`, because they need asset-intent review.
  - `Card.ID.Attack`, because there is no formal `Rune.ID.Attack`; use `Rune.ID.AttackUp` for the attack-up rune or `Rune.Effect.Attack` for an attack effect.
  - `Rune.Card.*`, because each asset must choose `Rune.ID.*`, `Rune.Effect.*`, `Rune.Binding.*`, or `Rune.FlowRole.*` by context.
  - Set elements and map keys, because Unreal container hashes must be rebuilt explicitly instead of mutating keys in place.
- `Rune.Deck.*` remains disallowed as a formal tag root. Deck/CombatDeck is a runtime container; Rune semantics should be expressed through `Rune.Activation.*`, `Rune.Binding.*`, `Rune.Trigger.*`, `Rune.FlowRole.*`, `Rune.Effect.*`, and `Rune.ID.*`.
- LightAttack/HeavyAttack are no longer formal action concepts. Legacy `LightAtk*` is a migration source to Attack, and legacy `HeavyAtk*` is a migration source to WeaponSkill. The project has confirmed the old Heavy card/rune is the WeaponSkill detonation card, so `Card.ID.Heavy` / `Rune.ID.Heavy` migrate to `Rune.ID.WeaponSkillFinisher`, and `Card.Effect.Heavy` / `Rune.Effect.Heavy` migrate to `Rune.Effect.Detonate`.
- First-run story/tutorial progress should use `Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.weapon_skill_finisher_obtained`. The old `...heavy_card_obtained` progress tag is compatibility-only until StoryRuleSet and encounter assets are regenerated/resaved.
- `Tutorial.Hint.*` declarations were moved out of `Config/DefaultGameplayTags.ini` into `Config/Tags/TutorialTag.ini`. Deprecated hints such as `Tutorial.Hint.HeavyCard` and `Tutorial.Hint.Finisher` remain loadable for old tutorial assets, while new first-run WeaponSkill finisher hints should use `Tutorial.Hint.WeaponSkillFinisher`.
- `Ability.*` declarations were moved out of `Config/DefaultGameplayTags.ini` into `Config/Tags/AbilityTag.ini`. `Ability.Musket.Light/Heavy` are compatibility ability identifiers only; runtime state still uses `Character.State.Skill.Attack` and `Character.State.Skill.WeaponSkill`.
- `GameplayEffect.*` declarations were moved out of `Config/DefaultGameplayTags.ini` into `Config/Tags/GameplayEffectTag.ini`, except item-specific additions such as `GameplayEffect.DamageType.Fire`, which remain with their owning item dictionary.

## 2026-06-24 Config ownership cleanup

- `Config/DefaultGameplayTags.ini` now contains only global GameplayTags settings and the existing `GameplayCue.Character. SuperResist` redirect. It should not be used as the default bucket for system-owned tag declarations.
- `Currency.*` declarations moved to `Config/Tags/CurrencyTag.ini`.
- `Data.*` SetByCaller declarations moved to `Config/Tags/Data.ini`.
- `GameplayCue.*` declarations moved to `Config/Tags/GameplayCueTag.ini`.
- `GameplayEvent.*` declarations moved to `Config/Tags/GameplayEventTag.ini`.
- `Loot.*` declarations moved to `Config/Tags/LootTag.ini`.
- `StateMachine.Idle` moved to `Config/Tags/StateMachineTag.ini`.
- `State.Musket.Aiming` and `State.Musket.Reloading` moved to `Config/Tags/CharacterTag.ini` as legacy compatibility state tags. New runtime logic should prefer `Character.State.Skill.WeaponSkill` / `Character.State.Skill.Reload` plus weapon-specific data.
- `Action.Rune.MoonlightShield`, `Action.Rune.MoonlightSlash`, `Action.Rune.MoonlightSplit`, and `Action.Rune.ShadowMarkDetonate` moved to `Config/Tags/PlayerGameplayTag.ini` with the existing `Action.Rune.*` event family.
- `Event.Moonlight.*` declarations moved to `Config/Tags/RuneTag.ini` as Moonlight rune internal events.
- `DummyDeathFlowSetup` now creates/updates `/Game/Story/Flows/Tutorial/FA_DummyDeath_DropWeaponSkillFinisherCard`. It tries `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_WeaponSkillFinisher` first and falls back to the legacy `DA_Rune512_Heavy` asset until the generated rune asset is migrated and resaved.
- `RuneCardBatchGenerator` now targets `DA_Rune512_WeaponSkillFinisher` and `FA_Rune512_WeaponSkillFinisher_Base` for the old Heavy-card role. If the legacy `FA_Rune512_Heavy_Base` asset exists, it is used only as a duplication template/fallback so the authored bonus branch can be carried forward to the new asset name.
- New WeaponSkill finisher bonus damage generation uses the generic SetByCaller damage GE template path instead of the deprecated `/Game/Code/GAS/Abilities/Finisher/GE_FinisherDamage` path.
- `GameplayTagAssetMigrationCommandlet` now auto-migrates the old Heavy card/rune tags to the formal WeaponSkill finisher tags: `Card.ID.Heavy` / `Rune.ID.Heavy` -> `Rune.ID.WeaponSkillFinisher`, and `Card.Effect.Heavy` / `Rune.Effect.Heavy` -> `Rune.Effect.Detonate`.
- The existing `GameplayTagAssetMigrationCommandlet` dry-run still needs a new editor build before execution, because the current `UnrealEditor-DevKitEditor.dll` predates the new commandlet source. Do not run `-Apply` until the dry-run report has been reviewed.

## 2026-06-24 Buff/Rune merge supersession

This section supersedes the earlier Phase 1 `Card.* -> Rune.*` migration target.

- `Buff.*` is now the formal runtime/query vocabulary for card/rune/buff semantics.
- `Card.ID.*`, `Card.Effect.*`, `Rune.ID.*`, `Rune.Effect.*`, and `Buff.Status.*` migrate to flat `Buff.*` when they represent runtime effect semantics.
- Rune/Card identity, action slot, trigger timing, flow role, activation mode, rarity, element metadata, and numeric values belong in DA/table fields rather than formal GameplayTags.
- `Action.Rune.*` and `Event.Rune.*` do not remain as formal event roots. Internal buff/card/rune gameplay events migrate to `Buff.Event.*`.
- `GameplayCue.Rune.*` does not remain as a formal presentation root. Buff/card/rune presentation cues migrate to `GameplayCue.Buff.*`.
- Deprecated QTE Finisher card/rune effect tags remain report-only and should not be auto-promoted into the formal Buff runtime path. The old QTE Finisher gameplay cue is still migrated from `GameplayCue.Rune.FinisherCharge` to `GameplayCue.Buff.FinisherCharge` so assets no longer depend on the Rune cue root.

Current event/cue migration targets:

| Old tag | New tag |
| --- | --- |
| `Action.Rune.KnockbackApplied` | `Buff.Event.KnockbackApplied` |
| `Action.Rune.SlashWaveHit` | `Buff.Event.SlashWaveHit` |
| `Action.Rune.MoonlightBurnHit` | `Buff.Event.Moonlight.BurnHit` |
| `Action.Rune.MoonlightPoisonHit` | `Buff.Event.Moonlight.PoisonHit` |
| `Action.Rune.MoonlightPoisonExpired` | `Buff.Event.Moonlight.PoisonExpired` |
| `Action.Rune.MoonlightShield` | `Buff.Event.Moonlight.Shield` |
| `Action.Rune.MoonlightSlash` | `Buff.Event.Moonlight.Slash` |
| `Action.Rune.MoonlightSplit` | `Buff.Event.Moonlight.Split` |
| `Action.Rune.ShadowMarkDetonate` | `Buff.Event.ShadowMarkDetonate` |
| `GameplayCue.Rune.AtkUp` | `GameplayCue.Buff.AttackUp` |
| `GameplayCue.Rune.Burn` | `GameplayCue.Buff.Fire` |
| `GameplayCue.Rune.Burn.Vfx` | `GameplayCue.Buff.Fire.Vfx` |
| `GameplayCue.Rune.Cursed` | `GameplayCue.Buff.Curse` |
| `GameplayCue.Rune.DeathPoison` | `GameplayCue.Buff.DeathPoison` |
| `GameplayCue.Rune.Fearless` | `GameplayCue.Buff.Fearless` |
| `GameplayCue.Rune.FinisherCharge` | `GameplayCue.Buff.FinisherCharge` |
| `GameplayCue.Rune.KillExplosion` | `GameplayCue.Buff.KillExplosion` |
| `GameplayCue.Rune.MoonlightSlash.Fire` | `GameplayCue.Buff.MoonlightSlash.Fire` |
| `GameplayCue.Rune.MoonlightSlash.Hit` | `GameplayCue.Buff.MoonlightSlash.Hit` |
| `GameplayCue.Rune.MoonlightSlash.Pierce` | `GameplayCue.Buff.MoonlightSlash.Pierce` |
| `GameplayCue.Rune.MoonlightSlash.Split` | `GameplayCue.Buff.MoonlightSlash.Split` |
| `GameplayCue.Rune.Poison.Vfx` | `GameplayCue.Buff.Poison.Vfx` |
| `GameplayCue.Rune.Shield` | `GameplayCue.Buff.Shield` |
