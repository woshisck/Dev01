# GA Tag 字段使用指南

> 适用范围：所有 GameplayAbility Blueprint  
> 配套文档：`Tag_SituationalGuide.md`、`GameplayTag_MasterGuide.md`

---

## 一、GA 里的 5 个 Tag 字段

每个 GA Blueprint 的 **Tags** 区域有 5 个字段，各自独立、职责不同：

```
┌─────────────────────────────────────────────────────────┐
│ Tags                                                    │
│  Ability Tags            → 这个 GA 的身份证             │
│  Cancel Abilities With   → 激活时取消哪些 GA            │
│  Block Abilities With    → 激活时屏蔽哪些 GA            │
│  Activation Owned Tags   → 激活期间往 ASC 挂的 Tag      │
│  Activation Blocked Tags → ASC 有这些 Tag 时无法激活    │
└─────────────────────────────────────────────────────────┘
```

---

## 二、字段详解

### 2.1 Ability Tags — 这个 GA 是什么

**作用**：GA 的"名字"，供外部系统查找/取消/匹配。  
**影响**：被 `CancelAbilitiesWithTag`、`BlockAbilitiesWithTags`、`TryActivateRandomAbilitiesByTag`、StateConflict 的 Block/Cancel Tags 引用。

| 填什么 | 说明 |
|---|---|
| 这个 GA 代表的行为 Tag | 如 `Enemy.Melee.LAtk1`、`Action.Dead` |
| 不填 | 没有外部系统需要通过 Tag 找到这个 GA 时 |

**重要规则**：只有"有代码或系统通过它查询这个 GA"时才填，没有读取方的 Tag 是冗余的。

---

### 2.2 Cancel Abilities With Tag — 激活时取消哪些 GA

**作用**：这个 GA 激活的瞬间，立即打断并取消带有这些 Tag 的 GA。  
**影响**：类似"我来了，你们走"。

| 填什么 | 说明 |
|---|---|
| 需要被我打断的 GA 的 AbilityTag | 如闪避激活时填攻击 GA 的 Tag |
| 通常不填 | 大多数场景交给 StateConflict 统一处理更清晰 |

---

### 2.3 Block Abilities With Tag — 激活时屏蔽哪些 GA

**作用**：这个 GA 激活期间，阻止带有这些 Tag 的 GA 被激活（但不取消已激活的）。  
**影响**：GA 结束时屏蔽自动解除。

| 填什么 | 说明 |
|---|---|
| 我激活时不允许同时激活的 GA 的 AbilityTag | 少用，大多数场景交给 StateConflict |
| 通常不填 | — |

---

### 2.4 Activation Owned Tags — 激活期间往 ASC 挂的 Tag

**作用**：GA 激活时自动把这些 Tag 添加到 ASC（角色自身），GA 结束时自动移除。  
**影响**：这是 **StateConflict 系统的核心输入**——状态 Tag 挂上去，OnTagUpdated 触发，冲突规则执行。

| 填什么 | 说明 |
|---|---|
| `Buff.Status.*` 状态 Tag | 代表"当前正在做什么"，死亡填 Dead，受击填 HitReact |
| `PlayerState.AbilityCast.*` | 玩家攻击 GA 填当前连招段，供 StateConflict 读取 |
| 不填 | 这个 GA 不需要对外广播"我正在执行" |

**规则**：只用 `Buff.Status.*` 或 `PlayerState.AbilityCast.*`，不用 `Action.*`（Action 是信号，不是持续状态）。

---

### 2.5 Activation Blocked Tags — ASC 有这些 Tag 时无法激活

**作用**：激活前检查 ASC，如果有任何一个 Tag 匹配，GA 拒绝激活。  
**影响**：这是比 StateConflict 更直接的"个人防线"。

| 填什么 | 说明 |
|---|---|
| `Buff.Status.Dead` | 死亡状态下不允许激活（攻击 GA、移动 GA 等都应填） |
| 其他互斥状态 | 按具体需求填 |
| 通常不填 | StateConflict 的 Block Tags 已经覆盖时无需重复 |

**和 StateConflict Block Tags 的区别**：
- StateConflict Block Tags：批量屏蔽一类 GA（通过父 Tag 匹配），集中配置
- Activation Blocked Tags：单个 GA 自己声明"我在什么情况下不能激活"，作为补充和兜底

---

## 三、四种 GA 类型的配置示例

### 类型 A：敌人攻击 GA（`GA_Enemy_Melee_LAtk1`）

```
Ability Tags:            Enemy.Melee.LAtk1
Cancel Abilities With:   （空）
Block Abilities With:    （空）
Activation Owned Tags:   （空）
Activation Blocked Tags: Buff.Status.Dead
```

**解释**：
- `Ability Tags` 填 `Enemy.Melee.LAtk1`：BT Task 用 `TryActivateRandomAbilitiesByTag(Enemy.Melee)` 随机选攻击时靠这个匹配；StateConflict 的 `Buff.Status.Dead` 规则的 Block/Cancel Tags 里填的是 `Enemy`（父 Tag），能批量屏蔽所有 `Enemy.*` 子级 GA。
- `Activation Blocked Tags` 填 `Buff.Status.Dead`：双重保险，死亡后即使 StateConflict 有延迟，这个 GA 自身也拒绝激活。
- `Activation Owned Tags` 不填：敌人攻击不需要对外广播状态（StateConflict 靠 AbilityTags 识别，不靠状态 Tag）。

---

### 类型 B：玩家攻击 GA（`GA_LightAtk_Combo1`）

```
Ability Tags:            PlayerState.AbilityCast.LightAtk.Combo1
Cancel Abilities With:   （空）
Block Abilities With:    （空）
Activation Owned Tags:   PlayerState.AbilityCast.LightAtk.Combo1
Activation Blocked Tags: Buff.Status.Dead, Buff.Status.HitReact, Buff.Status.Knockback
```

**解释**：
- `Ability Tags` 和 `Activation Owned Tags` 填同一个 Tag：外部可以通过 Tag 找到这个 GA（取消/查询），同时这个 GA 激活时把状态广播到 ASC 供 StateConflict 读取。
- `Activation Blocked Tags` 填三个状态：死亡/受击/击退时都不能发动攻击。StateConflict 也做了 Block，这里是额外防线。

---

### 类型 C：通用响应 GA（`GA_Dead`、`GA_HitReaction`、`GA_Knockback`）

以 `GA_Dead` 为例：

```
Ability Tags:            Action.Dead
Cancel Abilities With:   （空）
Block Abilities With:    （空）
Activation Owned Tags:   Buff.Status.Dead
Activation Blocked Tags: （空）
```

**解释**：
- `Ability Tags` 填 `Action.Dead`：这个 GA 靠 `AbilityTriggers` 由 `SendGameplayEventToActor(Action.Dead)` 触发，AbilityTags 同时供 StateConflict 的 CancelTags 查找（死亡时取消其他 GA 不会误取消自身）。
- `Activation Owned Tags` 填 `Buff.Status.Dead`：这是 StateConflict 系统的核心 — 挂上这个 Tag 后，`OnTagUpdated` 触发，自动屏蔽所有其他 GA、停止 AI、禁止移动。
- `Activation Blocked Tags` 不填：死亡 GA 自身不需要被其他状态阻止。

`GA_HitReaction` 同理：

```
Ability Tags:            Action.HitReact
Activation Owned Tags:   Buff.Status.HitReact
Activation Blocked Tags: Buff.Status.Dead    ← 死亡时受击 GA 不应触发
```

---

### 类型 D：符文/被动 GA（`GA_Passive_KnockBack`）

```
Ability Tags:            （空）
Cancel Abilities With:   （空）
Block Abilities With:    （空）
Activation Owned Tags:   （空）
Activation Blocked Tags: （空）
```

**解释**：
- 符文 GA 通过 `AbilityTriggers` 监听 `Buff.Trigger.OnHit` 等信号触发。
- 不需要 AbilityTags，因为没有系统需要通过 Tag 取消或查找符文 GA。
- 不填 ActivationOwnedTags，因为符文效果的状态由 GE GrantedTags 管理，不归 GA 管。
- AssetTags（不在这 5 个字段里）填 `Buff.Trigger.OnHit` + `Buff.Rune.Type.Attack` 等元数据，供背包系统筛选和触发条件匹配。

---

## 四、常见错误

| 错误 | 正确做法 |
|---|---|
| `Activation Owned Tags` 填 `Action.Dead` | `Action.*` 是信号，不是持续状态，应填 `Buff.Status.Dead` |
| 所有字段都填满 | 没有读取方的 Tag 是噪音，按职责最小填写 |
| 攻击 GA 不填 `Activation Blocked Tags` | 至少要防 `Buff.Status.Dead`，死亡后攻击 GA 不应激活 |
| Cancel Tags 和 StateConflict 重复配置 | 选一种方式，优先用 StateConflict（集中管理） |
| `Ability Tags` 填的 Tag 和状态 Tag 混用 | AbilityTags 是身份，状态归 ActivationOwnedTags |

---

## 五、速查表

| 我想要…… | 填哪个字段 | 填什么 |
|---|---|---|
| 让 BT Task 能随机选到这个 GA | Ability Tags | `Enemy.Melee.LAtk1` |
| StateConflict 能 Block/Cancel 这个 GA | Ability Tags | 对应命名空间 Tag |
| 这个 GA 激活时停止 AI / 移动 | Activation Owned Tags | `Buff.Status.Dead` 等（需 BlockCategoryMap 配置） |
| 死亡时这个 GA 不能激活 | Activation Blocked Tags | `Buff.Status.Dead` |
| 这个 GA 激活时取消正在进行的攻击 | Cancel Abilities With | 攻击 GA 的 AbilityTag |
| 这个 GA 激活时，攻击 GA 暂时无法触发 | Block Abilities With | 攻击 GA 的 AbilityTag（或交给 StateConflict） |
