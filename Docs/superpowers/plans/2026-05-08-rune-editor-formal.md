# Rune Editor Formal Plan - 512 MVP

日期：2026-05-08
分支：`codex/rune-editor-formal-plan`

## 目标

制作一个正式版的 UE 符文编辑器。512 版本先服务当前已经存在的符文运行能力：`URuneDataAsset` 负责数据，`UFlowAsset` / BuffFlow 负责逻辑链条，GAS 的 `FActiveGameplayEffectHandle` 负责运行生命周期。

策划在编辑器里只看到符文名称、分类、触发、持续策略、逻辑节点、变量节点和校验结果；GE、GA、ASC、GCN、ActiveGE Handle 等细节作为运行层实现隐藏起来。

## 512 MVP 范围

本阶段优先做 512 已经需要、且能和现有工程直接对齐的能力：

- 左侧：符文列表、搜索、类型/稀有度筛选。
- 中间：选中符文的生命周期摘要、Blueprint Style Flow Graph、512 节点目录、基础连线操作。
- 右侧：基础数据、触发与生命周期、Flow 绑定、节点属性检查器、校验结果。
- 节点拆分：按现有 `Source/DevKit/Public/BuffFlow/Nodes` 节点整理成策划视角的分类。
- 变量系统：先采用已有 pure/value 节点，例如 `GetRuneInfo`、`GetRuneTuningValue`、`GetAttribute`、`Literals`、`MathFloat`、`MathInt`、`CalcDamage`。
- 背包形状：保留为兼容数据和高级区，不进入 512 主流程，不影响符文运行、校验和节点编辑。

## 节点分类

512 版本的编辑器节点目录按现有 BuffFlow 节点拆分：

| 策划分类 | 典型节点 | 用途 |
| --- | --- | --- |
| Trigger | `OnDamageDealt`、`OnDash`、`OnKill`、`OnCritHit`、`OnDamageReceived`、`OnPeriodic`、`WaitGameplayEvent` | 监听运行事件，驱动符文链条开始或继续 |
| Condition | `IfStatement`、`HasTag`、`CheckTargetType`、`CheckDistance`、`Probability`、`CompareFloat`、`CompareInt`、`DoOnce` | 分支、过滤、概率和一次性触发 |
| Variable | `GetRuneInfo`、`GetRuneTuningValue`、`GetAttribute`、`Literals`、`MathFloat`、`MathInt`、`CalcDamage` | pure/value 读数，减少不必要的数据引脚 |
| Effect | `ApplyRuneEffectProfile`、`ApplyEffect`、`ApplyExecution`、`ApplyGEInRadius`、`ApplyAttributeModifier`、`DoDamage`、`AreaDamage`、`GrantGA`、`AddTag`、`RemoveTag`、`SendGameplayEvent` | 真正改变战斗状态的动作 |
| Spawn | `SpawnRuneProjectileProfile`、`SpawnRuneAreaProfile`、`SpawnRuneGroundPathEffect`、`SpawnRangedProjectiles`、`SpawnActorAtLocation` | 生成投射物、范围、路径和辅助 Actor |
| Visual | `PlayRuneVFXProfile`、`PlayNiagara`、`PlayFlipbookVFX`、`SpawnGameplayCueAtLocation`、`SpawnGameplayCueOnActor`、`DestroyNiagara`、`PlayMontage` | 表现层和可清理视觉资源 |
| Lifecycle | `FinishBuff`、`Delay`、`TrackMovement`、`IncrementPhase`、`DecrementPhase`、`PhaseDecayTimer` | 延迟、结束、阶段和运行过程控制 |
| Advanced | `DisableBackpackCells` 等背包/兼容/实验节点 | 保留兼容，不作为 512 主工作流必选节点 |

## 运行逻辑决策

512 版本可以沿用 GAS 的思路：

1. 符文被激活时，运行层应用一个 Rune Runtime GE 或 Rune GE spec。
2. GE spec 携带 RuneIdTag、RuneGuid、SourceRune、SetByCaller 数值和上下文。
3. ActiveGE 生效后启动对应的 BuffFlow。
4. Flow 节点可以继续授予 GA、播放 GameplayCue、生成投射物/区域、添加 Tag。
5. ActiveGE 被移除、到期、驱散或手动清理时，运行层停止 Flow，并清理本次符文拥有的 GA、Cue、Tag、Actor、Niagara 等资源。

注意：这里说的 “GE 被移除” 是 `FActiveGameplayEffect` 运行实例结束，不是 `UGameplayEffect` 资源被 GC。

## 编辑器变量策略

变量系统参考蓝图变量和 pure 节点，但 512 先不做完整自定义变量图系统。

512 的第一阶段做法：

- 复用已有 pure/value 节点读取常用上下文。
- 把 `RuneInfo`、`RuneTuningValue`、Owner/Target Attribute、Stack、Card Context、Table Value 作为“变量节点货架”呈现。
- 节点目录中标记哪些节点是 pure/value，哪些节点会产生运行副作用。

后续正式版增强：

- 在 `URuneDataAsset` 或 Rune Editor 中增加命名变量定义。
- 自动生成对应的 pure read 节点或 FlowGraph 菜单项。
- 校验变量引用是否存在、类型是否一致、默认值是否配置。

## 开发阶段

### Phase 0：计划与预览

- [x] 建立正式开发分支 `codex/rune-editor-formal-plan`。
- [x] 建立网页预览 `Docs/Prototypes/RuneEditorPreview/index.html`。
- [x] 将网页预览收敛到 512 MVP，背包形状移入兼容区。
- [x] 将计划收敛到 512 MVP，不把背包形状作为主线需求。

### Phase 1：512 节点目录

- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/RuneEditorNodeCatalog.h`。
- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/RuneEditorNodeCatalog.cpp`。
- [x] 从 `UBFNode_Base` 的子类反射构建节点目录。
- [x] 将现有 BuffFlow 原始 Category 映射到 512 策划分类。
- [x] `Disable Backpack Cells` 等兼容节点标记为 Advanced，不影响 512 主流程。
- [x] 新增 `Tools/RuneEditor/run_smoke_test.ps1`，防止节点目录和 UI 入口缺失。

### Phase 2：Rune Editor 窗口骨架

- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h`。
- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp`。
- [x] 左侧显示 RuneDA 列表、搜索与刷新。
- [x] 中间显示 512 节点目录和选中符文的 Flow 绑定摘要。
- [x] 右侧显示选中符文基础信息、触发信息、FlowAsset、512 兼容状态。
- [x] 背包形状只显示为 Advanced/Compatibility 状态，不参与主工作流判断。

### Phase 3：原生创建与使用

- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/RuneEditorAuthoring.h`。
- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/RuneEditorAuthoring.cpp`。
- [x] 在 Rune Editor 中提供 `New Rune`，直接创建新的 `URuneDataAsset`。
- [x] `New Rune` 自动创建并绑定新的 `UFlowAsset`，不依赖旧资产。
- [x] 自动写入或复用 `Rune.ID.*` GameplayTag。
- [x] 在 Details 中提供 `Save Basic Info`，可直接修改 RuneName、RuneIdTag、HUD Summary。
- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/RuneEditorFlowAuthoring.h`。
- [x] 新增 `Source/DevKitEditor/Private/RuneEditor/RuneEditorFlowAuthoring.cpp`。
- [x] 在 Rune Editor 中显示当前 FlowAsset 的 Flow 节点列表。
- [x] 将中间主编辑区替换为内嵌 `SGraphEditor`，直接编辑当前 `FlowAsset->GetGraph()`。
- [x] 图编辑区支持 FlowGraph 原生节点显示、拖线、右键节点菜单、选择和图操作。
- [x] 从 512 节点目录选择节点后，可通过 `Add Node After Selection` 创建节点并接入选中节点之后。
- [x] 通过 `Relink Selected To Entry` 将 Entry 输出重新连接到当前选中节点，用于快速修链。
- [x] 右侧 `Node Inspector` 显示选中 Flow 节点的属性，可直接编辑节点字段。
- [x] 保留 `Open Flow`，复杂图编辑仍可进入原生 FlowGraph。
- [x] 提供 `Run Rune`，在 PIE/有效 World 中对选中 Actor 的 `UBuffFlowComponent` 启动该符文 Flow。

### Phase 4：编辑器菜单入口

- [x] 在 `Source/DevKitEditor/DevKitEditor.cpp` 注册 `DevKitRuneEditor` Nomad Tab。
- [x] 在 `Tools > DevKit Data` 下增加 `Rune Editor`。
- [x] 保留 `Rune Balance` 作为批量数值编辑入口。

### Phase 5：512 校验

- [ ] 复用已有 RuneDA 校验规则。
- [ ] 增加单符文校验：RuneIdTag、RuneName、TriggerType、FlowAsset、TuningScalars。
- [ ] 512 主校验不要求 Shape 必填。
- [ ] Advanced 校验单独提示 Shape/Backpack 兼容信息，不阻断 512 符文使用。

### Phase 6：GAS 生命周期桥

- [ ] 新增运行层 Rune lifecycle component/helper。
- [ ] ActiveGE 生效时启动 BuffFlow。
- [ ] ActiveGE 移除时停止 BuffFlow 并清理本次符文拥有的资源。
- [ ] 梳理 GA、GameplayCue、spawn actor、Niagara 的 owner/cleanup 记录。

这个阶段改动运行逻辑，必须单独做编译和 PIE/命令行回归；不和编辑器 UI 骨架混在同一个风险切片里。

## 当前制作切片

本次先完成：

1. [x] 更新网页预览为 512 MVP。
2. [x] 添加 512 节点目录。
3. [x] 添加 Rune Editor 窗口骨架。
4. [x] 注册 `Tools > DevKit Data > Rune Editor`。
5. [x] 运行 `Tools/RuneEditor/run_smoke_test.ps1`。
6. [x] 增加 `New Rune` / `Save Basic Info` / `Run Rune`。
7. [x] 增加 Flow 节点列表、512 节点目录选择、节点属性检查器、添加节点和 Entry 重连。
8. [x] 编译 `DevKitEditor` 通过。

## 验收标准

- 网页预览明确显示 `512 MVP`，背包形状位于兼容/高级区。
- UE 菜单出现 `Rune Editor`。
- Rune Editor 能展示 RuneDA 数量、选中符文摘要和 512 节点分类。
- Rune Editor 能直接创建新的 RuneDA 和 FlowAsset。
- Rune Editor 能保存基础符文信息。
- Rune Editor 能在中间区域直接显示 Blueprint 风格 FlowGraph 画布。
- Rune Editor 能在图画布中选择节点，并在右侧编辑节点字段。
- Rune Editor 能从 512 节点目录添加节点并接入基础执行线。
- Rune Editor 能快速将 Entry 重新连接到选中节点。
- Rune Editor 能打开 FlowGraph 进行复杂节点添加和连线。
- Rune Editor 能在 PIE/有效测试 Actor 上启动符文 Flow。
- 节点目录能识别 Trigger、Condition、Variable、Effect、Spawn、Visual、Lifecycle、Advanced。
- `Disable Backpack Cells` 被归入 Advanced/Compatibility。
- 烟测通过。
- 编译通过，或明确记录无法编译的环境原因。
