# CommonUI UI重构 + 背包金币系统 — 工作报告 2026-04-16

## 本次迭代目标

将背包 UI 和三选一 UI 从手动 `SetVisibility / SetPause / SetBlockGameInput` 管理模式
迁移至 **CommonUI `ActivateWidget / DeactivateWidget`** 模式，
同时将金币系统从 `PlayerCharacterBase` 迁移到 `BackpackGridComponent`，并添加买卖符文接口。

---

## 变更概览

### 架构调整

| 变更 | 旧方案 | 新方案 |
|------|--------|--------|
| BackpackScreenWidget 基类 | `UUserWidget` | `UCommonActivatableWidget` |
| LootSelectionWidget 基类 | `UUserWidget` | `UCommonActivatableWidget` |
| UI 开关方式 | 手动 `SetVisibility` + `SetBlockGameInput` | `ActivateWidget()` / `DeactivateWidget()` |
| 输入模式切换 | Controller 手动调 `SetInputMode` | `NativeOnActivated/Deactivated` 各自处理 |
| 战斗 HUD 显隐 | 无 | 菜单激活时隐藏，全部关闭后恢复 |
| 金币归属 | `PlayerCharacterBase` | `BackpackGridComponent` |
| 符文买卖接口 | 无 | `BuyRune` / `SellRune` |

---

## 文件变更清单

### `Source/DevKit/Public/Data/RuneDataAsset.h`

`FRuneConfig` 结构体新增字段：

```cpp
/** 购买价格（金币）。卖出价 = GoldCost / 2，由系统自动计算。0 = 免费 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
int32 GoldCost = 0;
```

---

### `Source/DevKit/Public/Component/BackpackGridComponent.h`

新增金币委托和完整经济系统接口：

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

// Economy 区块
UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
int32 Gold = 0;

UPROPERTY(BlueprintAssignable, Category = "Economy|Events")
FOnGoldChanged OnGoldChanged;

void AddGold(int32 Amount);
bool SpendGold(int32 Amount);
bool CanAffordRune(const URuneDataAsset* DA) const;
bool BuyRune(URuneDataAsset* DA);
bool SellRune(FGuid RuneGuid);
```

---

### `Source/DevKit/Private/Component/BackpackGridComponent.cpp`

实现全部经济接口：

- `AddGold`：`Gold += Amount; OnGoldChanged.Broadcast(Gold)`（Amount ≤ 0 时忽略）
- `SpendGold`：金币不足返回 false，不扣除
- `CanAffordRune`：纯查询，`Gold >= DA->RuneInfo.RuneConfig.GoldCost`
- `BuyRune`：调用 `SpendGold(DA->RuneInfo.RuneConfig.GoldCost)`
- `SellRune`：先读 `PlacedRune.Rune.RuneConfig.GoldCost / 2`，调用已有 `RemoveRune()`（触发 BuffFlow 停止 + `OnRuneRemoved`），再 `AddGold(refund)`

---

### `Source/DevKit/Public/Character/PlayerCharacterBase.h`

删除整个货币区块：
- `int32 Gold`
- `FGoldChangedDelegate OnGoldChanged`
- `void AddGold(int32 Amount)`
- `int32 GetGold() const`

---

### `Source/DevKit/Private/Character/PlayerCharacterBase.cpp`

- 删除 `AddGold()` 实现
- `RestoreRunStateFromGI()` 中金币恢复改为：
  ```cpp
  if (BackpackGridComponent)
  {
      BackpackGridComponent->Gold = FMath::Max(0, State.CurrentGold);
      BackpackGridComponent->OnGoldChanged.Broadcast(BackpackGridComponent->Gold);
  }
  ```

---

### `Source/DevKit/Private/GameModes/YogGameMode.cpp`

更新 3 处对 Player 金币的调用：
- 金币奖励：`Player->BackpackGridComponent->AddGold(GoldReward)`
- RunState 保存（×2）：`NewState.CurrentGold = Player->BackpackGridComponent ? Player->BackpackGridComponent->Gold : 0`

---

### `Source/DevKit/Public/UI/BackpackScreenWidget.h`

| 变更 | 详情 |
|------|------|
| 基类 | `UUserWidget` → `UCommonActivatableWidget` |
| 删除 | `OpenBackpack()` / `CloseBackpack()` |
| 新增覆盖 | `GetDesiredInputConfig()` / `NativeOnActivated()` / `NativeOnDeactivated()` |
| 新增 UI 绑定 | `TObjectPtr<UButton> SellButton` (BindWidgetOptional) |
| 新增私有方法 | `void OnSellButtonClicked()` |

---

### `Source/DevKit/Private/UI/BackpackScreenWidget.cpp`

| 变更 | 详情 |
|------|------|
| `GetDesiredInputConfig` | 返回 `ECommonInputMode::All`（GameAndUI），Tab 键仍可触发 ToggleBackpack |
| `NativeOnActivated` | SetPause(true) + GameAndUI 输入 + SetUserFocus + 刷新网格 |
| `NativeOnDeactivated` | 清除拖拽/选中状态 + SetPause(false) + GameOnly 输入 |
| `NativeConstruct` | 绑定 `SellButton->OnClicked → OnSellButtonClicked` |
| `OnSellButtonClicked` | 读取 `SelectedCell` 的符文 GUID → 调用 `Backpack->SellRune(RuneGuid)` |
| Tab/Special_Left | `CloseBackpack()` → `DeactivateWidget()` |

---

### `Source/DevKit/Public/UI/LootSelectionWidget.h`

| 变更 | 详情 |
|------|------|
| 基类 | `UUserWidget` → `UCommonActivatableWidget` |
| 新增覆盖 | `GetDesiredInputConfig()` / `NativeOnActivated()` / `NativeOnDeactivated()` |
| 新增事件 | `OnCardFocused(int32 FocusedIndex)` — BlueprintImplementableEvent |

---

### `Source/DevKit/Private/UI/LootSelectionWidget.cpp`

| 变更 | 详情 |
|------|------|
| `GetDesiredInputConfig` | 返回 `ECommonInputMode::Menu`（UIOnly），LMB 完全给 Slate |
| `NativeOnActivated` | SetPause(true) + UIOnly 输入 + SetUserFocus |
| `NativeOnDeactivated` | SetPause(false) + GameOnly 输入 |
| `HandleLootGenerated` | 移除 SetVisibility；改为 `ActivateWidget()` + `OnCardFocused(0)` |
| `SelectRuneLoot` | 移除手动隐藏；改为 `DeactivateWidget()` |
| D-Pad 导航 | 新增 `OnCardFocused(CurrentHighlightIndex)` 调用，通知 BP 更新卡片高亮 |

---

### `Source/DevKit/Public/Character/YogPlayerControllerBase.h`

新增：
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
TSubclassOf<UUserWidget> CombatHUDClass;

void OnMenuWidgetActivated();
void OnMenuWidgetDeactivated();

// private:
int32 ActiveMenuCount = 0;
TObjectPtr<UUserWidget> CombatHUDWidget;
```

---

### `Source/DevKit/Private/Character/YogPlayerControllerBase.cpp`

**BeginPlay**：
- `CombatHUDWidget` 创建并 `AddToViewport(0)`（最底层）
- `LootSelectionWidget` / `BackpackWidget` 创建并 `AddToViewport(10)`
- 两个 Widget 不再手动 `SetVisibility`（由 CommonUI 控制）
- 绑定 `OnWidgetActivated` / `OnWidgetDeactivated` 委托（`AddUObject`）

**ToggleBackpack**：
```cpp
if (BackpackWidget->IsActivated())
    BackpackWidget->DeactivateWidget();
else
    BackpackWidget->ActivateWidget();
```

**OnMenuWidgetActivated / OnMenuWidgetDeactivated**：
- `ActiveMenuCount` 计数器管理多层菜单共存
- 归零时恢复 `CombatHUDWidget` 可见性 + 清除 `bBlockGameInput`

---

### `Content/UI/Playtest_UI/WBP_RuneInfoCard.uasset`

已删除（`git rm`）。由 `WBP_BackpackScreen` 直接内联符文信息，无需独立 Widget。

---

## Blueprint 侧任务清单

以下工作需要在 UE Editor 中完成：

1. **重建 `WBP_BackpackScreen`**
   - 父类：`BackpackScreenWidget`（继承自 UCommonActivatableWidget）
   - Designer 中放置 `Button` 并命名为 `SellButton`（名称精确匹配，供 C++ 自动绑定）
   - 其余面板布局同原设计（BackpackGrid / PendingRuneGrid / 符文信息区域等）

2. **重建 `WBP_LootSelection`**（可基于原版修改）
   - 父类：`LootSelectionWidget`
   - 实现 `OnCardFocused(FocusedIndex)` 事件：显示选中卡片，隐藏其余两张（可加动画）

3. **`B_YogPlayerControllerBase`**
   - Details 面板：`CombatHUDClass` 填入战斗 HUD widget 类（如 `WBP_CombatHUD`）

4. **符文数据资产（DA）**
   - 各符文 DA 的 `GoldCost` 字段填写金币价格（0 = 免费）

---

## 关键设计决策

### 为什么不用 CommonActivatableWidgetStack？

`UCommonActivatableWidgetStack` 需要作为 UserWidget 子节点添加到视口，
需要额外的容器 Widget。为简化实现，本次采用"直接 AddToViewport + 委托通知 Controller" 方案。
`GetDesiredInputConfig()` 的重写保证了未来接入 Stack 时的兼容性。

### 为什么 Gold 放在 BackpackGridComponent？

- 背包（符文容量/格子）与经济（购买/卖出）天然耦合
- `BackpackGridComponent` 已有 `SaveGame` 持久化机制，Gold 直接加 `SaveGame` UPROPERTY 即可
- 避免 `PlayerCharacterBase` 持续膨胀（SRP 原则）

### CommonUI 输入模式差异

| Widget | InputMode | 原因 |
|--------|-----------|------|
| BackpackScreenWidget | `ECommonInputMode::All` | Tab 键需要触发 Enhanced Input 关闭背包 |
| LootSelectionWidget | `ECommonInputMode::Menu` | LMB 必须完全给 Slate，不被攻击 Action 消耗 |

---

## 已知限制 / 后续

- `SellRune` 目前返还 `GoldCost / 2`（整数除法），符文价格为奇数时向下取整
- 暂无"买符文"的 UI 入口（BuyRune 接口已就绪，等商店系统接入）
- CommonUI 完整 Stack 支持（Widget Push/Pop 动画）可在内容扩充阶段接入
