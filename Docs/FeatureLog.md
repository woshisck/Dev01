# 星骸降临 Feature Log

> 用途：记录每个完成功能的关键信息，出现 Bug 时直接把对应条目发给 Claude。  
> 格式：每条 log 包含 **功能名 / 涉及文件 / 核心接口 / 配置入口 / 已知限制**。

---

## 2026-04-16

### [CAM-001] 相机管理系统 — AYogCameraPawn

**状态**：已实现，蓝图配置待完成  
**Commit**：`3b04ec14`

| 项目 | 内容 |
|------|------|
| 核心文件 | `YogCameraPawn.h/.cpp`、`CameraConstraintActor.h/.cpp` |
| 依赖文件 | `YogGameMode`（敌人注册表）、`GA_PlayerDash`（SetDashMode）、`YogPlayerControllerBase`（Input_CameraLook） |
| 状态枚举 | `EYogCameraStates`：Dash / CombatFocus / CombatSearch / PickupFocus / LookAhead / FocusCharacter |
| 蓝图接入 | `BP_YogPlayerController` → `Camera Pawn Class = BP_YogCameraPawn` |
| 手柄输入 | `IMC` 绑定 `IA_CameraLook` → Gamepad Right Thumbstick 2D-Axis |
| 边界约束 | 关卡中放置 `ACameraConstraintActor`，Details 填多边形顶点 |
| 震动接口 | `GetOwnCamera → NotifyHeavyHit() / NotifyCritHit()` |
| 设计文档 | [Camera_Design.md](Design/Systems/Camera_Design.md) |

**已知限制**
- 每个关卡只能放一个 `CameraConstraintActor`（自动查找第一个）
- 冲刺时 LookAheadAlpha 强制重置为 0，避免残余偏移

---

### [UI-001] 背包手柄适配 + 战斗日志系统

**状态**：C++ 完成，蓝图 WBP_RuneTooltip / WBP_LootSelection 事件绑定待完成  
**Commit**：`809ebe5b`

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackScreenWidget.h/.cpp`、`LootSelectionWidget.h/.cpp`、`RuneTooltipWidget.h/.cpp` |
| 战斗日志 | `CombatLogStatics.h/.cpp`、`YogAbilitySystemComponent`（PushEntry） |
| 手柄键位（背包） | D-Pad 移动光标 / A 抓取放置 / B 取消 / Y 移除 |
| 手柄键位（三选一） | D-Pad 左右切换 / A 确认 |
| Tooltip 接口 | `ShowRuneInfo(RuneDA)` / `HideTooltip()`（蓝图 event 实现） |
| Swap 逻辑 | 拖到有符文格自动互换，失败时回滚 |
| 设计文档 | [BackpackGamepadAndUI.md](Design/BackpackGamepadAndUI.md) |

**已知限制**
- Swap 失败回滚后光标位置保持原格子，不跳转
- 浮空拖拽效果（1.08×放大）仅在鼠标拖拽时触发，手柄 A 键两步式不触发浮空

---

### [UI-002] CommonUI UI重构 + 背包金币系统

**状态**：C++ 完成，蓝图 WBP_BackpackScreen / WBP_LootSelection 需重建  
**Commit**：本次提交

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackScreenWidget.h/.cpp`、`LootSelectionWidget.h/.cpp`、`BackpackGridComponent.h/.cpp`、`YogPlayerControllerBase.h/.cpp` |
| 基类变更 | BackpackScreenWidget / LootSelectionWidget 均改为 `UCommonActivatableWidget` |
| UI 开关 | `ActivateWidget()` / `DeactivateWidget()`，Controller 侧 `ActiveMenuCount` 计数 |
| 战斗HUD管理 | `CombatHUDClass`（BP Details填入）→ 菜单激活时隐藏，全部关闭后恢复 |
| 输入模式 | 背包：`ECommonInputMode::All`；三选一：`ECommonInputMode::Menu` |
| 手柄导航 | D-Pad 切换卡片 → `OnCardFocused(Index)` 广播给 BP（蓝图实现隐藏其余两张） |
| 金币接口 | `BackpackGridComponent::AddGold / SpendGold / CanAffordRune / BuyRune / SellRune` |
| 金币委托 | `FOnGoldChanged`（Dynamic），UI 监听刷新显示 |
| GoldCost | `FRuneConfig` 新增 `GoldCost` 字段；卖出价 = GoldCost / 2 |
| SellButton | `WBP_BackpackScreen` 放置 `Button` 命名 `SellButton`，C++ 自动绑定 → `SellRune` |
| 设计文档 | [CommonUI重构工作报告](WorkReports/UICommonUI_WorkReport_20260416.md) |

**蓝图待完成**
- 重建 `WBP_BackpackScreen`（父类 BackpackScreenWidget，加 SellButton）
- 重建 `WBP_LootSelection`（实现 `OnCardFocused` 事件）
- `B_YogPlayerControllerBase` Details 填入 `CombatHUDClass`

---

### [LOOP-001] 主循环 — 波次刷怪 + 切关 + 三选一

**状态**：逻辑层完整，UI 接入完成  
**Commit**：`64bb6aae`

| 项目 | 内容 |
|------|------|
| 核心文件 | `YogGameMode.h/.cpp` |
| 触发条件 | AllEnemiesDead / PercentKilled_50 / PercentKilled_20 / Timer |
| 兜底池 | `FallbackLootPool`（无 ActiveRoomData 时自动使用） |
| 跨关状态 | `FRunState`：HP / 金币 / 背包符文（YogSaveSubsystem 持久化） |
| 配置入口 | `DA_Campaign` + `DA_Room`（敌人池 / 难度 / 传送门目标） |
| 设计文档 | [MainLoop_Design.md](Design/Systems/MainLoop_Design.md) |

**已知限制**
- 波次全批次刷出失败时需手动调 `CheckWaveTrigger()`（已内置，不需要外部调用）
- 同一关卡中 PortalIndex 必须唯一，重复会导致门分配错误

---

### [COMBAT-001] 近战攻击 + 连击系统

**状态**：完整  
**Commit**：`e271ce5f`（韧性） + 早期 commit

| 项目 | 内容 |
|------|------|
| 核心文件 | `GA_MeleeAttack.h/.cpp` |
| 连击控制 | `ComboWindow` / `EarlyExit` / `ClearBuffer` |
| 命中判定 | `AN_MeleeDamage`（AnimNotify）→ `IsInAnnulus`（环形扇区） |
| 韧性系统 | `Stagger.Action` Tag + GE 驱动，GA_MeleeAttack 写入 |
| 伤害容器 | `EffectContainerMap`（DA 配置，Key = GameplayTag） |
| 配置入口 | `BP_GA_MeleeAttack` Details → EffectContainerMap / ComboMontages |
| 设计文档 | [AttackDamage_Design.md](Design/Systems/AttackDamage_Design.md) |

**已知限制**
- InnerR 补偿：`bAutoOffset=true` 时 EffectiveOuterRadius = OuterRadius + InnerR
- 连击 Buffer 仅记录最后一次输入，不排队多次

---

### [COMBAT-002] 冲刺系统 — GA_PlayerDash

**状态**：完整  
**Commit**：`d30dd48a` 区段

| 项目 | 内容 |
|------|------|
| 核心文件 | `GA_PlayerDash.h/.cpp` |
| 越障算法 | 终点步进法（6×50cm），无 SetActorLocation |
| 碰撞通道 | WorldDynamic + Pawn → Overlap（与蓝图对齐） |
| 无敌帧 | 冲刺期间 Tag `State.Invincible` 激活 |
| 相机联动 | Activate → `SetDashMode(true)`；End → `SetDashMode(false)` |
| 配置入口 | `BP_GA_PlayerDash` Details → AbilityDA / Tag |
| 设计文档 | [Dash_Design.md](Design/Systems/Dash_Design.md) |

**已知限制**
- 敌人胶囊需对 DashTrace 通道设为 Ignore，否则会被敌人卡住

---

### [BACKPACK-001] 背包网格 + 热度激活系统

**状态**：逻辑层完整，UI 显示完成  
**Commit**：背包系统系列

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackGridComponent.h/.cpp`、`BackpackScreenWidget.h/.cpp` |
| 热度联动 | Phase 0-3 驱动激活区大小，`CanPhaseUp` 精确控制 |
| 永久符文格 | 内圈固定激活，不受热度影响 |
| 自动入格 | 三选一后 `AddRuneToInventory` 优先自动寻位，满才进 PendingRunes |
| 移动逻辑 | 点选格子 → 点击空格子 → MoveRune（C++ 处理） |
| 多格符文 | Shape.Cells 支持，当前展示版本固定 1×1 |
| 配置入口 | `WBP_BackpackScreen` → AvailableRunes（测试用）、BackpackGridComponent DA |
| 设计文档 | [BackpackSystem_Technical.md](Design/Systems/BackpackSystem_Technical.md) |

**已知限制**
- 多格异形符文 UI 显示未实现（Shape 数据已存，只是渲染未做）
- AvailableRunes 为测试展示库，不消耗；PendingRunes 为实际获得符文，放入后消耗

---

### [MAP-001] 传送门系统 — APortal

**状态**：完整  
**Commit**：系统细化系列

| 项目 | 内容 |
|------|------|
| 核心文件 | `Portal.h/.cpp` |
| 状态接口 | `EnablePortal()` / `DisablePortal()` / `NeverOpen()`（均为 BlueprintNativeEvent） |
| 美术配置 | `ClosedArt` / `NeverOpenArt` / `DestinationArtMap`（TMap<FName, FPortalArtConfig>）|
| 分配逻辑 | GameMode 调 `Open(LevelName, RoomDA)` 后自动查 DestinationArtMap 切换美术 |
| 配置入口 | Portal 蓝图 Details → Index / DestinationArtMap / ClosedArt / NeverOpenArt |
| 设计文档 | [Portal_Design.md](Design/Systems/Portal_Design.md) |

**已知限制**
- PortalIndex 必须与 `DA_Campaign.PortalDestinations[i].PortalIndex` 对应，错位会导致门永不开启

---

### [COMBAT-003] 武器系统 — WeaponSpawner + WeaponInstance + 热度发光

**状态**：完整  
**Commit**：本次提交

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponSpawner.h/.cpp`、`WeaponInstance.h/.cpp`、`WeaponDefinition.h/.cpp`、`PlayerCharacterBase.h/.cpp` |
| 拾取方式 | Overlap 进入范围 → `PendingWeaponSpawner` 登记，按 E → `TryPickupWeapon`（与 RewardPickup 同模式） |
| 换武器逻辑 | 旧 Spawner 恢复原色（`OriginalMeshMaterials`），旧 WeaponInstance Destroy，热度委托 RemoveDynamic |
| 热度发光 | `OnHeatPhaseChanged(Phase)` 通过 Overlay Material + `EmissiveColor` 参数驱动 Fresnel 边缘光；Phase 1=白 / 2=绿 / 3=橙黄 / 4=过热红 |
| 触发源 | `PlayerCharacterBase` 监听 GAS Tag `Buff.Status.Heat.Phase.1/2/3`（`RegisterGameplayTagEvent`），tag 变化自动广播 `OnHeatPhaseChanged` |
| 追赶同步 | 拾取时查 ASC 当前 Phase 立即广播，兼容升阶早于拾取的情况 |
| 切关恢复 | `WeaponDefinition::SetupWeaponToCharacter` 同样绑定委托 + 追赶同步，切关后发光不丢失 |
| 配置入口 | `DA_WPN_*` → `HeatOverlayMaterial` 填入 Overlay 材质；`BP_WeaponSpawner` → `BlackedOutMaterial` |
| Overlay 材质需求 | 混合模式 Additive，Unlit；暴露 `EmissiveColor`（Vector3）参数；Fresnel 控制边缘衰减 |

**已知限制**

- `ActorsToSpawn` 数组有多项时只有最后一个 WeaponInstance 绑定热度委托（单武器设计，暂无多件武器需求）
- `BlackedOutMaterial` 需在 `BP_WeaponSpawner` CDO 填入，不在 DA 配置

---

## 格式说明

```
### [系统缩写-序号] 功能名称

**状态**：完整 / 逻辑完整蓝图待配 / 进行中
**Commit**：hash

| 核心文件 | ... |
| 配置入口 | ... |
| 已知限制 | ... |
```

**系统缩写**：CAM 相机 / UI 界面 / COMBAT 战斗 / LOOP 主循环 / BACKPACK 背包 / MAP 地图关卡 / BUFF 符文Buff / AUDIO 音效
