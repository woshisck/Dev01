## Codex 审查结果

### 发现的问题
- `StarterRuneToGrant` 设计为单个 `FSoftObjectPath`，但现有授予接口是 `AddCraftedStarterRune(FPrimaryAssetId)`；需要明确资产引用与 `PrimaryAssetId` 转换，否则 BeginPlay 可能无法授予。
- “同一 DA 可多次叠加”和“需要去重”互相矛盾；现有 `AddCraftedStarterRune` 使用 `AddUnique`，多级节点授予同一 RuneDA 时会花费成功但不会增加效果。
- 单个 `StarterRuneToGrant` 字段无法支持“每级授予不同 DA 形成进化链”，除非改成数组或把每一级拆成独立节点。
- 方案说 `TryPurchaseNode` 忽略 `MysticLevelRequired`，但现有 `CanPurchaseNode` 仍会检查该字段，实施步骤未覆盖。
- `AHubFacilityActor` 的 `OnInteract()` 不会被当前 E 键流程自动调用；现有交互分支只处理 Weapon/Pickup/Altar/Shop/Portal，需要补 `PendingHubFacility` 或通用交互接口。
- 运行时 UI 方案未接入现有 `YogUIManagerSubsystem`、`EYogUIScreenId`、`DA_YogUIRegistry` 和 CommonActivatableWidget 输入策略，直接 Push/显示会和鼠标、焦点、关闭栈冲突。
- 结算页触发点与现有死亡复活/GameOver 流程冲突；若在 `HandlePlayerDeath` 广播，可能在可复活时提前展示结算。

### 潜在风险
- `TryPurchaseNode` 内直接调用 `AddCraftedStarterRune` 会触发额外 `CommitSave`，购买流程不是单次原子提交。
- 将编辑器布局坐标写入运行时 `FMetaUpgradeNodeRow` 会污染 DataTable/CSV，并增加策划导入导出维护成本。
- `AddCurrency` 被用于奖励、扣款和调试发放；用它无差别累计 `MetaCurrencyGained` 容易把非跑局收益计入结算。
- 资源房奖励如果没有独立“本房间已发放”语义，后续重构或多路径结算时可能重复发货币。
- 新增 `RuneGrant` 后，编辑器工具、校验、CSV 导入导出、节点显示标签都需要同步更新，否则配置侧容易产生无效节点。

### 改进建议
- 将 RuneGrant 数据建模为 `TArray<TSoftObjectPtr<URuneDataAsset>> StarterRunesToGrantByLevel`，或明确只支持唯一符文并把 MaxLevel 限制为 1。
- 在 `TryPurchaseNode` 内直接修改 `Meta.CraftedStarterRunes`，使用内部 helper，最后统一广播与保存。
- 新增通用交互目标：`PendingInteractable` / `IInteractable::TryInteract(Player)`，Hub、Shop、Portal 可逐步统一到同一路径。
- 新增 `MetaUpgradeTree`、`RunSummary` 的 ScreenId 与 UIRegistry 条目，Widget 基类建议继承项目现有 CommonUI 基类。
- 资源房奖励建议放在 `EnterArrangementPhase` 成功切到 Arrangement 后，由 `GrantRoomMetaRewards(ActiveRoomData)` 统一发放、累计结算、批量保存。
- 编辑器拓扑布局优先考虑单独 `DA_MetaUpgradeTreeLayout`；若仍写入 DataTable，至少把校验、CSV、代理编辑、默认值一次补齐。
- 增加自动化测试：RuneGrant 购买、同 DA 多级语义、货币奖励累计、死亡/复活不弹结算、返回主城清理 RunState。

### 需要向用户确认的问题
- 同一个 RuneDA 多次解锁时，期望是叠加多实例，还是只解锁一次？
- 前置节点要求是“至少 1 级”还是“前置满级”？现有注释与实现语义不完全一致。
- 神秘侧是否完全不参与购买限制，还是只暂时不用“神秘点”但仍保留神秘侧等级门槛？
- 死亡后流程是直接结算回主城，还是先保留现有复活/GameOver 菜单？
- 结算页货币显示要展示毛收入、净增量，还是按来源拆分？

### 需要手动创建的引擎资产
（列出所有需要在 UE5 编辑器中手动创建的蓝图、DA、DT、材质等资产，留空则填无）
- `WBP_MetaUpgradeTree`
- `WBP_MetaNodeCard`
- `WBP_RunSummary`
- `BP_HubUpgradeTerminal`
- `BP_HubRunStartPortal`（或复用/派生现有 Portal BP）
- `BP_HubArchive`（二期需要）
- `L_HubTown`
- `DA_Room_HubTown`（`bIsHubRoom=true`，配置 `PortalDestinations`）
- `DA_YogUIRegistry` 新增/配置 MetaUpgradeTree、RunSummary 屏幕条目
- `DT_MetaUpgradeNodes` 新增/修改 RuneGrant 节点行
- `DT_MetaCurrencyRules` 新增/确认局外货币 Tag 行
- 各资源房 `DA_Room_*` 配置 `MetaCurrencyRewards`
- RuneGrant 引用的 `DA_Rune_*` 资产
- 主城设施用 Static Mesh / Material / Niagara / 音效资产（如不复用现有资源）