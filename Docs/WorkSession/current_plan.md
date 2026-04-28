# 武器类型守卫 — 自动隔离近战 / 远程 GA 激活 v2

> v2（2026-04-28）：整合 Codex 审查 6 项硬伤 + 用户 4 项决策 + 父类调研结果

## 需求描述

> 我现在的攻击是近战武器。
> 帮我做一个设置：自动判段武器是近战还是远程，如果是远程的时候启用远程 GA/GE，近战的时候只调用近战的 GA/GE。
> 这个配置可以写在武器 DA 里面做一个选项或者设置。用 tag 来管理也可以。

**起因**：日志显示 `LightAtk.Combo1/Combo2` 触发了 `GE_MusketBullet_Damage`——玩家装备的是近战武器，但火枪 GA 仍被激活并施加远程伤害。原因：
- `LightAtack()` 输入只发 `PlayerState.AbilityCast.LightAtk` tag
- 玩家 ASC 同时持有近战和火枪两套 GA（都用此 tag 作为 AbilityTag）
- `TryActivateAbilitiesByTag` 把所有匹配的都尝试激活 → 抢占无序

## 用户决策（2026-04-28）

| 维度 | 决策 |
|------|------|
| 多人需求 | **不确定，做可扩展预留** — 现阶段单机，Tag 操作封装成 helper，未来一处替换为 Replicated |
| BP 父类 | **可能混用，已查** — C++ 玩家 GA 全部继承 `UGA_PlayerMeleeAttack` ✅；`GA_WPN_*` BP 走另一条线（待确认是否在用）|
| 卸装/死亡清理 | **不加专用入口**，只走 `SetupWeaponToCharacter` 替换路径 |
| 无武器徒手 | **不允许** — 无 Tag → 所有攻击 GA 过滤掉，按 LMB 无反应（零额外代码）|

## 父类调研结果（关键）

```
玩家攻击 GA C++ 继承链（确认）:
  UGA_Player_LightAtk1/2/3/4
  UGA_Player_HeavyAtk1/2/3/4
  UGA_Player_DashAtk
       ↓ 全部继承
  UGA_PlayerMeleeAttack (中间基类，构造函数自动绑 GE_StatBefore/After)
       ↓
  UGA_MeleeAttack (通用基类，敌人 UGA_Enemy_LAtk* 也用)
       ↓
  UYogGameplayAbility
```

**含义**：在 `UGA_PlayerMeleeAttack` 构造函数加 `Weapon.Type.Melee` Required Tag → 所有玩家 C++ 近战 GA 自动获得 → **零 BP 改动** ✅

**遗留隐患**：`Content/Code/GAS/Abilities/DA/GA_WPN_LightAtk_Combo1~4` BP 资产存在，父类是 `UYogGameplayAbility`（不走 PlayerMeleeAttack）。日志中只见 `GA_Player_LightAtk*` 激活、未见 `GA_WPN_*` —— 大概率为旧资产未授予。**编码前需在编辑器确认 `B_PlayerOne` 的 GASTemplate.AbilityMap 是否引用 GA_WPN_* BP**，若引用需手动在 BP CDO 加 `Weapon.Type.Melee` Required Tag。

## 方案设计

### 选型：Tag 守卫（GAS 原生最干净）

装备时给 ASC 加 `Weapon.Type.Melee` 或 `Weapon.Type.Ranged` LooseTag，所有武器专用 GA 通过 `ActivationRequiredTags` 过滤。与现有 `ActivationBlockedTags`（如 `State.Musket.Aiming`）机制一致，最小侵入。

### Tag 命名空间（注册到 `Config/Tags/Equip.ini`）

```ini
[/Script/GameplayTags.GameplayTagsList]
GameplayTagList=(Tag="Weapon.Type",DevComment="武器类型父节点")
GameplayTagList=(Tag="Weapon.Type.Melee",DevComment="近战武器装备时挂在 ASC 上；玩家近战 GA 用作 ActivationRequiredTags")
GameplayTagList=(Tag="Weapon.Type.Ranged",DevComment="远程武器装备时挂在 ASC 上；火枪 GA 用作 ActivationRequiredTags")
```

### 数据层

```cpp
// Source/DevKit/Public/Item/Weapon/WeaponTypes.h（新建）
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Melee   UMETA(DisplayName = "近战"),
    Ranged  UMETA(DisplayName = "远程"),
};
```

```cpp
// WeaponDefinition.h 新增
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
EWeaponType WeaponType = EWeaponType::Melee;  // 默认近战，向后兼容
```

### Tag 维护 helper（封装在 ASC，方便未来切多人）

```cpp
// YogAbilitySystemComponent.h 新增
UFUNCTION(BlueprintCallable, Category = "Weapon")
void ApplyWeaponTypeTag(EWeaponType Type);   // 先 Clear 再 Set Count=1

UFUNCTION(BlueprintCallable, Category = "Weapon")
void ClearWeaponTypeTags();   // 用 SetLooseGameplayTagCount(Tag, 0) 清零，不是 Remove
```

> **关键**：使用 `SetLooseGameplayTagCount(Tag, 0)` 替代 `RemoveLooseGameplayTag` —— Codex 指出后者只减一次计数，重复装备/恢复时残留累计 → Melee+Ranged 同时激活。`SetLooseGameplayTagCount(Tag, 0)` 一次清零无残留。
>
> **多人扩展点**：未来若要联网，把这两个函数内部从 `AddLooseGameplayTag` 改成 `AddReplicatedLooseGameplayTag`（或改用 Infinite GE 承载）即可，调用方零改动。

### GA 父类加 RequiredTag

```cpp
// UGA_MusketBase 构造函数（GA_MusketBase.cpp）
ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("Weapon.Type.Ranged"));

// UGA_PlayerMeleeAttack 构造函数（GA_PlayerMeleeAttacks.cpp）
ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("Weapon.Type.Melee"));
```

| GA 类 | 加 RequiredTag |
|-------|---------------|
| `UGA_MusketBase` 及子类（Light/Heavy/Reload/SprintAttack/SprintReload）| `Weapon.Type.Ranged` ✅ |
| `UGA_PlayerMeleeAttack` 及子类（LightAtk1~4 / HeavyAtk1~4 / DashAtk）| `Weapon.Type.Melee` ✅ |
| `UGA_MeleeAttack`（通用基类，敌人也用）| **不加** ✅ |
| 通用 GA（GA_Dash / GA_HitReact / GA_Dead / GA_Knockback）| **不加** ✅ |
| 符文 GA / FA | **不加** ✅（符文跨武器通用）|

### 装备/卸装时维护 Tag

```cpp
// WeaponDefinition::SetupWeaponToCharacter 改造
// 旧武器清理代码块下方
if (UYogAbilitySystemComponent* ASC = ReceivingChar->GetASC())
{
    ASC->ClearWeaponTypeTags();
}

// 装备完成后（背包配置注入之后）
if (UYogAbilitySystemComponent* ASC = ReceivingChar->GetASC())
{
    ASC->ApplyWeaponTypeTag(WeaponType);
    UE_LOG(LogTemp, Warning, TEXT("[WeaponDef] Equipped %s | WeaponType=%s | Melee Tag Count=%d Ranged=%d"),
        *GetName(),
        WeaponType == EWeaponType::Melee ? TEXT("Melee") : TEXT("Ranged"),
        ASC->GetTagCount(MeleeTag), ASC->GetTagCount(RangedTag));
}
```

### 流程图

```
玩家按 LMB
    ↓
Controller::LightAtack()
    ↓
TryActivateAbilitiesByTag(PlayerState.AbilityCast.LightAtk)
    ↓
ASC 遍历所有匹配的 GA：
    - UGA_Player_LightAtk1 (Required=Weapon.Type.Melee)  ──┐
    - UGA_Musket_LightAttack (Required=Weapon.Type.Ranged) ┤
                                                            ↓
                                            ASC 检查 Loose Tag：
                                            装备近战 → 仅 LightAtk1 通过
                                            装备火枪 → 仅 Musket_LightAttack 通过
                                            无武器   → 全部不通过（按 LMB 无反应）
```

## 实现步骤

### 步骤 1 — 注册 Tag（`Config/Tags/Equip.ini`）
在文件顶部 `[/Script/GameplayTags.GameplayTagsList]` 段下追加 3 条：
```ini
GameplayTagList=(Tag="Weapon.Type",DevComment="武器类型父节点")
GameplayTagList=(Tag="Weapon.Type.Melee",DevComment="近战武器装备时挂在 ASC，玩家近战 GA 用作 RequiredTag")
GameplayTagList=(Tag="Weapon.Type.Ranged",DevComment="远程武器装备时挂在 ASC，火枪 GA 用作 RequiredTag")
```

### 步骤 2 — 新建 `WeaponTypes.h`
路径：`Source/DevKit/Public/Item/Weapon/WeaponTypes.h`，定义 `EWeaponType`。

### 步骤 3 — `WeaponDefinition.h` 加 `WeaponType` 字段
include `WeaponTypes.h`，加 UPROPERTY，默认 Melee。

### 步骤 4 — ASC helper
- `YogAbilitySystemComponent.h` 加两个 UFUNCTION 声明
- `YogAbilitySystemComponent.cpp` 实现：用 `SetLooseGameplayTagCount(Tag, 0)` 清零 + `AddLooseGameplayTag` 设置

### 步骤 5 — GA 父类加 RequiredTag
- `GA_MusketBase.cpp` 构造函数：`ActivationRequiredTags.AddTag("Weapon.Type.Ranged")`
- `GA_PlayerMeleeAttacks.cpp` `UGA_PlayerMeleeAttack()` 构造函数：`ActivationRequiredTags.AddTag("Weapon.Type.Melee")`

### 步骤 6 — `WeaponDefinition::SetupWeaponToCharacter` 调用
- 旧武器清理段后：`ASC->ClearWeaponTypeTags()`
- 装备完成段：`ASC->ApplyWeaponTypeTag(WeaponType)` + 诊断日志

### 步骤 7 — 编辑器配置（用户操作）
- 现有近战武器 DA：`WeaponType = Melee`（默认值，可不动）
- 火枪武器 DA：`WeaponType = Ranged`
- **关键**：检查 `B_PlayerOne` 的 GASTemplate.AbilityMap 是否引用 `GA_WPN_*` BP；如有则在 BP CDO 上手动加 `Weapon.Type.Melee` Required Tag，或迁移到 C++ 类

### 步骤 8 — 同步文档
- `Docs/Systems/Weapon/WeaponSystem_Technical.md` 加 WeaponType 字段说明
- `Docs/Tags/Tag_Namespaces.md` 加 `Weapon.Type.*` 命名空间说明
- `Docs/FeatureLog.md` 写一条记录

## 涉及文件

| 文件 | 改动 |
|------|------|
| `Config/Tags/Equip.ini` | +3 个 `Weapon.Type.*` Tag（带 ini 头） |
| `Source/DevKit/Public/Item/Weapon/WeaponTypes.h` | **新建**，定义 `EWeaponType` |
| `Source/DevKit/Public/Item/Weapon/WeaponDefinition.h` | +`EWeaponType WeaponType` 字段 + include |
| `Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h` | +`ApplyWeaponTypeTag` / `ClearWeaponTypeTags` 声明 |
| `Source/DevKit/Private/AbilitySystem/YogAbilitySystemComponent.cpp` | 实现两个 helper（`SetLooseGameplayTagCount(Tag, 0)`）|
| `Source/DevKit/Private/Item/Weapon/WeaponDefinition.cpp` | `SetupWeaponToCharacter` 调用 helper + 诊断日志 |
| `Source/DevKit/Private/AbilitySystem/Abilities/GA_MusketBase.cpp` | 构造函数加 `Weapon.Type.Ranged` |
| `Source/DevKit/Private/AbilitySystem/Abilities/GA_PlayerMeleeAttacks.cpp` | `UGA_PlayerMeleeAttack` 构造函数加 `Weapon.Type.Melee` |
| `Docs/Systems/Weapon/WeaponSystem_Technical.md` | +WeaponType 字段、Tag 守卫说明 |
| `Docs/Tags/Tag_Namespaces.md` | +`Weapon.Type.*` 命名空间 |
| `Docs/FeatureLog.md` | +功能日志条目 |

## 风险与缓解（v2 已整合 Codex 6 项硬伤）

| # | 风险 | 缓解措施 |
|---|------|----------|
| 1 | LooseTag 计数累积残留 | 用 `SetLooseGameplayTagCount(Tag, 0)` 清零（封装在 helper 内）|
| 2 | BP CDO 已序列化 RequiredTags 不响应 C++ 父类新增 | 文档明示"若某 BP 不响应武器切换，去 BP CDO 检查 ActivationRequiredTags"。所有玩家 C++ 子类无 BP CDO 不受影响 |
| 3 | 卸装/死亡时 Tag 残留 | 用户决定先不加专用清理入口；用 `SetLooseGameplayTagCount(Tag, 0)` 兜底，重复装备同类型也安全 |
| 4 | `Equip.ini` 格式写错 → Tag 不注册 | 步骤 1 给出完整 ini 头格式 |
| 5 | 多人/客户端预测 LooseTag 不复制 | helper 封装预留，未来一处改 Replicated |
| 6 | `HeavyAttackReleased` 是 SendGameplayEvent 不走 RequiredTag | 不影响——若装备近战，Heavy GA 一开始就被 RequiredTag 拦截无法激活，松开事件无 GA 响应 |
| 7 | `GA_WPN_*` BP 资产可能绕过 | 步骤 7 强制用户在编辑器确认 |
| 8 | 敌人 GA 误受影响 | `UGA_MeleeAttack` 父类不加 Tag，敌人 GA_Enemy_LAtk* 直继 → 不受影响 ✅ |
| 9 | 符文/AI GA 误配 RequiredTag | 文档明示"通用 GA / 符文 GA 不加 RequiredTag" |

## 待确认问题（编码前必查）

1. **`B_PlayerOne` 的 GASTemplate.AbilityMap 里是否引用 `GA_WPN_LightAtk_Combo*` BP？**
   - 若引用 → 需手动在 BP CDO 加 RequiredTag，或迁移到 C++ 类
   - 若未引用 → 可放心，BP 闲置无影响

2. **`UWeaponData`（旧）路径是否还有玩家武器在用？** — 调研发现 `UWeaponData::GrantAbilityToOwner` 是空实现，本方案只改 `UWeaponDefinition`。若有武器仍走 `UWeaponData` 流程，会绕过 Tag 守卫。

## 验证步骤

1. 编译通过
2. PIE 装备近战武器（默认）
   - 输出日志：`[WeaponDef] Equipped ... WeaponType=Melee | Melee Tag Count=1 Ranged=0`
   - 按 LMB → 仅 `GA_Player_LightAtk*` 激活，无 `GA_Musket_*` 日志
3. 切换到火枪武器
   - 输出日志：`[WeaponDef] Equipped ... WeaponType=Ranged | Melee Tag Count=0 Ranged=1`
   - 按 LMB → 仅 `GA_Musket_LightAttack` 激活
4. 切回近战
   - Tag 应该正确切换，不残留
5. 拔武器（如调试或死亡）
   - 按 LMB → 无任何攻击 GA 激活（验证「无武器不能攻击」）

## 多人扩展锚点（未来）

当游戏需要联网时，只需修改 `UYogAbilitySystemComponent::ApplyWeaponTypeTag` 实现：

```cpp
// 单机版（当前）
AddLooseGameplayTag(Tag);

// 多人版（未来）
AddReplicatedLooseGameplayTag(Tag);  // 或用 Infinite GE 承载
```

调用方（`SetupWeaponToCharacter`）零改动。
