# 传送门系统配置指南

> 适用范围：关卡切换 / 传送门放置 / 目标关卡配置  
> 适用人群：策划  
> 配套文档：[WBP_PortalPreview_Layout](WBP_PortalPreview_Layout.md)、[WBP_PortalDirection_Layout](WBP_PortalDirection_Layout.md)  
> 最后更新：2026-04-26（v3：交互式按 E 进入 + 双层 HUD 预览 + 渐黑过场 + DisplayName 配置 + BP 视觉接口）

---

## 概述

传送门系统需要三步配置才能完整运作：

| 步骤 | 位置 | 操作 |
|---|---|---|
| 1 | 关卡场景中 | 放置 APortal Actor，设置 Index |
| 2 | DA_Campaign 数据资产 | 填写每扇门对应的目标关卡池 |
| 3 | GameMode 蓝图 | 指定 RewardPickupClass |

---

## 步骤一：在关卡中放置传送门

1. 打开目标关卡（UE 场景编辑器）
2. 在 Content Browser 找到 `APortal` 或其蓝图子类（如 `BP_Portal`）
3. 拖入场景，放置到合适位置
4. 在 **Details 面板 → Portal 分类** 中设置 `Index`

**Index 规则**：
- 从 `0` 开始编号
- 同一关卡内的所有 Portal 的 Index 必须唯一
- Index 不需要连续（允许 0、1、5 这样的编号）

> 类比：门的 Index 就是门牌号，数据表里靠这个号找到对应的配置。

---

## 步骤二：在 DA_Campaign 中配置目标关卡

1. 打开 Content Browser → 找到对应的 `DA_Campaign_XXX` 资产
2. 展开 `FloorTable` → 找到目标关卡的 `FloorEntry`
3. 展开 **`PortalDestinations`** 数组
4. 为每扇门添加一个元素，填写：

| 字段 | 说明 |
|---|---|
| `PortalIndex` | 填写对应 APortal 的 Index 值（与场景中设置的一致）|
| `NextLevelPool` | 目标关卡名列表（填 UE 关卡资产名，可填多个）|

**示例配置**：

```
FloorTable[0]（第 1 关）
  PortalDestinations:
    [0] PortalIndex = 0,  NextLevelPool = ["Level_Forest_A", "Level_Forest_B"]
    [1] PortalIndex = 1,  NextLevelPool = ["Level_Prison_A"]
    [2] PortalIndex = 2,  NextLevelPool = ["Level_Market"]
```

关卡结束时：
- 门 0、1、2 按随机顺序处理，第一个必开，后续 50% 概率开
- 每扇开启的门从自己的 `NextLevelPool` 随机选一个目标

---

## 步骤三：在 GameMode 中配置奖励拾取物

1. 打开 **BP_YogGameMode**（或项目使用的 GameMode 蓝图）
2. 进入 **Class Defaults**
3. 找到 **LevelFlow 分类 → RewardPickupClass**
4. 填写 `BP_RewardPickup`（或自定义的 ARewardPickup 子类）

若不配置此字段，关卡结束后不会生成拾取物，玩家无法触发战利品选择界面。

---

## 永不开启门（NeverOpen）的蓝图配置

场景中放置的传送门，如果没有在 `PortalDestinations` 中登记，关卡开始时系统会自动调用 `NeverOpen()` 事件。

**在 BP_Portal 中实现该事件（必做）：**

1. 打开 `BP_Portal`，在事件图表中搜索 `Event Never Open`
2. 连接以下节点：
   - `SetCollisionEnabled`（CollisionVolume，NoCollision）→ 关闭碰撞，防止玩家误进
   - `SetVisibility`（门的粒子/雾效组件，false）→ 隐藏门效果
   - `SetVisibility`（静态封堵网格 StaticDecoration，true）→ 可选：显示砖墙/封堵装饰

> ⚠️ 如果未实现该事件，永不开启的门外观上和关闭门相同（调用过 `DisablePortal`），但碰撞仍存在，玩家会被挡住。

---

## 奖励拾取物使用说明（按 E 拾取）

关卡结算后生成的 `ARewardPickup`（奖励拾取物）不会自动触发，玩家需要**走近后按 E 键**才能触发符文选择界面。

| 步骤 | 行为 |
|---|---|
| 玩家进入碰撞范围 | 系统自动登记（可在此时显示"按 E 拾取"提示，UI 待实现）|
| 玩家按 E 键 | 触发 `TryPickup()`，弹出符文三选一界面，拾取物消失 |
| 玩家离开范围（未按 E）| 登记清除，再次进入才能重新拾取 |

> 类比：《以撒》的道具台面，靠近显示说明，确认才吃。

---

## 注意事项

| 情况 | 结果 |
|---|---|
| Portal.Index 与 PortalDestinations 中无匹配 | 该门被标记 NeverOpen，视觉降为纯装饰 |
| NextLevelPool 为空 | 该配置不参与随机开门逻辑 |
| 所有门的 NextLevelPool 均为空 | 关卡结束后没有任何门可进 ⚠️ |
| 关卡中没有 APortal Actor | ActivatePortals 找不到门，不报错，但无出路 |
| RewardPickupClass 为空 | 无拾取物生成，战利品选择界面无法弹出 |
| BP_Portal 未实现 Event Never Open | 永不开启的门碰撞仍存在，视觉不变，无明显区分 |

---

## 随机开门规则（策划须知）

关卡结束时系统按以下规则决定哪些门开启：

1. `PortalDestinations` 列表随机打乱顺序
2. **第一个门必定开启**（保证玩家至少有一条出路）
3. 后续每扇门各有 **50% 概率**开启
4. 每扇开启的门随机选一个目标关卡

**建议**：每个 FloorEntry 至少配置 2-3 个门，保证玩家有分支选择。

---

## 完整配置示例（1 关 3 门）

**场景侧**：
- 放置 3 个 `BP_Portal`，Index 分别设为 `0`、`1`、`2`

**数据侧**（DA_Campaign_MainRun → FloorTable[0]）：
```
PortalDestinations:
  [0] PortalIndex=0, NextLevelPool=["L_Room_Forest_01", "L_Room_Forest_02"]
  [1] PortalIndex=1, NextLevelPool=["L_Room_Prison_01"]
  [2] PortalIndex=2, NextLevelPool=["L_Room_Market_01"]
```

**GameMode 侧**：
- RewardPickupClass = `BP_RewardPickup`

---

## 常见问题

**Q：传送门放置好了但关卡结束后门没有开？**  
A：检查以下几点：
1. APortal.Index 是否与 DA_Campaign 中的 PortalIndex 一致
2. 对应的 NextLevelPool 是否填写了合法的关卡名
3. GameMode 的 CampaignData 是否正确指向 DA_Campaign 资产

**Q：NextLevelPool 应该填什么格式？**  
A：填写 UE 关卡资产的名称（不含路径和扩展名），例如 `L_Room_Forest_01`。可以在关卡资产上右键 → Copy Reference 获取完整路径，只取最后的文件名部分。

**Q：门开启后视觉上没有变化？**  
A：视觉效果由蓝图的 `EnablePortal` 事件实现。请确认 BP_Portal 中已实现该蓝图事件（显示雾效消散等效果）。

---

## ⚠️ Claude 编写注意事项

- **切关必须经过 Portal**：不允许在代码里直接调 `UGameplayStatics::OpenLevel`，切关流程是：`APortal::ActivatePortal()` → 触发 `YogSaveSubsystem::SaveAll()` → 延帧后 OpenLevel，确保存档完整
- **DA_Campaign 决定关卡顺序**：`UCampaignDataAsset` 里配置关卡列表和传送门分支，`APortal` 读取 DA 决定目标关卡，不要在 Portal BP 里硬编码 Level 名称
- **随机门逻辑在 C++ 里**：多分支传送门的随机选择在 C++ `APortal::SelectRandomBranch()` 里，不要在 BP 里用 Random Integer 节点（无法统一控制随机种子）
- **玩家进入触发 Overlap**：Portal 的激活靠 `OnComponentBeginOverlap`，Overlap 组件的 ObjectType 只响应 `ECC_Pawn`，不要用 Tick 轮询距离
- **传送门动画走 Niagara**：Portal 的激活特效用 Niagara System，C++ 只调 `UNiagaraFunctionLibrary::SpawnSystemAtLocation`，不在 C++ 里创建粒子参数

---

## v3：交互式按 E 进入 + 双层 HUD 预览 + 渐黑过场

> 重大变更：传送门**不再"踩进就走"**。玩家走近 → HUD 浮窗显示下一关信息 → 按 E 触发渐黑过场后切关。

### 新增配置项一览

| 位置 | 字段 | 说明 |
|---|---|---|
| `URoomDataAsset.DisplayName` | FText | 玩家可见的房间显示名（"监狱地下·第二层"等）；空时回退 `RoomName.ToString()` |
| `BP_Portal.PortalEntryWalkDuration` | float (秒) | 角色自走入门时长，默认 0.7。不必抵达门位置；可在 BP_Portal Details 调整 |
| `BP_Portal.PortalEntryFailSafeBuffer` | float (秒) | 兜底超时余量，默认 0.5；卡墙时 (Walk + 此值) 后强制切关 |
| `BP_YogHUD.PortalPreviewClass` | TSubclassOf | 单例浮窗 WBP，赋 `WBP_PortalPreview` |
| `BP_YogHUD.PortalDirectionClass` | TSubclassOf | 方位指引 WBP，赋 `WBP_PortalDirection` |
| `BP_YogHUD.PortalWidgetSideOffset` | float (px) | 浮窗水平避让偏移，默认 240 |
| `BP_YogHUD.PortalWidgetZOffset` | float (px) | 浮窗距门屏幕投影上方偏移，默认 80 |
| `BP_YogHUD.PortalForceShowDistance` | float (cm) | 即使屏幕外，距玩家 < 此距离的门也算候选，默认 800 |
| `BP_YogHUD.PortalSwitchHysteresis` | float (cm) | Target 切换距离滞回，防中点抖动，默认 100 |
| `BP_YogHUD.PortalBlackoutDuration` | float (秒) | 进入过场渐黑时长，默认 0.5；目标饱和度/亮度复用 `LevelEndEffectDA.BlackoutSaturation/Gain` |

### 主城（HubRoom）特殊处理

- 主城传送门**完全不启用** HUD 引导（不浮窗、不箭头）—— 玩家自己探索
- 控制点：`AYogGameMode::EnterArrangementPhase` 末尾按 `ActiveRoomData->bIsHubRoom` 跳过 `HUD->ShowPortalGuidance()`

### 跨关 Buff 确定性（重要）

- 关卡结算时 `ActivatePortals` 会**为每扇门预骰一次** Buff（基于下一关 FloorConfig 难度档位）
- 玩家 PendingPortal 期间浮窗展示的就是真实即将施加的 N 个 Buff（不是池子）
- 按 E 时 Portal::TryEnter 把本门的 PreRolledBuffs 写入 `GI->PendingRoomBuffs`，下一关 `StartLevelSpawning` 直接使用，不再现场抽
- BuffCount=0 也合法（商店/事件房）；判定预骰是否存在用 `GI->PendingRoomData != null` 信号

### BP 视觉强化接口（可选实现）

`BP_Portal` 可以 override 以下事件追加视觉效果（不实现也能跑）：

| 事件 | 触发时机 | 典型用途 |
|---|---|---|
| `K2_OnHighlightChanged(bool)` | 玩家进出门 Box 时 | 门体边光强度切换 |
| `K2_OnPortalRangeEntered` | 玩家踏入门 Box | 播音效 / 粒子 |
| `K2_OnPortalRangeExited` | 玩家离开门 Box | 收音效 |
| `K2_OnEntrySequenceStart` | 按 E 触发 TryEnter 时 | 门体内吸特效 / 雾效爆开 |

`WBP_PortalPreview` 可以 override：

| 事件 | 触发时机 | 典型用途 |
|---|---|---|
| `K2_OnPreviewShown` | 浮窗刚切到新 Target Portal | 淡入动画 |
| `K2_OnInteractHintShown` | "按 E"提示首次显示 | 闪烁 / 抖动 |

### WBP 搭建任务

| WBP | 父类 | 规格文档 |
|---|---|---|
| `WBP_PortalPreview` | `UPortalPreviewWidget` | [WBP_PortalPreview_Layout.md](WBP_PortalPreview_Layout.md) |
| `WBP_PortalDirection` | `UPortalDirectionWidget` | [WBP_PortalDirection_Layout.md](WBP_PortalDirection_Layout.md) |

搭好后在 `BP_YogHUD` Details 中：
- `Portal Preview Class = WBP_PortalPreview`
- `Portal Direction Class = WBP_PortalDirection`

### 旧行为对比

| 行为 | v2（旧） | v3（新） |
|---|---|---|
| 玩家进 Box | 立即 EnterPortal → 切关 | 仅 PendingPortal 登记 + 浮窗"按 E"提示 |
| 切关触发 | OverlapBegin 自动 | PlayerController::Interact 按 E |
| 下一关 Buff | StartLevelSpawning 现场抽 | Portal::Open 预骰 → TryEnter 写 GI → 下一关读 GI |
| 切关视觉 | 直接 OpenLevel | 渐黑过场 → OpenLevel → 反向渐入 |

> ⚠️ 旧的"踩进就走"教程引导行为已移除。如教程关卡依赖此行为，需补"按 E 进入"提示
