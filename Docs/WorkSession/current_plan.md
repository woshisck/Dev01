# 开发方案：LootSelection 视觉高亮 C++ 化 + 卡片堆叠修复 + 教程节奏改造

> 上一版（WBP_LootSelection 改造 v2）已落地并 push。本次接续 PIE 实测发现的 5 个新问题。

## 需求描述

1. **OnCardFocused/OnSectionFocused 视觉高亮写入 C++** — 现在是 BlueprintImplementableEvent，让用户在 BP 自己实现高亮太麻烦，统一进 C++
2. **拾取后三选一卡片堆在一起**（截图证据：3 张卡视觉重叠在中央，名字/描述全部叠加） — RebuildCards 已 AddChild，但 HorizontalBox 没把它们横排出来
3. **教程出现太生硬** — 当前 First Rune 教程在 slow-mo 启动同帧弹出，画面边变黑边弹窗；ActivationZone 教程现在不需要（用户没配热度数据）
4. **关卡事件可管理化** — "都在 C++ 里我管理不了"，希望能在编辑器里配置首符文/关卡结束/玩家放置首张符文等事件触发什么 LevelFlow，而不是写死在 GameMode/BackpackGrid 里
5. **RuneInfoCard 是否需要修改** — 由于需求 1 要 C++ 化高亮，需要给 RuneInfoCard 加 SetSelected 接口

---

## 方案设计

### 全局判定：根因分析（卡片堆叠）

日志显示 `[LootSelection] ShowLootUI: NumOptions=3 SourcePickup=BP_RewardPickup_C_0` — `RebuildCards` 跑了 3 次 `AddChild`，但视觉上 3 张完全重叠。BindWidget 在 UE 中类型不匹配是编译错误，所以 **`CardContainer` 确实是 `UHorizontalBox`**。问题只可能出在子卡片本身：

- `WBP_RuneInfoCard` 根控件用了 CanvasPanel 但**没设固定 Desired Size** → HorizontalBox 排版按 `0×0` 计算，N 张全叠到同一像素位
- 或者 RuneInfoCard 里子控件全用 Canvas Slot 绝对定位，整体没"撑大"父级

**修复策略**（双保险）：
- C++ 兜底：在 `RebuildCards` 给 Wrapper 外面套一层 `USizeBox`，强制 `WidthOverride=320, HeightOverride=420`（避免依赖 WBP 设置）
- 文档侧：让用户在 `WBP_RuneInfoCard` 根改用 SizeBox 或显式给 CanvasPanel 设固定 DesiredOnScreenSize

### 子需求映射

| 需求 | 解法 | 文件 |
|---|---|---|
| ① 视觉高亮 C++ 化 | RuneInfoCard 加 `SetSelected(bool)`；LootSelectionWidget.FocusCard 内部调它；BP 事件保留作为附加扩展点（音效/粒子）但不再承担"画边框"职责 | `RuneInfoCardWidget.h/cpp` `LootSelectionWidget.cpp` |
| ② 卡片堆叠 | RebuildCards 套 USizeBox + 文档更新 | `LootSelectionWidget.cpp` `WBP_RuneInfoCard` 文档 |
| ③ 教程节奏 | TryFirstRuneTutorial 移到 `TickLevelEndEffect` 的 `RevealProgress >= 1` 之后；TutorialPopupWidget 加 0.25s 渐入；ActivationZone 触发位删掉（保留接口） | `YogGameMode.cpp` `YogHUD.cpp` `BackpackGridComponent.cpp` `GameDialogWidget.cpp` |
| ④ 关卡事件可管理化 | **本次只做最小开关**：在 `BP_GameMode` Defaults 暴露 `bEnableHeatPhaseTutorial = false`，让用户能关；完整"事件总线"作为 P2 单独立项 | `YogGameMode.h/cpp` `BackpackGridComponent.cpp` |
| ⑤ RuneInfoCard 改造 | 加 `SelectionBorder`（BindWidgetOptional UBorder/UImage）+ `SetSelected(bool)` + Tick 中插值缩放 | `RuneInfoCardWidget.h/cpp` |

### 关键设计决策

- **保留 BP 事件**：`OnCardFocused/OnSectionFocused/OnNavigateSelection` 仍是 BlueprintImplementableEvent，但 C++ 自己已处理"高亮+缩放"；BP 事件留作"加音效/粒子"等可选扩展，**不实现也不会影响视觉**
- **缩放在 Card 内部**：`URuneInfoCardWidget::SetSelected(bool)` 内部 SetRenderScale + Border 显隐，调用方只关心 true/false
- **教程时序**：把 `TryFirstRuneTutorial` 调用从 `EnterArrangementPhase` 抽到 `YogHUD::TickLevelEndEffect` 的 `if (RevealProgress >= 1.f)` 完成时刻，并加一个 `bFirstRuneTutorialFired` 一次性标记
- **bEnableHeatPhaseTutorial**：默认 `false`，用户配热度系统时再打开。这是一个临时方案，不是完整事件总线（避免本次范围爆炸）

### 关卡事件可管理化的"完整方案"（P2，本次不做）

不在本次实现，但在方案中明示后续路径：
- 给 `AYogGameMode` 暴露 `TMap<EGameLifecycleEvent, TObjectPtr<ULevelFlowAsset>>` 字段，事件包括：`LevelClear / FirstRuneAcquired / FirstRunePlaced / HeatPhaseEntered / RoomEntered`
- 每个事件触发时 SpawnFlow 跑对应 LevelFlowAsset
- 用户在 `WBP_GameMode_Default` Defaults 里就能配"关卡结束 → 跑哪个 LFA"
- 这能彻底替代当前硬编码的 `TryFirstRuneTutorial` / `TryHeatPhaseTutorial` 调用

---

## 实现步骤

### A. C++ 改动（按顺序）

#### A1. RuneInfoCard 加 SetSelected
- [Source/DevKit/Public/UI/RuneInfoCardWidget.h](../../Source/DevKit/Public/UI/RuneInfoCardWidget.h)
  - 新增 `UPROPERTY(BindWidgetOptional) TObjectPtr<UBorder> SelectionBorder;`（BP 端可选，C++ 控制显隐）
  - 新增 `UFUNCTION(BlueprintCallable) void SetSelected(bool bSelected);`
  - 新增私有 `bool bSelected = false; float CurrentScale = 1.0f; static constexpr float SelectedScale = 1.06f; static constexpr float ScaleInterpSpeed = 12.f;`
- [Source/DevKit/Private/UI/RuneInfoCardWidget.cpp](../../Source/DevKit/Private/UI/RuneInfoCardWidget.cpp)
  - `SetSelected`：`bSelected = b; if (SelectionBorder) SelectionBorder->SetVisibility(b ? Visible : Hidden);`
  - `NativeTick`：FInterpTo 当前 scale → 目标 scale；`SetRenderScale(FVector2D(CurrentScale))`
  - `ShowRune` 末尾：默认 `SetSelected(false)` 防止状态残留

#### A2. LootSelectionWidget.RebuildCards 套 SizeBox + 调 SetSelected
- [Source/DevKit/Private/UI/LootSelectionWidget.cpp](../../Source/DevKit/Private/UI/LootSelectionWidget.cpp)
  - `#include "Components/SizeBox.h"`
  - `RebuildCards` 内部：把 `Wrapper->AddChild(Card)` 改为 `SizeBox->AddChild(Card)` + `Wrapper->AddChild(SizeBox)`，SizeBox 设 320×420（暴露 `UPROPERTY(EditAnywhere) FVector2D CardSize = {320, 420};` 供调整）
  - `FocusCard` 内部：遍历 SpawnedCards 调 `C->SetSelected(i == CurrentCardIndex)`（替代单纯的 `SetGenericEffectsExpanded`，但展开逻辑保留）
  - `SetSection`：当 NewSection == Buttons 时遍历所有 Card 调 `SetSelected(false)`，避免按钮段时还高亮卡片

#### A3. BtnSkip / BtnBackpackPreview 高亮 C++ 化
- 新增私有：`UpdateButtonHighlight(int32 SelectedIdx)`
- `FocusButton` 末尾调用 — 给选中按钮加边框（用 SetRenderTransform 缩放 1.04 + Hovered 颜色覆盖 Style.Normal）
- 不依赖 BP，纯 C++ 修改 `WidgetStyle.Normal.TintColor`

#### A4. 教程节奏改造
- [Source/DevKit/Public/UI/YogHUD.h](../../Source/DevKit/Public/UI/YogHUD.h)
  - 新增 `bool bFirstRuneTutorialFired = false;`
- [Source/DevKit/Private/UI/YogHUD.cpp](../../Source/DevKit/Private/UI/YogHUD.cpp)
  - `TickLevelEndEffect` 在 `RevealProgress >= 1.f` 块内（揭幕完成时）：触发 `TryFirstRuneTutorial`，加一次性 guard
  - `TriggerLevelEndEffect` 时重置 `bFirstRuneTutorialFired = false`
- [Source/DevKit/Private/GameModes/YogGameMode.cpp](../../Source/DevKit/Private/GameModes/YogGameMode.cpp)
  - 删除 EnterArrangementPhase 中第 362-364 行的 `TM->TryFirstRuneTutorial(PC);` 调用（已经移到 HUD）
- [Source/DevKit/Public/GameModes/YogGameMode.h](../../Source/DevKit/Public/GameModes/YogGameMode.h)
  - 新增 `UPROPERTY(EditDefaultsOnly, Category = "Tutorial") bool bEnableHeatPhaseTutorial = false;`
- [Source/DevKit/Private/Component/BackpackGridComponent.cpp](../../Source/DevKit/Private/Component/BackpackGridComponent.cpp)
  - `HandleHeatTutorial` 头部加守卫：先 `if (UYogGameMode* GM = ...; GM && !GM->bEnableHeatPhaseTutorial) return;`

#### A5. TutorialPopup 弹出渐入
- [Source/DevKit/Public/UI/GameDialogWidget.h](../../Source/DevKit/Public/UI/GameDialogWidget.h)
  - 新增 `bool bIsFadingIn = false; float FadeInElapsed = 0.f; static constexpr float FadeInDuration = 0.25f;`
- [Source/DevKit/Private/UI/GameDialogWidget.cpp](../../Source/DevKit/Private/UI/GameDialogWidget.cpp)
  - `ShowPopup` 末尾：`SetRenderOpacity(0.f); bIsFadingIn = true; FadeInElapsed = 0.f;`
  - `NativeTick` 内：if (bIsFadingIn) { FadeInElapsed += dt; SetRenderOpacity(FMath::Clamp(FadeInElapsed/FadeInDuration, 0.f, 1.f)); if (FadeInElapsed >= FadeInDuration) bIsFadingIn = false; }
  - 备注：使用真实时间 dt（不受 dilation 影响）— `Tick` 已是 RealDelta；如果项目 Tick 是 GameTime，需用 `GetWorld()->GetRealTimeSeconds()` 替代

### B. WBP / 资产改动

#### B1. WBP_RuneInfoCard
- 根控件外层包 `SizeBox` 设 `WidthOverride=320 HeightOverride=420`（兜底；C++ 已套但双保险更稳）
- 在卡片最外层加一个 `Border` 控件，**命名 `SelectionBorder`**（C++ 自动绑定）：
  - `Brush Color`: #FFD966 半透明 (1.0, 0.85, 0.4, 0.4)
  - `Background Color`: 透明
  - `Padding`: 6（卡片向内缩 6px，Border 显示一圈高亮边）
  - **初始 Visibility = Hidden**（C++ 自动控制）
  - Slot 设 Fill / 顶层 Z-order

#### B2. WBP_LootSelection
- 按 [Docs/Systems/UI/WBP_LootSelection_Layout.md](../Systems/UI/WBP_LootSelection_Layout.md) 配置
- **额外检查**：CardContainer 是 UHorizontalBox（不是 Wrap/Vertical/Overlay）
- 不需要 BP 写任何 OnCardFocused 实现（C++ 已自管）

#### B3. 文档更新
- 更新 `Docs/Systems/UI/WBP_LootSelection_Layout.md`：把"Event Graph 实现"段改成"理论上**完全清空**，C++ 已自管视觉高亮"
- 更新（或新建）`Docs/Systems/UI/WBP_RuneInfoCard_Layout.md`：加 SelectionBorder 控件 + SizeBox 根

---

## 涉及文件

| 文件 | 操作 | 说明 |
|---|---|---|
| `Source/DevKit/Public/UI/RuneInfoCardWidget.h` | 编辑 | 加 SelectionBorder + SetSelected + 缩放字段 |
| `Source/DevKit/Private/UI/RuneInfoCardWidget.cpp` | 编辑 | SetSelected 实现 + Tick 缩放插值 + ShowRune 重置 |
| `Source/DevKit/Public/UI/LootSelectionWidget.h` | 编辑 | 加 CardSize UPROPERTY |
| `Source/DevKit/Private/UI/LootSelectionWidget.cpp` | 编辑 | RebuildCards 套 SizeBox + FocusCard/FocusButton 调 SetSelected + 按钮高亮 |
| `Source/DevKit/Public/UI/YogHUD.h` | 编辑 | 加 bFirstRuneTutorialFired |
| `Source/DevKit/Private/UI/YogHUD.cpp` | 编辑 | TickLevelEndEffect 揭幕完成时触发 TryFirstRuneTutorial |
| `Source/DevKit/Public/GameModes/YogGameMode.h` | 编辑 | 加 bEnableHeatPhaseTutorial 开关 |
| `Source/DevKit/Private/GameModes/YogGameMode.cpp` | 编辑 | 删除 EnterArrangementPhase 中直接调用 TryFirstRuneTutorial |
| `Source/DevKit/Private/Component/BackpackGridComponent.cpp` | 编辑 | HandleHeatTutorial 加 GameMode 开关守卫 |
| `Source/DevKit/Public/UI/GameDialogWidget.h` | 编辑 | 加 bIsFadingIn / FadeInElapsed 字段 |
| `Source/DevKit/Private/UI/GameDialogWidget.cpp` | 编辑 | ShowPopup 末尾启动渐入 + Tick 插值 |
| `Docs/Systems/UI/WBP_LootSelection_Layout.md` | 编辑 | 更新 Event Graph 段 |
| `Docs/Systems/UI/WBP_RuneInfoCard_Layout.md` | 新建 | 加 SizeBox 根 + SelectionBorder 说明 |
| `Content/UI/Playtest_UI/Runes/WBP_RuneInfoCard.uasset` | 用户编辑 | 加 SizeBox 根 + SelectionBorder 子控件 |
| `Content/UI/Playtest_UI/WBP_LootSelection.uasset` | 用户编辑 | 按文档配 BindWidget |

---

## 潜在风险

1. **SizeBox 写死 320×420** — 已通过 `UPROPERTY(EditAnywhere) FVector2D CardSize` 暴露到 Class Defaults，可在 BP 调整
2. **RuneInfoCard 缩放与背包共用** — 背包里的 RuneInfoCard 不该有缩放/高亮。`SetSelected` 默认 false，背包不调用就不触发 — 但要确认 `BackpackScreenWidget::ShowRune` 内部没误调 SetSelected(true)
3. **Tutorial 渐入影响时间** — 渐入期间玩家如果疯狂按 Esc 跳过，要保证不破坏 PauseEffect 计数；ConfirmClose 现有逻辑应该已处理
4. **bFirstRuneTutorialFired 持久化** — 这是 HUD 实例字段，玩家死亡重生后 HUD 重建可能误触发；和 TutorialState 配合（State 推进后 ShowByEventID 会被 HasPassedStage 拦掉），实际无副作用
5. **删除 GameMode 中的 TryFirstRuneTutorial 调用** — 必须确认 HUD 那边的 hook 一定能 fire，否则首符文教程永不出现。建议在 HUD 触发后也加一次 Log 验证
6. **关卡事件可管理化只做开关** — 用户期待"在编辑器里配"，但本次只暴露一个 bEnableHeatPhaseTutorial 远未达成"事件总线"。需要明确告知用户这是本次范围
7. **NativeTick 缩放插值** — RuneInfoCard 已有 NativeTick（处理 ShowRune 淡入），加 scale 插值在同一个 Tick 即可，无新性能开销

---

## 待确认问题

1. **关卡事件可管理化的范围**：是否同意本次只做 `bEnableHeatPhaseTutorial` 开关 + ActivationZone 教程默认关闭，"完整事件总线（GameMode → EGameLifecycleEvent → LevelFlow Asset 映射）"作为 P2 单独立项？还是希望本次就把事件总线建起来（工程量约本次 2-3 倍）？
2. **卡片缩放数值**：默认 1.06 + Border 边框 #FFD966 半透明，可以吗？还是要更明显（如 1.10 + 实色边框）？
3. **WBP_RuneInfoCard 改造**：SelectionBorder 用 UBorder 类型（支持 Padding）还是 UImage 类型（仅显示）？推荐 UBorder
4. **TutorialPopup 渐入时长**：0.25s 渐入是否合适？还是想更快/更慢？
5. **EnterArrangementPhase 中 TryFirstRuneTutorial 是不是真的可以删？**（即"教程必须延迟到 slow-mo 揭幕完成后"）确认一下，避免删错。
