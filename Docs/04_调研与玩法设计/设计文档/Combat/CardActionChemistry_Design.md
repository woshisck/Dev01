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
