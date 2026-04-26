# 06 UI / HUD — 已完成功能盘点

> 范围：背包 UI / 三选一 UI / 玻璃框 / 液态血条 / HUD 容器 / 武器拾取动画 / 教程 / 敌人方向箭头 / 暂停效果。
> 三选一 UI 的逻辑驱动 → [03_RunLoop.md](03_RunLoop.md) 的 [COMBAT-003a]；武器玻璃图标的热度变色逻辑见本分册 [UI-007b]。

---

## 背包 UI

### [BACKPACK-002 / UI-COMMONUI] 背包 UI（CommonUI 重构 + StyleDA + RuneInfoCard）
- **设计需求**：鼠标 + 手柄双操作；样式参数 DA 化（无需重编译调视觉）；RuneInfoCard 由 C++ 自动调 ShowRune / HideCard。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/BackpackScreenWidget.h` + `Private/UI/BackpackScreenWidget.cpp`（`UCommonActivatableWidget`）
  - `Public/UI/BackpackGridWidget.h` + `Private/UI/BackpackGridWidget.cpp`
  - `Public/UI/BackpackStyleDataAsset.h`（`DA_BackpackStyle` — 格子状态颜色 / 激活区颜色 / 尺寸 / `InactiveZoneOpacity` / `Phase1/2/3GlowColor` / `HeatZone0/1/2Color`）
  - `Public/UI/RuneInfoCardWidget.h` + `Private/UI/RuneInfoCardWidget.cpp`
  - `Public/UI/RuneSlotWidget.h` + `.cpp`（`SetSlotState` / `ShakeAndFlash` 红闪 + 阻尼正弦抖动）
  - `Public/UI/PendingGridWidget.h` + `Private/UI/PendingGridWidget.cpp`（待放置区，[REFACTOR-018] 重构）
- **设计文档**：[BackpackSystem_Guide](../Systems/Rune/BackpackSystem_Guide.md) · memory: `project_backpack_ui_arch.md`（**待转写**为 `Docs/Systems/Rune/BackpackUI_Architecture.md`）
- **验收方式**：
  1. 按 Tab 应打开背包；ESC / B 应关闭；HUD 隐藏
  2. WBP_BackpackScreen Details → Style DA = DA_BackpackStyle，改 DA 颜色应即时生效

### [UI-021] 背包交互重构：单击抓取 / 长按 3s 退回 / 换格自动拾起 / 悬停绿框
- **设计需求**：拖拽不顺手，改成"点一下抓起、再点放下"；长按 3s 自动退回 PendingGrid；放到有符文的格子触发交换 + 旧符文自动抓起。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/BackpackScreenWidget.h`（`bGrabbingRune` / `LongPressHoldTime` / `LongPressDuration=3.0f` / `SendGrabbedRuneToPending`）
- **验收方式**：
  1. 单击有符文格 → 应进抓取状态（黄框 + 信息卡）
  2. 抓取后按住 3s → 该符文应自动塞到 PendingGrid 第一个空格
  3. 抓取符文放到另一格已有符文 → 旧符文应自动接管抓取状态

### [UI-022] 符文边框 NativePaint（替换 CanvasPanel Overlay + 像素精确对齐）
- **设计需求**：嵌套布局（VerticalBox→SizeBox→UniformGridPanel）下 RuneBorderCanvas 定位失准 — 用 Slate 直接画。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/BackpackGridWidget.h`（`NativePaint` override，`FSlateDrawElement::MakeLines`）
  - 颜色分层：选中金黄 (2.5px) / 悬浮绿 (2.0px) / 其余灰 (1.5px)
- **验收方式**：背包嵌套在垂直布局里时符文包围框应对齐每格的实际像素位置

### [UI-005] 背包热度阶段点按钮（HeatPhaseDot）
- **设计需求**：玩家可点"阶段点"或按 1/2/3 预览各阶激活区 — 不必升阶就看后期布局。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/BackpackGridWidget.h`（HeatPhaseDot0/1/2 + `FOnHeatPhaseButtonClicked` delegate / `RefreshHeatPhaseButtons(PreviewPhase, bIsGamepadMode)`）
  - `Public/UI/BackpackScreenWidget.h`（订阅 delegate）
- **验收方式**：开背包按 1 / 2 / 3 应单独亮对应阶段激活区；再按一次应回到三阶叠加

### [UI-007a] 热度区三色叠加 / 单阶切换显示
> （ID 拆分说明：原 `UI-007` 同时表示"热度区显示"和"武器玻璃图标变色"，此处取热度区显示部分；图标变色见本分册 [UI-007b]）
- **设计需求**：默认看到三阶分层（Zone0 优先级最高），点 Dot 单独看一阶 — 视觉上体现热度由内向外递减。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/BackpackScreenWidget.h`（`EBackpackCellState::EmptyZone1` / `EmptyZone2`）
  - `Public/UI/BackpackStyleDataAsset.h`（`HeatZone0/1/2Color`）
- **验收方式**：背包默认应看到 3 圈不同颜色叠加区域；按 1 应只显示 Zone0 其余置灰

### [UI-010] 背包格默认半透（InactiveZoneOpacity）
- **设计需求**：未放置符文的空格不要太抢眼；热度检视模式下非聚焦区同样压暗。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/BackpackStyleDataAsset.h`（`InactiveZoneOpacity` 默认 0.35）
  - `Public/UI/RuneSlotWidget.h` + `.cpp`（`ZoneOpacity` 参数传入 SetSlotState 乘 BGColor.A）
- **验收方式**：开背包时空格应半透；按 1 切单阶后非聚焦区空格应进一步压暗

### [FIX-027] 背包战斗锁定（按 ELevelPhase）
> 主条目在 [03_RunLoop.md](03_RunLoop.md) 的 [FIX-027]；UI 触发点：Public/UI/BackpackScreenWidget.cpp 5 处 `IsInCombatPhase()` 检查 + ShakeAndFlash 视觉提示

---

## 三选一 UI

### [COMBAT-003a / FIX-022 / FIX-023 副] 三选一 UI（LootSelectionWidget）
> 主条目在 [03_RunLoop.md](03_RunLoop.md) 的 [COMBAT-003a]，此处只列 UI 层
- **设计需求**：每个 RewardPickup 拾取时直接弹卡；Widget 在 HUD BeginPlay 持久创建（ZOrder 15）；CommonUI 完全隔离防被 NativeOnFocusLost 误销毁。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/LootSelectionWidget.h` + `Private/UI/LootSelectionWidget.cpp`（CommonUI 三处截断）
  - `Public/UI/YogHUD.h` + `.cpp`（`ShowLootSelectionUI` / `IsInViewport()` 自动重建）
- **设计文档**：[LootSelection_Technical](../Systems/UI/LootSelection_Technical.md)
- **验收方式**：拾取 RewardPickup 应弹三选一卡 1 次（不会重复弹 3 次）；选完应自动 OpenBackpack（[UI-014]）

### [UI-014] 三选一结束后自动打开背包
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/UI/LootSelectionWidget.cpp`（`SelectRuneLoot` 在 `DeactivateWidget()` 之后调 `HUD->OpenBackpack()`）
  - `Public/UI/YogHUD.h`（`BackpackScreenClass` / `BackpackWidget` / `OpenBackpack`）
- **验收方式**：选完符文应自动开背包；玩家可立即放置新符文

---

## HUD 主容器与血条

### [UI-024] HUD 主容器（YogHUDRootWidget + BindWidget）
- **设计需求**：一个 WBP 摆所有常驻控件（血条 / 敌人箭头 / 武器玻璃图标 / 热度条），HUD 不再逐个 CreateWidget — 位置用锚点可视化调。
- **状态**：✅ C++完成；⚙ WBP_HUDRoot 待建（HUD-001）
- **核心文件**：
  - `Public/UI/YogHUDRootWidget.h` + `Private/UI/YogHUDRootWidget.cpp`
  - `Public/UI/YogHUD.h` + `.cpp`（`MainHUDClass` / `MainHUDWidget`）
  - BindWidget 字段：`PlayerHealthBar`(必须) / `EnemyArrow`(可选) / `WeaponGlassIcon`(可选) / `HeatBar`(可选)
- **验收方式**：BP_YogHUD MainHUDClass = WBP_HUDRoot → HUD 应只 CreateWidget 一次；各子控件按 BindWidget 自动连接

### [UI-023] 液态血条（LiquidHealthBarWidget + LiquidHealthBar.ush）
- **设计需求**：血量变化有"液体晃动"反馈 — 幅度 ∝ 血量变化量，指数阻尼。
- **状态**：✅ C++完成；⚙ WBP_PlayerHealthBar 待建（HUD-001）
- **核心文件**：
  - `Public/UI/LiquidHealthBarWidget.h` + `Private/UI/LiquidHealthBarWidget.cpp`（DMI + `FillPercent` / `SloshAmplitude` / `SloshPhase`，`NativeTick` 指数阻尼）
  - `Source/DevKit/Shaders/LiquidHealthBar.ush`
  - `Private/UI/YogHUD.cpp`（`BindHealthAttributes` 绑 GAS GetHealthAttribute）
- **验收方式**：受击血量减少时血条应液态晃动；血量稳定后晃动应在 1s 内停

### [HeatBarWidget] 热度条（HUD 常驻）
- **状态**：✅ C++完成
- **核心文件**：`Public/UI/HeatBarWidget.h`

---

## 武器 UI（拾取流程）

### [UI-008] 武器拾取浮窗（WeaponFloatWidget）
- **设计需求**：走近武器看名称 / 描述 / 激活区点阵 / 起始符文列表；浮窗位置自动避开屏幕中心。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/WeaponFloatWidget.h` + `Private/UI/WeaponFloatWidget.cpp`
  - `Public/Item/Weapon/WeaponInfoDA.h`（`WeaponName` / `WeaponDescription` / `Thumbnail` / `Zone1/2/3Image`）
- **验收方式**：走近 WeaponSpawner 应弹浮窗显示武器信息；离开范围应隐藏

### [FEAT-015 / UI-013] 武器拾取动画三阶段（Float → Trail → GlassIcon）
- **设计需求**：拾取后浮窗折叠 → 缩小 → 飞到左下角玻璃图标 — 给玩家"获得新武器"的仪式感。
- **状态**：✅ C++完成；⚙ WBP_WeaponTrail（UI-002）+ WBP_WeaponGlassIcon（UI-001）+ WeaponGlassAnimDA 待配
- **核心文件**：
  - `Public/UI/WeaponTrailWidget.h` + `Private/UI/WeaponTrailWidget.cpp`（流光拖尾，`FOnWeaponFlyProgress` 委托每帧广播）
  - `Public/UI/WeaponGlassIconWidget.h` + `Private/UI/WeaponGlassIconWidget.cpp`
  - `Public/UI/WeaponGlassAnimDA.h`（`AutoCollapseDelay` / `CollapseDuration` / `ShrinkDuration` / `FlyDuration` / `GlassIconSize` / `HUDOffsetFromBottomLeft`）
  - `Public/UI/WeaponThumbnailFlyWidget.h`
  - `Source/DevKit/Shaders/WeaponTrail.ush`
- **设计文档**：[WeaponPickupAnim_Technical](../Systems/UI/WeaponPickupAnim_Technical.md)
- **验收方式**：拾取武器 → 浮窗 InfoContainer 隐藏 → 缩小 → 飞向左下角图标 → 流光拖尾从浮窗起点画到目标
- **已知**：WBP_WeaponFloat 背景若不在 InfoContainer 内会留白（FIX-010 已知方案）

### [UI-007b] 武器玻璃图标热度颜色（WeaponGlassIconWidget 自订阅热度阶段）
> （ID 拆分自原 `UI-007`，热度区显示部分见 [UI-007a]）
- **设计需求**：左下角玻璃图标显示当前热度颜色；升阶时自动变色；切关后颜色恢复。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/WeaponGlassIconWidget.h` + `.cpp`（`NativeConstruct` 自订阅 `PlayerCharacterBase::OnHeatPhaseChanged`）
  - `Public/Character/PlayerCharacterBase.h`（`GetCurrentHeatPhase()` BlueprintPure / `HeatStyleDA`）
  - WBP 层级：Overlay → GlassBG → GlassBGCenter → **HeatColorOverlay** → GlassBorderImage
- **配置入口**：`B_PlayerOne` Details → Heat\|Visual → `HeatStyleDA` 填 `DA_BackpackStyle`
- **验收方式**：玩家升 Phase → 武器玻璃图标颜色应跟随；切关后颜色应恢复

### [UI-015 / FEAT-016] 液态玻璃框（GlassFrameWidget）
- **设计需求**：Apple 液态玻璃感 UI 框 — 中心毛玻璃模糊 + 边框 SDF 折射 + 角落炫彩；可复用于背包格 / 武器图标 / HUD 缩略框。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/GlassFrameWidget.h` + `Private/UI/GlassFrameWidget.cpp`（`ApplyGlassStyle` 批量写参数）
  - `Source/DevKit/Shaders/GlassFrameUI.ush`
  - `Source/DevKit/Shaders/GlassBlurMask.ush`
  - 资产：`M_GlassFrame`（UI Domain，Custom Node include `/Project/GlassFrameUI.ush`）/ `MI_GlassFrame` / `WBP_GlassFrame`
- **设计文档**：[GlassFrame_Technical](../Systems/UI/GlassFrame_Technical.md)
- **验收方式**：放 WBP_GlassFrame 到任意 WBP，应显示磨砂玻璃效果，边缘有 SDF 高光，中心模糊背景

---

## 教程系统

### [TUT-001 / UI-006] 新手引导（TutorialManager + GameDialogWidget）
- **设计需求**：武器拾取 / 战斗后弹引导；弹窗"知道了"才关闭；状态机持久化（写入 SaveGame.TutorialState）；两段流（武器 → 战斗后）。
- **状态**：✅ C++完成（含 [FIX-006/07/09/12] CommonUI 输入残留 / 幽灵输入 / 关闭操控锁死等系列修复）
- **核心文件**：
  - `Public/Tutorial/TutorialManager.h` + `Private/Tutorial/TutorialManager.cpp`（`TryWeaponTutorial` / `TryPostCombatTutorial` / `OnPopupClosed`）
  - `Public/Tutorial/TutorialHintDataAsset.h`（`ETutorialState`：None / NeedWeaponTutorial / WeaponTutorialDone / NeedPostCombatTutorial / Completed）
  - `Public/UI/GameDialogWidget.h` + `Private/UI/GameDialogWidget.cpp`（`UTutorialPopupWidget`，`bIsInteractable` 防幽灵输入，`BP_OnPopupClosing` BlueprintNativeEvent 默认调 `ConfirmClose`）
  - `Public/UI/DialogContentDA.h` / `Public/UI/TutorialRegistryDA.h`
  - `Public/UI/InfoPopupWidget.h`
- **设计文档**：[TutorialPopup_Copy](../Systems/UI/TutorialPopup_Copy.md)（文案）
- **验收方式**：
  1. 第一次拾取武器应弹武器引导；点"知道了"应关闭，玩家恢复操控
  2. 第一次三选一选完应弹战斗后引导；之后不再弹

---

## 其他 HUD 元素

### [UI-009 / FIX-028] 敌人方向箭头（EnemyArrowWidget）
- **设计需求**：所有存活敌人均不在屏幕内 + 玩家 1.5s 内未受伤时，屏幕边缘出箭头指向最近敌人。
- **状态**：✅ C++完成；⚙ HB_PlayerMain Details → `EnemyArrowWidgetClass` 赋 WBP_EnemyArrow（FIX-028 还原）
- **核心文件**：
  - `Public/UI/EnemyArrowWidget.h` + `Private/UI/EnemyArrowWidget.cpp`（`NativeTick` 守卫：`if (ArrowImages.IsEmpty() && RootCanvas) RebuildArrowPool()`）
  - `Private/GameModes/YogGameMode.cpp`（`GetAllAliveEnemies`）
  - WBP：`Content/UI/Playtest_UI/CombatInfo/WBP_EnemyArrow.uasset`（根 Canvas Panel 命名 `RootCanvas`）
- **配置参数**：`AppearDelay`(1.5s) / `MaxArrows`(3) / `ArrowProjectionZOffset`(60cm) 等
- **验收方式**：所有敌人离屏 1.5s 后屏幕边缘应出现最多 3 个箭头指向最近敌人；任一敌人进屏 / 玩家受伤应隐藏

### [UI-012] 暂停弹窗屏幕变暗（PauseEffect）
- **设计需求**：所有暂停式弹窗激活时画面渐变至低饱和 + 变暗，关闭后恢复；多弹窗叠加要计数。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/UI/YogHUD.h` + `Private/UI/YogHUD.cpp`（unbound `APostProcessVolume`，`PausePopupCount` 计数）
  - 各暂停弹窗 NativeOnActivated → `BeginPauseEffect()`；NativeOnDeactivated → `EndPauseEffect()`
- **配置参数**：`PauseFadeDuration`(0.25s) / `PauseTargetSaturation`(0.10) / `PauseTargetGain`(0.40)
- **验收方式**：开背包 / 弹三选一 / 教程弹窗 → 画面应变暗 + 低饱和；关闭后应恢复

### [DamageBreakdownWidget] 伤害构成显示（COMBAT-003 衍生）
- **状态**：✅ C++完成
- **核心文件**：`Public/UI/DamageBreakdownWidget.h`，订阅 `FOnDamageBreakdown`（Dynamic）

### [CombatLogStatics / CombatLogWidget] 战斗日志
- **状态**：✅ C++完成（[UI-001]）
- **核心文件**：
  - `Public/UI/CombatLogStatics.h` + `Public/UI/CombatLogWidget.h`
  - `Public/AbilitySystem/YogAbilitySystemComponent.h`（PushEntry 接口）

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| HUD-001 | 创建 `WBP_HUDRoot`（Parent = `YogHUDRootWidget`）+ `WBP_PlayerHealthBar`（Parent = `LiquidHealthBarWidget`），配 BP_YogHUD → `MainHUDClass` | [TASKS](../PM/TASKS.md) |
| UI-001 | `WBP_WeaponGlassIcon` + `WeaponGlassAnimDA` 编辑器配置 | [WeaponPickupAnim_Technical](../Systems/UI/WeaponPickupAnim_Technical.md) |
| UI-002 | `WBP_WeaponTrail` 创建（根全屏 Canvas + Image 命名 TrailLine + M_WeaponTrail） | 同上 |
| FIX-028 配置 | HB_PlayerMain Details → Enemy Arrow → `EnemyArrowWidgetClass` 赋 WBP_EnemyArrow | [FeatureLog FIX-028](../FeatureLog.md) |
| FIX-010 | `WBP_WeaponFloat` 白屏 Bug：将背景移入 InfoContainer | [FeatureLog FEAT-015](../FeatureLog.md) |
