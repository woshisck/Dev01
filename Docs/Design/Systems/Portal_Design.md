# 传送门与关卡奖励系统设计

> 适用范围：关卡结算流程 / 切关机制 / 战利品触发  
> 适用人群：策划 + 程序  
> 配套文档：[传送门配置指南](../FeatureConfig/Portal_ConfigGuide.md)、[关卡系统技术文档](LevelSystem_ProgrammerDoc.md)、[跨关状态持久化](CrossLevelState_Technical.md)  
> 最后更新：2026-04-12（与代码对齐：FPortalDestConfig.RoomPool 类型修正；FFloorConfig 字段修正；UCampaignDataAsset 单 RoomPool；开门规则精确描述）

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
            ├─ 发放金币（按 FFloorConfig.GoldMin/Max 随机）
            ├─ 在 LastEnemyKillLocation 生成 ARewardPickup
            └─ ActivatePortals()
                 ├─ 骰出下一关房间类型（所有门共享同一次类型骰子）
                 ├─ Fisher-Yates 洗牌 ActiveRoomData.PortalDestinations
                 └─ 遍历洗牌后的列表，依次开门（第一个必开，后续 50%）

玩家靠近 ARewardPickup
  └─ OnOverlapBegin()
       └─ Player->PendingPickup = this（登记引用，等待主动交互）

玩家按 E 键（IA_Interact）
  └─ YogPlayerControllerBase::Interact()
       └─ Player->PendingPickup->TryPickup(Player)
            ├─ GM->GenerateLootOptions()
            │    └─ OnLootGenerated 广播（3 个符文选项）→ UI 弹出
            └─ Destroy()（拾取物销毁）

玩家选择符文
  └─ GM->SelectLoot(Index)
       └─ Player->AddRuneToInventory()

玩家走进已开启的 APortal
  └─ OnOverlapBegin()
       └─ EnterPortal（BlueprintNativeEvent）
            └─ GM->TransitionToLevel(ChosenRoom->RoomName, ChosenRoom)
                 ├─ 锁背包
                 ├─ 保存 RunState 到 GameInstance
                 ├─ GI->PendingRoomData = SelectedRoom（下一关的 DA_Room）
                 ├─ GI->PendingNextFloor = CurrentFloor + 1
                 └─ OpenLevel(ChosenRoom->RoomName)
```

### 2.2 Actor 职责分工

| Actor | 职责 |
|---|---|
| `APortal` | 承载门的视觉效果和碰撞；记录目标关卡名和 DA_Room；触发切关 |
| `ARewardPickup` | 触发战利品选择流程；接触后登记引用，按 E 触发 |
| `AYogGameMode` | 决定哪些门开启；分配目标 DA_Room；执行切关和状态保存 |

---

## 三、数据结构

### 3.1 APortal 关键字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `Index` | `int32` | 场景内唯一标识（与 DA_Room.PortalDestinations 中的 PortalIndex 对应）|
| `bIsOpen` | `bool` | 是否已开启（BeginPlay 时为 false）|
| `bWillNeverOpen` | `bool` | 关卡开始时确定永不开启（未登记在 PortalDestinations）；由 GameMode 设置 |
| `SelectedLevel` | `FName` | 目标关卡名（= ChosenRoom->RoomName），由 GameMode 通过 Open() 写入 |
| `SelectedRoom` | `URoomDataAsset*` | 目标房间 DA，由 GameMode 通过 Open() 写入 |

### 3.2 相关数据结构

**FPortalDestConfig**（单个传送门的目标配置，存在 **DA_Room.PortalDestinations** 中）：

| 字段 | 类型 | 说明 |
|---|---|---|
| `PortalIndex` | `int32` | 对应场景中 APortal.Index |
| `RoomPool` | `TArray<TObjectPtr<URoomDataAsset>>` | 此门专属的 DA_Room 候选池（按类型 Tag 过滤后随机选）；为空时回退到 Campaign 全局 RoomPool |

> 关卡文件名 = 选中的 `DA_Room.RoomName`，不需要单独配置关卡名列表。

**FFloorConfig**（每关宏观配置，存在 **DA_Campaign.FloorTable** 中）：

| 字段 | 类型 | 说明 |
|---|---|---|
| `FloorNumber` | `int32` | 第几关（参考序号）|
| `TotalDifficultyScore` | `int32` | 总难度分（驱动档位选择 + 波次预算）|
| `bForceElite` | `bool` | 强制精英关 |
| `EliteChance` | `float` | 精英关概率（0-1）|
| `ShopChance` | `float` | 商店概率（0-1）|
| `EventChance` | `float` | 事件房概率（0-1）|
| `CommonWeight/RareWeight/EpicWeight` | `float` | 符文稀有度权重 |
| `GoldMin/GoldMax` | `int32` | 结算金币范围 |
| `BuffCount` | `int32` | 从 DA_Room.BuffPool 选几个 Buff 施加给怪 |

**UCampaignDataAsset**（全局 DA_Room 候选池）：

| 字段 | 类型 | 说明 |
|---|---|---|
| `FloorTable` | `TArray<FFloorConfig>` | 关卡序列 |
| `LayerTag` | `FGameplayTag` | 层级 Tag，过滤用 |
| `RoomPool` | `TArray<TObjectPtr<URoomDataAsset>>` | **所有类型** DA_Room 混放，按 RoomTags 过滤选取 |
| `DefaultStartingRoom` | `TObjectPtr<URoomDataAsset>` | 第一关强制使用 |

> DA_Campaign 只有一个 `RoomPool`（混放所有类型），不分 Normal/Elite/Shop 池。系统用类型 Tag 自动筛选。

### 3.3 YogGameMode 关键字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `LastEnemyKillLocation` | `FVector` | 最后一个被击杀敌人的位置（由 EnemyCharacterBase::Die() 写入）|
| `RewardPickupClass` | `TSubclassOf<AActor>` | 在 GameMode BP 中指定奖励拾取物类 |
| `ActiveRoomData` | `URoomDataAsset*` | 当前关卡的 DA_Room（含 PortalDestinations）|

---

## 四、随机开门规则与房间选取

### 4.1 开门规则（Fisher-Yates，保证至少 1 扇开启）

传送门配置读自 **ActiveRoomData.PortalDestinations**（DA_Room 里配置）：

1. 将 `PortalDestinations` 数组进行 Fisher-Yates 洗牌
2. 洗牌后顺序处理每个配置：
   - **第一个** → 必须开启（保证至少 1 门可用）
   - **后续每个** → 50% 概率开启

### 4.2 下一关房间类型骰子（所有门共享一次骰子）

所有门共享同一次类型骰子（不是每门独立骰）：

```
读取下一关的 FFloorConfig（FloorTable[CurrentFloor]）
  如果 bForceElite = true → 类型固定为 Elite
  否则：
    Roll = random(0.0, 1.0)
    Roll < EliteChance                              → Room.Type.Elite
    Roll < EliteChance + ShopChance                → Room.Type.Shop
    Roll < EliteChance + ShopChance + EventChance  → Room.Type.Event
    else                                           → Room.Type.Normal
```

骰出 `RequiredRoomType` 后，所有门按此类型选 DA_Room。

### 4.3 DA_Room 选取（SelectRoomByTag，四级优先级）

每扇门独立执行 SelectRoomByTag(&Cfg, RequiredRoomType)：

```
1. 门专属 RoomPool（FPortalDestConfig.RoomPool）中，找类型+层级 Tag 都匹配的 DA_Room → 优先
2. Campaign 全局 RoomPool，找类型+层级 Tag 都匹配的 DA_Room
3. 若第 1、2 步都找不到（如 Elite 池为空）：
   门专属 RoomPool 中任意选一个（无视类型，降级兜底）
4. 若门专属池为空：Campaign RoomPool 中 Normal 类型兜底
```

选中后：`ChosenRoom->RoomName` = 关卡文件名，直接传给 `OpenLevel()`。

### 4.4 示例

```
当前关 DA_Room.PortalDestinations（洗牌后假设顺序）：
  {PortalIndex=1, RoomPool=[DA_Room_Forest_Normal, DA_Room_Forest_Elite]}
  {PortalIndex=0, RoomPool=[DA_Room_Prison_Normal]}
  {PortalIndex=2, RoomPool=[]}   ← 门专属池为空，走 Campaign 全局池

下一关 FloorConfig: EliteChance=0.3, ShopChance=0.1, EventChance=0.0
骰子结果：Roll=0.15 → Elite

门1（Index=1）→ 必开 → 门专属池找 Elite → DA_Room_Forest_Elite
  → RoomName = "Level_Forest_01" → 开门

门0（Index=0）→ 50% 概率开 → 门专属池找 Elite → 没有 → Campaign 全局池找 Elite → DA_Room_Prison_Elite
  → RoomName = "Level_Prison_01" → 开门（若概率满足）

门2（Index=2）→ 50% 概率开 → 门专属池为空 → Campaign 全局池找 Elite → DA_Room_Prison_Elite
  → 开门（若概率满足）
```

### 4.5 ⚠️ 待完善

- `MinOpenPortals` / `MaxOpenPortals` 最少/最多开门数量配置（当前固定"第一个必开 + 后续 50%"）

---

## 五、视觉效果规范

门的视觉效果（如雾效、光效）由蓝图实现：

| 事件 | 触发时机 | BP 职责 |
|---|---|---|
| `DisablePortal` | BeginPlay（关卡开始）| 显示封闭状态（雾效、暗色）|
| `EnablePortal` | `Open()` 被调用时 | 消散封闭效果，显示可进入状态 |
| `NeverOpen` | `StartLevelSpawning()` 期间，由 GameMode 调用 | 隐藏门效果，切换为纯装饰状态，禁用碰撞 |

三个方法均为 `BlueprintImplementableEvent`，C++ 侧不做任何视觉操作。

### 5.1 NeverOpen 门的 BP 实现建议

```
Event Never Open
  ├─ SetCollisionEnabled(CollisionVolume, NoCollision)
  ├─ HideDeactivate / SetVisibility(DoorFX, false)
  └─ SetVisibility(StaticDecoMesh, true)（可选：显示静态封堵装饰）
```

> ⚠️ 注意：`DisablePortal`（关卡开始）和 `NeverOpen`（GameMode 初始化后）均会被调用。  
> `NeverOpen` 调用时机比 `DisablePortal` 稍晚，确保最终状态正确即可，两者之间不会有可见闪烁。

---

## 六、注意事项

| 情况 | 行为 |
|---|---|
| `Portal.Index` 与 `PortalDestinations` 中无匹配项 | 该门被标记 `bWillNeverOpen=true`，调用 `NeverOpen()`，显示为纯装饰 |
| 门专属 `RoomPool` 为空 | 系统回退到 Campaign 全局 RoomPool |
| Campaign 全局 RoomPool 和门专属池都找不到所需类型 | 回退到 Normal 类型兜底 |
| `DA_Room.RoomName` 为空 | 该门跳过，不开启（避免 OpenLevel 传空名） |
| `RewardPickupClass` 未在 GameMode 中配置 | 不生成拾取物，玩家无法触发战利品界面 |
| `LastEnemyKillLocation` 为 ZeroVector | RewardPickup 生成在玩家当前位置（兜底） |
| 传送门未放入当前关卡场景 | `ActivatePortals()` 找不到 APortal，不报错，但无门可进 |

---

## 七、相关代码位置

| 功能 | 文件 |
|---|---|
| 传送门 Actor | `Source/DevKit/Public/Map/Portal.h` / `Private/Map/Portal.cpp` |
| 奖励拾取物 | `Source/DevKit/Public/Map/RewardPickup.h` / `Private/Map/RewardPickup.cpp` |
| 开门/切关逻辑 | `Source/DevKit/Private/GameModes/YogGameMode.cpp`（`ActivatePortals` / `TransitionToLevel`）|
| 击杀位置记录 | `Source/DevKit/Private/Character/EnemyCharacterBase.cpp`（`Die()`）|
| 传送门目标结构 | `Source/DevKit/Public/Data/RoomDataAsset.h`（`FPortalDestConfig` / `URoomDataAsset.PortalDestinations`）|
| 关卡序列结构 | `Source/DevKit/Public/Data/CampaignDataAsset.h`（`FFloorConfig` / `UCampaignDataAsset`）|
