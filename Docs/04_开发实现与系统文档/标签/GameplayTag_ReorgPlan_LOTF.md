# GameplayTag 重整方案（结合 LOTF 参考）

日期：2026-06-24
状态：当前 DevKit Tag 命名结论。后续 GameplayTag 清理、Redirect、资产迁移以本文为准。

## 1. LOTF 规范简述与评价

LOTF 的重点不是照搬它的根节点名称，而是把 GameplayTag 当成工程资产治理：

| 规则 | LOTF 做法 | DevKit 借鉴 |
| --- | --- | --- |
| 主字典 | 维护设计师可见的权威 tag 字典 | 稳定 tag 放进明确 owner 配置文件 |
| 高变更 flag | 独立管理 `GameState.Flags.*` | 剧情、教程、任务、存档布尔值统一走 `GameState.Flags.*` |
| C++ 访问 | 核心 tag 通过常量/Native Tag 收口 | 后续把高频运行时 tag 收口到统一访问层 |
| 改名 | 先兼容，再迁移，最后 redirect | 不硬删旧 tag；先新增正式 tag 和兼容代码 |
| UI tag | UI tag 只做 UI 路由 | HUD 展示 Buff，但不拥有 Buff 业务状态 |

评价：LOTF 适合大型项目的治理方式，优点是边界清晰、改名安全、C++ 查询稳定。DevKit 不照搬它的所有根节点，但采用它的治理原则。

## 2. DevKit 一级根结论

一级 root 只放“大系统/大职责”，不放临时玩法名。

| 根节点 | 职责 | 示例 |
| --- | --- | --- |
| `Character.*` | 角色通用动作状态、3C 运行状态 | `Character.State.Skill.Attack` |
| `AI.*` | AI 行为、决策、黑板语义 | `AI.State.Patrol` |
| `Enemy.*` | 敌人专属技能身份、敌人配置筛选 | `Enemy.Skill.Skill1` |
| `EntityType.*` | 实体身份分类 | `EntityType.Hero` |
| `Interact.*` | 场景可交互对象分类 | `Interact.Weapon.Melee.SingleHandBlunt` |
| `Weapon.*` | 武器类型、装备后的武器分类 | `Weapon.Type.Ranged.Bow` |
| `Buff.*` | Buff/Debuff/持续状态/合并后的卡牌符文效果语义 | `Buff.Fire` |
| `GameplayEvent.*` | GAS 事件总线 | `GameplayEvent.Combat.Damaged` |
| `GameplayCue.*` | VFX/SFX/镜头表现 | `GameplayCue.Combat.HitImpact.Sword` |
| `Level.*` | 关卡、房间、局内循环 | `Level.Room.Type.Normal` |
| `Director.*` | 剧情、教程、任务、固定房间、掉落覆盖 | `Director.Event.Tutorial.Start` |
| `UI.*` | UI 层级、输入动作、扩展点 | `UI.Layer.HUD` |
| `GameState.*` | 存档、世界、教程、任务 Flag | `GameState.Flags.Tutorial.BasicAttackDone` |

不新增正式 `Combat.*` root。战斗状态由 `Character.State.*` 表示；战斗事件和表现已经分别有 `GameplayEvent.Combat.*` / `GameplayCue.Combat.*`。

## 3. 关键结论

### 3.1 Combat 不作为正式一级系统

`Combat` 是玩法域，不是一个稳定 owner。它会同时涉及 Character、Weapon、Buff、AI、GameplayEvent、GameplayCue、UI。若建立 `Combat.*`，很容易出现同一事实重复表达：

```text
Character.State.Skill.Attack
Combat.State.Attack
PlayerState.AbilityCast.Attack
```

最终结论：

- 攻击、技能、战技、冲刺执行状态：`Character.State.*`
- 命中、受伤、伤害事件：`GameplayEvent.Combat.*`
- 命中特效、音效、镜头表现：`GameplayCue.Combat.*`
- Buff/Debuff/卡牌符文效果：`Buff.*`

### 3.2 Buff 独立为主系统，并合并 Rune/Card tag 语义

`Buff.*` 是正式一级根。它不只是“状态图标”，而是运行时可查询的效果语义，包括 Buff、Debuff、持续状态、卡牌/符文效果上下文。

推荐：

```text
Buff.Fire
Buff.Poison
Buff.Moonlight
Buff.AttackUp
Buff.WeaponSkillFinisher
Buff.Detonate
Buff.RecoveryCancelBonus
```

不推荐新内容使用：

```text
Buff.Status.*
Buff.ID.*
Buff.Keyword.*
Buff.Binding.*
Rune.ID.*
Rune.Effect.*
Rune.Binding.*
Rune.FlowRole.*
Rune.Trigger.*
Card.ID.*
Card.Effect.*
Action.Rune.*
Event.Rune.*
GameplayCue.Rune.*
```

原因：身份、触发、槽位、流程角色、稀有度、数值等都可由 DA/表格字段表达；GameplayTag 只保留需要运行时查询、阻断、HUD 展示、连携匹配的效果语义。

卡牌/符文内部事件统一使用 `Buff.Event.*`，例如 `Buff.Event.SlashWaveHit`、`Buff.Event.Moonlight.BurnHit`。卡牌/符文表现统一使用 `GameplayCue.Buff.*`，例如 `GameplayCue.Buff.Fire`、`GameplayCue.Buff.MoonlightSlash.Hit`。

### 3.3 玩家和敌人的通用状态不拆两套

死亡、受击、击退、眩晕、技能执行中属于通用角色状态，统一使用：

```text
Character.State.Skill.Attack
Character.State.Skill.Active
Character.State.Skill.WeaponSkill
Character.State.Movement.Dash
Buff.Dead
Buff.HitReact
Buff.Knockback
```

不拆成 `Player.State.*` 和 `Enemy.State.*` 的原因：Block 规则、HUD/AI 查询、通用 GA 复用都会变成两套维护。

### 3.4 Rune/Card 作为资产概念保留，Tag 语义并入 Buff

可以继续存在：

- `URuneDataAsset`
- Rune Editor
- CombatDeckComponent
- Rune/Card 设计文档和资产路径

但新 GameplayTag 不再使用 `Rune.ID.*` / `Rune.Effect.*` / `Action.Rune.*` / `GameplayCue.Rune.*`。旧 `Card.*`、`Rune.*`、`Buff.Status.*`、`Action.Rune.*`、`GameplayCue.Rune.*` 保留为迁移来源和旧资产加载兼容。

映射方向：

```text
Card.ID.Burn                  -> Buff.Fire
Card.Effect.Burn              -> Buff.Fire
Rune.ID.Burn                  -> Buff.Fire
Rune.Effect.Burn              -> Buff.Fire
Buff.Status.Burning           -> Buff.Fire
Rune.ID.WeaponSkillFinisher   -> Buff.WeaponSkillFinisher
Rune.Effect.Detonate          -> Buff.Detonate
```

### 3.5 Director 提升为主系统

Story/Tutorial 后续提升为 `Director.*` 系统。Director 通过接口调度其他系统，不直接拥有底层战斗、关卡、UI、存档状态。

| 场景 | Director 负责 |
| --- | --- |
| 对话 | 锁输入、驱动角色移动、打开对话 UI |
| 限时击杀任务 | 调用 Level/Enemy/Reward 创建任务规则 |
| 教程固定房间 | 覆盖随机房间、固定刷怪、固定掉落、固定提示 |
| 新手流程 | 读写 `GameState.Flags.Tutorial.*` 并决定下一步 |

Save 只保存进度和结果；教程什么时候发生、怎么覆盖房间和掉落，由 Director 决定。

### 3.6 UI 只表现，不承载业务状态

HUD 上的 Buff 图标、玩家状态、敌人状态，数据来源应是：

```text
ASC ActiveGameplayEffect / GameplayTag
  -> ViewModel / UI Event
  -> HUD Icon / Timer / Stack Count
```

`UI.*` 只用于 UI 层级、输入动作、扩展点，不创建 `UI.Player.Buff.*` 这类业务状态 tag。

## 4. 迁移计划

1. 冻结本文作为当前命名标准。
2. 新增正式 `Buff.*` tags，并把代码运行时查询切到 `Buff.*`。
3. 保留旧 `Card.*`、`Rune.*`、`Buff.Status.*` 定义，作为旧资产加载和迁移来源。
4. `GameplayTagAssetMigrationCommandlet` dry-run，人工审查 report-only 项。
5. 确认可迁移后执行 `-Apply`，在编辑器中重保存资产。
6. 扫描旧引用归零后，删除旧定义并添加最终 `GameplayTagRedirects`。
7. deprecated QTE Finisher 资产保留但不启用运行时入口。
8. Unreal 编译/编辑器重保存只在明确要求时执行。
