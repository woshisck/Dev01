# 星骸降临 Feature Log

> 用途：记录每个完成功能的关键信息，出现 Bug 时直接把对应条目发给 Claude。  
> 格式：每条 log 包含 **功能名 / 涉及文件 / 核心接口 / 配置入口 / 已知限制**。

---

## 2026-04-28

### [Weapon-TypeGuard] 武器类型守卫 — 自动隔离近战/远程 GA 激活

**状态**：C++ 完成已编译；火枪 DA 需在编辑器把 `WeaponType` 字段设为 `Ranged`（近战默认 `Melee` 不需动）

| 项目 | 内容 |
|------|------|
| 起因 | 日志显示装备近战时 LightAtk Combo1/2 仍触发了 `GE_MusketBullet_Damage` —— 玩家 ASC 同时持有近战 + 火枪 GA，TryActivateAbilitiesByTag 抢占无序 |
| 涉及文件 | `Config/Tags/Equip.ini`、`Source/.../Item/Weapon/WeaponTypes.h`(新建)、`WeaponDefinition.{h,cpp}`、`YogAbilitySystemComponent.{h,cpp}`、`GA_PlayerMeleeAttacks.cpp`、`GA_MusketBase.cpp` |
| 方案 | Tag 守卫：装备时 ASC 挂 `Weapon.Type.Melee/Ranged` LooseTag → 玩家近战/火枪 GA 父类构造函数加 ActivationRequiredTags → 装备什么类型只激活什么类型的 GA；通用 GA / 符文 / 敌人 GA 不动 |
| WeaponType 字段 | `WeaponDefinition.h` 加 `EWeaponType WeaponType = Melee`（向后兼容默认值）|
| Tag helper | `UYogAbilitySystemComponent::ApplyWeaponTypeTag(EWeaponType)` / `ClearWeaponTypeTags()`，内部用 `SetLooseGameplayTagCount(Tag, 0)` 清零避免计数残留（Codex P1 关键改进）|
| GA 父类 | `UGA_PlayerMeleeAttack` ctor + `Weapon.Type.Melee`；`UGA_MusketBase` ctor + `Weapon.Type.Ranged`；零 BP 改动 |
| 装备衔接 | `WeaponDefinition::SetupWeaponToCharacter` 函数顶部 ClearWeaponTypeTags、底部 ApplyWeaponTypeTag；含诊断日志 |
| 多人扩展点 | helper 封装预留；未来切多人只需把 `AddLooseGameplayTag` 换成 `AddReplicatedLooseGameplayTag`，调用方零改动 |
| Codex 审查 | v2 方案整合 6 项硬伤（计数残留 / BP CDO 序列化 / 卸装清理 / ini 格式 / 多人预留 / HeavyRelease 路径） |
| 已知待办 | 编辑器侧把火枪 WeaponDefinition DA 的 `WeaponType` 设为 `Ranged`；测试装备切换时按 LMB 是否只激活当前类型 GA；确认 `B_PlayerOne` GASTemplate.AbilityMap 是否引用 `GA_WPN_*` BP 资产（如有需手动加 RequiredTag） |

### [Rune-EnemyBuff] 5 个敌人专属符文设计 + 必要 C++ 改动

**状态**：C++ + ini 已完成编译；编辑器资产（5×FA / 5×DA / GE_PoisonSplash）按指南手工创建

| 项目 | 内容 |
|------|------|
| 涉及文件 | `Source/.../DamageExecution.cpp`、`Source/.../GA_Dead.cpp`、`Source/.../BackpackGridComponent.cpp`、`Config/DefaultGameplayTags.ini`、`Config/Tags/BuffTag.ini`、`Docs/Systems/Rune/EnemyBuff_ProductionGuide.md` |
| 5 个敌人符文 | E001 无畏（HP<75% 触发霸体+20%减伤）/ E002 死亡之毒（动画播完 AOE 毒液溅射）/ E003 死咒（死亡时给击杀者 MaxHealth×0.9 最多 3 层）/ E004 激怒（首次受击 AttackSpeed×1.4）/ E005 铁甲（Spawn 即激活 ArmorHP+80） |
| FA 形态 | Category B Actor Ability Passive（`[Start]` 节点起步），`YogGameMode::SpawnEnemyFromPool` 自动启动，敌人销毁随 Actor Cleanup |
| DmgTaken clamp 修复 | `DamageExecution.cpp:85` 由 `FMath::Max(x,1.f)` 改为 `(x<=0)?1.f:Max(x,0.01f)`：capture 失败兜底 1.0；成功则允许 < 1.0（无畏 ×0.8 减伤生效） |
| DeathAnimComplete 事件 | `GA_Dead::StartDeathDelay` 在 IsValid 守卫后广播 `Ability.Event.DeathAnimComplete`（Payload.Instigator=Target=Avatar 自身），供 E002 监听 |
| CritHit 事件 | `DamageExecution.cpp` 暴击时除 `OnCritHit` 委托外，额外发送 `Ability.Event.Attack.CritHit` 事件（Payload.Magnitude=FinalDamage） |
| 新增 Tag | `Ability.Event.DeathAnimComplete`、`Ability.Event.Attack.CritHit`、`Buff.Status.SuperArmor`、`Buff.Status.Enraged`、`Buff.Status.Cursed` |
| BuffGiver 语义修复 | `BackpackGridComponent.cpp` 触发型符文调度器：`GiverActor` 由 `Payload.Instigator??Owner` 改为永远 `GetOwner()`，避免 OnDamageReceived 时 BuffGiver 被错置成攻击者；事件来源走 `LastEventContext.DamageCauser` |
| SuperArmor Fresnel 自动衔接 | `YogAbilitySystemComponent::OnTagUpdated` 加 `Buff.Status.SuperArmor` 分支：tag 加/减自动调 `StartSuperArmorFlash`/`StopSuperArmorFlash`，覆盖 C++ Poise 自动霸体 + FA AddTag（无畏 E001）两条路径；删除 Poise 内的手动调用避免双触发 |
| 双重审查 | codex_code_review 双轮通过：第一轮发现 P1（BuffGiver 语义）已修复；第二轮零 P1（仅 P2 提示 `Builds/` 目录不应入库，已加入 .gitignore） |
| 已知待办 | 无畏触发 GC、死咒咒印 GC + 血条紫色 UI 仍需在编辑器制作 |

---

## 2026-04-27

### [UI-010] 传送门浮窗符文行扩展：描述 + GenericEffects 内联展示

**状态**：C++ 完成已编译；浮窗高度按内容自然撑开（用户决策不加 ScrollBox）

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/PortalPreviewWidget.cpp`、`Docs/Systems/Level/WBP_PortalPreview_Layout.md` |
| 行结构改造 | 每条预骰符文行从 `[Icon, Name]` 改为 `HorizontalBox[ Icon 28×28(VAlign Top) \| VerticalBox{ Name(13, AutoWrap) / Description(11, AutoWrap)? / GenericEffects×N(11, AutoWrap)? } ]`；行间距 6px、子行间距 2px |
| 数据来源 | `URuneDataAsset.RuneInfo.RuneConfig` 的 `RuneDescription` + `GenericEffects[i]->{DisplayName, Description}` |
| GenericEffect 行格式 | `FText::Format` "• 名：描"，按 4 种情况降级（名+描 / 只名 / 只描 / 全空跳过），避免空冒号 |
| 样式集中维护 | cpp 顶部匿名 namespace 抽 `BuffNameFontSize/DescFontSize/EffectFontSize`、3 档颜色常量、`BuffRowSpacing/SubLineSpacing`、`SetTextSize` helper（保留原字体只改 Size） |
| RuneName 漏配兜底 | `ResolveRuneDisplayName`：非 Shipping 显示资产名 + UE_LOG Warning 定位漏配；Shipping 显示"未命名符文" + Warning |
| 浮窗高度策略 | 试过外包 ScrollBox + MaxDesiredHeight 但符文不显示（ScrollBox-VBox 双层结构疑似打断宽度链路），最终决策：不加 ScrollBox，浮窗高度随符文条数 / 描述长度自然撑高 |
| 双重审查 | codex_plan_review（设计）+ codex_code_review（代码）双轮通过；用户根据审查补 NameTB 的 `SetAutoWrapText(true)` 防长资产名溢出 |
| 已知限制 | 房间类型 Tag 仍走精确名比较（`Room.Type.Elite` 等），不支持子标签如 `Room.Type.Elite.Boss`；与 `PortalDirectionWidget` 房间类型样式重复维护，待后续统一抽 helper |

### [UI-009] 背包系统 4 项改造：Bug A/B 修复 + 预览补强 + 操作提示浮窗

**状态**：C++ 完成已编译并 push 到 origin/main；WBP_BackpackScreen 需在编辑器侧加 EndPreviewButton + OperationHintWidget

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackScreenWidget.h/.cpp`、`Content/UI/Playtest_UI/WBP_BackpackScreen.uasset` |
| Bug A 修复 | 删除"长按 3s 自动退回 pending"机制（无视觉反馈、易误触）；删 `LongPressDuration/HoldTime/bLongPressActive/LongPressCell` 字段 + NativeTick 计时块 + MouseButtonDown 长按初始化；取回路径仅保留拖拽到 pending / 手柄 PendingGamepadConfirm |
| Bug B 修复 | pending → 主背包拖拽显示完整 Shape + GrabOffset 校正；新增 `ShapePreviewCanvas` BindWidgetOptional + `ShowShapePreview/UpdateShapePreviewPosition/HideShapePreview` helpers；CellPx 改用 `GridGeometry.GetLocalSize().X / Backpack->GridWidth` 实际值（不再用 StyleDA->CellSize 数据字段） |
| 预览模式补强 | `EndPreviewButton` BindWidgetOptional（与 CloseButton 互斥显隐 Visible/Collapsed）；`SetPreviewMode` 清理所有 grab/drag 中间状态；4 处精确 guard：NativeOnMouseButtonDown 占用/pending 分支、ClickCell 占用、RotatePendingRune、RemoveRuneAtSelectedCell；占用格在预览/战斗下：FlashAndShakeCell + 状态消息（与战斗一致） |
| Pending 黄框残留 bug | 抽 `ClearPendingFocus(bool bClearSelection)` helper；鼠标点击主背包入口 + ClickCell 占用格 → `ClearPendingFocus(true)`；彻底清 `bCursorInPendingArea + PendingSelectedIdx`，避免按 R 仍旋转 pending（Codex 复审第二轮决策） |
| 操作提示浮窗 | `OperationHintWidget` UWidget BindWidgetOptional（任意 Border/SizeBox/Canvas）；`UpdateOperationHintVisibility()` 在 NativeTick 调用，缓存 `bOperationHintVisible` 避免每帧 SetVisibility；触发：`bGrabbingRune && !bIsPreviewMode && !IsInCombatPhase()` |
| 双重审查 | 经 Codex 双重审查：codex_plan_review（设计）+ codex_code_review（代码）双轮通过；用户确认决策（彻底清焦点 + 抽 helper + 缓存可见性） |
| 用户编辑器操作 | (1) WBP_BackpackScreen 加 `OperationHintWidget`（建议右下角 Border 容器，内放 CommonRichTextBlock + BP_InputActionDecorator，文案如「按 `<input action="MoveCursor"/>` 移动 / `<input action="RotateRune"/>` 旋转」；初始 Visibility=Collapsed，C++ 接管显隐）；(2) `EndPreviewButton`（与 CloseButton 同位，纯字幕"按 Esc 退出预览"也可，C++ 接管显隐与点击） |

### [MERGE-001] 撤回合作者 origin/main 大规模删除，保留本地 UI/Tutorial/Portal/Weapon/通用效果体系

**状态**：merge commit `b8674ebd` 已 push；合作者 pull 后会拿回他删过的所有文件

| 项目 | 内容 |
|------|------|
| 背景 | 合作者推送 10 commits 在 origin/main（-5130 行），删除大量本地正在用的 C++ 类与设计文档 |
| 撤回的删除（C++） | `UYogCommonRichTextBlock` / `UInputActionRichTextDecorator` / `UInfoPopupWidget` / `UGenericRuneEffectDA` / `UGenericEffectListWidget` / `UGenericEffectEntryWidget` / `UPortalPreviewWidget` / `UPortalDirectionWidget` / `FPortalPreviewInfo` / `ULevelInfoPopupDA` / `ULENode_ShowInfoPopup` / `UTutorialRegistryDA` 等 |
| 撤回的删除（文档） | `Docs/Survey/01-06_*.md`（玩家研究问卷依据）、`Docs/Systems/Rune/BackpackUI_Architecture.md`、`Docs/Systems/Level/WBP_PortalPreview_Layout.md`、`Docs/Systems/UI/WBP_LootSelection_Layout.md` 等 |
| 撤回的重构 | YogHUD / TutorialManager / Portal / RewardPickup / WeaponSpawner / GameMode 等被部分回滚到 e7200ab2 状态 |
| 决策依据 | 用户 2026-04-27 确认"全部本地"：本地仍在重构/使用这些类与文档，远程删除会破坏本地 UI 体系 |
| 后续行动 | 需告知合作者：他 pull 后会发现"删过的东西又回来了"；建议沟通哪些是"过期清理"哪些是"误删"；过期清理的部分待协商后专门做一个清理 commit |

### [SYS-001] 关卡生命周期事件总线 + LootSelection 视觉高亮 C++ 化 + 卡片堆叠修复

**状态**：C++ 完成已编译；用户需在编辑器侧加 SelectionBorder/HighlightBorder + 创建 LFA 配 BP_GameMode_Default 才能完整体验

| 项目 | 内容 |
|------|------|
| 核心文件 | `GameModes/GameLifecycleTypes.h`（新）、`GameModes/YogGameMode.h/cpp`、`UI/YogHUD.h/cpp`、`Component/BackpackGridComponent.cpp`、`UI/RuneInfoCardWidget.h/cpp`、`UI/LootSelectionWidget.h/cpp` |
| **事件总线** | `EGameLifecycleEvent` 枚举（7 个事件：LevelClear / LevelClearRevealed / FirstRuneAcquired / FirstRunePlaced / HeatPhaseEntered / PlayerDeath / GameStart）；`AYogGameMode::LifecycleEventFlows` TMap 在 BP_GameMode_Default 配映射；`TriggerLifecycleEvent(Event)` 查表后启 `LifecycleFlowComponent` 跑 LevelFlowAsset；一次性事件用 `FiredOnceEvents` 去重，且**仅在成功启动 Flow 后才记录**（避免未配 LFA 时静默吞事件） |
| HUD 解耦 | `YogHUD::OnLevelEndEffectFinished` FSimpleMulticastDelegate 在揭幕完成时广播；`GameMode::TryBindHUDDelegates` 延帧重试订阅（HUD 由 PC->ClientRestart 异步创建，最多重试 60 帧 ~1s）；HUD 不再直接调业务逻辑（如 TryFirstRuneTutorial），改广播给 GameMode |
| 触发位重构 | `EnterArrangementPhase` 删除硬编码 `TM->TryFirstRuneTutorial` 改为 `TriggerLifecycleEvent(LevelClear)`；`BackpackGridComponent::HandleHeatTutorial` 删除 `TM->TryHeatPhaseTutorial` 改为 `TriggerLifecycleEvent(HeatPhaseEntered)` 且不再 RemoveDynamic（让 GameMode 一次性去重独自负责，便于运行时补配 LFA 后下次 phase 变化仍能触发） |
| RuneInfoCard 选中态 | `SetSelected(bool)` + `BindWidgetOptional SelectionBorder`（UBorder）+ `SelectedRenderScale`（默认 1.06）；`NativeConstruct` 设 `RenderTransformPivot(0.5, 0.5)` 中心化；`NativeTick` 用 `FInterpTo` 平滑缩放；`ShowRune` 不主动清选中态，由调用方统一管理；通用接口可在背包/详情页复用 |
| LootSelection 卡片堆叠修复 | `RebuildCards` 给每张卡套 `USizeBox`（MinDesired/MaxDesired，默认 280×380 ~ 420×560）兜底 — 解决 WBP_RuneInfoCard 没设 Desired Size 时 HBox 把所有卡按 0×0 排版重叠的问题；`MinCardSize/MaxCardSize/CardHorizontalPadding` UPROPERTY 暴露，BP 可调 |
| LootSelection 视觉高亮 C++ 化 | `FocusCard` 遍历 SpawnedCards 调 `SetSelected(i==CurrentCardIndex)`；`FocusButton` 调 `ClearAllCardSelection() + UpdateButtonHighlight(SelectedIdx)`；按钮高亮通过外层 `BindWidgetOptional UBorder`（SkipHighlightBorder/PreviewHighlightBorder）显隐切换，不污染 Button.WidgetStyle |
| 鼠标点击索引修复 | `OnCardClicked0..4` 通过 `CardToOptionIndex` 映射到原始 Options 索引；绑定时用**可见卡索引** `SpawnedCards.Num()`（非原始 Options 索引 i），避免无效项被 continue 跳过时第一张可见卡误绑到 OnCardClicked1 导致越界 |
| 教程节奏修复 | First Rune 教程从 EnterArrangementPhase 同帧弹改为揭幕完成（`RevealProgress >= 1`）后才触发；ActivationZone 教程默认不出（用户没配 LFA = 沉默）；现有 `Anim_FadeIn` UMG 动画处理弹窗渐入，不必加 C++ 渐入 |
| 已知 P3 限制 | (1) `UFlowComponent::StartRootFlow()` 返回 void，无法严格判定 Flow 是否真正启动；(2) `TryBindHUDDelegates` 60 帧上限，正常 ClientRestart 路径下 HUD 在 ~3 帧创建远低于 60 帧；(3) `HideCard` 没重置 `bSelected/CurrentRenderScale`（下次 ShowRune 会被覆盖，无副作用） |
| 用户编辑器操作 | (1) WBP_RuneInfoCard 加 SelectionBorder（UBorder，DrawAs=Box，Margin=0.4，Tint=#FFD966 半透明）；(2) WBP_LootSelection 加 SkipHighlightBorder + PreviewHighlightBorder；(3) 创建 LFA_FirstRuneTutorial（节点：ShowTutorial EventID="tutorial_first_rune"）；(4) BP_GameMode_Default 配 `LifecycleEventFlows[LevelClearRevealed] = LFA_FirstRuneTutorial` |

---

## 2026-04-26

### [UI-008] LootSelection 改造：动态卡牌 + 跳过/重掷/预览背包 + 通用效果聚焦 + RuneInfoCard RichText

**状态**：C++ 完成已编译；WBP_LootSelection 需在编辑器按规格重做（CardContainer + BtnSkip + BtnBackpackPreview）

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/LootSelectionWidget.h/.cpp`（重写）、`UI/RuneInfoCardWidget.h/.cpp`、`UI/GenericEffectListWidget.h/.cpp`（新）、`UI/YogCommonRichTextBlock.h/.cpp`（新）、`UI/InputActionRichTextDecorator.h/.cpp`（新）、`Data/GenericRuneEffectDA.h/.cpp`（新）、`Data/RuneDataAsset.h`、`UI/YogHUD.h/.cpp`、`UI/BackpackScreenWidget.h/.cpp`、`UI/GameDialogWidget.h/.cpp`、`UI/WeaponFloatWidget.h`、`Map/RewardPickup.h/.cpp`、`Item/Weapon/WeaponSpawner.cpp` |
| 功能说明 | 三选一战利品 UI 重构 — 卡片数量动态（N≤5）、底部跳过/预览背包按钮、聚焦时显示通用效果浮窗（如击退）、鼠标点击 + 完整手柄支持（DPad/LB/RB/Stick X/A/B/Y） |
| 跳过流程 | RewardPickup 不再立刻 Destroy；选符文 → `ConsumeAndDestroy`；跳过 → `ResetForSkip(Player)` 复位 bPickedUp/PendingPickup/浮窗，玩家走开再回按 E 可重开 |
| 选完→开背包 | 沿用现有行为：`SelectRuneLoot` 末尾调 `HUD->OpenBackpack()`（整理模式） |
| 背包预览 | `HUD::OpenBackpackForPreview(OnClosed)` 封装；`UBackpackScreenWidget::SetPreviewMode(true)` 禁拖拽/旋转/出售/抓取/长按；NativeOnDeactivated 跳过 SyncPendingToPlayer 但保留 EndPauseEffect 配对 |
| 多 pickup 排队 | `AYogHUD` 新增 `LootQueue` (FQueuedLootRequest 数组) FIFO；`bLootSelectionActive` 标记；`OnLootSelectionFinished` 弹下一项 |
| 卡片→选项映射 | `CardToOptionIndex` 数组解决无效项（非 Rune / 空 RuneAsset）跳过后键盘选中错位问题 |
| 通用效果子窗 | `URuneInfoCardWidget::SetGenericEffectsExpanded(bool)`；默认 true 兼容背包；LootSelection RebuildCards 全设 false，FocusCard 时仅给当前卡 true；底层 `UGenericEffectListWidget` + `UGenericEffectEntryWidget` 由 C++ 全自动填充（BindWidget 写控件名即可） |
| 通用效果数据 | `UGenericRuneEffectDA`：DisplayName + Description + Icon + EffectFlow（仅信息归档不触发）；`FRuneConfig::GenericEffects` 数组引用 |
| RichText 体系 | `UYogCommonRichTextBlock` 子类绕开引擎 EditCondition 锁，新增 FontStyleClass / OverrideFontSize / OverrideColor 三字段叠加；`UInputActionRichTextDecorator` 解析 `<input action="X"/>` 自动按当前输入设备显示按键图标 |
| Tutorial RichText | `WBP_TutorialPopup` BodyText/BodySubText 改为 YogCommonRichTextBlock，DialogContent DA 文案中可写按键标签 |
| Weapon 浮窗按键 | `WBP_WeaponFloat` 加 `PickupHintText`（CommonRichTextBlock）显示「按 [E] 拾取武器」；同时移除 `WeaponSpawner::OnPlayerBeginOverlap` 中头顶 WidgetComponent 切换 |
| 待定决策 | [PendingDecisions.md](PM/PendingDecisions.md) DEC-001：背包详情卡通用效果默认展开（待玩家测试反馈再评审） |
| 重掷接口 | `RerollCard(int32)` 留空实现 + 日志，本版本不做 UI 动画 |
| HUD Tick 修复 | `AYogHUD::Tick` 把 `TickPortalPreview/TickBlackoutFade` 移到所有早返回之前（修复既有 bug：暂停 fade 完成后 Portal 引导/Blackout 卡死） |
| 设计文档 | [current_plan.md](WorkSession/current_plan.md) v2 含完整方案、用户决策、Codex 4 轮审查迭代记录 |

---

### [LEVEL-006] 传送门交互式选择 + 双层预览（HUD 方位指引 / 单例浮窗）+ 渐黑过场

**状态**：C++ 完成已编译；WBP_PortalPreview / WBP_PortalDirection 需在编辑器按规格搭建并赋到 BP_YogHUD

| 项目 | 内容 |
|------|------|
| 核心文件 | `Map/Portal.h/.cpp`、`Character/PlayerCharacterBase.h`、`Character/YogPlayerControllerBase.cpp`、`System/YogGameInstanceBase.h/.cpp`、`GameModes/YogGameMode.h/.cpp`、`Data/RoomDataAsset.h`、`UI/YogHUD.h/.cpp`、`UI/PortalPreviewWidget.h/.cpp`（新）、`UI/PortalDirectionWidget.h/.cpp`（新） |
| 功能说明 | 关卡结算后玩家不再"踩进就走"；屏幕外门画方位箭头，屏幕内门显示单例浮窗（房间名/类型徽章/已确定 Buff/战利品摘要），按 E 触发渐黑过场后切关 |
| 跨关 Buff 确定性 | Portal::Open 时按下一关 FloorConfig 难度档预骰 BuffPool；玩家确认进入时 TryEnter 把 PreRolledBuffs 写入 GI->PendingRoomBuffs；下一关 StartLevelSpawning 用 `bUsedPendingRoomData` 信号判定（即使 BuffCount=0 也合法）|
| HUD 单例浮窗 | `AYogHUD::TickPortalPreview` 按"PendingPortal 优先 → 否则距玩家最近 + 屏幕内可见或<800cm"挑 Target，100cm 滞回防中点抖动；位置每帧跟门屏幕投影 + 相机右向量避让 |
| 方位箭头 | `UPortalDirectionWidget` 仿 EnemyArrow，仅对屏幕外开启门画箭头 + 房间名标签；玩家进入任意 Box 时全隐 |
| 渐黑过场 | `APortal::TryEnter` 主导：bEntryInProgress 防重入 → 锁输入/锁背包 → 启 Timer 每 0.05s AddMovementInput 朝门方向 0.7s（不必抵达）→ HUD::BeginBlackoutFade 复用 LevelEndEffectDA 的 Saturation/Gain 三参 → 切关；超时 +0.5s 兜底；任何前置失败走 AbortEntry 显式恢复输入 |
| 下一关淡入 | 切关后 GI->bPlayLevelIntroFadeIn=true；下一关 HUD::BeginPlay 立即清标志 → 瞬时贴 Blackout → 调 EndBlackoutFade 反向插值（线性 PostProcess，不画圆形揭幕）|
| Blackout PP 隔离 | 新增 `BlackoutPPVolume`（独立 Actor），不与 PausePPVolume 互扰；目标值复用 `LevelEndEffectDA.BlackoutSaturation/Gain` |
| 类型徽章颜色 | `.cpp` 静态映射 `Room.Type.Normal/Elite/Shop/Event` → 灰/橙红/金/紫；徽章背景 + 标签字色统一 |
| 主城（HubRoom）| `EnterArrangementPhase` 末尾按 `bIsHubRoom` 跳过 ShowPortalGuidance — 主城既不显示浮窗也不显示箭头 |
| BP 视觉强化接口 | APortal: `K2_OnHighlightChanged(bool) / K2_OnPortalRangeEntered / K2_OnPortalRangeExited / K2_OnEntrySequenceStart`；UPortalPreviewWidget: `K2_OnPreviewShown / K2_OnPreviewHidden / K2_OnInteractHintShown`；C++ 不实现，BP_Portal/WBP 自由扩展 |
| 新增 DA 字段 | `URoomDataAsset.DisplayName : FText` —— 玩家可见的房间显示名；空时回退 `RoomName.ToString()` |
| 新增 GI 字段 | `PendingRoomBuffs : TArray<FBuffEntry>`、`bPlayLevelIntroFadeIn : bool`；`ClearRunState` 同点清这两项 + 已有的 PendingRoomData/PendingNextFloor/PendingRunState |
| 新增 GameMode | 静态 `ResolveTier(Room, TotalScore, LowMax, HighMin)` —— ActivatePortals 预骰 Buff 与 StartLevelSpawning 共用，避免选档逻辑漂移 |
| 行为变更 | 删除 `APortal::OnOverlapBegin` 自动调 EnterPortal 的路径；旧 `EnterPortal_Implementation` 保留作 BP override 钩，新流程不调它 |
| 已知限制 | 浮窗"屏幕内可见"未做 LineTrace 遮挡检测（用户决策）；门 Box 单值 PendingPortal 假设关卡间距保证不重叠（用户决策）；EnterPortal_Implementation 旧路径残留供 BP override 兼容 |
| 设计文档 | [WBP_PortalPreview_Layout.md](Systems/Level/WBP_PortalPreview_Layout.md)、[WBP_PortalDirection_Layout.md](Systems/Level/WBP_PortalDirection_Layout.md)、[Portal_ConfigGuide.md](Systems/Level/Portal_ConfigGuide.md)（已更新） |
| WBP 待办 | 编辑器内：① 创建 WBP_PortalPreview（父类 UPortalPreviewWidget）按规格搭建 ② 创建 WBP_PortalDirection（父类 UPortalDirectionWidget，根 RootCanvas + ArrowTexture 字段）③ BP_YogHUD Details 填 PortalPreviewClass / PortalDirectionClass |

---

### [COMBAT-009] AN_MeleeDamage HitStop 模式选择器 + 命中事件广播

**状态**：C++ 完成已编译；AN_MeleeDamage 上直接配置即可使用

| 项目 | 内容 |
|------|------|
| 核心文件 | `Animation/AN_MeleeDamage.h/.cpp`、`BuffFlow/Nodes/BFNode_HitStop.cpp`、`Character/YogCharacterBase.h`、`AbilitySystem/Abilities/GA_MeleeAttack.cpp` |
| 功能说明 | AN_MeleeDamage 新增 `EHitStopMode` 枚举选择器（None/Freeze/Slow），选择模式后对应参数自动显示/隐藏，无需在 FA 中写 HitStop Tag 即可自包含触发 |
| HitStop 触发 | AN 选择 Freeze/Slow → Notify 时暂存到 `Character->PendingHitStopOverride` → `BFNode_HitStop` 读取并激活对应阶段 |
| Tag 共存 | FA Tag 触发（暴击等特殊场景）与 AN 模式驱动可同时生效；AN 覆盖参数只作用于 AN 指定的模式，Tag 触发的模式使用节点默认值 |
| 命中事件广播 | 新增 `OnHitEventTags`（`TArray<FGameplayTag>`）：命中目标时向攻击者 `SendGameplayEventToActor` 广播，供镜头抖动、音效、屏幕特效等系统监听 |
| 安全清理 | `GA_MeleeAttack::EndAbility` 中清空 `PendingOnHitEventTags`，防止蒙太奇被打断后残留 |
| 新增枚举 | `EHitStopMode`（None/Freeze/Slow），定义在 `AN_MeleeDamage.h`，`YogCharacterBase.h` 前向声明 |
| 设计文档 | [HitStop_Design.md](Design/Combat/HitStop_Design.md) |

---

## 2026-04-23

### [UI-007] 武器玻璃图标热度颜色 — WeaponGlassIconWidget 自订阅热度阶段

**状态**：C++ 完成已编译；WBP 层级需在编辑器按规格搭建 HeatColorOverlay

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponGlassIconWidget.h/.cpp`、`PlayerCharacterBase.h/.cpp`、`YogHUD.h/.cpp` |
| 功能说明 | 武器拾取后左下角玻璃图标显示当前热度颜色；升阶时自动变色；切关卡后颜色恢复 |
| 架构变更 | Widget 在 `NativeConstruct` 自订阅 `PlayerCharacterBase::OnHeatPhaseChanged`，读取 `PlayerCharacterBase::HeatStyleDA` 获取颜色，不再由 YogHUD 推送 |
| 新增接口 | `PlayerCharacterBase::GetCurrentHeatPhase()` — BlueprintPure，返回当前阶段 0/1/2/3 |
| 移除接口 | `WeaponGlassIconWidget::SetHeatColor()`、`YogHUD::ApplyGlassIconHeatColor()`、`YogHUD::BackpackStyleDA` |
| YogHUD 保留 | `bHasWeapon` — 跨关卡记录是否已拾取武器，重建 Widget 后调用 `Show()` 恢复 |
| WBP 层级 | Overlay → GlassBG → GlassBGCenter → **HeatColorOverlay**（Image，Fill，Translucent） → GlassBorderImage |
| 颜色来源 | `DA_BackpackStyle`（通过 `PlayerCharacterBase::HeatStyleDA` 访问，与背包格子颜色同源） |
| 配置入口 | `B_PlayerOne` Details → Heat\|Visual → **HeatStyleDA** 填入 `DA_BackpackStyle` |
| 已知限制 | `NativeConstruct` 时若 PlayerPawn 尚未 Possess（极少数情况），初始颜色延迟到 `Show()` 调用时同步 |

---

## 2026-04-23

### [FEAT-033] 冲刺空气墙 — BP_DashBarrier + GA_PlayerDash 精确停止

**状态**：C++ 完成已编译；需在编辑器创建 `BP_DashBarrier` Blueprint（见下方配置说明）并放置到关卡边界

| 项目 | 内容 |
|------|------|
| 核心文件 | `AbilitySystem/Abilities/GA_PlayerDash.cpp`（`GetFurthestValidDashDistance` 新增 DashBarrier 检测） |
| 工作原理 | 前向 DashTrace 扫描命中 `ActorHasTag("DashBarrier")` 的 Actor 时，直接返回 `HitDist`，跳过越障延伸步逻辑；`AnimScale` 精确缩至停止点，动画不会穿墙 |
| 兼容性 | 对普通台阶/障碍物的越障延伸（6步×50cm）行为**完全保留**，互不影响 |
| 蓝图配置 | 创建 `BP_DashBarrier`（父类 Actor）→ 添加 BoxCollision → Actor Tags 加 `DashBarrier` |
| 碰撞设置 | BoxCollision：`DashTrace`（GameTraceChannel1）= Block；`WorldStatic` = Block；其余 Ignore |
| 典型用法 | 放置在不允许冲刺进入的区域边界；不需 Mesh，默认不可见；调试可临时 `SetHiddenInGame(false)` |

---

### [FEAT-032] HitStop 命中停顿系统重构：AN → FA + Tag 驱动 + 蒙太奇暂停

**状态**：C++ 完成已编译；需在编辑器创建 `FA_PlayerHitStop` FlowAsset 并挂载到玩家，删除蒙太奇上旧的 `AN Hit Stop` Notify

| 项目 | 内容 |
|------|------|
| 核心文件 | `Animation/HitStopManager.h/.cpp`、`BuffFlow/Nodes/BFNode_HitStop.h/.cpp` |
| 删除文件 | `Animation/AN_HitStop.h/.cpp`（全量删除，不再使用 AnimNotify 触发） |
| 新 Tag | `Buff.Status.HitStop.Freeze`（冻结帧）、`Buff.Status.HitStop.Slow`（延缓帧）；写入 `Config/Tags/BuffTag.ini` |
| 冻结帧逻辑 | `Montage_Pause` → 真实时间计时（FrozenDuration 秒）→ `Montage_Resume`；不暂停全局时间 |
| 延缓帧逻辑 | `Montage_SetPlayRate(SlowRate)` → 真实时间计时（SlowDuration 秒）→ `SetPlayRate(CatchUpRate)` 追帧 → 恢复 1.0；追帧时长自动计算：`SlowDuration × (1-SlowRate) / (CatchUpRate-1)` |
| FA 接线 | `[OnDamageDealt] → [BFNode_HitStop]`（无 Tag 时静默跳过） |
| 暴击冻结帧 | 暴击 FA：`[OnCritHit] → [AddTag: Buff.Status.HitStop.Freeze]`；OnCritHit 先于 OnDamageDealt 触发，顺序天然正确 |
| 设计文档 | `Docs/Design/Combat/HitStop_Design.md` |

---

### [FIX-028] EnemyArrow 系统修复：WBP 还原 + NativeTick 崩溃防御

**状态**：C++ 完成已编译；需在 HB_PlayerMain Details → Enemy Arrow → `EnemyArrowWidgetClass` 赋值 `WBP_EnemyArrow`

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/EnemyArrowWidget.cpp`、`Content/UI/Playtest_UI/CombatInfo/WBP_EnemyArrow.uasset` |
| WBP 还原 | `WBP_EnemyArrow` 被 `0c7c83f5` 提交意外删除，从 `5f6f088a` 恢复到 `Content/UI/Playtest_UI/CombatInfo/` |
| 崩溃原因 | `RootCanvas`（BindWidget）为 null 时 `RebuildArrowPool` 提前返回，`ArrowImages` 为空，`NativeTick` 第 121 行访问 `ArrowImages[0]` 越界 |
| 修复方案 | `NativeTick` 排序前加守卫：`if (ArrowImages.IsEmpty() && RootCanvas) RebuildArrowPool();` + `if (ArrowImages.IsEmpty()) { HideAll(); return; }` |
| WBP 必要结构 | 根控件必须为 `Canvas Panel`，命名精确为 `RootCanvas`；Anchors 全屏，Hit Test = Invisible |
| 显示条件 | 所有存活敌人均不在屏幕内 **且** 距上次受伤/屏幕内敌人超过 `AppearDelay`（默认 1.5s）才显示箭头 |

---

### [FEAT-030] 信息浮窗（不暂停）：LENode_ShowTutorial 增加 bPauseGame 选项

**状态**：C++ 完成已编译；LevelFlow DA 里取消勾选 `Pause Game` 即可使用

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/GameDialogWidget.h/.cpp`、`Tutorial/TutorialManager.h/.cpp`、`LevelFlow/Nodes/LENode_ShowTutorial.h/.cpp` |
| 接口变化 | `ShowPopup(Pages, bPauseGame=true)`、`ShowByEventID(EventID, PC, bPauseGame=true)` — 均向后兼容 |
| 节点字段 | `LENode_ShowTutorial` Details → `Pause Game`（默认勾选 = 原有行为，取消 = 纯信息提示） |
| 影响范围 | `bPauseGame=false` 时：不调 `SetGamePaused`、不触发 `BeginPauseEffect`/`EndPauseEffect`；鼠标显示/隐藏逻辑不变 |
| 典型用法 | 触发区域信息提示：`ALevelEventTrigger` → DA 里 `Show Tutorial Popup`（取消勾选 Pause Game）→ OnClosed → End |

---

### [FEAT-031] LevelFlow 新节点：LENode_WaitForLootSelected（等待玩家选符文）

**状态**：C++ 完成已编译；在 LevelFlow DA Flow 编辑器中可用

| 项目 | 内容 |
|------|------|
| 核心文件 | `LevelFlow/Nodes/LENode_WaitForLootSelected.h/.cpp`、`GameModes/YogGameMode.h/.cpp` |
| 委托 | `AYogGameMode::OnLootSelected`（`FOnLootSelected`，非 Dynamic）；在 `SelectLoot()` 末尾广播 |
| 节点引脚 | In → 开始监听；OnSelected → 玩家选完任意符文后触发 |
| 典型连法 | `Start → WaitForLootSelected → ShowTutorial（bPauseGame=false）→ End` |
| 注意 | 每次 `SelectLoot` 均会广播，若同关卡多次触发三选一，节点只等第一次（`TriggerOutput` 后自动 Cleanup） |

---

### [FEAT-029] 祭坛交互系统（AltarActor + 三功能 UI）

**状态**：C++ 完成已编译；需在 Editor 创建 DA/BP/WBP 资产并放置到事件关卡场景

| 项目 | 内容 |
|------|------|
| 核心文件 | `Map/AltarActor.h/.cpp`、`Data/AltarDataAsset.h`、`UI/AltarMenuWidget.h/.cpp`、`UI/RunePurificationWidget.h/.cpp`、`UI/SacrificeSelectionWidget.h/.cpp`、`Component/BackpackGridComponent.h/.cpp`（新增 TryRemoveRuneCell） |
| 三功能 | 净化（删格子）/ 升级（存根，按钮置灰）/ 献祭（三选一获取符文） |
| 交互接口 | `AAltarActor` 实现 `IPlayerInteraction`；`bIsActive` 仅在 `ELevelPhase::Arrangement` 为 true；`TryInteract(Player)` 打开 `UAltarMenuWidget` |
| 净化逻辑 | `RunePurificationWidget`：Phase0 展示符文列表 → `SelectRune(Guid)` → Phase1 展示可选格子（Shape.Cells 除 (0,0)）→ `SelectCell` → `ConfirmPurification` 调 `BackpackGridComponent::TryRemoveRuneCell` |
| TryRemoveRuneCell | `GridOccupancy` 清格、`Shape.Cells.RemoveAt`、`RefreshAllActivations`、广播 `OnRuneCellRemoved`；(0,0) pivot 格拒绝删除 |
| 献祭逻辑 | `SacrificeSelectionWidget`：从 `UAltarDataAsset.SacrificeRunePool` 随机抽三个 → `SelectSacrificeOption` → 展示代价文字 → `ConfirmSacrifice` 调 `Player->AddRuneToInventory`；代价 FA 由 GrantedRune 激活时执行 |
| 取消行为 | 净化任意阶段取消关闭；献祭 Phase1 取消回 Phase0，Phase0 取消关闭 |
| DA 配置 | 创建 `UAltarDataAsset`（`DA_Altar_*`），填 `SacrificeRunePool`（条目含符文 + `CostDescription`） |
| Editor 操作 | BP_AltarActor（配 AltarData + 三个 WidgetClass）+ WBP_AltarMenu + WBP_RunePurification + WBP_SacrificeSelection；放置到事件关卡 |
| 已知限制 | 符文升级功能为存根（`OnUpgradeRequested` BP 事件，无逻辑）；献祭代价执行依赖 GrantedRune FA 设计 |

---

### [COMBAT-008] 霸体金光 + 敌人韧性 DA 化

**状态**：C++ 完成已编译；需在各 DA_Enemy_* 的 Enemy — Poise 分类填写参数

| 项目 | 内容 |
|------|------|
| 核心文件 | `Data/EnemyData.h`、`Character/EnemyCharacterBase.cpp`、`Character/YogCharacterBase.h/.cpp`、`AbilitySystem/YogAbilitySystemComponent.cpp` |
| DA 新字段 | `SuperArmorThreshold`（默认 3）/ `SuperArmorDuration`（默认 2s）——`EnemyCharacterBase::BeginPlay` 自动推到 ASC |
| 金光触发 | 连续受击 ≥ Threshold → ASC 调 `StartSuperArmorFlash()` → Tick 以 `SuperArmorPulseFreq`（默认 6Hz）驱动金黄正弦脉冲（`FLinearColor(5,3,0)`） |
| 金光结束 | `SuperArmorDuration` 到期 → ASC 调 `StopSuperArmorFlash()` → Overlay 移除 |
| 优先级 | 命中白闪 > 霸体金光 > 攻击前红光；白闪结束后金光自动恢复 |
| 可调参数 | `SuperArmorPulseFreq`（蓝图 Details → Combat\|Visual，默认 6）/ `SuperArmorThreshold` / `SuperArmorDuration`（DA） |
| 技术文档 | [CharacterFlash_Technical](../Systems/VFX/CharacterFlash_Technical.md) |

---

### [UI-023] 液态血条系统 — LiquidHealthBarWidget + LiquidHealthBar.ush

**状态**：C++ 完成已编译；需在 WBP_HUDRoot 中放置 WB_PlayerHealthBar（变量名 `PlayerHealthBar`），并在 WB_PlayerHealthBar Details 调整 `FillWindowEnd`

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/LiquidHealthBarWidget.h/.cpp`、`Shaders/LiquidHealthBar.ush`、`UI/YogHUD.cpp` |
| 材质驱动 | C++ 在 `NativeConstruct` 创建 DMI，写 `FillPercent / SloshAmplitude / SloshPhase` 三个 ScalarParameter |
| 液态晃动 | `SetHealthPercent` 触发晃动（幅度 ∝ 血量变化量），`NativeTick` 指数阻尼 `exp(-DampingRate * dt)`，幅度 <0.0004 停 Tick |
| UV 映射 | `FillWindowEnd`（默认 0.5）：帧纹理透明管道右边界；`LiquidDynMat->SetScalarParameterValue("FillPercent", CurrentPct * FillWindowEnd)` |
| GAS 绑定 | `YogHUD::BindHealthAttributes` 绑 `GetHealthAttribute` + `GetMaxHealthAttribute` 委托；Pawn 未就绪时等 `OnPossessedPawnChanged` |
| 颜色配置 | `LiquidColorDeep / LiquidColorSurface / GlintColor`（在 WBP Details 里调色） |
| 配置入口 | `WB_PlayerHealthBar` Details → `LiquidMaterial`（MI_LiquidHealthBar）/ `LiquidFillImage`（控件变量名）/ `FillWindowEnd` |

---

### [UI-024] HUD 主容器架构 — YogHUDRootWidget

**状态**：C++ 完成已编译；需在 Editor 创建 WBP_HUDRoot（父类 YogHUDRootWidget）并在 BP_YogHUD Details → MainHUDClass 赋值

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/YogHUDRootWidget.h/.cpp`、`UI/YogHUD.h/.cpp` |
| 架构变化 | 原来各 HUD 控件由 `YogHUD.h` 分别持有并逐个 `CreateWidget + AddToViewport`；改为一个 `UYogHUDRootWidget` 容器，所有常驻子控件在 WBP Canvas Panel 里摆放，位置用锚点可视化调整 |
| BindWidget 字段 | `PlayerHealthBar`（必须，`ULiquidHealthBarWidget`）/ `EnemyArrow`（可选）/ `WeaponGlassIcon`（可选）/ `HeatBar`（可选，`UHeatBarWidget`） |
| 创建方式 | `YogHUD::BeginPlay` 仅 `CreateWidget<UYogHUDRootWidget>(…, MainHUDClass)` 一次，`AddToViewport(1)` |
| 访问路径 | `MainHUDWidget->PlayerHealthBar`、`MainHUDWidget->WeaponGlassIcon` 等 |
| WBP 配置 | 父类选 `YogHUDRootWidget`；各子控件的 **变量名** 必须与 BindWidget 字段名完全一致；Is Variable = ON |
| 配置入口 | `BP_YogHUD` Details → HUD → `MainHUDClass` = `WBP_HUDRoot` |

---

## 2026-04-22

### [FIX-022] 三选一 Loot UI — 以拾取物为主导 + 多拾取物独立触发

**状态**：C++ 完成已编译；需在 HB_PlayerMain Details → Loot → `LootSelectionWidgetClass` 赋值 `WBP_LootSelection`

| 项目 | 内容 |
|------|------|
| 核心文件 | `Map/RewardPickup.cpp`、`GameModes/YogGameMode.h/.cpp`、`UI/YogHUD.h/.cpp`、`UI/LootSelectionWidget.h/.cpp` |
| 核心接口 | `YogGameMode::GenerateIndependentLootOptions()` / `AYogHUD::ShowLootSelectionUI()` / `ULootSelectionWidget::ShowLootUI()` |
| 架构变化 | 每个 `ARewardPickup` 拾取时直接调 `HUD->ShowLootSelectionUI`，不走 `OnLootGenerated` 广播；Widget 在 HUD BeginPlay 持久创建（ZOrder 15），HUD 检测 `IsInViewport()` 自动重建 |
| 原 Bug | ① 共享 `LootAssignedThisLevel` TSet 耗尽符文池 → 改为本地 TSet；② 广播 + HUD 直调 + NativeConstruct pending 三路并发 → ShowLootUI 被调 3 次 |
| 输入恢复顺序 | `SetPause(false)` → `SetInputMode(GameOnly)` → `OpenBackpack()`（顺序不能颠倒） |
| 技术文档 | [LootSelection_Technical](../Systems/UI/LootSelection_Technical.md) |

---

### [FIX-023] LootSelectionWidget CommonUI 完全隔离 — 防止 NativeOnFocusLost 销毁 Widget

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/LootSelectionWidget.h/.cpp` |
| 根因 | `SetUserFocus()` 把焦点交给子控件时触发 `NativeOnFocusLost` → `Super` → `BP_OnFocusLost` → WBP `Event On Focus Lost` 有延迟 `RemoveFromParent` → Widget 在选择后被销毁 |
| 诊断过程 | `NativeOnDeactivated` 无日志（CommonUI DeactivateWidget 路径未走）；`NativeOnFocusLost cause=0` 出现在 ShowLootUI 期间，NativeDestruct 在 SelectRuneLoot 之后 → 确认为 BP 事件延迟销毁 |
| 修复 | 三处截断：① `GetDesiredInputConfig()` 返回空 optional；② `NativeOnDeactivated` 不调 Super；③ `NativeOnFocusLost` 不调 Super（阻断 BP_OnFocusLost） |
| 重建兜底 | `ShowLootSelectionUI` 改用 `IsInViewport()` 检查（`GetIsEnabled()` 在 Widget 被移出 Viewport 后仍返回 true，无法正确检测） |
| WBP 建议 | 删除 WBP_LootSelection 中 `Event On Focus Lost` 下的所有节点 |
| 技术文档 | [LootSelection_Technical](../Systems/UI/LootSelection_Technical.md) |

---

### [FEAT-023] 关卡结束视觉特效 — 时间膨胀 + 渐黑 + 圆形揭幕

**状态**：C++ 完成已编译；需在 Editor 创建 WBP_LevelEndReveal + DA_LevelEndEffect 并配置 BP_YogHUD

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/LevelEndRevealWidget.h/.cpp`、`Data/LevelEndEffectDA.h`、`UI/YogHUD.h/.cpp` |
| 数据资产 | `LevelEndEffectDA`（SlowMoScale / SlowMoDuration / BlackoutFadeDuration / RevealDuration / RevealEdgeSharpness / RevealMaterial） |
| 时序 | [0, BlackoutFadeDuration) 渐黑 → 保持全黑 → t=SlowMoDuration 恢复时速 → 圆形揭幕扩散 |
| 圆形揭幕 | `LevelEndRevealWidget`：全屏 Image 命名 `RevealImage`，`InitReveal()` 创建 DynMat，YogHUD Tick 每帧写 `RevealProgress` |
| 揭幕中心 | `TriggerLevelEndEffect(LootWorldPos)` 传入 Loot 世界坐标，HUD 转屏幕 UV 写入材质 `RevealCenter` |
| 配置入口 | `BP_YogHUD` → `LevelEndEffectDA`=`DA_LevelEndEffect`；`LevelEndRevealWidgetClass`=`WBP_LevelEndReveal` |

---

### [FEAT-024] 开场镜头标记 — ALevelIntroCameraMarker

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `Map/LevelIntroCameraMarker.h/.cpp` |
| 功能 | 关卡中放置 Actor → 调 `TriggerIntro()` → 立即切到标记视角 → 停留 `HoldDuration` → 平滑移回玩家 |
| 参数 | `HoldDuration`（默认 2s）、`MoveDuration`（默认 1.5s）、`bDisableInputDuringIntro`（默认 true） |
| 调用 | 从 LevelFlow 或 `BeginPlay` 调 `TriggerIntro()`，内部用 Timer 序列管理 ViewTarget 切换 |

---

### [FEAT-025] 符文旋转系统 + Loot 最佳落点算法

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `Data/RuneDataAsset.h/.cpp`、`UI/BackpackScreenWidget.h/.cpp`、`GameModes/YogGameMode.h/.cpp` |
| 符文旋转 | `FRuneInstance.Rotation`（0-3，每次 90° 顺时针）；`FRuneShape::Rotate90()` 返回旋转后形状；`GetPivotOffset(N)` 补偿 Pivot 坐标偏移 |
| 操作接口 | `RotateSelectedRune()`（格子内已选符文）、`RotatePendingRune()`（待放置区符文） |
| Loot 落点 | `FindLootSpawnLocation()`：8 方向候选，依次筛选无碰撞 + 相机可见 + 屏幕内（5% 边缘余量），全部失败退回玩家原位 |

---

### [FIX-027] 背包战斗锁定改为基于 ELevelPhase — 进入关卡即锁定

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackScreenWidget.cpp` |
| 根因 | `IsInCombatPhase()` 原来调 `HasAliveEnemies()`；进入关卡到第一波刷怪前 `AliveEnemies` 为空，返回 false，背包在此窗口可操作 |
| 修复 | 改为 `GM->CurrentPhase == ELevelPhase::Combat`；`CurrentPhase` 默认值即为 `Combat`，进关即锁 |
| 解锁时机 | `EnterArrangementPhase()` 设 `CurrentPhase = Arrangement`（在 `CheckLevelComplete()` 内由最后一只敌人死亡触发）|
| Hub Room 兼容 | `StartLevelSpawning` 对主城房间直接写 `CurrentPhase = Arrangement`，锁定逻辑自动跳过 |

**关卡流程对应表：**

| 时间点 | CurrentPhase | 背包 |
|--------|-------------|------|
| 进入关卡（刷怪前）| Combat | 🔒 锁定 |
| 敌人存活中 | Combat | 🔒 锁定 |
| 所有敌人死亡 → CheckLevelComplete → EnterArrangementPhase | Arrangement | ✅ 可操作 |
| 主城 Hub Room（StartLevelSpawning 直接设置）| Arrangement | ✅ 可操作 |

---

### [UI-021] 背包交互重构 — 单击抓取 / 长按退回 / 换格自动拾起 / 悬停绿框

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackScreenWidget.h/.cpp` |
| 单击抓取 | 鼠标左键点击有符文格 → 立即进入 `bGrabbingRune=true`，黄框高亮，信息卡弹出；不再需要二次拖拽 |
| 长按退回 | 抓取状态下持续按住鼠标 3s（`LongPressDuration=3.0f`）→ `SendGrabbedRuneToPending()`：自动取出放入待放置区第一个空格并清除抓取状态 |
| 换格自动拾起 | 将符文放入有其他符文的格子（swap）成功后，被替换出来的符文自动进入抓取状态（`bGrabbingRune=true`，`GrabbedFromCell=PivotA`），无需再次点击 |
| 悬停绿框 | 鼠标移动或 D-Pad 移动时（无符文抓取中），当前悬停格所在的符文显示绿色包围框（NativePaint `CachedHoverGuid`）；进入抓取状态后绿框自动消失 |
| 新增私有字段 | `LongPressHoldTime`、`bLongPressActive`、`LongPressCell`、`LongPressDuration`（3s）、`SendGrabbedRuneToPending()` |

---

### [UI-022] 符文边框 NativePaint — 替换 CanvasPanel Overlay + 像素精确对齐

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackGridWidget.h/.cpp` |
| 替换原因 | `RuneBorderCanvas`（CanvasPanel Overlay）在 BackpackGrid 嵌套于 VerticalBox→SizeBox 时定位失准 |
| 方案 | `UBackpackGridWidget::NativePaint` override：`FSlateDrawElement::MakeLines` 直接在 Widget 渲染层绘制包围框，无需额外 CanvasPanel |
| 对齐精度 | 每条边框的 TL/BR 坐标从对应角格的 `CachedSlots[idx]->GetCachedGeometry()` 直接读取绝对位置再转换，完全消除层级嵌套引入的偏移 |
| 颜色分层 | 选中符文：金黄（2.5px）/ 悬浮符文：绿（2.0px）/ 其余符文：灰（1.5px） |
| 缓存字段 | `CachedBackpackRef`、`CachedSelectedGuid`、`CachedHoverGuid`（`RefreshCells` 更新，`NativePaint` 只读） |

---

### [FIX-021] 战斗锁定补全三处红闪+抖动调用

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackScreenWidget.cpp` |
| 根因 | `ShakeAndFlash()` 已在 [FIX-019] 实现，但 `BackpackScreenWidget` 的三处战斗锁定检查点（鼠标点击放下 / 鼠标点击拾起 / 手柄 A 键拾起）均未调用它，导致只弹文字提示无视觉反馈 |
| 修复 | 三处 `IsInCombatPhase()` 命中后均追加 `BackpackGridWidget->FlashAndShakeCell(Col, Row)` 调用 |

---

### [FIX-024] 跨关热度丢失修复 — FRunState 保存与恢复重构

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `System/YogGameInstanceBase.h`、`GameModes/YogGameMode.cpp`、`Character/PlayerCharacterBase.cpp` |
| 根因 | `FRunState` 有 `CurrentPhase` 字段但无热度值字段；恢复时调 `ResetHeatToPhaseFloor()` 总是将热度重置为 0 |
| 修复 | `FRunState` 新增 `CurrentHeat`（float）字段；两处切关触发点（`ConfirmArrangementAndTransition` / Portal TransitionToLevel）保存当时热度绝对值；恢复时 `SetNumericAttributeBase` + `OnHeatValueChanged` |
| 两阶溢出处理 | `CurrentHeat > MaxHeat` 时先 `IncrementPhase()` 再 `HeatToSet -= MaxHeat`，实现两阶保留符文的跨阶恢复 |
| 默认行为 | 无热度携带符文时 `CurrentHeat = 0`，热度清空 |

---

### [FIX-025] 满阶热度溢出修复 — BaseAttributeSet + BFNode 双重拦截

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `AbilitySystem/Attribute/BaseAttributeSet.cpp`、`BuffFlow/Nodes/BFNode_IncrementPhase.cpp` |
| 问题 | 热度阶段已为 3（最高）时，热度满溢仍触发升阶逻辑 → `BGC->IncrementPhase()` 无实际效果但 `BFNode_IncrementPhase` Out 引脚仍触发，导致 FA 下游清零节点将热度置 0 |
| 修复①（AttributeSet）| `PostGameplayEffectExecute`：`BGC->GetCurrentPhase() >= 3` 时改为 `SetHeat(GetMaxHeat())`，不再广播 `OnPhaseUpReady` |
| 修复②（BFNode）| `BFNode_IncrementPhase::ExecuteInput`：比较 `PhaseBefore` 与调用后实际值，仅阶段真正变化时才触发 Out 引脚 |

---

### [FIX-026] SacrificeGrace 跨关未保存修复

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `System/YogGameInstanceBase.h`、`GameModes/YogGameMode.cpp`、`Character/PlayerCharacterBase.cpp` |
| 根因 | `FRunState` 无 `ActiveSacrificeGrace` 字段，切关后献祭恩赐 Buff 丢失 |
| 修复 | `FRunState` 新增 `TObjectPtr<USacrificeGraceDA> ActiveSacrificeGrace`；切关时两处读 `Player->ActiveSacrificeGrace` 写入；恢复时 `RestoreRunStateFromGI()` 读回 |

---

### [FEAT-026] 热度携带符文 — 余烬（一阶）/ 炽核（两阶）

**状态**：C++ 完成已编译；需在 Editor 创建两个 FA 并赋值 Tag

| 项目 | 内容 |
|------|------|
| 核心文件 | `Config/DefaultGameplayTags.ini`、`GameModes/YogGameMode.cpp` |
| 新增 Tag | `Buff.HeatCarry.OnePhase`（余烬）/ `Buff.HeatCarry.TwoPhase`（炽核）|
| 保存逻辑 | 切关时检查玩家 ASC：炽核 → `CurrentHeat = MaxHeat * 2`；余烬 → `CurrentHeat = MaxHeat`；无符文 → `CurrentHeat = 0` |
| 恢复逻辑 | `RestoreRunStateFromGI()`：`CurrentHeat > MaxHeat` 时额外执行一次 `IncrementPhase()` 实现跨阶，剩余热度写入当前阶段 |
| Editor 操作 | 创建 `FA_Rune_余烬`：`Start → BFNode_GrantTag(Buff.HeatCarry.OnePhase)`；`FA_Rune_炽核`：`Start → BFNode_GrantTag(Buff.HeatCarry.TwoPhase)` |

---

### [FEAT-027] 武器初始符文自动放置 — 拾取时注入热度一激活区

**状态**：C++ 完成已编译；需在 `DA_Weapon_*` 的 `InitialRunes` 数组填入起始符文

| 项目 | 内容 |
|------|------|
| 核心文件 | `Item/Weapon/WeaponSpawner.cpp`（`TryPickupWeapon` 步骤 4b） |
| 触发时机 | 拾取武器后 `ApplyBackpackConfig` 完成后立即执行 |
| 放置逻辑 | `GetActivationZoneCellsForPhase(0)` 获取热度一激活区所有格子；对每个 `InitialRunes[i]` 创建 `FRuneInstance` 后逐格 `TryPlaceRune`，成功即 break |
| 数据来源 | `UWeaponDefinition::InitialRunes`（`TArray<URuneDataAsset*>`），只需在 DA 填值 |
| 限制 | 激活区格子不足时多余符文静默跳过；切关恢复走 `PlacedRunes` 序列化路径，无需重复放置 |

---

## 2026-04-20

### [INPUT-001] 火绳枪输入系统完整接入 — Reload 绑定 + 重攻击松键修复 + GA Tag 激活

**状态**：已完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `Character/YogPlayerControllerBase.h/.cpp`、`AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h/.cpp`、以及所有 6 个 Musket GA cpp |
| 根因（重攻击松键） | 项目使用 `TryActivateAbilitiesByTag`，不走 InputID 路径，`UAbilityTask_WaitInputRelease::OnRelease` 永远不触发 |
| 修复方案 | `Input_HeavyAttack` 额外绑定 `ETriggerEvent::Completed` → `HeavyAttackReleased()`；该函数调 `ASC->HandleGameplayEvent(GameplayEvent.Musket.HeavyRelease)`；GA 改用 `UAbilityTask_WaitGameplayEvent` 等待该事件后 `DoFire()` |
| Reload 绑定 | 新增 `Input_Reload` UPROPERTY；绑 `MusketReload()` → `TryActivateAbilitiesByTag(PlayerState.AbilityCast.Reload)` |
| GA Tag 接入 | 6 个 GA 各自在 `AbilityTags` 补加 `PlayerState.AbilityCast.*`；LightAttack 追加 `ActivationBlockedTags: Buff.Status.DashInvincible` |
| 新增 Tags | `GameplayEvent.Musket.HeavyRelease`、`PlayerState.AbilityCast.LightAtk/HeavyAtk/Dash/Reload` 已写入 `DefaultGameplayTags.ini` |
| 用户需在 Editor 操作 | 创建 `IA_Reload`（Digital，Keyboard R）→ 赋给 `BP_PlayerController.Input_Reload`；`IMC_Default` 加入该 Action |

---

### [FIX-012] Tutorial Popup 关闭后操控锁死 — CommonUI 输入模式显式恢复

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/GameDialogWidget.cpp` |
| 根因 | `Super::NativeOnDeactivated()` 将 Menu 模式从 CommonUI Stack 弹出，但若 Stack 上没有其他 Game 模式 Widget，输入不会自动切回 GameOnly，玩家控制锁死 |
| 修复 | Super 调用后立即 `PC->SetInputMode(FInputModeGameOnly())`；Deactivate 路径调用安全，不影响 Activate 时 CommonUI 建立的 Focus 路由 |

---

### [VFX-005] 热度升阶发光颜色统一走 BackpackStyleDA

**状态**：已完成已编译，需在 `B_PlayerOne` 中赋值 HeatStyleDA

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackStyleDataAsset.h`、`Character/PlayerCharacterBase.h/.cpp`、`Item/Weapon/WeaponInstance.cpp` |
| 新增 DA 字段 | `Phase1/2/3GlowColor`（分类"热度升阶发光颜色"）：HDR 线性空间，数值可 >1 增强发光 |
| 接入 | `PlayerCharacterBase` 新增 `HeatStyleDA`（EditDefaultsOnly）；玩家身体和武器 `OnHeatPhaseChanged` 均优先读 DA 颜色 |
| 配置入口 | `B_PlayerOne` Details → Heat\|Visual → `HeatStyleDA` 填 `DA_BackpackStyle` |

---

### [AI-002] BT 攻击前摇红光集成 + BTTask_PreAttackFlash + BTDecorator_HasAbilityWithTag

**状态**：已完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `AI/BTTask_ActivateAbilityByTag.h/.cpp`、`AI/BTTask_PreAttackFlash.h/.cpp`、`AI/BTDecorator_HasAbilityWithTag.h/.cpp` |
| bPreAttackFlash | `BTTask_ActivateAbilityByTag` 新增勾选项（默认 true）：技能激活时同步 `StartPreAttackFlash()`，技能结束（正常/打断）均调 `StopPreAttackFlash()`，完全同步于 GA 生命周期 |
| BTTask_PreAttackFlash | 独立前摇任务：`FlashDuration` 秒后返回 Succeeded，Abort 立即停止；用于不与 GA 绑定的前摇预警 |
| BTDecorator_HasAbilityWithTag | 装饰器：检查 ASC 是否有匹配 Tag 的可激活 GA；无则直接跳过节点，防止激活无蒙太奇 GA |

---

### [FIX-013] PendingGrid 不做 Zone Dimming + BackpackScreen 激活时同步 PreviewPhase

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/PendingGridWidget.cpp`、`UI/BackpackScreenWidget.cpp` |
| PendingGrid | 待放置区格子 `ZoneOpacity` 固定 `1.f`，不受 InactiveZoneOpacity 影响（待放区无激活区概念，全格等亮） |
| BackpackScreen | `NativeOnActivated` 末尾从 `BackpackGridComponent::GetCurrentPhase()` 同步 `PreviewPhase`，避免打开背包时热度阶段显示与实际不符 |

---

### [FIX-009] Tutorial Popup 幽灵输入修复 — bIsInteractable 防护

**状态**：已修复已编译，WBP 需手动配置双关键帧动画

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/GameDialogWidget.h/.cpp` |
| 根因 | FadeIn 动画在 WBP 中只配置了 t=0 单帧（opacity=0），动画瞬间完成，`bIsInteractable` 几乎未起效 |
| C++ 修复 | `NativeOnActivated` 播放 FadeIn 前设 `bIsInteractable=false`，`BindToAnimationFinished` 回调后置 true |
| WBP 修复 | Anim_FadeIn：t=0 opacity=0 → t=0.3s opacity=1；Anim_FadeOut 独立两帧（勿共享） |
| 接口 | `OnNextPressed()` 首行判 `if (!bIsInteractable) return;` |

---

### [FEAT-015] 武器拾取 HUD 动画系统（WeaponFloat → Trail → GlassIcon）

**状态**：C++ 全链路完成已编译；WBP 配置待完成（白屏 Bug 待修 — 背景需在 InfoContainer 内）

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/WeaponFloatWidget.h/.cpp`、`UI/WeaponTrailWidget.h/.cpp`、`UI/YogHUD.h/.cpp`、`Item/Weapon/WeaponSpawner.cpp` |
| 数据资产 | `UI/WeaponGlassAnimDA.h`（AutoCollapseDelay / CollapseDuration / ShrinkDuration / FlyDuration / GlassIconSize / HUDOffsetFromBottomLeft）|
| 动画三阶段 | Collapsing（InfoContainer 瞬间隐藏）→ Shrinking（Scale 1→TargetShrinkScale）→ Flying（Translation 至左下角图标）|
| 流光拖尾 | `WeaponTrailWidget`：全屏 Canvas 叠层，CanvasPanelSlot + RenderTransformAngle 实现旋转线段；材质参数 Alpha / Progress |
| 流光着色器 | `Shaders/WeaponTrail.ush` — 边缘衰减 + 尾部淡出 + 流光脉冲 + 头部白核 + Emissive；include `/Project/WeaponTrail.ush` |
| HUD 管理 | `TriggerWeaponPickup` → 定时器 → `TriggerWeaponCollapse` → 创建 Trail + 绑委托 → `OnFlyProgressUpdate` → `OnWeaponFlyComplete` 淡出 |
| 触发接入 | `WeaponSpawner::TryPickupWeapon` 末尾调 `HUD->TriggerWeaponPickup(WeaponDefinition)` |
| 广播委托 | `FOnWeaponFlyProgress`（非动态 Multicast）— Flying 首帧捕获绝对起点，每帧广播 (FlyAbsStart, CurrentPos, Alpha) |
| BP 配置 | BP_YogHUD：`TrailWidgetClass`=WBP_WeaponTrail；WBP_WeaponTrail：根全屏 Canvas + Image 命名 TrailLine；M_WeaponTrail：Custom Node include WeaponTrail.ush |
| 已知问题 | WBP_WeaponFloat 背景若在 InfoContainer 外，折叠后留白；修复：将背景移入 InfoContainer |

---

### [FEAT-016] 液态玻璃框 UI（GlassFrame）

**状态**：C++ + 材质 + WBP 完成；WeaponGlassIconWidget 继承可用

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/GlassFrameWidget.h/.cpp`、`Shaders/GlassFrameUI.ush`、`Shaders/GlassBlurMask.ush` |
| 材质资产 | `M_GlassFrame`（UI Domain，Custom Node include `/Project/GlassFrameUI.ush`）、`MI_GlassFrame` |
| WBP | `WBP_GlassFrame`（背包格背景用）、`WBP_WeaponGlassIcon`（武器落点图标）|
| 玻璃效果 | Apple 液态玻璃：极细 SDF 边缘线 + 顶部镜面高光（Fresnel）+ 底边炫彩（atan2 Hue + Time）；内部近透明，无重色边框 |
| 双层模糊 | `GlassBG`（低模糊，全区域）+ `GlassBGCenter`（高模糊，Apply Alpha to Blur=ON，M_GlassBlurMask 径向渐变遮罩） |
| C++ 参数 | `CornerRadius` / `BorderWidth` / `FresnelPower` / `IridIntensity` / `IridSpeed` / `BlurStrength`（边缘）/ `CenterBlurStrength`（中心） |
| 武器图标接口 | `ShowForWeapon(Thumbnail, DA)`、`StartExpandAndHide()`（打开背包前调用）|
| 配置入口 | WBP 需添加 `GlassBGCenter` BackgroundBlur；M_GlassBlurMask 用 Custom Node include `/Project/GlassBlurMask.ush` |

---

### [UI-017] 弹药计数器 HUD — WBP_AmmoCounter（火绳枪配套）

**状态**：C++ 已落地已编译，WBP_AmmoCounter 蓝图待创建

| 项目 | 内容 |
|------|------|
| 核心文件 | `WBP_AmmoCounter.h/.cpp` |
| 用途 | 火绳枪弹药数量图标式 HUD；横排显示 MaxAmmo 个图标，当前弹量金色，空仓灰色 |
| 数据来源 | 订阅 ASC 的 `CurrentAmmo` / `MaxAmmo` 属性变化，无需 BP 代码自动刷新 |
| WBP 配置 | Designer 放 HorizontalBox 命名 `BulletIconBox`；BP 子类无需任何蓝图逻辑 |
| 可调参数 | `IconSize`（22×22px）、`IconPadding`（4px）、`FilledColor`（金色）、`EmptyColor`（灰色） |

---

## 2026-04-20（续）

### [REFACTOR-018] PendingGrid 全面重构 — 改用 RuneSlotWidget 稀疏格子

**状态**：C++ 完成已编译；WBP 需重建（见配置）

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/PendingGridWidget.h/.cpp`、`UI/BackpackScreenWidget.h/.cpp` |
| 重构内容 | 旧版（UImage/Overlay 线性列表）→ 新版（RuneSlotWidget 稀疏格子，与主背包视觉一致）|
| 数据层 | `BackpackScreenWidget::PendingGrid`：`TArray<FRuneInstance>`，大小 = PendingGridCols × PendingGridRows |
| 同步 | `SyncPendingFromPlayer()`（打开时）/ `SyncPendingToPlayer()`（关闭/操作后）|
| 视觉层 | `PendingGridWidget::BuildSlots()` 动态创建 RuneSlotWidget；`RefreshSlots(Grid, CursorIdx, GrabbedIdx)` |
| 空格显示 | ZoneOpacity 固定 1.0（Pending 区无热度分区，不压暗空格）|
| WBP 配置 | WBP_PendingGrid：根节点 SizeBox（名 `PendingGridSizeBox`）→ UniformGridPanel（名 `PendingRuneGrid`）；Details 填 `RuneSlotClass=WBP_RuneSlot`、`StyleDA`、`PendingGridCols`、`PendingGridRows` |
| 尺寸同步 | NativeConstruct 从 PendingGridWidget 读取 Cols/Rows → 赋给 PendingCols/PendingRows，避免数据/视觉错位 |

---

### [FIX-019] 背包手柄导航恢复 + CloseButton + 战斗锁定

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackScreenWidget.h/.cpp`、`UI/RuneSlotWidget.h/.cpp`、`UI/BackpackGridWidget.h/.cpp` |
| 手柄 D-Pad | 左移至 col=0 → `bCursorInPendingArea=true`，右移至末列 → 返回主格子 col=0 |
| 手柄 A（FaceBottom）| 主格子：抓/放符文；Pending 区：`PendingGamepadConfirm` |
| 手柄 B（FaceRight）| 主格子：取消抓取；Pending 区：取消或退出回主格子；无抓取时关闭背包 |
| CloseButton | WBP 右上角 Button 命名 `CloseButton`，C++ 自动绑定 `DeactivateWidget()` |
| 战斗锁定 | `IsInCombatPhase()` 在 5 处操作点阻断（拖拽开始/落点/点格/手柄确认），弹 `OnStatusMessage("战斗阶段无法移动符文")` |
| SelectionBorder | RuneSlotWidget 新增 Image（命名 `SelectionBorder`），`SetSlotState(bSelected=true)` 时显示金黄边框 |
| ShakeAndFlash | RuneSlotWidget 新增 `ShakeAndFlash()`：红闪 + 阻尼正弦抖动（8·sin(30t)·e^{-8t}，0.4s）|

---

### [FEAT-020] 背包热度预览默认显示当前热度阶段

**状态**：C++ 完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/BackpackScreenWidget.cpp`（`NativeOnActivated`） |
| 改动 | 打开背包时 `PreviewPhase = Clamp(BackpackGridComponent::GetCurrentPhase(), 0, 2)` |
| 效果 | 背包打开后自动高亮当前热度阶段的激活区，无需玩家手动点 HeatPhaseDot |
| 关闭行为 | `NativeOnDeactivated` 仍重置为 -1（下次打开重新读取） |

---

### [UI-018] 三选一 UI C++ 化 + 战斗阶段符文锁定 ⚠️ 已回滚

**状态**：已实现后回滚——Slate 崩溃，根因待查

#### 实现内容（均已撤销，代码已还原 HEAD）

| 功能 | 方案 |
|------|------|
| 战斗阶段符文锁定 | `IsInCombatPhase()` 查 `YogGameMode::HasAliveEnemies()`；5 个交互点拦截并调 `FlashAndShakeCell` |
| 格子红闪+水平抖动 | `RuneSlotWidget::ShakeAndFlash()`：阻尼正弦 `X = 8·sin(30t)·e^{-8t}` 驱动偏移 |
| RuneInfoCard 卡片高亮 | `CardBG->SetColorAndOpacity(HighlightColor)`；手柄/悬停切换 |
| 三选一卡片 C++ 动态创建 | `HandleLootGenerated` 全移入 C++，用 `UHorizontalBox::AddChildToHorizontalBox` 平分空间 |

#### 崩溃信息

```
EXCEPTION_ACCESS_VIOLATION reading address 0xffffffffffffffff
Stack: UnrealEditor_SlateCore / UnrealEditor_Slate（无游戏代码帧）
```

推测 `BuildShapeGrid` 中 `NewObject<UImage>` 创建的子节点与 Slate 渲染生命周期冲突。

#### 下次优先排查

1. `BuildShapeGrid` 改为静态 UImage 数组（WBP Designer 预置，C++ 只刷新 brush），不在运行时 NewObject
2. 或改用自定义 `SLeafWidget` 绘制点阵，绕开 UMG 对象树
3. 三选一卡片高亮、战斗锁定两个功能独立实现，排除互相干扰

---

## 2026-04-19（第四次会话追加）

### [FEAT-013] 链路系统（Chain System）— BackpackGridComponent

**状态**：C++ 完成，已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `Component/BackpackGridComponent.h/.cpp`、`Data/RuneDataAsset.h` |
| 新增枚举 | `ERuneChainRole`（None/Producer/Consumer）、`EChainDirection`（8方向） |
| FRuneConfig 新增字段 | `ChainRole`、`ChainDirections`（Producer 专用） |
| Producer 逻辑 | 在激活区内时，按 ChainDirections 向相邻格传导激活（BFS 多跳） |
| Consumer 限制 | 系统层面阻止放入任何阶段激活区的格子（`ComputeAllPossibleActivationCells`） |
| 激活计算 | `RefreshAllActivations` 用 `EffectiveZone = DirectZone ∪ ChainZone` |
| 新私有方法 | `ComputeAllPossibleActivationCells`、`ComputeChainActivatedCells`、`ChainDirectionToOffset`、`IsRuneInZone` |

---

### [FEAT-014] 献祭恩赐系统（Sacrifice Grace）— 全局 Run Buff

**状态**：C++ 完成，已编译；需在编辑器创建配套 FA 和拾取物 BP

| 项目 | 内容 |
|------|------|
| 数据资产 | `Data/SacrificeGraceDA.h`（BaseDecayRate/DecayAccelPerSecond/MaxDecayRate/HPDrainPerSecond/BonusEffect/FlowAsset） |
| 衰退节点 | `BuffFlow/Nodes/BFNode_SacrificeDecay.h/.cpp`（In/Stop 引脚，1s 定时器，动态 GE 创建） |
| 玩家接口 | `PlayerCharacterBase::AcquireSacrificeGrace(DA)` — 满热度 + BonusEffect + 启动 FA |
| 关卡重入 | `BeginPlay` 中若 `ActiveSacrificeGrace` 非空自动重新激活（衰退速率重置，热度重充满） |
| 掉落逻辑 | `YogGameMode::EnterArrangementPhase()` 尾部：非主城关 15% 概率生成 `SacrificePickupClass` |
| 编辑器配置 | GameMode BP：`SacrificeGracePool`、`SacrificeDropChance`、`SacrificePickupClass` |
| 待完成 | 创建 FA（In→BFNode_SacrificeDecay）、创建拾取物 BP（显示 GameDialogWidget 接受/拒绝） |

---

## 2026-04-19（第三次会话）

### [VFX-003] 攻击前摇闪红 AnimNotifyState — ANS_PreAttackFlash

**状态**：C++ 完成，需在敌人攻击蒙太奇前摇帧配置

| 项目 | 内容 |
|------|------|
| 核心文件 | `Animation/ANS_PreAttackFlash.h/.cpp` |
| 驱动接口 | `AYogCharacterBase::StartPreAttackFlash()` / `StopPreAttackFlash()` |
| 适用范围 | 玩家和敌人通用（任何继承 `AYogCharacterBase` 的角色） |
| 使用方式 | 攻击蒙太奇前摇帧拖出 `ANS Pre Attack Flash` 区间，覆盖预警到命中帧 |
| 可调参数 | `PreAttackPulseFreq`（默认 4Hz）、`CharacterFlashMaterial`（敌人 BP Details 填入） |

---

### [FIX-008] ANS_AutoTarget 吸附死亡敌人 — IsAlive 过滤

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `Animation/ANS_AutoTarget.cpp` |
| 根因 | `FindBestTarget` 只检查 `IsValid()`（未销毁），不检查存活；`bContinuousTracking` 下目标已死但 CachedTarget 有效仍持续吸附 |
| 修复① | `FindBestTarget` 遍历时 Cast 到 `AYogCharacterBase`，`!IsAlive()` 则跳过 |
| 修复② | `NotifyTick` 中 CachedTarget 有效但 `!IsAlive()` 时触发重新搜索 |

---

### [COMBAT-007] GA_PlayerDash 台阶支持 + 诊断 CVar

**状态**：已完成已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `AbilitySystem/Abilities/GA_PlayerDash.cpp` |
| 台阶根因 | 球形扫描在角色当前 Z 做水平射线，台阶竖面 Block 导致冲刺提前停止 |
| 修复 | `GetFurthestValidDashDistance` 所有 Sweep 整体上移 `CMC->MaxStepHeight`（约 45cm），球从台阶竖面上方通过，CMC 根运动 Step-Up 正常生效 |
| 诊断 CVar | 控制台 `Dash.DebugTrace 1` 绘制扫描路径和命中信息；`0` 关闭，关闭时无任何性能开销 |
| 注意 | 点光源 `SphereVolume` 响应 DashTrace 会挡冲刺，需在对应 BP 中将该通道设为 Ignore |

---

## 2026-04-19（充能CD系统）

### [SKILL-001] 统一充能/CD 系统 — SkillChargeComponent

**状态**：C++ 已落地已编译，蓝图 GA 接入待配置

| 项目 | 内容 |
|------|------|
| 核心文件 | `SkillChargeComponent.h/.cpp`、`PlayerCharacterBase.cpp`、`PlayerAttributeSet.h/.cpp` |
| 设计 | 用后触发队列：每用 1 格启动独立 CDTimer，顺序回复；MaxCharge=1 即单次 CD，MaxCharge>1 即多格充能，同一组件处理 |
| GAS 属性 | `MaxDashCharge`（符文可加格数）/ `DashCooldownDuration`（符文可改 CD 速），运行时从属性读值 |
| 主要接口 | `InitWithASC()`、`RegisterSkill(Tag, MaxAttr, CDAttr)`、`HasCharge(Tag)`、`ConsumeCharge(Tag)` |
| 委托 | `OnChargeChanged(SkillTag, NewCharge)` 供 UI 刷新绑定 |
| 冲刺注册 | BeginPlay：`RegisterSkill("Ability.Dash", MaxDashCharge, DashCooldownDuration)` |
| 扩展新技能 | 只需在 PlayerCharacterBase.cpp 加一行 `RegisterSkill`，无需额外 C++ |

#### GA 蓝图接入模板

| 时机 | 节点 |
|------|------|
| CanActivateAbility | `GetSkillChargeComponent → HasCharge("Ability.Dash")` |
| ActivateAbility 开头 | `GetSkillChargeComponent → ConsumeCharge("Ability.Dash")` |

#### 符文修改示例

| 效果 | FA Apply Attribute Modifier 参数 |
|------|------|
| 冲刺 +1 格 | Attr=MaxDashCharge / Op=Additive / Val=1 / Infinite |
| CD 缩短 25% | Attr=DashCooldownDuration / Op=Multiplicative / Val=-0.25 / Infinite |

> FA 停止时 `BFNode_ApplyAttributeModifier.Cleanup()` 自动移除 GE，FA 内无需写 Stop 节点。

**配置前提**：ProjectSettings → GameplayTags 添加 `Ability.Dash`

设计文档：[充能系统指南](Design/Systems/SkillCharge_Guide.md)

---

## 2026-04-19（第二次会话追加）

### [FEAT-012] LevelFlow 系统完善 — LENode_Delay + OnClosed 回调 + 节点可见性隔离

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `LENode_Delay.h/.cpp`、`LENode_ShowTutorial.h/.cpp`、`LENode_Base.cpp`、`BFNode_Base.cpp`、`TutorialManager.h/.cpp` |
| LENode_Delay | 新建真实时间延迟节点（FTSTicker，不受 GlobalTimeDilation 影响）；`Duration` 秒后触发 Out |
| LENode_ShowTutorial | Out 改为 **OnClosed**：绑定 `TutorialManager::OnPopupClosed` 委托，玩家关闭弹窗后才触发下一节点；`ShowByEventID` 同步设置 `bPopupShowing=true` |
| TutorialManager | `NotifyPopupClosed()` 从 inline 改为 .cpp 实现，广播 `FSimpleMulticastDelegate OnPopupClosed` |
| 节点隔离 | `BFNode_Base` 构造函数设 `AllowedAssetClasses = {UNotifyFlowAsset}`；`LENode_Base` 设 `AllowedAssetClasses = {ULevelFlowAsset}`；两套节点在各自编辑器内互不可见 |

#### 使用方式（编辑器）

1. Content Browser 右键 → Data Asset → **Level Event Flow** → 命名 `DA_LevelEvent_XXX`
2. 双击打开 Flow 编辑器，右键只会出现 `LevelEvent` 分类节点（TimeDilation / Delay / Show Tutorial Popup）
3. 关卡放置 `BP_LevelEventTrigger`，Details → LevelFlow → 指定 DA，调整 Box 触发范围

---

### [FIX-009] WeaponSpawner 拾取黑化移除 + 浮窗拾取后隐藏

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponSpawner.h/.cpp` |
| 问题 | 拾取武器后 Spawner 网格变纯黑（BlackedOutMaterial），且浮窗永久消失，视觉上误以为无法再拾取 |
| 修复 | 删除 `BlackedOutMaterial`、`OriginalMeshMaterials`、`RestoreSpawnerMesh` 全部黑化逻辑 |
| 浮窗行为 | 加 `bPickedUp` 标志：拾取后浮窗永不再显示（Tick 中 `bPickedUp=true` 时直接 return）；Spawner 网格保持原始外观 |
| 再拾取 | 玩家离开区域再进入仍可按 E 换武器，不受限制 |

---

### [FIX-008] 武器浮窗消失 — WBP 引用 merge 后丢失 + bPopupShowing 安全网

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponSpawner.cpp`、`GameDialogWidget.cpp` |
| 根因 1 | merge 引入远端 commit（0c7c83f5）中 `BP_WeaponSpawner.uasset` 版本，该版本 `WeaponFloatWidgetClass` 为空（远端 WBP 已删）；本地 WBP 保留但 BP 引用丢失 |
| 修复 1 | `BeginPlay` 加路径兜底：`WeaponFloatWidgetClass` 为空时自动 `LoadClass<>("/Game/UI/Playtest_UI/WeaponInfo/WBP_WeaponFloat.WBP_WeaponFloat_C")` |
| 根因 2 | WBP 若 override `BP_OnPopupClosing` 只播动画但未调 `ConfirmClose()`，`bPopupShowing` 永不清除，Tick 每帧隐藏浮窗 |
| 修复 2 | `NativeOnDeactivated` 开头加 `TM->NotifyPopupClosed()`，Widget 无论通过何种路径关闭都能清除标志 |

---

## 2026-04-19（第三次会话追加）

### [UI-014] 三选一结束后自动打开背包

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `LootSelectionWidget.cpp`、`YogHUD.h/.cpp` |
| 触发时机 | `LootSelectionWidget::SelectRuneLoot` 在 `DeactivateWidget()` 之后调 `HUD->OpenBackpack()` |
| YogHUD 新增 | `BackpackScreenClass`（EditDefaultsOnly）、`BackpackWidget`（实例）、`OpenBackpack()` |
| 生命周期 | BackpackWidget 在 BeginPlay 创建 `AddToViewport(5)`；`OpenBackpack()` 调 `ActivateWidget()` |
| 配置入口 | HUD 蓝图 Details → **Backpack → Backpack Screen Class** = WBP_BackpackScreen |
| 注意 | 若背包 WBP 原先在其他 Blueprint 中创建，需移除，改由 YogHUD 统一管理避免实例重复 |

---

### [UI-013] 武器浮窗→玻璃图标流程框架 — WeaponGlass

**状态**：C++ 框架已落地已编译，WBP + DA 待编辑器配置

**核心文件**：`WeaponGlassAnimDA.h` / `WeaponGlassIconWidget.h/.cpp` / `WeaponFloatWidget.h/.cpp`（重构）/ `YogHUD.h/.cpp`（扩展）

#### 流程

```text
拾取武器 → HUD::TriggerWeaponPickup(Def) → WeaponFloat 展示
  → [AutoCollapseDelay] → Phase1 折叠(隐InfoContainer)
  → Phase2 缩小(Scale→GlassIconSize) → Phase3 飞行(Translation→左下角)
  → WeaponGlassIconWidget 常驻

打开背包 → HUD::NotifyBackpackOpening() → GlassIcon 放大+渐隐
```

#### YogHUD 新增接口

| 接口 | 说明 |
|------|------|
| `TriggerWeaponPickup(Def)` | 显示 WeaponFloat，启动 AutoCollapse 计时 |
| `TriggerWeaponCollapse()` | 手动跳过延迟立即折叠 |
| `NotifyBackpackOpening()` | GlassIcon 放大消失 |
| `GetWeaponGlassIconScreenCenter()` | 返回左下角锚点坐标 |
| `GetWeaponFloatWidget()` | 返回实例供 BP 调用 |

#### WeaponGlassAnimDA 参数速查

| 参数 | 默认 | 说明 |
|------|------|------|
| `AutoCollapseDelay` | 2.5s | 自动折叠延迟；0=手动 |
| `CollapseDuration` | 0.25s | Phase1 等待 |
| `ShrinkDuration` | 0.35s | Phase2 缩放 |
| `GlassIconSize` | 64×64px | 目标图标尺寸 |
| `FlyDuration` | 0.45s | Phase3 飞行 |
| `HUDOffsetFromBottomLeft` | (44,120)px | 图标距左下偏移 |
| `ExpandDuration` | 0.2s | 消失动画时长 |
| `ExpandScale` | 1.35 | 消失前最大放大倍率 |
| `ThumbnailFlyOpacity` | 0.45 | 飞行中缩略图透明度 |

#### WBP 配置

- **WBP_WeaponFloat**：缩略图命名 `WeaponThumbnail`；其余内容包在容器命名 `InfoContainer`
- **WBP_WeaponGlassIcon**（父类 WeaponGlassIconWidget）：Overlay 层 `GlassBG` / `GlassBorderImage`(M_GlassFrame) / `WeaponThumbnailImg` / `HeatColorOverlay`
- **HUD 蓝图**：填 `WeaponFloatClass`、`WeaponGlassIconClass`、`WeaponGlassAnimDA`

---

### [UI-012] 暂停弹窗屏幕变暗效果 — PauseEffect

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `YogHUD.h/.cpp`、`BackpackScreenWidget.cpp`、`LootSelectionWidget.cpp`、`GameDialogWidget.cpp` |
| 效果 | 所有暂停式弹窗激活时画面在 `PauseFadeDuration` 内渐变至低饱和+变暗，关闭后恢复 |
| 实现 | BeginPlay 生成 unbound `APostProcessVolume`；HUD Tick 设 `bTickEvenWhenPaused=true`；每帧插值 `ColorSaturation.W` / `ColorGain.W` |
| 计数器 | `PausePopupCount` 支持多弹窗叠加，全部关闭后才淡出 |
| 接入 | 各弹窗 `NativeOnActivated` → `BeginPauseEffect()`；`NativeOnDeactivated` → `EndPauseEffect()` |
| 配置入口 | HUD 蓝图 Details → PauseEffect |

| 参数 | 默认 | 说明 |
|------|------|------|
| `PauseFadeDuration` | 0.25s | 渐变时长 |
| `PauseTargetSaturation` | 0.10 | 目标饱和度（0=灰度） |
| `PauseTargetGain` | 0.40 | 目标亮度系数 |

---

## 2026-04-19（第四次会话追加）

### [VFX-004] 热度升阶玻璃边缘光 — 菲涅尔炫彩 + HLSL Include 体系

**状态**：C++ 已编译，材质需在编辑器更新 Custom 节点

| 项目 | 内容 |
|------|------|
| 核心文件 | `Shaders/GlassRim.ush`、`Shaders/PlayerGlowOverlay.ush`、`Shaders/WeaponGlowOverlay.ush`、`DevKit.h/.cpp`（FDevKitModule）、`PlayerCharacterBase.h/.cpp` |
| 效果 | 升阶时边缘菲涅尔区出现神秘炫彩；边缘 alpha 高于中心（更不透明）；炫彩随法线方向 + 时间缓慢流动 |
| 共享函数 | `GlassRim.ush`：`GR_HueToRGB` / `GR_Iridescence` / `GR_Fresnel`，供玩家和武器材质共用 |
| 模块注册 | `FDevKitModule::StartupModule()` 调 `AddShaderSourceDirectoryMapping("/Project", Shaders/)`，使 `.ush` 可跨电脑正常 include |
| 玩家参数 | `GlowIridIntensity`（默认 0.28）加到 `PlayerCharacterBase`，升阶时写入 DynMat `IridIntensity` |

#### 材质 Custom 节点更新方法（玩家 / 武器各做一次）

| 步骤 | 操作 |
|---|---|
| Include File Paths | 点 Custom 节点 → Details → Include File Paths → `+` → 填 `/Project/PlayerGlowOverlay.ush`（武器填 WeaponGlowOverlay.ush） |
| Code 字段 | `return PlayerGlowMain(UV, N, V, Time, EmissiveColor, SweepProgress, GlowAlpha, Power, BandWidth, SwipeCount, IridIntensity, IridSpeed);` |
| 新增 Inputs | Time（Time节点）、BandWidth（ScalarParam 默认 0.15）、IridIntensity（ScalarParam 默认 0.28）、IridSpeed（ScalarParam 默认 0.06） |

---

### [UI-015] 液态玻璃框 — GlassFrameWidget

**状态**：C++ 已落地已编译，材质 M_GlassFrame + WBP_GlassFrame 待编辑器创建

| 项目 | 内容 |
|------|------|
| 核心文件 | `GlassFrameWidget.h/.cpp` |
| 设计意向 | Apple 液态玻璃：中心毛玻璃模糊 + 边框 SDF 折射 + 角落炫彩 |
| 背景模糊 | `UBackgroundBlur`（命名 `GlassBG`）实时采样游戏帧，BlurStrength 控制强度 |
| 边框材质 | Custom HLSL：圆角 SDF + UV 菲涅尔 + Hue 炫彩，5 个 Scalar Parameter |
| 主要接口 | `ApplyGlassStyle()` — 批量写入所有材质参数；`GetGlassDynMat()` — 供 BP 扩展 |
| BindWidget | `GlassBG`（BackgroundBlur）、`GlassBorderImage`（Image），均为 Optional |
| 内容挂载 | NamedSlot `Content`，背包格子/武器图标/HUD缩略图放此处 |

#### 各场景参数速查

| 场景 | BlurStrength | CornerRadius | IridIntensity |
|---|---|---|---|
| 背包主界面 | 14 | 0.06 | 0.18 |
| 武器图案框 | 10 | 0.08 | 0.12 |
| HUD 缩略框 | 6 | 0.10 | 0.08 |

#### 编辑器剩余任务

1. 新建 Material `M_GlassFrame`（Domain=UI，Blend=Translucent）→ Custom 节点粘入 HLSL，添加 5 个 ScalarParam
2. 新建 `WBP_GlassFrame`，父类选 `GlassFrameWidget`，搭建三层控件（GlassBG / GlassBorderImage / Content）
3. Details → GlassBorderMaterial 填 `M_GlassFrame`
4. 在背包/武器/HUD 对应 WBP 中替换为 `WBP_GlassFrame` 作为容器

#### 技术参考

详见 [液态玻璃框技术文档](Docs/Design/UI/GlassFrame_Technical.md)

---

### [FIX-010] BTTask_ActivateAbilityByTag.h 缺失 — 补全头文件

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `Source/DevKit/Public/AI/BTTask_ActivateAbilityByTag.h`（新建） |
| 根因 | `.cpp` 实现存在但 `.h` 头文件丢失，导致整个模块 fatal error 无法编译 |
| 修复 | 从 `.cpp` 反推类声明，补全头文件；注意 `FActivateAbilityMemory` 结构体需 forward declare `UAbilitySystemComponent` 才能实例化模板 |

---

### [FIX-011] WeaponGlassIconWidget 编译错误 — SetRenderTransformScale 不存在

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponGlassIconWidget.cpp` |
| 根因 | `UWidget::SetRenderTransformScale(FVector2D)` 在 UE5.4 不存在 |
| 修复 | 替换为 `FWidgetTransform T; T.Scale = ...; SetRenderTransform(T);` |

---

### [FIX-007] CommonUI 输入模式残留 — 所有 UCommonActivatableWidget 手动还原 InputMode

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `GameDialogWidget.cpp`、`BackpackScreenWidget.cpp` |
| 根因 | 三个菜单 Widget 均用 `AddToViewport` 而非 CommonUI Stack；`Super::NativeOnDeactivated()` 无 Stack 时无法自动还原 `ECommonInputMode`，关闭后操控被 block（表现为"长按鼠标左键才能控制"） |
| BackpackScreenWidget | `NativeOnDeactivated` 末尾补 `PC->SetInputMode(FInputModeGameOnly())` |
| TutorialPopupWidget | `NativeOnActivated` 明确调 `SetShowMouseCursor(true)` + `SetInputMode(UIOnly)`；`NativeOnDeactivated` 把还原逻辑（`SetShowMouseCursor(false)` + `SetInputMode(GameOnly)` + `SetGamePaused(false)`）移到 `Super::` **之前**，防止 Super 的 CommonUI 回调覆盖 |
| LootSelectionWidget | 原本已正确实现，无需改动 |

---

### [FIX-006] Tutorial 弹窗"知道了"无法关闭 + 武器教程不再开背包

**状态**：已修复已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `GameDialogWidget.h/.cpp`、`TutorialManager.cpp` |
| 根因 | `BP_OnPopupClosing` 为 `BlueprintImplementableEvent`（纯 BP 虚函数），WBP 未 override 时调用完全无效 |
| 修复 | 改为 `BlueprintNativeEvent`，添加 `BP_OnPopupClosing_Implementation()` 默认调 `ConfirmClose()` → `DeactivateWidget()` |
| 武器教程 | 删除 `DoShowWeaponPopup` 中的 `WeakPC->OpenBackpack()` 调用，武器教程不再强制打开背包 |

---

### [UI-011] Tutorial 弹窗触发时机重构 + 时间膨胀 + 符文浮窗 + LevelFlow

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponSpawner.cpp`、`TutorialManager.h/.cpp`、`RewardPickup.h/.cpp`、`RuneRewardFloatWidget.h/.cpp`、`LevelFlowAsset.h`、`LevelEventTrigger.h/.cpp`、`LENode_*.h/.cpp` |
| 触发时机 | 由拾取后改为**进入 WeaponSpawner 范围时**（OnOverlapBegin）立即触发；浮窗由 `IsPopupShowing()` 保护，弹窗期间不出现 |
| 时间膨胀 | `TryWeaponTutorial`：先 `GlobalTimeDilation(0.08f)`，用 `FTSTicker`（真实时间）等 0.35s，再恢复并显示弹窗 |
| 渐出钩子 | `BP_OnPopupShown` / `BP_OnPopupClosing`（NativeEvent）/ `ConfirmClose()`：WBP 可 override 播动画，动画结束调 `ConfirmClose()` |
| 符文浮窗 | `ARewardPickup` + `URuneRewardFloatWidget`：动态显示可选符文列表（图标+名称），同 WeaponSpawner 屏幕侧向偏移 |
| LevelFlow | `ULevelFlowAsset`（独立于 BuffFlow）、`ALevelEventTrigger`（Box 触发器）、`LENode_TimeDilation`、`LENode_ShowTutorial` |
| HUD 清理 | 删除 `YogHUD` 对已废弃 `EnemyArrowWidget`（2026-04-19 前一 commit 已删源文件）的残留引用 |

---

## 2026-04-19

### [UI-010] 背包格子默认半透明 — InactiveZoneOpacity

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `BackpackStyleDataAsset.h`、`BackpackGridWidget.cpp`、`RuneSlotWidget.h/.cpp` |
| 效果 | 未放置符文的空格默认以 `InactiveZoneOpacity` 半透明显示；热度检视模式下非聚焦区同样使用此值 |
| 配置入口 | `DA_BackpackStyle` → 激活区特效 → **Inactive Zone Opacity**（默认 0.35） |
| 逻辑 | `RefreshCells` 计算每格 `ZoneOpacity`：PreviewPhase=-1 时空格降至 DA 值；PreviewPhase≥0 时聚焦区全亮其余降至 DA 值 |
| 实现方式 | `ZoneOpacity` 作为参数传入 `SetSlotState`，乘以 `BGColor.A`；有符文格子不受影响始终全亮 |

---

### [UI-009] 敌人方向箭头指示 — EnemyArrowWidget

**状态**：已实现已编译，WBP_EnemyArrow 蓝图已配置

| 项目 | 内容 |
|------|------|
| 核心文件 | `EnemyArrowWidget.h/.cpp`、`YogGameMode.h/.cpp`（`GetAllAliveEnemies`）、`YogHUD.h/.cpp` |
| 触发条件 | 同时满足：① 所有存活敌人均不在屏幕内 ② 玩家 `AppearDelay` 秒内未受伤 |
| 显示内容 | 最多 `MaxArrows` 个（默认 3）三角箭头，贴屏幕边缘，指向距离最近的敌人 |
| 后方敌人 | `IsOnScreen` 对摄像机后方敌人做坐标镜像，箭头出现在玩家"背向"的屏幕边缘 |
| 投影点修正 | `ArrowProjectionZOffset`（默认 60cm）将投影点从脚底抬至腰部，修正斜视角偏差 |
| 离屏判断双保险 | `OnScreenShrink`（屏幕内缩像素）+ `ForceOffScreenDistance`（世界距离阈值，默认 0=不启用） |
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
| `ForceOffScreenDistance` | 0cm | 超距强制离屏（0=不启用） |
| `ArrowProjectionZOffset` | 60cm | 投影点高度修正 |
| `ArrowAngleOffset` | 90° | 贴图朝向补偿（顶点朝上=90） |
| `ArrowColor` | 黄色(1,0.8,0.2,0.9) | 箭头颜色 |

---

### [UI-008] 武器拾取浮窗 — WBP_WeaponFloat

**状态**：已实现已编译

| 项目 | 内容 |
|------|------|
| 核心文件 | `WeaponInfoDA.h`、`WeaponFloatWidget.h/.cpp`、`WeaponSpawner.h/.cpp`、`WeaponDefinition.h` |
| 触发时机 | 玩家进入 WeaponSpawner CollisionVolume 且朝向武器（±105° 宽松判断） |
| 信息来源 | `WeaponDefinition` → `WeaponInfo`（UWeaponInfoDA）和 `InitialRunes`（TArray<URuneDataAsset>） |
| WeaponInfoDA 字段 | `WeaponName`、`WeaponDescription`、`WeaponSubDescription`、`Thumbnail`、`Zone1/2/3Image` |
| 点阵激活区 | 三个 CanvasPanel（ZoneGrid1/2/3），根据 `ZoneGridSize` 属性自动缩放，填满 SizeBox |
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

### [MAP-002] 关卡结束揭幕特效 — LevelEndReveal + YogHUD

**状态**：C++ 完整；WBP_LevelEndReveal + M_LevelEndReveal 需在编辑器配置材质参数

| 项目 | 内容 |
|------|------|
| 核心文件 | `UI/LevelEndRevealWidget.h/.cpp`、`Data/LevelEndEffectDA.h/.cpp`、`UI/YogHUD.h/.cpp` |
| 触发接口 | `YogHUD::TriggerLevelEndEffect(FVector LootWorldPos)` — 由 GameMode/LevelFlow 在关卡结束时调用 |
| 揭幕流程 | 延迟（DA 配置）→ 创建 WBP_LevelEndReveal → `InitReveal(Mat, LootScreenUV, EdgeSharpness)` → 每帧 Tick 驱动材质参数 `RevealProgress` |
| 数据资产 | `DA_LevelEndEffect`：Delay / RevealDuration / EdgeSharpness / RevealMaterial |
| WBP 配置 | `WBP_LevelEndReveal`：根 Image 命名 `RevealImage`，HAlign/VAlign = Fill，填入 `M_LevelEndReveal` |
| 参数传递 | `LootWorldPos` → `UGameplayStatics::ProjectWorldToScreen` 转 UV → 传入材质，圆心跟随掉落物位置 |
| HUD 配置 | `BP_YogHUD` Details → `LevelEndEffect` → 填 `DA_LevelEndEffect` 和 `LevelEndRevealWidgetClass` |

---

### [MAP-003] 关卡开场镜头标记 — LevelIntroCameraMarker

**状态**：C++ 完整；需在关卡中放置 Actor 并在 LevelFlow 调用 TriggerIntro

| 项目 | 内容 |
|------|------|
| 核心文件 | `Map/LevelIntroCameraMarker.h/.cpp` |
| 用途 | 关卡加载后切到标记处视角，停留后平滑移回玩家，制造开场镜头感 |
| 调用方式 | 从 LevelFlow 节点或 BeginPlay 调用 `ALevelIntroCameraMarker::TriggerIntro()` |
| 可配参数 | `HoldDuration`（停留时长，默认 2s）/ `MoveDuration`（移回玩家时长，默认 1.5s）/ `bDisableInputDuringIntro` |
| 镜头方向 | 通过 Actor 上的 `CameraComp` 组件在编辑器中调整朝向 |
| 已知限制 | 移回玩家目前用线性插值，无缓动曲线 |

---

### [FIX-010] 敌人攻击 BT Task 自动过滤无蒙太奇 Tag

**状态**：完整，编译后生效，BT 无需改动

| 项目 | 内容 |
|------|------|
| 核心文件 | `AI/BTTask_ActivateAbilityByTag.cpp` |
| 根因 | 所有 ATK GA 都 Grant 给敌人，但 `DA_AbilityMontage_*` 中部分 Tag 蒙太奇为 None，GA 激活后无法播放动画 |
| 修复逻辑 | `ExecuteTask` 在调用 `TryActivateRandomAbilitiesByTag` 前，先用 `AbilityData->HasAbility(Tag)` 过滤掉蒙太奇为 None 的 Tag，只把有效 Tag 传入随机激活 |
| 过滤链 | `Pawn → CharacterDataComponent → CharacterData → AbilityData → HasAbility(Tag)` |
| 无效 Tag 处理 | `ValidTags` 为空时直接返回 `Failed`，BT 正常处理失败分支，敌人不会卡住 |
| 配置入口 | 每个敌人的 `DA_AbilityMontage_*`：填入蒙太奇 = 启用该攻击；留 None = 自动跳过 |
| 已知限制 | 依赖 `CharacterDataComponent` 存在，纯蓝图敌人不走此路径 |

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
