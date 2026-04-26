# 02 构筑系统 — 已完成功能盘点

> 范围：符文 / 背包格 / 4 阶热度 / 链路 / 升级 / 献祭 / 热度携带 / Tag 命名空间。Roguelite 核心 build-up 体验。
> 背包 UI 视觉层 → [06_UI_HUD.md](06_UI_HUD.md)；武器初始符文自动放置 → [04_Weapons.md](04_Weapons.md)。

---

## 背包与热度

### [BACKPACK-001 / HEAT-001] 背包格 + 4 阶热度激活区
- **设计需求**：核心 build-up 体验 — "热度越高激活区越大"；内圈永久符文格不受热度影响；4 个阶段（0/1/2/3）对应不同范围。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Component/BackpackGridComponent.h` + `Private/Component/BackpackGridComponent.cpp`
  - `Public/Data/RuneDataAsset.h`（`FRuneShape` / `FRuneInstance` / `FRuneConfig`）
  - `Public/AbilitySystem/Attribute/PlayerAttributeSet.h`（`Heat` / `MaxHeat` 属性）
- **设计文档**：[BackpackSystem_Technical](../Systems/Rune/BackpackSystem_Technical.md)
- **验收方式**：
  1. 升阶后激活区扩大，原本未激活的格子里的符文应触发 OnRuneActivated
  2. 内圈永久格在 Phase 0 即可激活

### [RUNE-UPG] 符文升级（Lv.I / II / III 自动合并）
- **设计需求**：同名符文堆叠自动升级；倍率 Lv.I=1.0 / II=1.5 / III=2.0；满级（III）后过滤奖励池不再出现。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Component/BackpackGridComponent.h` + `.cpp`（`FRuneInstance.Level` + 升级路径）
- **设计文档**：memory: `project_rune_upgrade_design.md` （**待转写**为 `Docs/Systems/Rune/RuneUpgrade_Design.md`）
- **验收方式**：放入第 2 个同名符文应自动合并为 Lv.II；放第 3 个为 Lv.III；之后 GenerateLootOptions 不应再出该符文

### [FEAT-013] 链路系统（Producer / Consumer）
- **设计需求**：符文之间能"传导激活"（Producer 在激活区时按方向传导给相邻 Consumer），鼓励组合构筑。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Component/BackpackGridComponent.h` + `.cpp`（`ComputeAllPossibleActivationCells` / `ComputeChainActivatedCells` / `ChainDirectionToOffset` / `IsRuneInZone` 私有方法）
  - `Public/Data/RuneDataAsset.h`（`ERuneChainRole` / `EChainDirection` / `FRuneConfig.ChainRole` + `ChainDirections`）
- **设计文档**：[BackpackSystem_Technical](../Systems/Rune/BackpackSystem_Technical.md)
- **验收方式**：把 Producer 放进激活区，相邻方向的 Consumer 即使不在激活区内也应被激活（BFS 多跳）

---

## 符文激活逻辑

### [BuffFlow / FA] 可视化 Buff / 符文逻辑节点图
- **设计需求**：所有 buff / effect 逻辑必须能在 Flow Graph 中拼出来（memory `feedback_flow_first.md` — **将转写为 [Conventions/BuffFlow.md](../Conventions/BuffFlow.md)**），不写代码也能配出新效果。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/BuffFlow/BuffFlowComponent.h` + `Public/BuffFlow/BuffFlowTypes.h`
  - `Public/BuffFlow/Nodes/BFNode_Base.h` + 40+ 节点（OnDamageDealt / OnCritHit / OnKill / ApplyEffect / GrantTag / Probability / Delay / DoOnce / SpawnNiagara / ...）
  - `Public/BuffFlow/NotifyFlowAsset.h`
- **设计文档**：[BuffFlow_NodeUsageGuide](../Systems/Rune/BuffFlow_NodeUsageGuide.md) · [RuneLogic_Complete_Guide](../Systems/Rune/RuneLogic_Complete_Guide.md)
- **验收方式**：新建 `FA_Test`（`UNotifyFlowAsset` 子类），右键应只看到 `BuffFlow` 分类节点；Start → BFNode_AddTag → 应执行无报错

### [FA 通用架构] 敌人 / 动作 GA 也走 FA 三层模型
- **设计需求**：不只是符文，所有 GA / buff 都用同一套节点图 — 敌人攻击 / 玩家技能能复用 FA 编排。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/AbilitySystem/Abilities/YogGameplayAbility.h` 触发 BuffFlowComponent 子图
- **设计文档**：[FA_UniversalArchitecture](../Systems/Rune/FA_UniversalArchitecture.md)

### [BuffFlow CustomInput DataPin] 子图父图传参
- **设计需求**：模块化拆分 FA 时要能跨图传值，不必把所有逻辑塞一张图。
- **状态**：✅ C++完成
- **设计文档**：[BuffFlow_CustomInputDataPin](../Systems/Rune/BuffFlow_CustomInputDataPin.md)
- **验收方式**：在 FA 父图调用子图时，CustomInput DataPin 上的值应正确传到子图内对应 Pin

---

## 跨关携带 / 长效 Buff

### [FEAT-026] 热度携带符文（余烬 / 炽核）
- **设计需求**：切关时玩家可"带走"一阶（余烬）或两阶（炽核）热度，奖励高风险打法 — 切关不一定从 0 重启。
- **状态**：✅ C++完成（Tag + GI 字段 + 切关 / 恢复逻辑已埋）；⚙ FA 待做（HEAT-CARRY-001/002）
- **核心文件**：
  - `Config/Tags/BuffTag.ini`（`Buff.HeatCarry.OnePhase` / `Buff.HeatCarry.TwoPhase`）
  - `Private/GameModes/YogGameMode.cpp`（切关时检查玩家 ASC 写 CurrentHeat）
  - `Public/System/YogGameInstanceBase.h`（`FRunState.CurrentHeat`）
  - `Private/Character/PlayerCharacterBase.cpp`（`RestoreRunStateFromGI` 跨阶恢复）
- **验收方式**：FA 创建后，玩家拾取余烬符文 → 升 Phase 1 → 切关 → 下一关应保持 Phase 1（而非归 0）

### [FEAT-014 / SACR-001] 献祭恩赐（Run-wide Buff + HP 衰退）
- **设计需求**：玩家可拿"用 HP 换强力恩赐"的高风险全局 Buff；衰退速率随时间加速直到自然结束或主动停止。
- **状态**：✅ C++完成；⚙ FA + 拾取物 BP 待配（CONTENT-001）
- **核心文件**：
  - `Public/Data/SacrificeGraceDA.h`（`BaseDecayRate` / `DecayAccelPerSecond` / `MaxDecayRate` / `HPDrainPerSecond` / `BonusEffect` / `FlowAsset`）
  - `Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h` + `Private/BuffFlow/Nodes/BFNode_SacrificeDecay.cpp`（1s Timer，动态 GE 创建）
  - `Public/Character/PlayerCharacterBase.h`（`AcquireSacrificeGrace(DA)` / `ActiveSacrificeGrace`）
  - `Private/GameModes/YogGameMode.cpp`（`EnterArrangementPhase` 末尾 15% 概率掉拾取物）
- **设计文档**：[EditorSetup_ChainAndSacrifice](../TODO/EditorSetup_ChainAndSacrifice.md)
- **验收方式**：拾取 SacrificePickup → 接受 → 玩家应满热度 + 持续掉血；切关后献祭恩赐应保留（FRunState.ActiveSacrificeGrace）

---

## 祭坛交互

### [FEAT-029] 祭坛交互（净化 / 升级 / 献祭）
- **设计需求**：事件房要能让玩家"删格 / 三选一献祭"；升级先存根；只在 Arrangement Phase 可交互。
- **状态**：✅ C++完成；⚙ DA / BP / 三个 WBP 待建（ALTAR-001/002/003）
- **核心文件**：
  - `Public/Map/AltarActor.h` + `Private/Map/AltarActor.cpp`（实现 `IPlayerInteraction`，`bIsActive` 仅 Arrangement Phase 为 true）
  - `Public/Data/AltarDataAsset.h`（`SacrificeRunePool`）
  - `Public/UI/AltarMenuWidget.h` + `Public/UI/RunePurificationWidget.h` + `Public/UI/SacrificeSelectionWidget.h` 三个 WBP 控件类
  - `Public/Component/BackpackGridComponent.h`（新增 `TryRemoveRuneCell` — 清 GridOccupancy + Shape.Cells.RemoveAt + 广播 `OnRuneCellRemoved`；(0,0) pivot 拒绝删除）
- **设计文档**：[FeatureLog FEAT-029](../FeatureLog.md)
- **验收方式**：
  1. Arrangement 阶段走近祭坛按 E 应弹菜单；Combat 阶段不响应
  2. 净化选格子 → 确认 → 该格符文应消失但 (0,0) pivot 格删不掉
  3. 献祭选项确认 → 玩家背包应新增对应符文且 GrantedRune FA 触发

---

## 符文操作辅助

### [FEAT-025] 符文旋转系统
- **设计需求**：异形符文（多格 Shape）可旋转 90° 适配格子布局；包括格内已选符文与待放置区。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Data/RuneDataAsset.h`（`FRuneInstance.Rotation` 0-3，`FRuneShape::Rotate90()`，`GetPivotOffset(N)`）
  - `Public/UI/BackpackScreenWidget.h` + `.cpp`（`RotateSelectedRune` / `RotatePendingRune`）
- **验收方式**：选中多格符文按 R，符文应顺时针 90° 旋转；待放置区符文按 R 同理

---

## Tag 体系（构筑系统的底层依赖）

### [Tag 命名空间] 6 大命名空间 + 决策树
- **设计需求**：跨系统 Tag 不冲突；新增 Tag 有规范决策树告诉开发者放哪。
- **状态**：✅ 配置完成
- **核心文件**：
  - `Config/Tags/BuffTag.ini` / `ActionTag.ini` / `PlayerTag.ini` / `EnemyTag.ini` / `AbilityTag.ini` / `GameplayEvent.ini`（命名空间见 memory `reference_tag_namespaces.md` — **将转写为 [Tags/Tag_Namespaces.md](../Tags/Tag_Namespaces.md)**）
- **设计文档**：[GameplayTag_MasterGuide](../Tags/GameplayTag_MasterGuide.md) · [Tag_SituationalGuide](../Tags/Tag_SituationalGuide.md)
- **验收方式**：新增 Tag 时按决策树选命名空间；ProjectSettings → GameplayTags 中新 Tag 应出现在对应 .ini 加载源

### [状态冲突表] DA 填表方式声明 Tag 互斥
- **设计需求**：玩家 / 敌人身上"晕眩"和"狂暴"不能同时挂；规则要能在 DA 配，不写代码。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Data/StateConflictDataAsset.h`
  - `Public/Data/GameplayTagRelation.h`
- **设计文档**：[StateConflict_TagBlock](../Tags/StateConflict/StateConflict_TagBlock.md) · [StateConflict_Technical](../Tags/StateConflict/StateConflict_Technical.md)
- **验收方式**：在 DA 配 "Buff.Status.Stun blocks Buff.Status.Berserk"，玩家身上挂 Stun 时再尝试加 Berserk 应被拦截

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| HEAT-CARRY-001 | `FA_Rune_余烬`：`Start → BFNode_GrantTag(Buff.HeatCarry.OnePhase)` | [TASKS](../PM/TASKS.md) |
| HEAT-CARRY-002 | `FA_Rune_炽核`：`Start → BFNode_GrantTag(Buff.HeatCarry.TwoPhase)` | [TASKS](../PM/TASKS.md) |
| ALTAR-001 | 创建 `DA_Altar_EventRoom`（`UAltarDataAsset`），填献祭符文池 + 代价文字 | [TASKS](../PM/TASKS.md) |
| ALTAR-002 | 创建 `BP_AltarActor`（`AAltarActor` 子类），配 mesh + InteractBox + AltarData + 三个 WidgetClass | 同上 |
| ALTAR-003 | 创建 `WBP_AltarMenu` / `WBP_RunePurification` / `WBP_SacrificeSelection`，按钮调对应 BlueprintCallable | 同上 |
| CONTENT-001 | 献祭恩赐 FA（`Start → BFNode_SacrificeDecay`）+ `BP_SacrificePickup` | [EditorSetup_ChainAndSacrifice](../TODO/EditorSetup_ChainAndSacrifice.md) |
| RUNE-P0-1/2/3 | 1017 / 1018 / 1019 高感知测试符文 FA | [TestRune_HighPerception_Guide](../Systems/Rune/TestRune_HighPerception_Guide.md) |
