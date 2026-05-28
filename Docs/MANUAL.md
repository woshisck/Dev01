# 星骸降临 开发者手册

> 最后更新：2026-05-28  
> 适用对象：参与本项目的程序、策划、TA，及 AI 助手（Claude/Codex）

---

## 一、快速上手

### 三步找到你要的文档

| 你想做什么 | 第一步 | 第二步 |
| --- | --- | --- |
| 了解某系统怎么运作 | 找对应模块的 `README.md`（见第三章） | 读其中"架构说明"链接 |
| 知道怎么配置某功能 | `03_配置与编辑器手册/README.md` | 找对应功能子目录 |
| 查接口可以调哪些函数 | 第四章「接口速查」 | 点击对应文档链接 |
| 查当前开发任务 | `05_项目管理与进度/PM/TASKS.md` | — |
| 查某 Tag 的规范 | `01_长期系统文档/标签/GameplayTag_MasterGuide.md` | — |
| 开始一个新功能 | 读第五章「工作流程」 | — |

### 文档目录一句话说明

```
Docs/
├── MANUAL.md                    ← 你在这里（全局手册）
├── INDEX.md                     ← 快速入口表
├── 00_入口与规范/                 规范和迁移记录
├── 01_长期系统文档/               跨版本长期有效的技术文档
│   ├── 编码规范/                 C++/Widget/Material/GAS/AN/DA 约定
│   ├── 标签/                    GameplayTag 规范体系
│   └── 系统/                    各功能模块的架构 + 接口文档
├── 02_版本计划与需求/             当前核心方案、需求拆分、协作接口
├── 03_配置与编辑器手册/           策划在编辑器里操作的配置说明
├── 04_调研与玩法设计/             调研、玩法规则、卡牌/符文设计
├── 05_项目管理与进度/             任务看板、进度快照、功能完成记录
├── 90_自动生成报告/               Commandlet 等工具自动输出
└── 99_归档/                      历史记录、旧方案（只读，不作为最新依据）
```

---

## 二、项目架构总览

### 三层运行时架构

```
GameInstance 层（全局，跨关卡存活）
  ├── UStoryEngineSubsystem        故事引擎 — 规则匹配 + 动作执行
  ├── UFirstRunTutorialDirectorSubsystem  导演 — 教程流程控制
  ├── UYogMetaProgressionSubsystem 元进度 — 局外货币/升级/功能解锁
  └── UYogSaveSubsystem            存档 — 唯一持久化入口

GameMode / World 层（单局，切关卡后重建）
  ├── AYogGameMode                 关卡流程 + 遭遇战 + 传送门
  ├── UStoryRuleSetDA              规则集资产（由 StoryEngine 管理）
  └── ALevelFlowActor              LevelFlow 驱动的关卡事件序列

Character / Component 层（角色，每帧）
  ├── AYogCharacterBase            GAS 宿主，持 AbilitySystemComponent
  ├── UBackpackGridComponent       背包格子 + 符文 Buff 管理
  └── UBuffFlowComponent           FA/FlowGraph 驱动的 Buff 执行
```

### 系统依赖关系（简图）

```
Director ──调用──► StoryEngine ──调用──► TutorialManager
                        │                    │
                        ▼                    ▼
              MetaProgression ◄── 查询 ── BackpackGrid
                        │
                        ▼
                    SaveSubsystem（唯一写盘出口）

GameMode ──通知──► Director（Arrangement / 玩家死亡 / 背包开启）
```

完整依赖图：[SystemDependencyMap.md](01_长期系统文档/系统/SystemDependencyMap.md)

### 各系统一句话说明

| 系统 | 职责 | 架构文档 |
| --- | --- | --- |
| StoryEngineSubsystem | 规则集匹配 + 故事动作执行 | [→](01_长期系统文档/系统/Story/StoryEngine_Architecture.md) |
| FirstRunTutorialDirectorSubsystem | 教程关卡序列 + 剧情节点控制 | [→](01_长期系统文档/系统/Story/DirectorInterfaces.md) |
| YogMetaProgressionSubsystem | 局外货币/升级树/功能解锁 | [→](01_长期系统文档/系统/Progression/MetaProgression_Architecture.md) |
| YogSaveSubsystem | 3 槽位存档 + Checkpoint + 异步写盘 | [→](01_长期系统文档/系统/Progression/SaveSubsystem_Architecture.md) |
| AYogGameMode | 局内流程调度（Arrangement/遭遇/清关/死亡） | — |
| BackpackGridComponent | 格子占位 + Buff grant/revoke | [→](01_长期系统文档/系统/Rune/README.md) |
| BuffFlowComponent | FA/FlowGraph 节点执行 | [→](01_长期系统文档/系统/Rune/BuffFlow_Architecture.md) |

---

## 三、功能模块导航

> 每个模块按三个角度组织文档：  
> - **架构说明** — 系统设计、数据流、关键决策  
> - **使用指南** — 如何配置、如何调用、操作步骤  
> - **接口参考** — 对外暴露的函数/接口/钩子清单

---

### 3C — 角色 / 摄像机 / 控制

> 玩家角色的运动感受、镜头跟随、输入响应。

| 类型 | 文档 |
| --- | --- |
| 架构说明 | [Camera_Design.md](01_长期系统文档/系统/Camera/Camera_Design.md) |
| 镜头遮挡淡出 | [CameraOcclusionFade_Guide.md](01_长期系统文档/系统/Camera/CameraOcclusionFade_Guide.md) |
| CameraVolume 配置 | [CameraVolume_Occlusion_Manager_Usage.md](01_长期系统文档/系统/Camera/CameraVolume_Occlusion_Manager_Usage.md) |
| 代码写法规范 | [Docs/Conventions/Camera.md](Conventions/Camera.md)（备注：VInterpTo 起点用 GetCameraLocation） |
| 斜视角方向推导 | 摄像机 Right 向量对齐屏幕，不用世界 XY |

关联：输入映射 → `CommonUI BDA_GamePadInputData` / `KeyboardInputData`（见 Tag 速查）

---

### 战斗 — Combat

> 攻击判定、连击、闪避、架势、终结技。

| 类型 | 文档 |
| --- | --- |
| 攻击伤害配置 | [AttackDamage_ConfigGuide.md](01_长期系统文档/系统/Combat/AttackDamage_ConfigGuide.md) |
| 连击系统 | [EnemyCombo_ConfigGuide.md](01_长期系统文档/系统/Combat/EnemyCombo_ConfigGuide.md) |
| 近战 AN + 符文钩子 | [MeleeCombo_NotifyRune_Guide.md](01_长期系统文档/系统/Combat/MeleeCombo_NotifyRune_Guide.md) |
| 闪避设计 | [Dash_Design.md](01_长期系统文档/系统/Combat/Dash_Design.md) |
| 架势系统 | [PoiseSystem_ConfigGuide.md](01_长期系统文档/系统/Combat/PoiseSystem_ConfigGuide.md) |
| 技能蓄力 | [SkillCharge_Guide.md](01_长期系统文档/系统/Combat/SkillCharge_Guide.md) |
| 终结技卡 | [FinisherCard_*.md](01_长期系统文档/系统/Combat/) |

编码前必读：[GAS.md](01_长期系统文档/编码规范/GAS.md) · [AnimNotify.md](01_长期系统文档/编码规范/AnimNotify.md)

---

### 符文 — Rune

> 背包构筑核心。BuffFlow 是符文/Buff 的统一实现体系。

| 类型 | 文档 |
| --- | --- |
| 架构设计 | [BuffFlow_Architecture.md](01_长期系统文档/系统/Rune/BuffFlow_Architecture.md) |
| 完整开发指南 | [RuneLogic_Complete_Guide.md](01_长期系统文档/系统/Rune/RuneLogic_Complete_Guide.md) — **新功能先读这里** |
| 程序接口 | [BuffFlow_ProgrammerGuide.md](01_长期系统文档/系统/Rune/BuffFlow_ProgrammerGuide.md) |
| FA 节点参考 | [FA_UniversalArchitecture.md](01_长期系统文档/系统/Rune/FA_UniversalArchitecture.md) |
| 月光卡 FA 节点 | [FA_Moonlight_NodeSequences.md](01_长期系统文档/系统/Rune/FA_Moonlight_NodeSequences.md) |
| RuneEditor 使用 | [RuneEditor_UserGuide.md](01_长期系统文档/系统/Rune/RuneEditor_UserGuide.md) |
| 连携调参 | [RuneComboTuning_Requirements.md](01_长期系统文档/系统/Rune/RuneComboTuning_Requirements.md) |
| 敌人 Buff 制作 | [EnemyBuff_ProductionGuide.md](01_长期系统文档/系统/Rune/EnemyBuff_ProductionGuide.md) |
| 升级系统设计 | Docs/Systems/Rune/RuneUpgrade_Design.md |

**关键规则**：`Buff` 和 `符文` 在本项目中均指 `RuneDA + BuffFlow`，开发前先查 BuffFlow 文档，不要另立一套。

---

### 剧情 — Story

> 故事引擎 + 导演 + 教程流程控制。

| 类型 | 文档 |
| --- | --- |
| StoryEngine 架构 | [StoryEngine_Architecture.md](01_长期系统文档/系统/Story/StoryEngine_Architecture.md) |
| 导演接口汇总 | [DirectorInterfaces.md](01_长期系统文档/系统/Story/DirectorInterfaces.md) — **剧情开发先查这里** |
| StoryEngine 设计 | [StoryEngine_Design.md](01_长期系统文档/系统/Story/StoryEngine_Design.md) |
| FA 故事节点参考 | [StoryFA_NodeReference.md](01_长期系统文档/系统/Story/StoryFA_NodeReference.md) |
| 教程流水线状态 | [StoryPipeline/FirstRunTutorial_StatusAndTodo.md](StoryPipeline/FirstRunTutorial_StatusAndTodo.md) |
| 教程文案 | [StorySource/FirstRunTutorial_Story.md](StorySource/FirstRunTutorial_Story.md) |

**三大接口入口**：

```cpp
// 触发剧情事件（走规则匹配）
StoryEngine->BroadcastStoryEventWithPayload(EventTag, ContextTag, ...);

// 直接执行动作（绕过规则）
StoryEngine->ExecuteStoryAction(FStoryAction, Context);

// 写入进度标志
StoryEngine->SetStoryFlag(FlagTag, EStoryFlagScope::Save, true);
```

**标志位作用域**：`Save`（永久）/ `Run`（本局）/ `Session`（本次启动）。  
注意：`OncePerMap` 是规则触发策略（`EStoryRuleFirePolicy`），不是标志位作用域。

---

### 关卡 — Level

> 房间数据、传送门、Buff 池、跨关卡状态。

| 类型 | 文档 |
| --- | --- |
| 关卡系统程序文档 | [LevelSystem_ProgrammerDoc.md](01_长期系统文档/系统/Level/LevelSystem_ProgrammerDoc.md) |
| 关卡系统配置指南 | [LevelSystem_ConfigGuide.md](01_长期系统文档/系统/Level/LevelSystem_ConfigGuide.md) |
| 传送门配置 | [Portal_ConfigGuide.md](01_长期系统文档/系统/Level/Portal_ConfigGuide.md) |
| 传送门预览 WBP | [WBP_PortalPreview_Layout.md](01_长期系统文档/系统/Level/WBP_PortalPreview_Layout.md) |
| Buff 池配置 | [BuffPool_ConfigGuide.md](01_长期系统文档/系统/Level/BuffPool_ConfigGuide.md) |
| 跨关卡状态 | [CrossLevelState_Technical.md](01_长期系统文档/系统/Level/CrossLevelState_Technical.md) |

**导演控制关卡的方式**（Arrangement 阶段写入 `FStoryNextRoomPlan`）：

| 需求 | 字段 |
| --- | --- |
| 指定关卡资产 | `RoomDataOverride` |
| 指定奖励内容 | `RewardOptionsOverride` |
| 指定 Buff 池 | `BuffsOverride` |
| 强制单传送门 | `bForceSinglePortal` |
| 抑制清房奖励 | `bSuppressRoomClearRewardPickup` |

---

### 成长 — Progression

> 元进度（局外升级）+ 存档系统。

| 类型 | 文档 |
| --- | --- |
| 元进度架构 | [MetaProgression_Architecture.md](01_长期系统文档/系统/Progression/MetaProgression_Architecture.md) |
| 存档系统架构 | [SaveSubsystem_Architecture.md](01_长期系统文档/系统/Progression/SaveSubsystem_Architecture.md) |

**关键约定**：
- 存档系统是**唯一合法的持久化入口**，其他系统不直接操作存档文件
- 元进度 DataTable 通过 `UMetaProgressionSettings`（`Config/DefaultGame.ini`）加载，不在 GameInstance BP 赋值
- 教程结束必须三步一起执行：`MarkFirstRunTutorialCompleted()` + `ClearRunCheckpoint()` + `GI->ClearRunState()`

---

### UI

> Widget 层级、HUD、背包、教程弹窗、战斗日志。

| 类型 | 文档 |
| --- | --- |
| Widget 编码规范 | [Widget.md](01_长期系统文档/编码规范/Widget.md) — **所有 WBP 开发先读** |
| UI 系统目录 | [01_长期系统文档/系统/UI/](01_长期系统文档/系统/UI/) |
| 教程弹窗布局 | WBP_TutorialPopup：780×420，Canvas 中心锚，详见内存 reference_tutorial_popup_layout |
| WBP 布局规格标准 | [Docs/Design/UI/WBP_LayoutSpec_Standard.md](Design/UI/WBP_LayoutSpec_Standard.md) |
| CommonUI 输入图标 | BDA_GamePadInputData / KeyboardInputData |

**UI 核心规则**（摘要）：
- 颜色用项目 DA，不硬编码
- 生命周期由外部管理，Widget 不自主销毁
- 输入通过 CommonUI 路由，不直接绑定 `GetPlayerController`

---

### 特效 — VFX

> 材质 Custom Node、Niagara、角色特效（Overlay + CharacterFlash）。

| 类型 | 文档 |
| --- | --- |
| 材质编写规范 | [Material_Authoring_Guide.md](01_长期系统文档/系统/VFX/Material_Authoring_Guide.md) |
| 角色 Flash 接口 | [CharacterFlash_Technical.md](01_长期系统文档/系统/VFX/CharacterFlash_Technical.md) |
| VFX 制作指南 | [VFX_CreationGuide.md](01_长期系统文档/系统/VFX/VFX_CreationGuide.md) |

**材质核心规则**：
- Custom Node 引用 `.ush` 文件，不写内联 HLSL 长代码
- Fresnel 边缘光颜色来自项目 MPC，不硬编码
- Overlay Material 驱动角色特效，C++ Tick 控制参数

---

### 武器 — Weapon

> 武器框架、火绳枪系统、投射物。

| 类型 | 文档 |
| --- | --- |
| 武器系统架构 | [WeaponSystem_Technical.md](01_长期系统文档/系统/Weapon/WeaponSystem_Technical.md) |
| 火绳枪系统设计 | [Musket_System_Design.md](01_长期系统文档/系统/Weapon/Musket_System_Design.md) |
| 火绳枪使用指南 | [Musket_System_Guide.md](01_长期系统文档/系统/Weapon/Musket_System_Guide.md) |
| 投射物关卡门控 | [RangedWeaponProjectileGate.md](01_长期系统文档/系统/Weapon/RangedWeaponProjectileGate.md) |

**当前状态**：火绳枪 C++ 框架已建，BP GA/GE/材质待实现。

---

### 敌人 — Enemy

> AI 行为、刷怪配置、Combo 编辑器。

| 类型 | 文档 |
| --- | --- |
| 敌人旋转配置 | [EnemyRotation_ConfigGuide.md](01_长期系统文档/系统/AI/EnemyRotation_ConfigGuide.md) |
| Mob Spawner | [MobSpawner_Technical.md](01_长期系统文档/系统/Mob/MobSpawner_Technical.md) |
| 敌人 Combo 配置 | [EnemyCombo_ConfigGuide.md](01_长期系统文档/系统/Combat/EnemyCombo_ConfigGuide.md) |
| 敌人 Buff 制作 | [EnemyBuff_ProductionGuide.md](01_长期系统文档/系统/Rune/EnemyBuff_ProductionGuide.md) |
| 配置说明（综合） | [03_配置与编辑器手册/核心配置说明/敌人/](03_配置与编辑器手册/核心配置说明/敌人/) |

---

## 四、接口速查

### 导演 → 各系统 接口

| 目标 | 接口 | 调用时机 |
| --- | --- | --- |
| 控制下一关内容 | `GI->SetPendingStoryNextRoomPlan(Plan)` | Arrangement 阶段 |
| 触发剧情规则 | `StoryEngine->BroadcastStoryEventWithPayload(...)` | 运行时 |
| 直接执行动作 | `StoryEngine->ExecuteStoryAction(Action, Context)` | 运行时 |
| 写入进度标志 | `StoryEngine->SetStoryFlag(Tag, Scope, bool)` | 运行时 |
| 解锁局外功能 | `MetaProgression->UnlockFeature(FeatureTag)` | 剧情触发时 |
| 强制遭遇战 | `GameMode->StartForcedSurvivalEncounter()` | 终结技获取后 |
| 教程结束（完整） | 见下方三步流程 | 教程最终战结束 |

教程结束三步（必须一起执行）：
```cpp
SaveSubsystem->MarkFirstRunTutorialCompleted();
SaveSubsystem->ClearRunCheckpoint();
GI->ClearRunState();
```

### 外部系统 → 导演 钩子

| 钩子 | 调用方 | 说明 |
| --- | --- | --- |
| `HandleArrangementPhase(GameMode*)` | AYogGameMode | 每次房间生成前 |
| `HandleRewardRuneAdded(Rune, Player)` | 奖励系统 | 玩家拾取符文奖励时 |
| `HandleSacrificeConfirmed(Rune, Player)` | 献祭系统 | 玩家确认献祭时 |
| `HandleScriptedDefeatDeath(GameMode*)` | AYogGameMode | 教程最终战结束时 |
| `HandleFirstBackpackOpened(PC*)` | BackpackScreenWidget | 背包首次打开时 |

完整接口文档：[DirectorInterfaces.md](01_长期系统文档/系统/Story/DirectorInterfaces.md)

### StoryEngine 动作类型（ExecuteStoryAction 可执行）

| 类型 | 效果 |
| --- | --- |
| `SetFlag / ClearFlag` | 写入/清除故事标志位 |
| `ShowInfoHint` | 底部提示条 |
| `ShowTutorialPopup` | 教程弹窗 |
| `UnlockFeature` | 调用 MetaProgression.UnlockFeature |
| `SetQuestTask` | 设置任务目标 |
| `PlayLevelFlow` | 运行 LevelFlowAsset |
| `TriggerStoryEvent` | 广播另一个故事事件 |

---

## 五、常用工作流

### 新建一个符文

1. 读 [RuneLogic_Complete_Guide.md](01_长期系统文档/系统/Rune/RuneLogic_Complete_Guide.md)
2. 在 RuneEditor 创建 `RuneDA`，填写基础字段（Tag、Tier、格子大小）
3. 新建 FlowGraph 资产，在 `BuffFlowComponent` 注册
4. 若有 GE 需求，查 [GAS.md](01_长期系统文档/编码规范/GAS.md)；若有 AN 需求，查 [AnimNotify.md](01_长期系统文档/编码规范/AnimNotify.md)
5. 在 PIE 测试：背包放入符文 → 验证效果触发

### 触发一个剧情事件

```cpp
// 方式 A：走规则匹配（推荐，有条件和触发策略控制）
auto* SE = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>();
SE->BroadcastStoryEventWithPayload(
    Tag_Story_Event_Xxx,   // EventTag
    FGameplayTag::EmptyTag, FGameplayTag::EmptyTag, FGameplayTag::EmptyTag,
    nullptr, GetOwningPlayerController()
);

// 方式 B：直接执行动作（绕过规则，适合导演精确控制）
FStoryAction Action;
Action.ActionType = EStoryActionType::ShowTutorialPopup;
Action.PopupId = FName("tutorial_backpack");
SE->ExecuteStoryAction(Action, Context);
```

### 新建一个敌人

1. 查 [03_配置与编辑器手册/核心配置说明/敌人/](03_配置与编辑器手册/核心配置说明/敌人/)
2. 创建敌人 DA，配置 AI 行为树 + 旋转参数
3. 配置 Combo（查 [EnemyCombo_ConfigGuide.md](01_长期系统文档/系统/Combat/EnemyCombo_ConfigGuide.md)）
4. 若需要 Buff，查 [EnemyBuff_ProductionGuide.md](01_长期系统文档/系统/Rune/EnemyBuff_ProductionGuide.md)
5. 在 MobSpawner 关卡中测试刷怪和攻击行为

### 修改 UI 布局

1. 读 [Widget.md](01_长期系统文档/编码规范/Widget.md) 规范
2. 颜色从项目颜色 DA 取，不硬编码
3. `BindWidget` 变量名与 Designer 中名称完全一致
4. WBP 布局文档格式参考 [WBP_LayoutSpec_Standard.md](Design/UI/WBP_LayoutSpec_Standard.md)
5. PIE 中测试：验证不同分辨率下锚点是否正确

---

## 六、编码规范索引

在 `01_长期系统文档/编码规范/` 下，编写对应资产类型前**必读**对应规范：

| 资产类型 | 规范文档 |
| --- | --- |
| C++ 代码风格 | `编码规范/CppCodingStyle_Guide.md` |
| GAS（GA/GE/AttributeSet） | `编码规范/GAS.md` |
| Widget（WBP） | `编码规范/Widget.md` |
| Material（材质） | `编码规范/Material.md` |
| DataAsset / DA | `编码规范/DataAsset.md` |
| AnimNotify / AN | `编码规范/AnimNotify.md` |
| 数据填写约定 | `编码规范/DataAuthoring.md` |
| FlowGraph 节点红线 | `编码规范/NodeCreation_RedLines.md` |

Tag 规范（单独体系）：

| 查询场景 | 文档 |
| --- | --- |
| Tag 命名空间 + 挂载时机 | [GameplayTag_MasterGuide.md](01_长期系统文档/标签/GameplayTag_MasterGuide.md) |
| GA 5 字段配置规则 | [GA_TagFields_Guide.md](01_长期系统文档/标签/GA_TagFields_Guide.md) |
| Buff/Status Tag 规范 | [Buff_Tag_Spec.md](01_长期系统文档/标签/Buff_Tag_Spec.md) |
| 状态冲突处理 | [StateConflict/](01_长期系统文档/标签/StateConflict/) |

---

## 七、文档维护规则

### 什么时候必须更新文档

| 开发事件 | 必须同步更新的文档 |
| --- | --- |
| 新建 Subsystem / Manager / 核心 Component | `01_长期系统文档/系统/<模块>/` 下建接口文档 |
| 导演新增对某系统的调用 | [DirectorInterfaces.md](01_长期系统文档/系统/Story/DirectorInterfaces.md) |
| 现有接口签名变更 | 对应系统的接口文档 |
| 新增 GameplayTag | [GameplayTag_MasterGuide.md](01_长期系统文档/标签/GameplayTag_MasterGuide.md) |
| 完成一个功能 | [FeatureLog.md](05_项目管理与进度/FeatureLog.md) |

**原则：不要等整理期再补，改代码时同步改文档。**

### 新增文档放哪里

先查 [分类规则.md](00_入口与规范/分类规则.md)，决策树：
- 系统架构/接口 → `01_长期系统文档/系统/<模块>/`
- 编码约定 → `01_长期系统文档/编码规范/`
- 配置操作说明 → `03_配置与编辑器手册/核心配置说明/<类别>/`
- 当前开发方案 → `02_版本计划与需求/核心方案/01_开发方案/<功能>/`
- 过期/旧方案 → `99_归档/`

### 接口文档格式

```markdown
### 接口名（函数签名）

- **调用时机**：Arrangement 前 / 运行时 / 任意时
- **参数**：逐参数说明
- **副作用**：调用后改变哪些状态
- **实现状态**：✓ 已实现 / ✗ 未实现（占位）
- **注意**：特殊约束或已知问题
```

---

*手册完。如需查看某系统的详细接口，点击第三章各模块的「接口文档」链接。*
