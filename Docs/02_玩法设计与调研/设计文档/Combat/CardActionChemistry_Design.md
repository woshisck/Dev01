# Card + Action Chemistry Design

## Purpose

This document defines how ComboGraph-driven attacks and Rune combat cards should combine into a fresh action-card system.

The goal is not:

- Attack happens, then a card effect is pasted on top.
- Cards are only hidden stat buffs.
- The player ignores deck order and just sees random VFX.

The goal is:

- ComboGraph provides the action grammar.
- Rune cards provide tactical meaning.
- A resolver layer combines card, attack, target state, and combat context into different outcomes.

The player should think: "This card is next, so I should attack differently."

## Core Design Rule

Every combat card must answer one question:

> How does this card make the player change movement, timing, target choice, attack type, positioning, or combo route?

If the answer is only "deal more damage", the card is a low-priority filler card, not a signature card.

## 核心概念：投射

“投射”不是普通的能量条，也不是单纯释放卡牌技能。

它指的是：

> 玩家把牌里的规则、意象、神力或诅咒，借由角色的动作强行压进现实世界。

卡牌本身只是潜在的意义。动作、命中、连段、时机和站位，才是让这份意义进入现实的媒介。

例如当前卡是“月”：

```text
未攻击时：
  月只是一张牌，一个可能发生的概念。

轻攻击命中时：
  月开始被稳定投射。
  敌人被月光标记，刀痕出现银光，攻击轨迹留下月弧。

重攻击命中时：
  月被完整显现。
  这一刀变成月刃，标记被引爆，敌人被拉入月光裂隙。
```

可以把相关概念拆成：

```text
卡牌 = 意义 / 规则 / 神力 / 诅咒
动作 = 投射媒介
命中 = 投射接触现实
轻攻击 = 稳定投射
重攻击 = 强制显现
连携 = 多个意义互相咬合
过载 = 投射强度超过世界承受范围
排异 = 世界反抗这种外来规则
```

因此，本系统不应该让玩家感觉自己只是在“打够能量然后放技能”。

能量条逻辑是：

```text
我打够了，所以可以放技能。
```

投射逻辑应该是：

```text
我用正确的动作，把这张牌变成了现实。
```

这也解释了为什么强力连携、神明卡和自动大招会提高排异：它们不是普通招式，而是在短时间内让世界接受本不属于它的规则。

## System Roles

| System | Role |
| --- | --- |
| ComboGraph | Owns action flow: light, heavy, dash attack, charged attack, combo branches, animation timing, cancel windows. |
| CombatDeckComponent | Owns card order, draw/consume/shuffle, reward insertion, link and finisher state. |
| RuneDataAsset / CombatCard | Owns card identity, card tags, base flow, link recipes, and action-specific behavior data. |
| BuffFlow / FA | Executes gameplay effects: damage, status, VFX, projectile, knockback, tag grant, delayed logic. |
| CardActionResolver | New chemistry layer. Reads ActionContext + current card + target context, then chooses mutation, flow, branch, or link. |

## ActionContext

Each meaningful ComboGraph attack should emit an ActionContext before card resolution and enrich it on hit.

Suggested fields:

```cpp
USTRUCT(BlueprintType)
struct FCardActionContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ActionTag; // Action.Attack.Light, Action.Attack.Heavy, Action.Attack.Dash, etc.

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag WeaponTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ComboIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ChargeRatio = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPerfectTiming = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDashAttack = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTargetNearWall = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTargetAirborne = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer TargetStateTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag PreviousCardTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag CurrentCardTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag NextCardTag;
};
```

The exact fields can be trimmed later. The important design point is that card effects must know how the player attacked.

## Runtime Flow

```text
Player input
  -> ComboGraph selects attack node
  -> Attack emits FCardActionContext
  -> CombatDeckComponent previews/resolves current card
  -> CardActionResolver evaluates card + action context
  -> Resolver chooses one or more:
       1. modify attack parameters
       2. trigger BuffFlow / FA
       3. open a ComboGraph branch
       4. queue a follow-up effect
       5. trigger link recipe
       6. add finisher progress
  -> attack hit confirms context
  -> final effects execute
```

## Card Behavior Modes

Cards should support multiple behavior modes, not only "OnHit apply effect".

| Mode | Meaning | Example |
| --- | --- | --- |
| AddEffect | Run a normal BuffFlow on hit or attack start. | Burn applies burning on hit. |
| ModifyAttack | Change the attack itself. | Heavy attack becomes a launcher. |
| OpenComboBranch | Unlock a temporary ComboGraph route. | Curse card opens a special finisher route after three action types. |
| QueueFollowUp | Store a delayed or conditional effect. | Moon trail detonates on next heavy. |
| TriggerLink | Combine previous/current card plus action context. | Bleed + Knockback + Wall = rupture explosion. |
| BuildFinisher | Add progress or enable a finisher card. | Perfect heavy while finisher card is next adds bonus charge. |

## 攻击触发与卡牌消耗策略

不要把“卡牌触发”简单等同于“任意攻击命中后立刻发动并消耗当前卡”。如果每一次轻攻击命中都能完整触发卡牌，玩家最优策略会变成反复轻攻击刷效果，卡牌与动作之间的选择空间会被压扁。

推荐把卡牌与动作的关系拆成三个层级：

| 动作类型 | 设计职责 | 常见结果 |
| --- | --- | --- |
| 轻攻击 | 表达、铺垫、试探 | 小效果、标记、叠层、刷新、引导目标，通常不消耗卡牌。 |
| 重攻击 | 承诺、兑现、结算 | 消耗当前卡，触发强效果、击飞、引爆、破裂、开分支或大幅修改攻击。 |
| 冲刺/蓄力/完美时机 | 战术触发 | 用更明确的位移、时机或风险换取特殊卡牌结果。 |

核心原则：

> 轻攻击可以“和卡牌互动”，但重攻击、冲刺攻击、蓄力攻击、完美时机或特定场景条件，才应该经常负责“消耗并兑现卡牌”。

这样轻攻击仍然有价值，但它更多是在准备局面，而不是免费兑现所有卡牌收益。玩家可以用轻攻击建立标记、叠层、调整敌人位置或确认节奏，然后选择是否用更昂贵、更有风险的动作把当前卡牌打出去。

示例规则：

| 卡牌 | 轻攻击 | 重攻击 / 特殊动作 |
| --- | --- | --- |
| Knockback | 小硬直或轻推，通常不消耗卡牌。 | 重攻击消耗卡牌并击飞；靠墙时触发墙撞爆发；冲刺攻击把敌人向前携带。 |
| Moonlight | 给目标上月光标记，通常不消耗卡牌。 | 冲刺攻击留下月光轨迹；重攻击消耗卡牌并引爆标记或轨迹。 |
| Bleed | 施加少量流血层数，但每条连段内限制触发次数。 | 重攻击消耗卡牌并撕裂流血；穿过流血目标的冲刺攻击可扩散流血。 |
| Shield | 命中后获得短暂小护盾，或为下一次防御窗口做准备。 | 重攻击启动阶段获得 guard point；成功承受攻击后消耗卡牌并释放反击冲击波。 |

为了支持这种设计，CardActionResolver 不应该只回答“触发什么效果”，还应该同时回答“当前卡是否被消耗”。

可以为卡牌规则增加消耗策略：

```cpp
UENUM(BlueprintType)
enum class ECardConsumePolicy : uint8
{
    DoNotConsume,
    ConsumeOnValidHit,
    ConsumeOnAttackStart,
    ConsumeOnHeavyCommit,
    ConsumeOnSpecialContext,
    ConsumeOnLinkSuccess
};
```

也可以给动作一个简单的承诺度权重，用于平衡触发强度：

```text
Light = 1
Dash = 2
Heavy = 3
ChargedHeavy = 4
PerfectTiming = +1
NearWall / Airborne / Marked = unlock rule or bonus multiplier
```

原型阶段建议先采用非常明确的默认规则：

```text
轻攻击可以触发弱表达或铺垫效果，但默认不消耗卡牌。
重攻击、冲刺攻击、蓄力攻击、完美时机、连段终段或特定场景条件，才默认允许消耗卡牌。
```

一句话总结：

> 轻攻击负责“说出这张卡”，重攻击和特殊动作负责“花掉这张卡”。

## Suggested CombatCard Data

Add or extend data like this on Rune combat cards:

```cpp
UENUM(BlueprintType)
enum class ECardTriggerTiming : uint8
{
    OnAttackStart,
    OnHit,
    OnHeavyRelease,
    OnDashAttack,
    OnPerfectDodge,
    OnWallCollision,
    OnKill
};

UENUM(BlueprintType)
enum class ECardActionBehaviorMode : uint8
{
    AddEffect,
    ModifyAttack,
    OpenComboBranch,
    QueueFollowUp,
    TriggerLink,
    BuildFinisher
};

USTRUCT(BlueprintType)
struct FCardActionRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardTriggerTiming TriggerTiming = ECardTriggerTiming::OnHit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer RequiredActionTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer PreferredActionTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer BlockedActionTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer RequiredContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardActionBehaviorMode BehaviorMode = ECardActionBehaviorMode::AddEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UFlowAsset> FlowOverride = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag ComboBranchTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Priority = 0.0f;
};
```

Authoring principle:

- BaseFlow is the fallback behavior.
- CardActionRules define special chemistry with specific attacks or contexts.
- Highest-priority valid rule wins, unless the card explicitly supports stacking rules.

## Fresh Combo Examples

### Knockback Card

| Action | Result |
| --- | --- |
| Light attack | Small push and interrupt. |
| Heavy attack | Strong launch. |
| Dash attack | Carry target forward, collision focused. |
| Target near wall | Wall impact explosion. |
| Previous card was Bleed | Wall impact ruptures bleed in an area. |

Player behavior: angle enemies into walls or groups before spending the card.

### Moonlight Link Card

| Action | Result |
| --- | --- |
| Light attack | Mark target with moonlight. |
| Dash attack | Leave a moon trail through enemies. |
| Heavy attack after trail | Detonate trail. |
| Previous card was Burn | Trail becomes flame path. |
| Previous card was Poison | Trail becomes poison mist. |

Player behavior: use movement to draw a damage path, then choose when to detonate.

### Shield Card

| Action | Result |
| --- | --- |
| Light attack | Gain brief shield on hit. |
| Heavy startup | Gain guard point. |
| Hit during guard point | Release counter shockwave. |
| Dash attack | Shield bash with short stun. |

Player behavior: intentionally time heavy startup into enemy attacks instead of only dodging.

### Split Card

| Action | Result |
| --- | --- |
| Light ranged shot | Split into weak pellets. |
| Charged shot | Pierce first target, then split behind it. |
| Dash attack | Side split, good for close range. |
| Target marked by Moonlight | Split projectiles seek marked targets. |

Player behavior: choose range and angle based on next card.

### Curse Card

| Action | Result |
| --- | --- |
| Any hit | Apply delayed curse. |
| Three different attack types during curse | Open special ComboGraph finisher branch. |
| Enemy dies before timer | Curse fizzles or spreads weakly. |
| Heavy hit at final second | Manual detonation with bonus. |

Player behavior: vary attack types and manage timing instead of repeating one optimal attack.

## Link Recipe Direction

Link recipes should include action context, not only card adjacency.

Example:

```text
PreviousCard = Bleed
CurrentCard = Knockback
ActionTag = Action.Attack.Heavy
ContextTag = Context.Target.NearWall
Result = FA_Link_BleedWallRupture
```

This makes deck planning and ACT positioning cooperate.

## ComboGraph Integration

ComboGraph should expose three kinds of hooks:

1. Pre-attack resolve

   Used for attack mutation before animation or early montage windows.

2. Hit resolve

   Used for target-specific effects once hit data exists.

3. Branch request

   Used when a card opens a temporary ComboGraph route.

Suggested event shape:

```text
ComboGraph Node: HeavyAttack_02
  -> Emit Action.Attack.Heavy + ComboIndex 2
  -> Ask CardActionResolver for PreAttackMutation
  -> Play montage with modified parameters
  -> On AN_MeleeDamage hit, ask resolver for HitResult effects
  -> If resolver returns BranchTag, enable matching ComboGraph branch window
```

## UI Requirements

This system only works if the player can read it.

Minimum readable UI:

- Show current card and at least next 2 cards.
- Show card type: normal, link, finisher.
- Highlight when current action has special chemistry with current card.
- Show short toast for link result, such as "Bleed Rupture" or "Moon Trail".
- Do not explain long rules during combat; use icons, color, and concise result names.

## Prototype Scope

Do not prototype with many cards.

Recommended first prototype:

Actions:

- Light attack
- Heavy attack
- Dash attack

Cards:

- Knockback
- Bleed
- Moonlight
- Shield
- Split
- Burn

Required proof:

- Each card behaves differently for at least two action types.
- At least three card pairs have action/context-based link recipes.
- At least one card opens or modifies a ComboGraph branch.
- At least one combo requires positioning, such as wall or enemy grouping.

## Acceptance Criteria

The design is working when:

1. A player can see the next card and intentionally change how they attack.
2. Different attack types produce visibly different card outcomes.
3. Link effects feel caused by player action, not random card adjacency.
4. Cards create new tactical verbs: angle, delay, dash-through, guard, detonate, group, launch.
5. Pure damage-up cards are not the dominant reward type.

## Design Red Lines

- Do not let cards silently trigger with no readable cause.
- Do not make every card resolve only on hit.
- Do not make deck order irrelevant.
- Do not make ComboGraph ignore cards.
- Do not make Rune cards own animation flow directly; they should request mutations or branches through the resolver.
- Do not add complex UI explanations as a substitute for readable combat outcomes.

## Summary

The fresh version of this system is:

> ACT skill decides how a card expresses itself.

Deck order creates planning. ComboGraph creates execution. Rune cards create meaning. The CardActionResolver is the place where those three become a new combat language.
