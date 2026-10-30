# 三选一 Loot UI 系统 — 技术文档

> 更新：2026-04-22

---

## 架构概述

每个 `ARewardPickup` 被拾取时**独立驱动**一次三选一界面。拾取物不读取 GameMode：选项由 `UYogLootSubsystem` 生成，发放也由它完成。

```
ARewardPickup::TryPickup()
  └─ UYogLootSubsystem::GenerateRuneOptions(LootTable)   ← 本世界去重，null 表退回 Project Settings 的 DefaultRewardLootTable
       └─ AYogHUD::QueueLootSelection(Options, Pickup)    ← 若 Widget 不在 Viewport 则重建
            └─ ULootSelectionWidget::ShowLootUI()         ← 设置可见性 + 暂停 + 输入模式（调用 1 次）
                 └─ SelectRuneLoot → UYogLootSubsystem::GrantLoot → Pickup::ConsumeAndDestroy
```

---

## 关键设计决策

### 1. Widget 持久化，不走 CommonUI Stack

`LootSelectionWidget` 在 `AYogHUD::BeginPlay()` 创建，`AddToViewport(15)` 并保持整个游戏生命周期。
**不使用** `ActivateWidget()` / `DeactivateWidget()`，只通过 `SetVisibility` 控制显隐。

**为什么不放进 CommonUI Stack**：CommonUI Stack 的 `DeactivateWidget()` 会触发 `bDestroyOnDeactivation` → `RemoveFromParent()`；Widget 被销毁后绑定失效，第二次拾取时无法响应。

### 2. 三处 CommonUI 隔离

| 覆盖点 | 做法 | 原因 |
|---|---|---|
| `GetDesiredInputConfig()` | 返回空 `TOptional<FUIInputConfig>()` | CommonUI 不管理此 Widget 的输入栈，防止 SetInputMode 变化时触发 DeactivateWidget |
| `NativeOnDeactivated()` | 不调 `Super` | Super 检查 `bDestroyOnDeactivation`（WBP 里可能为 true）→ `RemoveFromParent()` |
| `NativeOnFocusLost()` | 不调 `Super` | Super → `BP_OnFocusLost` → WBP 的 `Event On Focus Lost` 逻辑 → 延迟 `RemoveFromParent()` |

### 3. HUD 的 IsInViewport() 重建检查

```cpp
// YogHUD::ShowLootSelectionUI
if (!LootSelectionWidget || !LootSelectionWidget->IsInViewport())
{
    LootSelectionWidget = CreateWidget<ULootSelectionWidget>(...);
    LootSelectionWidget->AddToViewport(15);
}
LootSelectionWidget->ShowLootUI(Options);
```

用 `IsInViewport()` 而非 `GetIsEnabled()`。`UPROPERTY()` 保持对象存活，即使 Widget 已被移出 Viewport，指针仍非 null，`GetIsEnabled()` 仍返回 true——只有 `IsInViewport()` 能正确检测。

### 4. SelectRuneLoot 的输入恢复顺序

```cpp
// 必须先恢复游戏状态，再开背包
PC->SetPause(false);
PC->SetShowMouseCursor(false);
PC->SetInputMode(FInputModeGameOnly());
HUD->EndPauseEffect();
HUD->OpenBackpack();   // BackpackWidget 自己设置 UIOnly 输入模式
```

顺序不能颠倒：如果先 `OpenBackpack()` 再 `SetInputMode(GameOnly)`，GameOnly 会覆盖背包设置的 UIOnly。

---

## C++ 接口一览

### AYogHUD

| 成员 | 类型 | 说明 |
|---|---|---|
| `LootSelectionWidgetClass` | `TSubclassOf<ULootSelectionWidget>` | 在 HB_PlayerMain Details 赋值 WBP_LootSelection |
| `LootSelectionWidget` | `UPROPERTY() TObjectPtr<>` | 持久实例，BeginPlay 创建 |
| `ShowLootSelectionUI(Options)` | `void` | 检查 IsInViewport，必要时重建，调用 ShowLootUI |

### UYogLootSubsystem（`UWorldSubsystem`）

| 成员 | 类型 | 说明 |
|---|---|---|
| `GenerateRuneOptions(Table, bAvoidRepeats)` | `TArray<FLootOption>` | 从 `URewardLootTableDataAsset` 抽符文；同世界内已出过的符文会被跳过，所以同房间多个拾取物互不重复 |
| `GrantLoot(Player, Option)` | `bool` | Rune / Gold / Material / WeaponSkill 的唯一发放入口，成功后广播 `OnLootSelected` |
| `OnLootSelected` | `FOnLootSelected` | 供 `LENode_WaitForLootSelected` 等待 |
| `ResolveMaterialCurrencyTag(Tag)` | `static` | 空 Tag 时退回 `Currency.Meta.Common.A` |

配置在 Project Settings > Yog：`DefaultRewardPickupClass`（击杀掉落和 Story 节点用）与 `DefaultRewardLootTable`（符文池 + 张数）。拾取物也可以在自身 `LootTable` 上指定独立的表。

### ULootSelectionWidget

| 方法 | 说明 |
|---|---|
| `ShowLootUI(Options)` | 主要入口：设数据、设可见性、暂停、SetInputMode(UIOnly)、SetUserFocus |
| `SelectRuneLoot(Index)` | 选择符文：`UYogLootSubsystem::GrantLoot` → 销毁拾取物 → 隐藏 Widget → 恢复输入 |

---

## Type Choice Step (optional)

A reward pickup can ask the player which **type** of loot they want before any loot is granted.
It is off by default, so existing room rewards and kill drops are unchanged.

```
Hold E -> ARewardPickup::TryPickup()
   |- bOfferLootTypeChoice off, or LootTypeChoices empty -> normal flow above
   `- on -> PushScreen(LootTypeChoice)  (ULootTypeChoiceWidget)
        |- Rune          -> ResolveLootTypeChoice -> HUD->QueueLootSelection (3 cards)
        |- Gold/Material/WeaponSkill -> GrantImmediateLoot -> ConsumeAndDestroy
        `- Back / Esc / leave range  -> ResetForSkip (pickup stays, can re-interact)
```

| Item | Notes |
|---|---|
| Config on pickup | `bOfferLootTypeChoice` + `LootTypeChoices` (`FLootTypeChoice`: type, amount, currency tag, weapon skill, label, icon) |
| Screen ID | `EYogUIScreenId::LootTypeChoice` (appended last, enum is serialized as uint8) |
| Widget | `ULootTypeChoiceWidget`, optional `ChoiceContainer` panel; builds a fallback layout without one |
| Rune branch | Reuses `AssignedLoot` when it contains runes, otherwise `UYogLootSubsystem::GenerateRuneOptions(LootTable)` |
| Input policy | Treated as an interactive menu screen: pauses the game, shows the cursor, disables pawn input |
| Missing registry row | `TryPickup` logs a warning and falls back to the normal flow instead of blocking the pickup |
| Lootable corpses | A corpse pickup whose class has the flag on will show this popup too |

---

## HUD Blueprint 配置

`HB_PlayerMain`（父类 `AYogHUD`）Details 面板：

| 分类 | 属性 | 值 |
|---|---|---|
| Loot | `LootSelectionWidgetClass` | `WBP_LootSelection` |

**注意**：不要在 HB_PlayerMain 的蓝图变量里再创建一个 LootSelectionWidget 实例——C++ BeginPlay 已经创建，重复创建会导致两个实例同时响应。

---

## WBP_LootSelection 注意事项

- **不要**在 `Event On Focus Lost` 里调 `Remove From Parent`——C++ 已覆盖 `NativeOnFocusLost` 使其无效，但 BP 节点保留会造成混淆，建议删除
- **不要**把此 Widget 放进 CommonUI Overlay/Stack 容器——直接 AddToViewport 即可
- `OnLootOptionsReady(LootOptions)` BP 事件：收到选项时填充 3 张符文卡片数据
- `OnCardFocused(FocusedIndex)` BP 事件：手柄高亮对应卡片

---

## 常见问题

| 症状 | 根因 | 检查点 |
|---|---|---|
| 第二个拾取物只暂停不显示 UI | Widget 被移出 Viewport，但指针非 null | HUD 里用 `IsInViewport()` 检查（已修复） |
| Widget 选完后被销毁 | WBP Event On Focus Lost → RemoveFromParent（CommonUI 传播） | `NativeOnFocusLost` 不调 Super（已修复） |
| 背包无法开启 | OpenBackpack 在 SetInputMode(GameOnly) 之前调用 | 确保恢复顺序：SetPause→SetInputMode→OpenBackpack |
