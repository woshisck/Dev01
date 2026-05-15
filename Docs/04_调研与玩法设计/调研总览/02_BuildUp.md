# 02 构筑系统 — 已完成功能盘点

> 范围：符文 / 背包格 / 4 阶热度 / 链路 / 升级 / 献祭 / 热度携带 / Tag 命名空间。Roguelite 核心 build-up 体验。
> 背包 UI 视觉层 → [06_UI_HUD.md](06_UI_HUD.md)；武器初始符文自动放置 → [04_Weapons.md](04_Weapons.md)。

---

## 功能总览

| 功能名称 | 需求简介 | 完成状态（代码）| 完成状态（编辑器）|
|---|---|---|---|
| 背包格 + 4 阶热度激活区 (BACKPACK-001 / HEAT-001) | "热度越高激活区越大"核心 build-up | ✅ 编译通过 | ✅ |
| 符文升级 Lv.I / II / III 自动合并 (RUNE-UPG) | 同名符文堆叠自动升级 + 满级过滤奖励池 | ✅ 编译通过 | — 数据驱动 |
| 链路系统 Producer / Consumer (FEAT-013) | 符文方向传导激活 | ✅ 编译通过 | ⚙ 符文 DA 填 ChainRole / ChainDirections |
| BuffFlow / FA 节点图体系 | 所有 buff / 效果走可视化节点（不写 GA / GE）| ✅ 编译通过 | ✅ FA 编辑器可用 |
| FA 通用架构（敌人 / 动作 GA 也走 FA）| GA 触发 BuffFlow 子图 | ✅ 编译通过 | ✅ |
| BuffFlow CustomInput DataPin | 子图父图传参 | ✅ 编译通过 | ✅ |
| 热度携带符文 Tag + 切关恢复 (FEAT-026) | 带走 1 / 2 阶热度 | ✅ 编译通过 | ⚙ FA_Rune_余烬 / 炽核 待做 |
| 献祭恩赐（Run-wide Buff + HP 衰退）(FEAT-014 / SACR-001) | 用 HP 换强力恩赐 | ✅ 编译通过 | ⚙ FA + BP_SacrificePickup 待做（CONTENT-001）|
| 武器初始符文自动放置 (FEAT-027) | 拾取武器自动塞热度一区 | ✅ 编译通过 | ⚙ DA_Weapon_*.InitialRunes 待填 |
| 祭坛交互（净化 / 升级 / 献祭）(FEAT-029) | 事件房删格 / 三选一献祭 | ✅ 编译通过 | ⚙ DA + BP + 三 WBP 待建（ALTAR-001~003）|
| 符文旋转系统 (FEAT-025) | 异形符文 90° 旋转 | ✅ 编译通过 | ✅ |
| Tag 命名空间体系（6 大命名空间 + 决策树）| 跨系统 Tag 不冲突 | ✅ 编译通过 | ✅ Config/Tags/*.ini 已配 |
| 状态冲突表（DA 填表声明 Tag 互斥）| Tag 互斥规则可配置 | ✅ 编译通过 | ⚙ 各 StateConflict DA 按需填 |

> 状态口径：✅ 完成 / ⚙ 待配置（注明待办）/ — 不涉及编辑器配置

---

## 背包与热度

### [BACKPACK-001 / HEAT-001] 背包格 + 4 阶热度激活区
- **设计需求**：核心 build-up 体验 — "热度越高激活区越大"；内圈永久符文格不受热度影响；4 个阶段（0/1/2/3）对应不同范围。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Component/BackpackGridComponent.h` + `Private/Component/BackpackGridComponent.cpp`
  - `Public/Data/RuneDataAsset.h`（`FRuneShape` / `FRuneInstance` / `FRuneConfig`）
  - `Public/AbilitySystem/Attribute/PlayerAttributeSet.h`（`Heat` / `MaxHeat` 属性）
- **设计文档**：[BackpackSystem_Technical](../../99_归档/旧方案/2D背包方案/BackpackSystem_Technical.md)
- **验收方式**：
  1. 升阶后激活区扩大，原本未激活的格子里的符文应触发 OnRuneActivated
  2. 内圈永久格在 Phase 0 即可激活

### [RUNE-UPG] 符文升级（Lv.I / II / III 自动合并）
- **设计需求**：同名符文堆叠自动升级；倍率 Lv.I=1.0 / II=1.5 / III=2.0；满级（III）后过滤奖励池不再出现。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Component/BackpackGridComponent.h` + `.cpp`（`FRuneInstance.Level` + 升级路径）
- **设计文档**：memory: `project_rune_upgrade_design.md` （**待转写**为 `Docs/01_长期系统文档/系统/Rune/RuneUpgrade_Design.md`）
- **验收方式**：放入第 2 个同名符文应自动合并为 Lv.II；放第 3 个为 Lv.III；之后 GenerateLootOptions 不应再出该符文

### [FEAT-013] 链路系统（Producer / Consumer）
- **设计需求**：符文之间能"传导激活"（Producer 在激活区时按方向传导给相邻 Consumer），鼓励组合构筑。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Component/BackpackGridComponent.h` + `.cpp`（`ComputeAllPossibleActivationCells` / `ComputeChainActivatedCells` / `ChainDirectionToOffset` / `IsRuneInZone` 私有方法）
  - `Public/Data/RuneDataAsset.h`（`ERuneChainRole` / `EChainDirection` / `FRuneConfig.ChainRole` + `ChainDirections`）
- **设计文档**：[BackpackSystem_Technical](../../99_归档/旧方案/2D背包方案/BackpackSystem_Technical.md)
- **验收方式**：把 Producer 放进激活区，相邻方向的 Consumer 即使不在激活区内也应被激活（BFS 多跳）

---

## 符文激活逻辑

### [BuffFlow / FA] 可视化 Buff / 符文逻辑节点图
- **设计需求**：所有 buff / effect 逻辑必须能在 Flow Graph 中拼出来（memory `feedback_flow_first.md` — **将转写为 [Conventions/BuffFlow.md](../../00_入口与规范/缺失引用记录.md)**），不写代码也能配出新效果。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/BuffFlow/BuffFlowComponent.h` + `Public/BuffFlow/BuffFlowTypes.h`
  - `Public/BuffFlow/Nodes/BFNode_Base.h` + 40+ 节点（OnDamageDealt / OnCritHit / OnKill / ApplyEffect / GrantTag / Probability / Delay / DoOnce / SpawnNiagara / ...）
  - `Public/BuffFlow/NotifyFlowAsset.h`
- **设计文档**：[BuffFlow_NodeUsageGuide](../../01_长期系统文档/系统/Rune/BuffFlow_NodeUsageGuide.md) · [RuneLogic_Complete_Guide](../../01_长期系统文档/系统/Rune/RuneLogic_Complete_Guide.md)
- **验收方式**：新建 `FA_Test`（`UNotifyFlowAsset` 子类），右键应只看到 `BuffFlow` 分类节点；Start → BFNode_AddTag → 应执行无报错

### [FA 通用架构] 敌人 / 动作 GA 也走 FA 三层模型
- **设计需求**：不只是符文，所有 GA / buff 都用同一套节点图 — 敌人攻击 / 玩家技能能复用 FA 编排。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/AbilitySystem/Abilities/YogGameplayAbility.h` 触发 BuffFlowComponent 子图
- **设计文档**：[FA_UniversalArchitecture](../../01_长期系统文档/系统/Rune/FA_UniversalArchitecture.md)

### [BuffFlow CustomInput DataPin] 子图父图传参
- **设计需求**：模块化拆分 FA 时要能跨图传值，不必把所有逻辑塞一张图。
- **状态**：✅ C++完成
- **设计文档**：[BuffFlow_CustomInputDataPin](../../01_长期系统文档/系统/Rune/BuffFlow_CustomInputDataPin.md)
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
- **设计文档**：[EditorSetup_ChainAndSacrifice](../../99_归档/TODO/EditorSetup_ChainAndSacrifice.md)
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
- **设计文档**：[FeatureLog FEAT-029](../../05_项目管理与进度/FeatureLog.md)
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
  - `Config/Tags/BuffTag.ini` / `ActionTag.ini` / `PlayerTag.ini` / `EnemyTag.ini` / `AbilityTag.ini` / `GameplayEvent.ini`（命名空间见 memory `reference_tag_namespaces.md` — **将转写为 [Tags/Tag_Namespaces.md](../../00_入口与规范/缺失引用记录.md)**）
- **设计文档**：[GameplayTag_MasterGuide](../../01_长期系统文档/标签/GameplayTag_MasterGuide.md) · [Tag_SituationalGuide](../../01_长期系统文档/标签/Tag_SituationalGuide.md)
- **验收方式**：新增 Tag 时按决策树选命名空间；ProjectSettings → GameplayTags 中新 Tag 应出现在对应 .ini 加载源

### [状态冲突表] DA 填表方式声明 Tag 互斥
- **设计需求**：玩家 / 敌人身上"晕眩"和"狂暴"不能同时挂；规则要能在 DA 配，不写代码。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Data/StateConflictDataAsset.h`
  - `Public/Data/GameplayTagRelation.h`
- **设计文档**：[StateConflict_TagBlock](../../01_长期系统文档/标签/StateConflict/StateConflict_TagBlock.md) · [StateConflict_Technical](../../01_长期系统文档/标签/StateConflict/StateConflict_Technical.md)
- **验收方式**：在 DA 配 "Buff.Status.Stun blocks Buff.Status.Berserk"，玩家身上挂 Stun 时再尝试加 Berserk 应被拦截

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| HEAT-CARRY-001 | `FA_Rune_余烬`：`Start → BFNode_GrantTag(Buff.HeatCarry.OnePhase)` | [TASKS](../../05_项目管理与进度/PM/TASKS.md) |
| HEAT-CARRY-002 | `FA_Rune_炽核`：`Start → BFNode_GrantTag(Buff.HeatCarry.TwoPhase)` | [TASKS](../../05_项目管理与进度/PM/TASKS.md) |
| ALTAR-001 | 创建 `DA_Altar_EventRoom`（`UAltarDataAsset`），填献祭符文池 + 代价文字 | [TASKS](../../05_项目管理与进度/PM/TASKS.md) |
| ALTAR-002 | 创建 `BP_AltarActor`（`AAltarActor` 子类），配 mesh + InteractBox + AltarData + 三个 WidgetClass | 同上 |
| ALTAR-003 | 创建 `WBP_AltarMenu` / `WBP_RunePurification` / `WBP_SacrificeSelection`，按钮调对应 BlueprintCallable | 同上 |
| CONTENT-001 | 献祭恩赐 FA（`Start → BFNode_SacrificeDecay`）+ `BP_SacrificePickup` | [EditorSetup_ChainAndSacrifice](../../99_归档/TODO/EditorSetup_ChainAndSacrifice.md) |
| RUNE-P0-1/2/3 | 1017 / 1018 / 1019 高感知测试符文 FA | [TestRune_HighPerception_Guide](../../01_长期系统文档/系统/Rune/TestRune_HighPerception_Guide.md) |
