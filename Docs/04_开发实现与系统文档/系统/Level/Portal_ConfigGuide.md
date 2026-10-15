# 传送门系统配置指南

> 适用范围：传送门放置 / 目标关卡配置 / 清怪后开门  
> 适用人群：策划  
> 配套文档：[关卡系统配置指南](LevelSystem_ConfigGuide.md)、[关卡系统程序文档](LevelSystem_ProgrammerDoc.md)  
> 最后更新：2026-10-15（目标关卡改由传送门自身配置；移除 APortal.Index 与 DA_Room.PortalDestinations）

---

## 概述

传送门的目标关卡现在直接写在**门自己身上**，不再经过 DA_Room 的索引表。一个房间要跑通只需两步：

| 步骤 | 位置 | 操作 |
|---|---|---|
| 1 | 关卡场景中 | 放置 `BP_Portal`，填写 `DestinationLevel` |
| 2 | 关卡场景中 | 放置 `AYogRoomDirector`，确认 `LuaModule` |
| 3 | GameMode 蓝图 | 指定 `RewardPickupClass`（需要战利品时）|

---

## 步骤一：放置传送门并填写目标关卡

1. 打开目标关卡（UE 场景编辑器）
2. 从 Content Browser 把 `BP_Portal` 拖入场景，摆到出口位置
3. 在 **Details 面板 → Portal 分类** 填写：

| 字段 | 说明 |
|---|---|
| `DestinationLevel` | 本门通往的关卡资产名，如 `FirstLevel`。留空则这扇门永远打不开 |
| `DestinationArtMap` | 可选。按目标关卡名切换门的网格与特效，Key 填 `DestinationLevel` 的同名值 |
| `ClosedArt` / `NeverOpenArt` | 关闭状态、永不开启状态的美术 |
| `PortalEntryWalkDuration` | 按住 E 后角色自走入门的时长，`0` = 跳过自走 |

一张地图想给玩家分支选择，就摆多扇门、各填不同的 `DestinationLevel`。门之间没有编号关系，摆几扇就是几扇。

> 不再有 Index。门与配置表的对应关系已经取消，门自己就是配置。

---

## 步骤二：放置房间导演（RoomDirector）

房间流程（关门 → 刷怪 → 清怪 → 开门）跑在 Lua 里，由场景中的 `AYogRoomDirector` 驱动。

1. 把 `B_RoomDirector` 拖入场景（每张地图一个）
2. 确认 **Room|Lua 分类 → `LuaModule`**，默认 `Maps.RoomDirector`
3. 调试时可勾 **Room|Debug → `bVerbose`**，屏幕上会打印刷怪数与开门时机

默认脚本 `Content/Script/Maps/RoomDirector.lua` 的流程：

```text
BeginPlay        收集场景内所有 APortal，全部 DisablePortal（玩家看不到开着的门）
OnRoomStart      跑完场景里每个 AMobSpawner，登记刷出来的怪
怪物死亡         计数递减，并重新扫描（兼顾补怪 / 复活）
计数归零         对每扇门调用 OpenPortal()
```

房间里一只怪都没有时，`OnRoomStart` 会直接开门，玩家不会被困住。

需要特殊流程（例如限时、分阶段开门）的地图，**复制一份 Lua 另存**（如 `Maps.Room_Forest01`），再把那张地图的 Director 的 `LuaModule` 指过去，不要改公共脚本。

---

## 步骤三：在 GameMode 中配置奖励拾取物

1. 打开 **BP_YogGameMode**（或项目使用的 GameMode 蓝图）
2. 进入 **Class Defaults**
3. 找到 **LevelFlow 分类 → RewardPickupClass**
4. 填写 `BP_RewardPickup`（或自定义的 `ARewardPickup` 子类）

若不配置此字段，关卡结束后不会生成拾取物，玩家无法触发战利品选择界面。

---

## 传送门预览文案

传送门的下一关预览会显示预骰到的敌人符文。每个符文的短描述优先读取：

```text
RuneInfo -> RuneConfig -> HUDSummaryText
```

该字段建议填写 1-2 行摘要。完整机制说明仍写在 `RuneDescription`，供背包详情、符文信息卡等大面板使用。`HUDSummaryText` 留空时，UI 会从 `RuneDescription` 自动截取短摘要。

---

## 永不开启门（NeverOpen）的蓝图配置

一扇门要做成纯装饰（封死的门洞），有两种做法：

| 做法 | 效果 |
|---|---|
| `DestinationLevel` 留空 | RoomDirector 开门时跳过它并打一条 Warning，门保持关闭 |
| Lua 或蓝图里调 `MarkUnavailable()` | 立即应用 `NeverOpenArt` 并关掉碰撞 |

**在 BP_Portal 中实现 `Event Never Open`（推荐）：**

1. 打开 `BP_Portal`，在事件图表中搜索 `Event Never Open`
2. 连接以下节点：
   - `SetCollisionEnabled`（CollisionVolume，NoCollision）→ 关闭碰撞，防止玩家误进
   - `SetVisibility`（门的粒子/雾效组件，false）→ 隐藏门效果
   - `SetVisibility`（静态封堵网格 StaticDecoration，true）→ 可选：显示砖墙/封堵装饰

> C++ 的 `NeverOpen_Implementation` 已经会套用 `NeverOpenArt` 并禁用碰撞；蓝图事件用于追加额外表现。

---

## 奖励拾取物使用说明（按住 E 拾取）

关卡结算后生成的 `ARewardPickup`（奖励拾取物）不会自动触发，玩家需要**走近后按住 E 键**才能触发符文选择界面。

| 步骤 | 行为 |
|---|---|
| 玩家进入碰撞范围 | 系统自动登记并显示"按住 E"提示 |
| 玩家按住 E 键 | 触发 `TryPickup()`，弹出符文三选一界面，拾取物消失 |
| 玩家离开范围（未按住 E）| 登记清除，再次进入才能重新拾取 |

> 类比：《以撒》的道具台面，靠近显示说明，确认才吃。

---

## 注意事项

| 情况 | 结果 |
|---|---|
| `DestinationLevel` 留空 | 该门不会开启，日志打印 `has no DestinationLevel` |
| `DestinationLevel` 填了不存在的关卡名 | 开门正常，按 E 后切关失败 |
| 场景里没有 `AYogRoomDirector` | 没人开门，清怪后玩家被困 ⚠️ |
| 场景里没有 `APortal` | 不报错，但该房间无出路 |
| `RewardPickupClass` 为空 | 无拾取物生成，战利品选择界面无法弹出 |
| 同一张地图摆多个 RoomDirector | 会重复跑刷怪与开门流程，务必只摆一个 |

---

## 常见问题

**Q：清完怪门没开？**  
A：按顺序排查：
1. 场景里有没有 `AYogRoomDirector`
2. 门的 `DestinationLevel` 是否填了
3. 勾上 Director 的 `bVerbose`，看屏幕打印的怪物计数有没有归零（有怪卡在场景外会让计数不归零）

**Q：`DestinationLevel` 应该填什么格式？**  
A：填 UE 关卡资产的名称（不含路径和扩展名），例如 `FirstLevel`。

**Q：门开启后视觉上没有变化？**  
A：开门会按 `SelectedLevel` 查 `DestinationArtMap`；该 Map 里没有对应条目时门保持原样。要么补齐 `DestinationArtMap`，要么在 BP_Portal 里实现 `On Portal Opened` 自行表现。

**Q：想让某扇门在剧情节点才开？**  
A：用 StoryFlow 的 `Enable Portal` 节点，按 Actor Tag 指定目标门。

---

## ⚠️ Claude 编写注意事项

- **房间流程写在 Lua，不写进 GameMode**：开门时机属于地图自己的逻辑，归 `AYogRoomDirector` + `Content/Script/Maps/*.lua`，`AYogGameMode` 不再参与传送门开启
- **切关必须经过 Portal**：不要直接调 `UGameplayStatics::OpenLevel`。流程是 `APortal::TryEnter()` → 锁输入 + HUD 渐黑 → `FinishEntry()` 写存档 → `AYogGameMode::TransitionToLevel()`
- **目标关卡来自门自身**：`APortal::DestinationLevel` 是唯一的权威来源，不要重新引入索引表或在 BP 里硬编码关卡名
- **门的交互靠 Overlap 登记 + 按键**：`OnComponentBeginOverlap` 只负责登记可交互对象，真正入门由玩家按住 E 触发，不要用 Tick 轮询距离
- **传送门特效走 Niagara**：门体与特效由 `FPortalArtConfig` 驱动，C++ 只做 `SetAsset` / `Activate`，不在 C++ 里拼粒子参数
