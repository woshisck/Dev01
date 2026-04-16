# 背包与符文激活系统 — 技术文档

> 适用范围：背包网格、符文放置/激活/移动、三选一拾取流程  
> 适用人群：程序  
> 配套文档：[背包系统配置指南](../FeatureConfig/BackpackSystem_Guide.md) · [BuffFlow 程序接入指南](../BuffFlow/BuffFlow_ProgrammerGuide.md)  
> 最后更新：2026-04-14

---

## 一、系统架构总览

```
[RewardPickup] 玩家按 E
  └─ GM::GenerateLootOptions()
       └─ 广播 OnLootGenerated → WBP_LootSelection 显示三选一
            └─ 玩家选择 → GM::SelectLoot(index)
                 └─ Player::AddRuneToInventory(Rune)
                      └─ BackpackGridComponent::TryPlaceRune()  ← 自动寻位放格子
                           └─ OnRunePlaced 广播 → WBP_BackpackScreen 刷新

[战斗中]
AttributeSet::PostAttributeChange(Heat)
  └─ BackpackGridComponent::OnHeatValueChanged(HeatValue)
       └─ RefreshAllActivations()
            ├─ 计算当前激活区（由 CurrentPhase 决定大小）
            └─ 符文在激活区 → ActivateRune() → BuffFlowComponent::StartBuffFlow()
               符文不在激活区 → DeactivateRune() → BuffFlowComponent::StopBuffFlow()
```

---

## 二、核心类说明

### `UBackpackGridComponent`

**路径：** `Source/DevKit/Public/Component/BackpackGridComponent.h`

| 方法 | 说明 |
|---|---|
| `TryPlaceRune(Rune, Pivot)` | 尝试在 Pivot 放置，失败返回 false（位置冲突）|
| `RemoveRune(RuneGuid)` | 移除指定符文，自动 Deactivate |
| `MoveRune(RuneGuid, NewPivot)` | 等价于 Remove + TryPlace，原子操作 |
| `GetActivationZoneCells()` | 返回当前阶段的激活区格子坐标（UI 高亮用）|
| `GetRuneIndexAtCell(Cell)` | 返回格子上的符文下标，-1 = 空 |
| `GetAllPlacedRunes()` | 返回所有已放置符文（UI 用）|
| `OnHeatValueChanged(HeatValue)` | 由 AttributeSet 调用，触发激活区重算 |
| `SetLocked(bool)` | 锁定背包（战斗阶段 = true，整理阶段 = false）|

**委托（UI 绑定用）：**

| 委托 | 触发时机 |
|---|---|
| `OnRunePlaced` | TryPlaceRune 成功后 |
| `OnRuneRemoved` | RemoveRune 成功后 |
| `OnRuneActivationChanged` | 符文激活/取消激活状态变化后 |

### `UBackpackScreenWidget`

**路径：** `Source/DevKit/Public/UI/BackpackScreenWidget.h`

蓝图父类，提供所有背包 UI 逻辑。蓝图继承后只需实现视觉事件，不需要处理数据。

| 方法/属性 | 类型 | 说明 |
|---|---|---|
| `AvailableRunes` | 属性 | 展示/调试用固定符文库，在 Details 填 DA_Rune_* |
| `GetRuneList()` | Pure | 返回 PendingRunes + AvailableRunes 合并列表 |
| `GetPendingRuneCount()` | Pure | PendingRunes 数量（区分三选一 vs 固定库）|
| `GetCellVisualState(Col,Row)` | Pure | 返回 `EBackpackCellState` 枚举，蓝图据此设颜色 |
| `IsCellInActivationZone(Col,Row)` | Pure | 格子是否在激活区 |
| `IsCellOccupied(Col,Row)` | Pure | 格子是否有符文 |
| `GetRuneAtCell(Col,Row)` | Pure | 获取格子上的符文实例 |
| `IsCellSelected(Col,Row)` | Pure | 格子是否被选中（用于高亮边框）|
| `SelectRuneFromList(Index)` | Callable | 从列表选中符文，再次调用取消 |
| `ClickCell(Col,Row)` | Callable | 格子点击，内部处理选中/放置/移动三种情况 |
| `RemoveRuneAtSelectedCell()` | Callable | 移除选中格子的符文 |
| `ClearSelection()` | Callable | 清除所有选中状态 |

**蓝图需实现的事件：**

| 事件 | 触发时机 | 蓝图应做的事 |
|---|---|---|
| `OnGridNeedsRefresh` | 格子数据变化 | 遍历 5×5，用 GetCellVisualState 刷新颜色 |
| `OnSelectionChanged` | 选中状态变化 | 刷新列表高亮、格子高亮、描述面板 |
| `OnRuneListChanged` | PendingRunes 变化 | 重新调用 GetRuneList 刷新左侧列表 |
| `OnStatusMessage(Text)` | 操作结果 | 显示提示文字 2 秒 |

---

## 三、数据结构

### `FRuneInstance`（符文运行时实例）

| 字段 | 类型 | 说明 |
|---|---|---|
| `RuneConfig.RuneName` | FName | 符文名称 |
| `RuneConfig.RuneIcon` | UTexture2D* | 符文图标 |
| `RuneConfig.RuneDescription` | FText | 符文描述 |
| `RuneConfig.RuneType` | ERuneType | Buff / Debuff / None |
| `Shape.Cells` | TArray\<FIntPoint\> | 背包格子形状（相对偏移，当前展示版固定 1x1）|
| `Flow.FlowAsset` | UFlowAsset* | BuffFlow 逻辑资产，激活时自动启动 |
| `RuneGuid` | FGuid | 唯一标识，RemoveRune/MoveRune 时使用 |
| `UpgradeLevel` | int32 | 升级等级：0=Lv.I，1=Lv.II，2=Lv.III（满级） |

### `FPlacedRune`（已放置符文状态）

| 字段 | 类型 | 说明 |
|---|---|---|
| `Rune` | FRuneInstance | 符文数据 |
| `Pivot` | FIntPoint | 左上角格子坐标（Col, Row）|
| `bIsActivated` | bool | 是否处于激活状态 |
| `bIsPermanent` | bool | 永久符文，跳过激活区检查 |

### `EBackpackCellState`（格子视觉状态）

| 枚举值 | 含义 | 建议颜色 |
|---|---|---|
| `Empty` | 空格子 | `#3A3A3A` 深灰 |
| `EmptyActive` | 空但在激活区 | `#1A3A6A` 深蓝 |
| `OccupiedActive` | 有符文且已激活 | `#2266CC` 亮蓝 |
| `OccupiedInactive` | 有符文但未激活 | `#7A4A1A` 暗橙 |

> 选中状态叠加黄色高亮 `#FFAA00`，覆盖以上颜色。

---

## 四、三选一拾取流程详解

```
初始关卡中预放置 3 个 BP_RewardPickup

玩家走近（Overlap）→ Player.PendingPickup = RewardPickup
玩家按 E（InputAction）→ RewardPickup::TryPickup()
  └─ bPickedUp = true（防重复拾取）
  └─ GM::GenerateLootOptions()
       ├─ 优先：ActiveRoomData.LootPool（关卡配置的符文池）
       ├─ 兜底：GameMode.FallbackLootPool（在 GameMode BP 配置）
       └─ 广播 OnLootGenerated(CurrentLootOptions)

WBP_LootSelection::HandleLootGenerated()
  └─ OnLootOptionsReady(LootOptions) → 显示 3 张符文卡片

玩家点选第 N 张 → SelectRuneLoot(N)
  └─ GM::SelectLoot(N)
       └─ Player::AddRuneToInventory(RuneAsset.CreateInstance())
            └─ BackpackGridComponent::TryPlaceRune() 逐格扫描自动放置
                 ├─ 放置成功 → OnRunePlaced 广播 → UI 刷新
                 └─ 背包已满 → 加入 PendingRunes（下次打开背包可手动放）
```

---

## 五、热度激活流程

```
玩家攻击命中 → BFNode_IncrementHeat（或 GE 修改 Heat 属性）
  └─ PlayerAttributeSet::PostAttributeChange(Heat)
       └─ BackpackGridComponent::OnHeatValueChanged(HeatValue)
            └─ RefreshAllActivations()
                 └─ ComputeActivationZone()（由 CurrentPhase 决定大小）
                      ├─ Phase 0：中心 1×1
                      ├─ Phase 1：中心 2×2
                      └─ Phase 2：中心 4×4
                 └─ 遍历 PlacedRunes：
                      bInZone = IsRuneInActivationZone(Placed)
                      bInZone && !bActivated → ActivateRune() → StartBuffFlow()
                      !bInZone && bActivated → DeactivateRune() → StopBuffFlow()
```

> ⚠️ `OnHeatValueChanged` 必须由外部调用（AttributeSet.PostAttributeChange），`BackpackGridComponent` 本身不监听属性。

---

## 六、符文升级系统

### 6.1 设计规则

| 规则 | 内容 |
| --- | --- |
| 重复限制 | 每种符文（按 RuneName 判断）在背包中只能存在**一张** |
| 拿到重复符文 | 背包中该符文自动升级（Lv.I → II → III），不新占格子 |
| 达到 Lv.III 后 | 该符文从奖励池中过滤，不再出现（后期规划转化为资源） |
| 升级效果 | 线性倍率：Lv.I × 1.0 / Lv.II × 1.5 / Lv.III × 2.0 |

> 升级倍率由 BuffFlow 中的 `UpgradeMultiplier` 节点读取 `UpgradeLevel` 实现，无需为每级单独制作 FlowAsset。

### 6.2 AddRuneToInventory 升级逻辑

```cpp
// PlayerCharacterBase.cpp
void APlayerCharacterBase::AddRuneToInventory(const FRuneInstance& Rune)
{
    if (BackpackGridComponent)
    {
        // 1. 检查背包里是否已有同名符文
        FPlacedRune* Existing = BackpackGridComponent->FindRuneByName(Rune.RuneConfig.RuneName);
        if (Existing)
        {
            if (Existing->Rune.UpgradeLevel < 2)  // 未满级（0/1 → 升到 1/2）
            {
                Existing->Rune.UpgradeLevel++;
                BackpackGridComponent->RefreshAllActivations();
                BackpackGridComponent->OnRuneActivationChanged.Broadcast();
            }
            // 满级（UpgradeLevel == 2）：GenerateLootOptions 已过滤，正常不会到这里
            return;
        }

        // 2. 新符文：自动寻位放置
        for (int32 Row = 0; Row < GridHeight; Row++)
            for (int32 Col = 0; Col < GridWidth; Col++)
                if (BackpackGridComponent->TryPlaceRune(Rune, FIntPoint(Col, Row)))
                    return;
    }
    PendingRunes.Add(Rune);  // 背包满，进待放置列表
}
```

### 6.3 GenerateLootOptions 过滤满级符文

```cpp
// YogGameMode.cpp - GenerateLootOptions 中
// 生成候选列表前，排除玩家背包中已达 Lv.III 的符文类型
TArray<FName> MaxLevelRunes = PlayerCharacter->BackpackGridComponent->GetMaxLevelRuneNames();
Pool = Pool.FilterByPredicate([&](const URuneDataAsset* DA) {
    return !MaxLevelRunes.Contains(DA->RuneInfo.RuneConfig.RuneName);
});
```

### 6.4 新增 BackpackGridComponent 接口

| 方法 | 说明 |
| --- | --- |
| `FindRuneByName(FName)` | 按名称查找已放置符文，返回指针（升级用） |
| `GetMaxLevelRuneNames()` | 返回所有 UpgradeLevel == 2 的符文名称集合（奖励过滤用） |

### 6.5 UI 展示升级等级

背包格子内符文图标右上角叠加等级文字：

| UpgradeLevel | 显示 |
| --- | --- |
| 0 | 无标记（Lv.I 为默认状态） |
| 1 | `II` |
| 2 | `III`（或用金色加以区分） |

三选一卡片中，若该符文玩家已有，卡片标题附加当前等级：`烈焰符文 [Lv.II → III]`。

---

## 七、AddRuneToInventory 自动寻位逻辑（原始版本）

> ⚠️ 此节为旧版本参考，已被第六节的升级逻辑取代。

```cpp
// 原逻辑（无升级判断，仅供参考）
void APlayerCharacterBase::AddRuneToInventory(const FRuneInstance& Rune)
{
    for (int32 Row = 0; Row < GridHeight; Row++)
        for (int32 Col = 0; Col < GridWidth; Col++)
            if (TryPlaceRune(Rune, FIntPoint(Col, Row)))
                return;
    PendingRunes.Add(Rune);
}
```

---

## 八、C++ 文件索引

| 文件 | 作用 |
|---|---|
| `Public/Component/BackpackGridComponent.h` | 背包网格组件（核心）|
| `Private/Component/BackpackGridComponent.cpp` | 格子占用、激活判断、BuffFlow 生命周期 |
| `Public/Data/RuneDataAsset.h` | 符文数据资产、FRuneInstance、FRuneShape 定义 |
| `Public/UI/BackpackScreenWidget.h` | 背包 UI 蓝图基类 |
| `Private/UI/BackpackScreenWidget.cpp` | UI 交互逻辑（选中/放置/移动/移除）|
| `Public/Character/PlayerCharacterBase.h` | AddRuneToInventory、PendingRunes |
| `Public/GameModes/YogGameMode.h` | GenerateLootOptions、FallbackLootPool、SelectLoot |
| `Public/Map/RewardPickup.h` | 场景拾取物，触发三选一 |
