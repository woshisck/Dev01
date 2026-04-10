# 传送门与关卡奖励系统设计

> 适用范围：关卡结算流程 / 切关机制 / 战利品触发  
> 适用人群：策划 + 程序  
> 配套文档：[传送门配置指南](../FeatureConfig/Portal_ConfigGuide.md)、[关卡系统技术文档](LevelSystem_ProgrammerDoc.md)、[跨关状态持久化](CrossLevelState_Technical.md)  
> 最后更新：2026-04-10

---

## 一、概述

传送门系统解决两个问题：

1. **关卡分支** — 每关结束时玩家面对多扇门，每扇门通向不同的下一关（从预设池中随机选）
2. **战利品触发** — 最后一个敌人死亡处生成拾取物，玩家走近触发符文选择界面，而不是立刻弹出 UI

> 类比：《哈迪斯》的出口门 + 《以撒》的道具触碰机制。

---

## 二、系统架构

### 2.1 完整流程

```
所有敌人消灭
  └─ YogGameMode::CheckLevelComplete()
       └─ EnterArrangementPhase()
            ├─ 解锁背包（SetLocked(false)）
            ├─ 发放金币（AddGold，按难度配置随机范围）
            ├─ 在 LastEnemyKillLocation 生成 ARewardPickup
            └─ ActivatePortals()（随机决定哪些门开启）

玩家靠近 ARewardPickup
  └─ OnOverlapBegin()
       └─ GM->GenerateLootOptions()
            └─ OnLootGenerated 广播（3 个符文选项）→ UI 弹出
       └─ Destroy()（拾取物销毁）

玩家选择符文
  └─ GM->SelectLoot(Index)
       └─ Player->AddRuneToInventory()

玩家走进已开启的 APortal
  └─ OnOverlapBegin()
       └─ EnterPortal（BlueprintNativeEvent）
            └─ GM->TransitionToLevel(SelectedLevel)
                 ├─ 锁背包
                 ├─ 保存 RunState 到 GameInstance
                 └─ OpenLevel(SelectedLevel)
```

### 2.2 Actor 职责分工

| Actor | 职责 |
|---|---|
| `APortal` | 承载门的视觉效果和碰撞；记录目标关卡；触发切关 |
| `ARewardPickup` | 触发战利品选择流程；触碰后销毁自身 |
| `AYogGameMode` | 决定哪些门开启；分配目标关卡；执行切关和状态保存 |

---

## 三、数据结构

### 3.1 APortal 关键字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `Index` | `int32` | 场景内唯一标识（与 CampaignData 中的 PortalIndex 对应）|
| `bIsOpen` | `bool` | 是否已开启（BeginPlay 时为 false）|
| `SelectedLevel` | `FName` | GameMode 分配的目标关卡名 |

### 3.2 CampaignDataAsset 相关结构

**FPortalDestConfig**（单个传送门的目标配置）：

| 字段 | 类型 | 说明 |
|---|---|---|
| `PortalIndex` | `int32` | 对应场景中 APortal.Index |
| `NextLevelPool` | `TArray<FName>` | 目标关卡随机池，关卡结束时随机抽一个 |

**FFloorEntry**（每关配置）新增字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `PortalDestinations` | `TArray<FPortalDestConfig>` | 本关所有传送门的目标配置 |

### 3.3 YogGameMode 关键字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `LastEnemyKillLocation` | `FVector` | 最后一个被击杀敌人的位置（由 EnemyCharacterBase::Die() 写入）|
| `RewardPickupClass` | `TSubclassOf<AActor>` | 在 GameMode BP 中指定奖励拾取物类 |

---

## 四、随机开门规则

### 4.1 当前规则（真随机，保证最少 1 扇开启）

1. 将 `PortalDestinations` 数组进行 Fisher-Yates 洗牌
2. 洗牌后顺序处理每个配置：
   - **第一个** → 必须开启（保证至少 1 门可用）
   - **后续每个** → 50% 概率开启
3. 每个开启的门从自己的 `NextLevelPool` 中随机选一个目标关卡

### 4.2 示例

```
PortalDestinations = [
    { PortalIndex: 0, NextLevelPool: ["Level_Forest_A", "Level_Forest_B"] },
    { PortalIndex: 1, NextLevelPool: ["Level_Prison_A"] },
    { PortalIndex: 2, NextLevelPool: ["Level_Market"] },
]

洗牌后假设顺序变为 [1, 0, 2]：
  门 1 → 必开 → 随机选 "Level_Prison_A"
  门 0 → 50% → 假设开 → 随机选 "Level_Forest_A"
  门 2 → 50% → 假设不开 → 关闭
```

### 4.3 ⚠️ 待完善

当前规则为纯随机，后续计划增加：
- `MinOpenPortals` — 最少开几扇
- `MaxOpenPortals` — 最多开几扇

相关字段预留位置：`FFloorEntry` 或 `FDifficultyConfig`（待设计）。

---

## 五、视觉效果规范

门的视觉效果（如雾效、光效）由蓝图实现：

| 事件 | 触发时机 | BP 职责 |
|---|---|---|
| `DisablePortal` | BeginPlay（关卡开始）| 显示封闭状态（雾效、暗色）|
| `EnablePortal` | `Open()` 被调用时 | 消散封闭效果，显示可进入状态 |

两个方法均为 `BlueprintImplementableEvent`，C++ 侧不做任何视觉操作。

---

## 六、注意事项

| 情况 | 行为 |
|---|---|
| `Portal.Index` 与 `PortalDestinations` 中无匹配项 | 该门永远不会被开启 |
| `NextLevelPool` 为空 | 该门不参与随机开启 |
| `RewardPickupClass` 未在 GameMode 中配置 | 不生成拾取物，玩家无法触发战利品界面 |
| `LastEnemyKillLocation` 为 ZeroVector | RewardPickup 生成在世界原点（通常意味着没有敌人被击杀）|
| 传送门未放入当前关卡场景 | `ActivatePortals()` 找不到 APortal，不报错，但无门可进 |

---

## 七、相关代码位置

| 功能 | 文件 |
|---|---|
| 传送门 Actor | `Source/DevKit/Public/Map/Portal.h` / `Private/Map/Portal.cpp` |
| 奖励拾取物 | `Source/DevKit/Public/Map/RewardPickup.h` / `Private/Map/RewardPickup.cpp` |
| 开门/切关逻辑 | `Source/DevKit/Private/GameModes/YogGameMode.cpp`（`ActivatePortals` / `TransitionToLevel`）|
| 击杀位置记录 | `Source/DevKit/Private/Character/EnemyCharacterBase.cpp`（`Die()`）|
| 数据结构 | `Source/DevKit/Public/Data/CampaignDataAsset.h`（`FPortalDestConfig` / `FFloorEntry`）|