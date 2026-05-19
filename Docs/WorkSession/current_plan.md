# 开发方案：主城 + 局外成长

## 需求描述

- 局外成长核心机制：解锁玩家永久技能卡牌（类似黑帝斯2大阿卡纳），花费局外货币购买升级节点，节点效果包括解锁一张起始符文（始终随玩家携带进局）
- 主城场景：做完整 3D 主城关卡（非菜单），玩家在主城内与设施交互触发局外功能
- 运行时升级树 UI + 编辑器升级树工具同步开发

---

## 现状速查

### 已完成可直接用
| 系统 | 状态 |
|------|------|
| `UYogMetaProgressionSubsystem` | 货币增减、节点购买、功能解锁 API 完整 |
| `FMetaProgressionData` 存档字段 | 序列化/反序列化正常 |
| `GrantCraftedStarterRunesAsync` | 每局 BeginPlay 时将 CraftedStarterRunes 授予为隐藏被动 |
| `AddCraftedStarterRune(FPrimaryAssetId)` | MetaProgressionSubsystem 接口已有 |
| `SMetaProgressionWorkbenchWidget` | 编辑器列表视图已有，需扩展为树形图 |

### 缺失
- `EMetaUpgradeEffectType::RuneGrant` — 节点购买后自动授予起始符文的效果类型
- 运行时升级树 UI Widget
- 主城设施框架（Hub Facility Actor）
- 局外资源结算页
- 资源房 → 货币联通

---

## 方案设计

### A. MetaTypes 扩展：RuneGrant 效果类型

在 `EMetaUpgradeEffectType` 中增加 `RuneGrant`：购买该节点时将指定 RuneDA 加入 `CraftedStarterRunes`，每局 BeginPlay 时自动授予为隐藏被动（现有 GrantCraftedStarterRunesAsync 管道已打通）。

节点可为多级（MaxLevel > 1），每升一级授予一张符文（同一 DA 可以加多次形成叠加效果，或配不同 DA 形成进化链）。

### B. 运行时升级树 Widget（主城内访问）

`UYogMetaUpgradeTreeWidgetBase` C++ 基类提供：
- 绑定子控件（节点卡片列表、货币显示、侧别切换）
- 购买入口（委托给 MetaProgressionSubsystem.TryPurchaseNode）
- 事件响应（OnNodePurchased / OnCurrencyChanged）

每个节点卡片用独立 `UMetaNodeCardWidgetBase` 展示：名称、等级进度条、花费、前置锁定状态、购买按钮。

### C. 编辑器升级树工具扩展

在现有 `SMetaProgressionWorkbenchWidget` 基础上：
- 将节点列表改为拓扑有向图（Slate Canvas + 连线）  
- 支持拖拽排布节点位置（序列化到 DataTable 的 EditorPosition 辅助字段）
- RuneGrant 类型节点显示绑定的 RuneDA 缩略图

### D. 主城设施框架（AHubFacilityActor）

C++ 基类：
- Box Collision 触发交互（玩家靠近显示"按 E 交互"提示）
- `OnInteract()` 虚函数，子 BP 覆写后推入对应 UI Widget
- `FText FacilityDisplayName`（HUD 交互提示用）

主要子类（均为 Blueprint）：
| 设施 | 触发 UI |
|------|---------|
| `BP_HubUpgradeTerminal` | `WBP_MetaUpgradeTree` |
| `BP_HubRunStartPortal` | 进入开局确认 → 加载第一层 |
| `BP_HubArchive`（二期） | 历史统计 / 成就查询 |

### E. 局外资源结算页

`UYogRunSummaryWidgetBase` C++ 基类：
- 接收结算数据：`FRunSummaryData { FloorReached, EnemiesKilled, MetaCurrencyGained (TMap), UnlockableNodeNames }`
- 展示本局战绩 + 获得局外资源 + 当前可解锁节点提示
- "返回主城"按钮 → 加载 L_HubTown

触发点：GameMode `HandlePlayerDeath` / 通关结算时广播 `OnRunEnded(FRunSummaryData)`

### F. 资源房 → 货币联通

`RoomDataAsset` 增加字段 `TArray<FMetaCurrencyCost> MetaCurrencyRewards`；房间清场后 GameMode 读取并调用 `MetaProgressionSubsystem.AddCurrency`。

---

## 实现步骤

1. **MetaTypes.h + YogMetaProgressionSubsystem.cpp**  
   — 加 `RuneGrant` 枚举值、`StarterRuneToGrant`（`FSoftObjectPath`）字段  
   — `TryPurchaseNode` 中处理 RuneGrant → 调用 `AddCraftedStarterRune`

2. **YogMetaProgressionSubsystem.h** — 加 `FRunSummaryData` 结构体、`OnRunEnded` 委托声明

3. **UYogMetaUpgradeTreeWidgetBase.h/.cpp** — C++ 基类（BindWidget + 购买逻辑 + 事件绑定）

4. **UMetaNodeCardWidgetBase.h/.cpp** — 节点卡片基类（状态驱动：Locked / Available / Purchased）

5. **UYogRunSummaryWidgetBase.h/.cpp** — 结算页基类（接收 FRunSummaryData，展示 + 返回主城按钮）

6. **AHubFacilityActor.h/.cpp** — 主城设施 C++ 基类（碰撞 + OnInteract 虚函数）

7. **RoomDataAsset.h** — 加 `MetaCurrencyRewards` 字段

8. **YogGameMode.cpp** — `HandlePlayerDeath` / 通关时构建 FRunSummaryData 并广播；房间清场时调用 AddCurrency

9. **SMetaProgressionWorkbenchWidget 扩展**（编辑器）— 拓扑图视图、RuneGrant 节点缩略图

10. **WBP + L_HubTown**（手动 Editor）— 蓝图 Widget 布局 + 主城关卡搭建

---

## 涉及文件

| 文件 | 改动 |
|------|------|
| `Source/DevKit/Public/MetaProgression/MetaTypes.h` | 加 `RuneGrant` 枚举值、`StarterRuneToGrant FSoftObjectPath` 字段 |
| `Source/DevKit/Public/MetaProgression/YogMetaProgressionSubsystem.h` | 加 `FRunSummaryData` 结构体、`OnRunEnded` 委托 |
| `Source/DevKit/Private/MetaProgression/YogMetaProgressionSubsystem.cpp` | `TryPurchaseNode` 处理 RuneGrant |
| `Source/DevKit/Public/UI/YogMetaUpgradeTreeWidgetBase.h` | 新建：升级树 Widget 基类 |
| `Source/DevKit/Private/UI/YogMetaUpgradeTreeWidgetBase.cpp` | 新建 |
| `Source/DevKit/Public/UI/MetaNodeCardWidgetBase.h` | 新建：节点卡片 Widget 基类 |
| `Source/DevKit/Private/UI/MetaNodeCardWidgetBase.cpp` | 新建 |
| `Source/DevKit/Public/UI/YogRunSummaryWidgetBase.h` | 新建：结算页 Widget 基类 |
| `Source/DevKit/Private/UI/YogRunSummaryWidgetBase.cpp` | 新建 |
| `Source/DevKit/Public/World/HubFacilityActor.h` | 新建：主城设施 Actor 基类 |
| `Source/DevKit/Private/World/HubFacilityActor.cpp` | 新建 |
| `Source/DevKit/Public/Data/RoomDataAsset.h` | 加 `MetaCurrencyRewards` 字段 |
| `Source/DevKit/Private/GameModes/YogGameMode.cpp` | 加 `HandlePlayerDeath` 结算广播 + 房间清场 AddCurrency 钩子 |
| `Source/DevKitEditor/Private/Tools/SMetaProgressionWorkbenchWidget.cpp` | 拓扑图视图扩展 |

---

## 潜在风险

- **RuneGrant 重复授予**：玩家多次进局，`CraftedStarterRunes` 中同一 DA 若被重复 Add 会叠加。需在 `AddCraftedStarterRune` 中加去重（或在 `TryPurchaseNode` 判断节点已购买则跳过）。目前逻辑未去重，需修复。
- **编辑器拓扑图排布持久化**：节点坐标存在 DataTable 的辅助字段中；若美术移动节点后忘记保存 DataTable，位置丢失。考虑单独存一个 EditorLayout 资产规避。
- **主城关卡加载时机**：死亡 → 结算页 → 主城，需要确保 PendingRunState 在加载主城前已清空（ClearRunCheckpoint 已有，但顺序需保证）。
- **RoomDataAsset 字段新增兼容性**：已存在的 RoomDataAsset 蓝图会多一个空 MetaCurrencyRewards 字段，默认为空数组，不触发 AddCurrency，行为兼容。

---

## 设计决策（已确认）

- **神秘点**：暂时不使用，所有节点购买只消耗货币体系。`MysticLevelRequired` 字段保留配置但 `TryPurchaseNode` 忽略它，`AddMysticPoints` 暂不调用。
- **结算数据**：使用累计方案——局内每次 `AddCurrency` 时同步累加 `FRunSummaryData.MetaCurrencyGained`（按 Tag 分计），跑局开始时清零，结算时直接读取。局内可花费局外货币（如商店），累计反映毛收入而非净增量。
- **编辑器节点坐标**：存 DataTable 辅助字段（`EditorPositionX/Y float`），简单直接。升级树拓扑图视图作为纯编辑器工具（扩展现有 SMetaProgressionWorkbenchWidget），不做运行时版本；玩家在主城通过 WBP_MetaUpgradeTree 购买节点，编辑器工具仅供策划配置用。
- **主城交互键**：复用现有 `E` 键 ItemInteract 路径（`AHubFacilityActor` 实现与 `AItemSpawner` 相同的 Overlap + Interact 接口）。
