# 三选一 Loot UI 系统 — 技术文档

> 更新：2026-04-22

---

## 架构概述

每个 `ARewardPickup` 被拾取时**独立驱动**一次三选一界面，不依赖 GameMode 广播或 CommonUI 堆栈。

```
ARewardPickup::TryPickup()
  └─ YogGameMode::GenerateIndependentLootOptions()   ← 生成选项，设 CurrentLootOptions，不广播
       └─ AYogHUD::ShowLootSelectionUI(Options)       ← 若 Widget 不在 Viewport 则重建
            └─ ULootSelectionWidget::ShowLootUI()     ← 设置可见性 + 暂停 + 输入模式（调用 1 次）
```

LevelFlow（非手动放置路径）仍使用 `GameMode::ShowLootOptions()` + `OnLootGenerated` 广播，Widget 持久存在可直接接收。

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

### AYogGameMode

| 成员 | 类型 | 说明 |
|---|---|---|
| `GenerateIndependentLootOptions()` | `TArray<FLootOption>` | 独立生成（本地 TSet，不广播），供 RewardPickup 使用 |
| `ShowLootOptions(Options)` | `void` | 只广播 `OnLootGenerated`，供 LevelFlow 路径使用 |
| `CurrentLootOptions` | `TArray<FLootOption>` | 两条路径都写入，`SelectLoot()` 从此读取 |
| `bLootOptionsPending` | `bool` | 标记是否有待处理选项（LevelFlow 重建 Widget 时复用） |

### ULootSelectionWidget

| 方法 | 说明 |
|---|---|
| `ShowLootUI(Options)` | 主要入口：设数据、设可见性、暂停、SetInputMode(UIOnly)、SetUserFocus |
| `SelectRuneLoot(Index)` | 选择符文：告知 GM → 隐藏 Widget → 恢复输入 → 开背包 |
| `ConfirmAndTransition()` | 整理完成按钮：调 GM::ConfirmArrangementAndTransition |

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
- `OnLevelPhaseChanged(NewPhase)` BP 事件：阶段切换时控制整个面板 Show/Hide
- `OnCardFocused(FocusedIndex)` BP 事件：手柄高亮对应卡片

---

## 常见问题

| 症状 | 根因 | 检查点 |
|---|---|---|
| 第二个拾取物只暂停不显示 UI | Widget 被移出 Viewport，但指针非 null | HUD 里用 `IsInViewport()` 检查（已修复） |
| ShowLootUI 被调用多次 | OnLootGenerated delegate + HUD 直调 + NativeConstruct pending 三路并发 | 确保 RewardPickup 路径只走 `GenerateIndependentLootOptions` + `HUD->ShowLootSelectionUI`，不广播 |
| Widget 选完后被销毁 | WBP Event On Focus Lost → RemoveFromParent（CommonUI 传播） | `NativeOnFocusLost` 不调 Super（已修复） |
| 背包无法开启 | OpenBackpack 在 SetInputMode(GameOnly) 之前调用 | 确保恢复顺序：SetPause→SetInputMode→OpenBackpack |
