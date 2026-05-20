## Codex 代码审查

### 代码问题
- `Source/DevKit/Public/World/HubFacilityActor.h:32-33`：`RequiredFeatureTag` 使用 `EditDefaultsOnly`，只能在类默认值/BP 默认值上配置，无法对同一个 HubFacilityActor 的不同关卡实例配置不同功能门槛。若设施是地图实例级配置，建议改为 `EditAnywhere`。
- `Source/DevKit/Private/UI/ActiveSkillBarWidget.cpp:71-79` 与 `Source/DevKit/Private/UI/ActiveSkillLoadoutWidget.cpp:65-73`：C++ 直接覆盖 `WidgetTree->RootWidget`，会绕开/覆盖 WBP 设计器里的布局。后续如果用蓝图子类做美术 UI，NativeConstruct 会替换根节点，容易导致蓝图布局失效。
- `Source/DevKitEditor/UI/ActiveSkillSetupCommandlet.cpp:140`：用 `Params.Contains("Apply")` 判断写入模式过于宽松，`-NoApply`、`-DryRunApplyCheck` 这类参数也会触发真实写入。建议改用明确参数解析，如 `FParse::Param(*Params, TEXT("Apply"))`。
- `Source/DevKit/Private/UI/ActiveSkillLoadoutWidget.cpp:19`、`Source/DevKitEditor/UI/ActiveSkillSetupCommandlet.cpp:24-27`：资产路径硬编码在 C++ 中，后续移动资产会造成运行时软加载/Commandlet 失效。建议集中到 settings/registry 或 PrimaryAsset 配置。

### 潜在 Bug
- `Source/DevKit/Private/World/HubFacilityActor.cpp:63-70`：`ApplyFeatureAvailability()` 只在 `BeginPlay()` 调用。`UYogMetaProgressionSubsystem` 已有 `OnFeatureUnlocked` 广播，但设施没有订阅；玩家在 Hub 内解锁 `Feature.Combat.ActiveSkill` 后，终端可能仍隐藏且无碰撞，必须重进地图才出现。
- `Source/DevKit/Private/UI/ActiveSkillBarWidget.cpp:184-191`：当某槽位曾经有 Icon，之后切到空槽/无 Icon 时，只改了 `ColorAndOpacity`，没有清空 `Image` 的 Brush，旧图标可能以深色残留显示。
- `Source/DevKitEditor/UI/ActiveSkillSetupCommandlet.cpp:241`、`258`：`NodeTable->AddRow()` 每次 Apply 都直接覆盖同名行。如果策划已在 `DT_MetaUpgradeNodes` 中手动调整过显示名、成本、前置、坐标等，重新运行 Commandlet 会静默覆盖这些配置。
- `Source/DevKit/Private/UI/ActiveSkillLoadoutWidget.cpp:53-62`：Loadout 关闭时无条件切回 `GameOnly` 并隐藏鼠标，没有检查是否还有其他 CommonUI/Menu 处于激活状态；叠加 UI 场景下可能破坏输入模式。
- `Source/DevKit/Private/UI/YogHUD.cpp:1379-1385`：`OnPawnPossessed` 只有 `NewPawn` 非空时才调用 `BindActiveSkillWidget`，控制器临时 UnPossess 时 ActiveSkillBar 仍可能绑定旧 Pawn 的组件并显示旧数据。
- `Source/DevKit/Private/Component/PlayerActiveSkillComponent.cpp:39-58`：主动技能槽解锁数只在组件 BeginPlay 初始化。若功能在当前 Hub 会话内解锁，Loadout 的按钮状态仍可能来自旧的 `UnlockedSlotCount`，需要配合 `OnFeatureUnlocked` 动态刷新。

### 性能隐患
- `Source/DevKit/Private/Component/PlayerActiveSkillComponent.cpp:19-35` 触发 `BroadcastSlotsChanged()` 后，`Source/DevKit/Private/UI/ActiveSkillBarWidget.cpp:232-235` 每帧刷新 UI 并触发蓝图事件。当前只有 2 个槽问题不大，但冷却期间每帧广播完整 slot view 和 BP event，后续槽位/蓝图逻辑增加后会放大开销。
- `Source/DevKit/Private/UI/ActiveSkillLoadoutWidget.cpp:196`：`DefaultSkillAsset.LoadSynchronous()` 在点击装备时同步加载资产，首次打开/点击可能卡顿。建议在构造/激活前预加载，或改为异步加载后启用按钮。

### 改进建议
- Hub 设施应订阅 `UYogMetaProgressionSubsystem::OnFeatureUnlocked`，匹配 `RequiredFeatureTag` 后重新执行 `ApplyFeatureAvailability()`；主动技能组件也应在同一事件中调用 `SetUnlockedSlotCount()`。
- ActiveSkillBar 的图标刷新应在无 Icon 时清空 Brush，并显式设置空槽占位 Brush/尺寸，避免旧图标残留和布局塌缩。
- Commandlet 写 DataTable 前先读取旧行，按需增量更新，或在报告中明确标出“将覆盖已有行”；保存前检查 `SavePackages` 返回值并把失败写入报告。
- Loadout 窗口建议通过 `UYogUIManagerSubsystem` 推入/弹出，而不是手动 `AddToViewport` + 手动输入模式切换，这样能复用已有层级、焦点和鼠标策略。
- UI 文案目前大量 `FText::FromString`，正式内容建议改为 `LOCTEXT/NSLOCTEXT`，方便本地化和统一文本管理。

### 需要手动配置的引擎资产
- `/Game/MetaProgression/DT_MetaUpgradeNodes`：Commandlet 只更新已存在的表，缺失时不会创建。
- `/Game/World/Hub/L_HubTown`：Commandlet 只加载并放置终端，地图缺失时不会创建。
- `/Game/Code/Core/ActiveSkills/DA_ActiveSkill_ShieldBurst`：Commandlet 可创建并设置基础技能字段，但 Icon/展示资源仍需在编辑器中补全。
- `/Game/Code/Core/Hub/BP_HubActiveSkillTerminal`：Commandlet 可创建并设置默认 Widget/FeatureTag；若不运行 Apply，需要手动创建和配置。