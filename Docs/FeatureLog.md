# 星骸降临 Feature Log

> 用途：记录每个完成功能的关键信息，出现 Bug 时直接把对应条目发给 Claude。  
> 格式：每条 log 包含 **功能名 / 涉及文件 / 核心接口 / 配置入口 / 已知限制**。

---

## 2026-04-19

### [UI-009] 敌人方向箭头指示 — EnemyArrowWidget

**状态**：已实现已编译，WBP_EnemyArrow 蓝图已配置

| 项目 | 内容 |
|------|------|
| 核心文件 | `EnemyArrowWidget.h/.cpp`、`YogGameMode.h/.cpp`（`GetAllAliveEnemies`）、`YogHUD.h/.cpp` |
| 触发条件 | 同时满足：① 所有存活敌人均不在屏幕内 ② 玩家 `AppearDelay` 秒内未受伤 |
| 显示内容 | 最多 `MaxArrows` 个（默认 3）三角箭头，贴屏幕边缘，指向距离最近的敌人 |
| 后方敌人 | `IsOnScreen` 对摄像机后方敌人做坐标镜像，箭头出现在玩家"背向"的屏幕边缘 |
| 投影点修正 | `ArrowProjectionZOffset`（默认 60cm）将投影点从脚底抬至腰部，修正斜视角偏差 |
| 离屏判断双保险 | `OnScreenShrink`（屏幕内缩像素）+ `ForceOffScreenDistance`（世界距离阈值，默认 1500cm） |
| 受伤重置 | 订阅 `YogAbilitySystemComponent::ReceivedDamage`，受伤时重置倒计时 |
| 敌人来源 | `YogGameMode::GetAllAliveEnemies()`，遍历 `AliveEnemies` 弱指针表，过滤 `IsAlive()` |
| WBP 配置 | `WBP_EnemyArrow`，根节点 Canvas Panel 命名 `RootCanvas`（全屏）；Details 填 ArrowTexture、各 Config 参数 |
| HUD 配置 | `BP_YogHUD` Details → EnemyArrow → `Enemy Arrow Widget Class = WBP_EnemyArrow` |

#### Config 参数速查

| 参数 | 默认 | 说明 |
| ---- | ---- | ---- |
| `AppearDelay` | 1.5s | 无伤多久后出现 |
| `MaxArrows` | 3 | 最多同时显示几个 |
| `ArrowSize` | 32px | 箭头图标大小 |
| `EdgeMargin` | 60px | 距屏幕边缘留白 |
| `OnScreenShrink` | 150px | 屏幕内缩判断区 |
| `ForceOffScreenDistance` | 1500cm | 超距强制离屏 |
| `ArrowProjectionZOffset` | 60cm | 投影点高度修正 |
| `ArrowColor` | 黄色(1,0.8,0.2,0.9) | 箭头颜色 |

**已知限制**

- 箭头贴图约定顶点朝上（-Y 方向），`CalcArrowAngle` 已加 90° 补偿；贴图方向不同需在蓝图中调整 `ArrowColor` 或旋转贴图本身

---

### [UI-008] 武器拾取浮窗 — WBP_WeaponFloat

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponInfoDA.h`、`WeaponFloatWidget.h/.cpp`、`WeaponSpawner.h/.cpp`、`WeaponDefinition.h` |
| 触发时机 | 玩家进入 WeaponSpawner CollisionVolume 且朝向武器（±105° 宽松判断） |
| 信息来源 | `WeaponDefinition` → `WeaponInfo`（UWeaponInfoDA）和 `InitialRunes`（TArray<URuneDataAsset>） |
| WeaponInfoDA 字段 | `WeaponName`、`WeaponDescription`、`WeaponSubDescription`、`Thumbnail`、`Zone1/2/3Image` |
| 点阵激活区 | 三个 CanvasPanel（ZoneGrid1/2/3），自动从 `ZoneGridSize` 属性缩放，与 WBP SizeBox 对齐 |
| 图像覆盖 | ZoneNImage 有贴图时替代点阵；无贴图时显示高亮点阵（蓝色激活，暗灰未激活） |
| 符文列表 | BuildRuneList 动态创建 HBox（40×40 图标 + VBox 名称/描述），填入 RuneListBox VerticalBox |
| 动态偏移 | Tick 每帧投影武器到屏幕，武器在左半→向右偏，右半→向左偏，使用摄像机 Right 向量（适配45°斜视角） |
| WBP 控件名 | `WeaponThumbnail`、`WeaponNameText`、`WeaponDescText`、`WeaponSubDescText`、`ZoneGrid1/2/3`、`Zone1/2/3Image`、`RuneListBox` |
| BP 配置 | `BP_WeaponSpawner` Details → 浮窗 → `WeaponFloatWidgetClass`、`WidgetSideOffset`（默认300cm）、`WidgetZOffset`（默认50cm） |
| WidgetComponent | Screen Space，初始隐藏，`SetRelativeLocation` 每帧更新偏移，随武器 Actor 移动 |

**已知限制**

- `InitialRunes` 自动放置到激活区格子的逻辑尚未实现（当前仅在浮窗列表展示）
- CommonActionWidget 按键提示为纯 WBP 配置，不由 C++ 控制

---

## 2026-04-18

### [UI-005] 背包热度阶段点按钮重构 — HeatPhaseDot + delegate 跨 Widget 通信

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackGridWidget.h/.cpp`、`BackpackScreenWidget.h/.cpp` |
| 根因 | `BindWidgetOptional` 不递归到嵌套 UserWidget 内部树，按钮必须在直接拥有它的 Widget C++ 类里绑定 |
| 架构 | BackpackGridWidget 持有 3 个 Button → 广播 `FOnHeatPhaseButtonClicked` delegate → BackpackScreenWidget 订阅处理 |
| 新接口 | `BackpackGridWidget::RefreshHeatPhaseButtons(PreviewPhase, bIsGamepadMode)` |
| WBP 位置 | HeatPhaseDot0/1/2 和 GamepadHintLabel 放在 **WBP_BackpackGrid**（不是 WBP_BackpackScreen）|
| 手柄提示 | 鼠标输入时隐藏；任意键盘/手柄输入时显示 "L1/R1 切换热度显示" |
| 设计文档 | [BackpackUI_StepByStep.md](../UI/BackpackUI_StepByStep.md) |

---

### [UI-006] 格子 1:1 强制约束 — GridSizeBox / PendingGridSizeBox

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackGridWidget.h/.cpp`、`PendingGridWidget.h/.cpp` |
| 方案 | SizeBox 包裹 UniformGridPanel；`BuildGrid` / `BuildSlots` 时计算 `CellSize + Padding*2` 精确写入 SizeBox |
| WBP 配置 | WBP_BackpackGrid 里放 SizeBox（变量名 `GridSizeBox`）→ UniformGridPanel（`BackpackGrid`） |
|  | WBP_PendingGrid 里放 SizeBox（变量名 `PendingGridSizeBox`）→ UniformGridPanel（`PendingRuneGrid`） |
| 已知限制 | SizeBox 的 Slot 必须设为 **Left / Top**（不要 Fill），否则 SizeBox 自身被拉伸 |

---

### [UI-007] 热度区三色叠加/单阶切换显示

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackScreenWidget.h`（enum）、`BackpackStyleDataAsset.h`、`RuneSlotWidget.cpp`、`BackpackGridWidget.cpp` |
| 新枚举值 | `EBackpackCellState::EmptyZone1`（热度2区）、`EmptyZone2`（热度3区）|
| DA 新字段 | `HeatZone0Color` / `HeatZone1Color` / `HeatZone2Color`（分类"热度阶段颜色"，自填）|
| 默认行为 | `PreviewPhase=-1`：三阶叠加全显，Zone0 优先级最高（最内层压盖最外层） |
| 切换行为 | 点 Dot / 按键盘 1/2/3 → `PreviewPhase=0/1/2`：只显示该阶格子，其余置灰；再按 → 恢复叠加 |
| 颜色建议 | 亮蓝→中蓝→暗蓝渐进，视觉上体现热度由内向外递减 |

---

### [VFX-001] 热度升阶发光特效 — 玩家身体 + 武器

**状态**：已实现已编译

| 项目 | 内容 |
|---|---|
| 核心文件 | `PlayerCharacterBase.h/.cpp`、`WeaponInstance.h/.cpp` |
| 触发 | GAS Tag `Buff.Status.Heat.Phase.1/2/3` 新增时自动触发 |
| 玩家配置 | `BP_PlayerCharacterBase` → Heat\|Visual → `PhaseUpPlayerOverlayMaterial` |
| 武器配置 | `WeaponDefinition` DA → `HeatOverlayMaterial` |
| 时序 | 扫射(`GlowSweepDuration`) + 保持(`GlowHoldDuration`) + 淡出(`GlowFadeDuration`) |
| 材质参数 | `SweepProgress`、`GlowAlpha`、`EmissiveColor`、`SwipeCount`、`Power` |
| 设计文档 | [CharacterFlash_Technical.md](Design/VFX/CharacterFlash_Technical.md) |

### [VFX-002] 命中闪白 / 攻击前闪红 — 敌人

**状态**：已实现已编译

| 项目 | 内容 |
|---|---|
| 核心文件 | `YogCharacterBase.h/.cpp` |
| 命中闪白触发 | `HealthChanged` 血量减少时自动调用 `StartHitFlash()` |
| 攻击前闪红触发 | 蓝图调用 `StartPreAttackFlash()` / `StopPreAttackFlash()` |
| 配置入口 | 敌人 BP Details → Combat\|Visual → `CharacterFlashMaterial` |
| 可调参数 | `HitFlashDuration`(默认0.12s)、`PreAttackPulseFreq`(默认4Hz) |
| 材质参数 | `FlashColor`(Vector)、`FlashAlpha`(Scalar)、`Power`(Scalar) |
| 设计文档 | [CharacterFlash_Technical.md](Design/VFX/CharacterFlash_Technical.md) |

---

## 2026-04-16

### [CAM-001] 相机管理系统 — AYogCameraPawn

**状态**：已实现，蓝图配置待完成  
**Commit**：`3b04ec14`

| 项目 | 内容 |
| --- | --- |
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
| --- | --- |
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

### [CAM-003] 相机输入 — 移除鼠标偏移、手柄右摇杆生效

**状态**：已完成，编译通过
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `YogPlayerCameraManager.h/.cpp`、`IMC_YogPlayerBase.uasset` |
| 鼠标偏移 | 从 `.cpp` 彻底移除读取鼠标位置的逻辑，与 `bAutoReadMouseOffset` 属性均删除 |
| 右摇杆 | 创建 `IA_CameraLook`（Axis2D），IMC 绑定 Gamepad Right Thumbstick 2D-Axis |
| 背包手柄键 | IMC 给 `IA_OpenBackpack` 添加 Gamepad Special Left（Select/View 键） |
| 接入方式 | `B_YogPlayerControllerBase` Details → `Input_CameraLook = IA_CameraLook` |

---

### [BACKPACK-002] 背包 UI — StyleDA + RuneInfoCard + 拖拽重写

**状态**：C++ 完成，蓝图 WBP_BackpackScreen 需按新结构重建
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `BackpackScreenWidget.h/.cpp`、`BackpackStyleDataAsset.h/.cpp`、`RuneInfoCardWidget.h/.cpp` |
| StyleDA | `UBackpackStyleDataAsset`：格子颜色 × 7 + 待放置区颜色 × 2 + 尺寸 × 4，无需重编译即可调视觉 |
| StyleDA 配置 | 创建 `DA_BackpackStyle`，拖到 WBP_BackpackScreen Details → Style DA |
| RuneInfoCard | `URuneInfoCardWidget`：`ShowRune(FRuneInstance)` / `HideCard()`；Designer 放 CardIcon / CardName / CardDesc / CardUpgrade |
| 格子状态枚举 | `EBackpackCellState`：Empty / EmptyActive / OccupiedActive / OccupiedInactive |
| 拖拽重写 | 所有拖拽事件由 BackpackScreenWidget 自身接管（格子 HitTestInvisible），移除旧 DragDropOperation 蓝图依赖 |
| 手柄导航 | D-Pad 方向键重复（首按立即响应，持续 0.3s 后每 0.1s 重复），A 确认/B 取消 |

**蓝图待完成**

- WBP_BackpackScreen：添加 `RuneInfoCard`（子 Widget，Visibility=Collapsed）、`StyleDA` 填入 DA_BackpackStyle
- WBP_RuneInfoCard：新建蓝图，放 CardIcon / CardName / CardDesc / CardUpgrade

---

### [COMBAT-003] 玩家攻击 GA 中间层 + 冲刺连招桥接保存

**状态**：C++ 完成，蓝图 GA 无需改动
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `GA_PlayerMeleeAttacks.h/.cpp`、`GA_PlayerDash.h/.cpp`、`YogAbilitySystemComponent.h/.cpp` |
| 中间基类 | `UGA_PlayerMeleeAttack`：构造函数自动绑定 `GE_StatBeforeATK` / `GE_StatAfterATK`，子 GA 不再需手动填写 |
| 具体 GA | `GA_Player_LightAtk1~4`、`GA_Player_HeavyAtk1~4`、`GA_Player_DashAtk` — 蓝图子类直接替换旧 GA |
| 连招桥接 | 冲刺 `CanActivateAbility` 检测 `Action.Combo.DashSavePoint` ANS Tag，命中时缓存当前连招进度 Tag |
| 桥接保存 | `EndAbility`（未取消）调用 `YASC->ApplyDashSave(PendingSaveComboTags)` 为下一击注入 LooseTag |
| 伤害明细 | `FDamageBreakdown`：BaseAttack / ActionMultiplier / FinalDamage / bIsCrit / DamageType 等字段 |
| 伤害委托 | `FOnDamageBreakdown`（Dynamic），DamageBreakdownWidget 订阅后实时显示伤害构成 |

**已知限制**

- `ApplyDashSave` 需在 `YogAbilitySystemComponent` 中实现（本次仅声明调用端）
- 连招桥接要求动画 ANS 在对应帧授予 `Action.Combo.DashSavePoint`

---

### [CAM-002] 相机平滑优化 — 消除根运动僵硬 + LookAhead 开关

**状态**：已完成，编译通过
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `YogPlayerCameraManager.h/.cpp` |
| 根运动平滑 | VInterpTo 起点改为 `GetCameraLocation()`（上一帧输出），消除攻击/冲刺时相机直接 snap 的僵硬感 |
| LookAhead 开关 | `bEnableLookAhead`（默认 false），关闭后相机不再前冲/回弹，消除移动眩晕感 |
| 移动跟随速度 | `MovingFollowSpeed = 8.f`（LookAhead 关闭时生效） |
| 静止归位速度 | `StationarySettleSpeed = 5.f`，静止后缓慢归位，不漂移 |
| 冲刺跟随速度 | `DashFollowSpeed = 18.f`，冲刺高速跟随消除单帧抖动 |
| 参数位置 | `BP_PlayerCameraManager` 蓝图 Details，无需重编译即可调节 |
| 设计文档 | [Camera_Design.md](Design/Systems/Camera_Design.md)（v2.1） |

**已知限制**

- `StationarySettleSpeed` 越低，停下后相机漂移时间越长；建议保持 5~15
- LookAhead 开启时前冲/回弹感明显，仅推荐用于镜头行程大的关卡

---

### [UI-002] CommonUI UI重构 + 背包金币系统

**状态**：C++ 完成，蓝图 WBP_BackpackScreen / WBP_LootSelection 需重建  
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
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
| --- | --- |
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
| --- | --- |
| 核心文件 | `GA_MeleeAttack.h/.cpp` |
| 连击控制 | `ComboWindow` / `EarlyExit` / `ClearBuffer` |
| 命中判定 | `AN_MeleeDamage`（AnimNotify）→ `IsInAnnulus`（环形扇区） |
| 韧性系统 | 已重构为 Resilience 属性比较 + 霸体机制，见 COMBAT-005 |
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
| --- | --- |
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
| --- | --- |
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
| --- | --- |
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
| --- | --- |
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

### [COMBAT-005] 韧性系统 — Poise + 霸体

**状态**：完整  
**Commit**：`16f1cab1`

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `YogAbilitySystemComponent.h/.cpp`、`GA_MeleeAttack.cpp`、`BaseAttributeSet.h` |
| 核心逻辑 | `ReceiveDamage` 中比较攻击方与防御方 Resilience，攻击方 ≤ 防御方则不触发受击 |
| 动作韧性 | `AN_MeleeDamage.ActResilience`（默认 20）由 `GA_MeleeAttack::OnEventReceived` 写入 `CurrentActionPoiseBonus`，`ReceiveDamage` 读取后立即清零 |
| 属性来源 | `BaseAttributeSet.Resilience`（已存在属性，直接复用，在角色属性数据表填写初始值） |
| 霸体机制 | 非玩家角色连续触发受击 ≥ `SuperArmorThreshold`（默认 3）次后，添加 `Buff.Status.SuperArmor` Tag，持续 `SuperArmorDuration`（默认 2s）；霸体期间免疫受击 |
| 计数重置 | 5 秒内无新受击触发则 `PoiseHitCount` 归零 |
| 配置入口 | 敌人角色蓝图 ASC → `Super Armor Threshold` / `Super Armor Duration`；角色数据表 → `Resilience` 初始值；蒙太奇 `AN_MeleeDamage.ActResilience` → 动作韧性 |
| 配置文档 | [PoiseSystem_ConfigGuide.md](FeatureConfig/PoiseSystem_ConfigGuide.md) |

推荐初始值参考：

| 角色 | Resilience | 说明 |
| --- | --- | --- |
| 玩家 | 100 | 基准值 |
| 普通敌人 | 50 | 轻击（20+100=120 > 50）可打出受击 |
| 精英敌人 | 150 | 需要重击（ActResilience 50+100=150）才能打出受击 |

**已知限制**

- 敌人攻击时的动作韧性尚未配置（敌人 GA 需手动写入 `CurrentActionPoiseBonus`）
- 玩家无霸体上限（被连击不会进入霸体，未来可按需扩展）

---

### [COMBAT-004] 冲刺连招保存 — DashSave 桥接系统

**状态**：完整  
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `GA_PlayerDash.h/.cpp`、`YogAbilitySystemComponent.h/.cpp`、`GA_PlayerMeleeAttacks.h/.cpp` |
| 功能描述 | 在攻击连招"桥接窗口"内冲刺，可保留连招进度继续接后续攻击 |
| 触发条件 | 蒙太奇通过 AnimNotifyState 授予 `Action.Combo.DashSavePoint` Tag |
| 保存逻辑 | `GA_PlayerDash::CanActivateAbility` 检测 DashSavePoint Tag，缓存当前连招进度 Tags 到 `PendingSaveComboTags` |
| 恢复逻辑 | `YogASC::ApplyDashSave` 将保存的 Tags 以 LooseGameplayTag 方式重新施加；下次攻击 `ActivateAbility` 时 `ConsumeDashSave` 清除 |
| 自动过期 | 2 秒内未接攻击则 `DashSaveExpired` 自动清理，防止 Tag 残留 |
| 消费节点 | `LightAtk4` / `HeavyAtk4` 的 `ActivateAbility` 中调用 `ConsumeDashSave` |
| Tag 依赖 | `Action.Combo.DashSavePoint`（ANS 授予）、`PlayerState.AbilityCast.CanCombo`、各 Combo 进度 Tag |

**已知限制**

- 目前仅 LightAtk4 / HeavyAtk4 接入消费逻辑；其他连招段若需要桥接需各自添加 `ConsumeDashSave` 调用
- 双冲刺连打时旧保存会被新保存覆盖（`ApplyDashSave` 内部先调 `ConsumeDashSave` 保护）

---

### [FEEL-001] 热度升阶手柄震动

**状态**：完整  
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `PlayerCharacterBase.h/.cpp` |
| 触发时机 | GAS Tag `Buff.Status.Heat.Phase.1/2/3` 新增时（即升阶瞬间） |
| 接口 | `APlayerController::ClientPlayForceFeedback(PhaseUpForceFeedback)` |
| 配置入口 | 角色蓝图（`BP_PlayerCharacterBase` 或 `B_PlayerOne`）→ Class Defaults → `Heat \| Feedback` → `Phase Up Force Feedback` |
| 资产位置 | `Content/Code/Core/ForceFeedbackEffect/FFE_HeatPhaseUp` |
| 曲线资产 | `FFE_HeatPhaseUp_ExternalCurve`（Duration 由曲线最后关键帧决定，非 Duration 字段） |

**已知限制**

- Phase 1 / 2 / 3 使用同一个震动效果；如需按阶段差异化需扩展为 TArray 配置

---

### [UI-003] 背包样式系统 — BackpackStyleDataAsset + RuneInfoCard

**状态**：完整  
**Commit**：`62c298a9` / `ef253286`

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `BackpackScreenWidget.h/.cpp`、`BackpackStyleDataAsset`、`RuneInfoCardWidget` |
| 样式 DA | `DA_BackpackStyle` — 统一管理格子颜色、边框、尺寸等视觉参数，无需改代码 |
| RuneInfoCard | `WBP_RuneInfoCard` — 独立 Widget，选中格子后由 C++ 自动调用 ShowRune/HideCard |
| 颜色系统 | 格子状态颜色（Empty / EmptyActive / OccupiedActive / OccupiedInact / Selected / Hover / GrabbedSource）迁移至 DA |
| 配置入口 | `WBP_BackpackScreen` → Details → `Backpack Style` 填入 `DA_BackpackStyle` |
| 格子渲染 | 每格为 UOverlay + UImage(RoundedBox brush) + UImage(icon)，C++ 动态生成，UniformGridPanel 保持空 |

---

### [UI-004] 拖拽浮空图标修复 — 绑过 DefaultDragVisual

**状态**：完整  
**Commit**：`ef253286`

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `BackpackScreenWidget.h/.cpp` |
| 问题 | UE `DefaultDragVisual` 始终从屏幕 (0,0) 飞向鼠标，无法直接出现在鼠标下方 |
| 解决方案 | 不设置 `DefaultDragVisual`，改用 `GrabbedRuneIcon`（Canvas Panel 根层 Image）；`NativeTick` 每帧用 `LastMouseAbsPos` 定位，`NativeOnDragOver` 每帧更新鼠标坐标 |
| 新增字段 | `bMouseDragging`、`MouseDragTex`、`LastMouseAbsPos`（私有，仅 Tick 用） |
| 手柄复用 | `GrabbedRuneIcon` 同时服务手柄抓取模式（`bGrabbingRune`），两个分支共用同一 Image |
| 设计文档 | [BackpackSystem_Guide.md](Design/FeatureConfig/BackpackSystem_Guide.md) §六 |

---

## 2026-04-18

### [UI-005] RuneInfoCard 视觉重构 — 点阵 + 背景图

**状态**：完整  
**Commit**：待填

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `RuneInfoCardWidget.h/.cpp`、`RuneDataAsset.h` |
| 点阵渲染 | 从 UniformGridPanel 改为 **CanvasPanel 绝对定位**；点固定 **8×8 px**，间隔 **2 px**，整体居中于 64×64 的 `ShapeGrid` CanvasPanel |
| 颜色 | 已占格蓝 `(0.20, 0.60, 1.00)`，空格暗灰 `(0.18, 0.18, 0.22, 0.6)` |
| DA 新字段 | `FRuneConfig::CardBackground`（`UTexture2D*`）— 信息卡背景贴图 |
| CardBG 绑定 | `RuneInfoCardWidget` 新增 `CardBG`（`BindWidgetOptional`）；有贴图时显示贴图，留空时显示不透明黑色 |
| CardEffect 绑定 | 新增 `CardEffect` TextBlock，与 `CardDesc` 分开显示效果描述 |
| WBP 布局 | `ShapeGrid` 在蓝图中为 **CanvasPanel，固定 64×64**；`WBP_RuneInfoCard` 根节点用 SizeBox 定宽高，`WBP_BackpackScreen` 中该实例勾选 **Size To Content**，只管位置 |
| 配置入口 | 各符文 DA → `Card Background` 字段填入背景贴图（留空 = 纯黑兜底） |

---

### [UI-006] 教程引导系统 — TutorialManager + TutorialPopupWidget

**状态**：C++ 完整，蓝图 WBP_TutorialPopup 待制作  
**Commit**：待填

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `Tutorial/TutorialManager.h/.cpp`、`UI/GameDialogWidget.h/.cpp`（类名 UTutorialPopupWidget）、`Tutorial/TutorialHintDataAsset.h`（ETutorialState） |
| 触发①（武器） | `WeaponSpawner::TryPickupWeapon` → `TM->TryWeaponTutorial(PC)`，延迟 0.4s，自动打开背包 + 弹窗 |
| 触发②（战斗后） | `YogGameMode::SelectLoot` → `TM->TryPostCombatTutorial(PC)`，延迟 0.2s，自动打开背包 + 弹窗 |
| 状态机 | `ETutorialState`：None / NeedWeaponTutorial / WeaponTutorialDone / NeedPostCombatTutorial / Completed |
| 持久化 | `UYogSaveGame::TutorialState`（默认 NeedWeaponTutorial，引导完成写 Completed） |
| bIsInCombat | `BackpackGridComponent::bIsInCombat`（P0 添加，GameMode 负责写入） |
| 新接口 | `YogPlayerControllerBase::OpenBackpack()` — TutorialManager 调用强制开背包 |
| HUD 配置 | `AYogHUD::TutorialPopupClass`（BP_HUD Details 填 WBP_TutorialPopup） |
| WBP 控件名 | `TitleText`（TextBlock）、`BodyText`（TextBlock）、`BtnConfirm`（Button）、`BtnConfirmLabel`（Button 内 TextBlock）— 名字必须精确匹配 |
| 设计文档 | [Tutorial_Design.md](Design/Systems/Tutorial_Design.md) |

已知限制：GameMode 未写入 `bIsInCombat`（待接入阶段切换时补充）；弹窗文字为硬编码 LOCTEXT，后期如需多语言可改 StringTable。

---

### [COMBAT-006] 命中停顿 + 全局时间缩放 — HitStopManager + AN_HitStop

**状态**：C++ 完成，AnimNotify 已可放蒙太奇；蓝图无需配置  
**Commit**：本次提交

| 项目 | 内容 |
| --- | --- |
| 核心文件 | `Animation/HitStopManager.h/.cpp`、`Animation/AN_HitStop.h/.cpp` |
| 运行机制 | `UTickableWorldSubsystem`：不受 TimeDilation 影响，Tick 持续调用；`FPlatformTime::Seconds()` 计量真实经过时长 |
| 两阶段效果 | 1. **Frozen**：全局 TimeDilation≈0.0001，持续 FrozenDuration 真实秒；2. **Slow**：TimeDilation=SlowTimeDilation，持续 SlowDuration 真实秒 |
| 中断策略 | Frozen 期间新 RequestHitStop 忽略（冻结优先级最高）；Slow 期间可被更重的新请求覆盖 |
| AnimNotify | 放在攻击蒙太奇命中帧，Notify Details 设三个参数即可触发 |
| 典型配置 | 轻击：F=50ms/S=0；重击：F=80ms/S=120ms@25%；暴击：F=60ms/S=150ms@20% |
| 防泄漏 | `Deinitialize` 强制 EndHitStop，关卡切换时不残留 TimeDilation |
| Notify 显示 | 蒙太奇编辑器中自动显示 `HitStop 50ms` 或 `HitStop F=80ms S=120ms@25%` |

**使用方式**

1. 打开攻击蒙太奇，在命中帧添加 Notify → 选择 `AN Hit Stop`
2. Notify Details 填写：FrozenDuration / SlowDuration / SlowTimeDilation
3. 无需蓝图或 GameMode 配置，系统自动启动

**已知限制**

- `SlowTimeDilation=0.0f` 会导致 Slow 阶段 TimeDilation 实际 clamp 到 0.01（引擎内部限制），推荐最低 0.1
- 同帧多次命中时第一次触发生效，后续同优先级忽略（设计预期）

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
