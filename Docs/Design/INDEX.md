# 设计文档索引

> 项目：星骸降临 Dev01  
> 更新：2026-04-18（文档库整理：删除8个过期文件，补录 BackpackUI_StepByStep）  
> 新增文档必须在此更新，参见 [DocWritingGuide.md](DocWritingGuide.md)

---

## Tag 系统（Tags/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [GameplayTag 总体设计指南](Tags/GameplayTag_MasterGuide.md) | 所有命名空间总览、创建决策树、命名规范 | 策划 + 程序 |
| [Buff Tag 规范](Tags/Buff_Tag_Spec.md) | `Buff.*` 命名空间 5 层模型详细规范 | 策划 + 程序 |
| [GA Tag 字段使用指南](Tags/GA_TagFields_Guide.md) | GA Blueprint 里 5 个 Tag 字段的用法和示例 | 策划 |
| [Tag 情景使用指南](Tags/Tag_SituationalGuide.md) | 23 个具体情景，"我想做 X 用哪个 Tag" | 策划 + 程序 |

---

## 状态冲突系统（StateConflict/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [状态冲突规则表 — 配置指南](StateConflict/StateConflict_TagBlock.md) | DA 填表规范、当前规则列表、常见错误 | 策划 |
| [状态冲突系统 — 技术文档](StateConflict/StateConflict_Technical.md) | 架构图、OnTagUpdated 流程、接入方式 | 程序 |

---

## BuffFlow / 符文系统（BuffFlow/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [BuffFlow 架构与计划](BuffFlow/BuffFlow_ArchitectureAndPlan.md) | 系统整体架构设计 | 程序 |
| [BuffFlow 设计指南](BuffFlow/BuffFlow_DesignGuide.md) | 符文/Buff 设计原则 | 策划 |
| [BuffFlow 节点参考](BuffFlow/BuffFlow_NodeReference.md) | 所有 BFNode 节点说明 | 策划 + 程序 |
| [BuffFlow 节点使用指南](BuffFlow/BuffFlow_NodeUsageGuide.md) | 节点具体用法和示例 | 策划 |
| [BuffFlow 程序接入指南](BuffFlow/BuffFlow_ProgrammerGuide.md) | C++ 接入、自定义节点 | 程序 |
| [BuffFlow 符文工作流](BuffFlow/BuffFlow_RuneWorkflow.md) | 符文从设计到实现的完整流程 | 策划 + 程序 |

---

## 功能配置指南（FeatureConfig/）

> 面向策划，无需改代码，直接在编辑器配置即可使用的功能。

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [背包系统蓝图制作指南](FeatureConfig/BackpackSystem_Guide.md) | WBP_BackpackScreen 制作步骤、FallbackLootPool 配置、Tab 键绑定、场景拾取物放置 | 策划 + 程序 |
| [攻击伤害配置指南](FeatureConfig/AttackDamage_ConfigGuide.md) | 配置 EffectContainerMap 使攻击造成伤害，含多段连击示例 | 策划 |
| [行为树攻击任务配置](FeatureConfig/BT_AttackTask_ConfigGuide.md) | `BTTask_ActivateAbilityByTag` 用法、随机攻击、距离判断 | 策划 |
| [敌人连击蒙太奇配置](FeatureConfig/EnemyCombo_ConfigGuide.md) | 多段连击蒙太奇结构 + AN_EnemyComboSection 配置 | 策划 |
| [死亡消解特效配置](FeatureConfig/DeathDissolve_ConfigGuide.md) | GA_Dead + GameplayCue 消解粒子配置 | 策划 |
| [传送门配置指南](FeatureConfig/Portal_ConfigGuide.md) | 编辑器配置 APortal Index / PortalDestinations / RewardPickupClass / NeverOpen BP 实现 | 策划 |
| [关卡 Buff 池配置指南](FeatureConfig/BuffPool_ConfigGuide.md) | 创建 BuffDataAsset + 在 DA_Room 中配置 BuffPool | 策划 |
| [敌人朝向修正配置指南](FeatureConfig/EnemyRotation_ConfigGuide.md) | BTT_RotateCorrect 配置（Interp Speed）+ 转身动画方案设计 | 策划 + 程序 |
| [蒙太奇命中符文配置指南](FeatureConfig/MeleeCombo_NotifyRune_Guide.md) | 在特定连段命中时触发一次性符文效果（FA 结构、AdditionalRuneEffects 配置、Debug 方法） | 策划 + 程序 |
| [韧性系统配置指南](FeatureConfig/PoiseSystem_ConfigGuide.md) | Resilience 属性初始值、动作韧性（ActResilience）、霸体阈值与时长配置 | 策划 + 程序 |
| [GM 调试命令指南](FeatureConfig/GMCommands_Guide.md) | `Yog_*` 控制台命令：热度/阶段/符文/属性/敌人/Debug 打印，含接入方式和常用测试场景 | 程序 + 策划 |
| [符文制作主指南](FeatureConfig/RuneMaster_ProductionGuide.md) | 1001-1021 全量，制作优先级（P0-P3+停用），每符文逻辑层+表现层（GC/Niagara/SFX/浮字）完整规格 | 策划 + 程序 |
| [测试符文制作指南（逻辑层）](FeatureConfig/TestRune_CreationGuide.md) | 1001-1016 详细 FA 流程 + DA 配置 + 测试要点（符文制作主指南的执行参考） | 策划 + 程序 |
| [高感知测试符文设计指南（逻辑层）](FeatureConfig/TestRune_HighPerception_Guide.md) | 1017-1021 详细 FA 流程 + DA 配置，禁用被动数值类设计原则 | 策划 + 程序 |

---

## 角色特效（VFX/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [角色闪光特效系统 — 技术文档](VFX/CharacterFlash_Technical.md) | 命中闪白 / 攻击前闪红 / 热度升阶发光：C++ 接口、材质参数、蓝图配置清单 | 程序 + 策划 |
| [角色特效制作规范](VFX/VFX_CreationGuide.md) | 新建特效标准流程、材质模板 HLSL、颜色约定、调参习惯；含 .ush 共享文件说明 | 程序 |

---

## UI 制作（UI/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [液态玻璃框技术文档](UI/GlassFrame_Technical.md) | WBP_GlassFrame 层级、材质 HLSL（.ush include 体系）、C++ 接口、各场景参数推荐值 | 程序 + 策划 |
| [背包 UI 零基础制作手册](UI/BackpackUI_StepByStep.md) | WBP_BackpackScreen / WBP_RuneInfoCard 蓝图搭建步骤，适合不熟悉 UE5 的人 | 策划（零基础） |
| [新手引导图像生成提示词](UI/Tutorial_ImagePrompts.md) | Nano Banana 图像提示词，覆盖引导全节点 UI 概念图 | 策划 |

---

## 子系统（Systems/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [游戏主循环设计](Systems/MainLoop_Design.md) | 核心循环流程、房间类型、奖励系统、章节结构、局外成长 | 策划 + 程序 |
| [攻击伤害系统 — 设计说明](Systems/AttackDamage_Design.md) | 攻击判定流程、伤害容器设计原理、与其他系统关系 | 策划 |
| [攻击伤害系统 — 技术文档](Systems/AttackDamage_Technical.md) | 架构图、核心函数、CDO 问题、改造方案（移 GA / C++ TargetType） | 程序 |
| [背包与符文激活系统 — 技术文档](Systems/BackpackSystem_Technical.md) | 架构图、数据结构、三选一拾取流程、热度激活流程、C++ 接口说明 | 程序 |
| [热度系统设计](Systems/HeatSystem_Design.md) | 热度阶段机制、衰减规则、符文联动 | 策划 + 程序 |
| [充能系统指南](Systems/SkillCharge_Guide.md) | SkillChargeComponent 配置和使用 | 策划 + 程序 |
| [关卡系统配置指南](Systems/LevelSystem_ConfigGuide.md) | DA_Room / DA_Campaign 配置（敌人池 / 难度 / MobSpawner 白名单 / 传送门）| 策划 |
| [关卡系统技术文档](Systems/LevelSystem_ProgrammerDoc.md) | 波次生成算法 / 补刷 / 错开刷新 / 切关流程 / Timer 汇总 | 程序 |
| [传送门与关卡奖励系统设计](Systems/Portal_Design.md) | 传送门多分支切关 + 奖励拾取物设计原理（含 NeverOpen + 按 E 拾取）| 策划 + 程序 |
| [跨关状态持久化技术文档](Systems/CrossLevelState_Technical.md) | FRunState 数据流、存储/恢复流程、调试方法 | 程序 |
| [冲刺（Dash）系统](Systems/Dash_Design.md) | 方向/旋转/无敌帧/穿透/碰撞通道/障碍检测/扩展规划（GA_PlayerDash C++） | 策划 + 程序 |
| [相机管理系统](Systems/Camera_Design.md) | 前瞻跟随 / 冲刺 1:1 / 战斗感知偏移 / 多边形边界约束 / 手柄输入偏移 / 震动事件 | 策划 + 程序 |
| [武器系统技术文档](Systems/WeaponSystem_Technical.md) | WeaponSpawner 拾取交互 / WeaponInstance 热度发光 / 切关恢复路径 / Overlay 材质配置 | 程序 |
| [新手引导系统设计](Systems/Tutorial_Design.md) | 8 步引导流程 / 武器浮窗 / 战斗锁定背包 / 热度预览切换 / 引导符文池 | 策划 + 程序 |
| [目标用户与玩家声音调研](Systems/PlayerResearch_Design.md) | 核心用户画像 / 玩家诉求 / 竞品分析 / 设计建议 / 市场估算 | 策划 |

---

## 玩家调研（PlayerResearch/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [玩家调研报告 v0.1（4.15）](PlayerResearch/PlayerResearch_Report_v0.1_20260415.md) | 调研方法 / 访谈提纲 / 假设验证标准 / 执行计划 | 策划 |
| [玩家调研问卷 v0.1（4.15）](PlayerResearch/PlayerResearch_Questionnaire_v0.1_20260415.md) | 完整在线问卷题目（44 题，分场景使用）| 策划 |
| [玩家调研 Q&A 题库 v0.2（4.15）](PlayerResearch/PlayerResearch_QA_20260415.md) | 热度/背包/动作/画像/可用性共 48 题问答（主持人预演用）| 策划 |

---

## 工作报告（WorkReports/）

> 功能完成记录，提供决策背景和实现细节。Bug 定位请优先查 [FeatureLog.md](../../FeatureLog.md)。

**当前进展 / 路线图（最新版）**

| 文档 | 内容 |
|---|---|
| [当前进展 2026-04-14](WorkReports/CurrentProgress_20260414.md) | 背包/符文升级系统完成，本阶段目标速览 |
| [开发路线图 2026-04-16](WorkReports/DevRoadmap_20260416.md) | 感知强化→内容扩充→深度构建三阶段计划（最新）|

**实现记录（按时间）**

| 文档 | 内容 |
|---|---|
| [热度系统 2026-04-06](WorkReports/HeatSystem_WorkReport_20260406.md) | 热度系统实现阶段总结 |
| [近战攻击 C++ 迁移 2026-04-10](WorkReports/MeleeAttack_CppMigration_20260410.md) | GA_MeleeAttack / TargetType_Melee / 连击链 / 命中框调试迁移到 C++ |
| [主循环工作报告 2026-04-10](WorkReports/MainLoop_WorkReport_20260410b.md) | 传送门/跨关状态完成记录、架构变更、下阶段计划 |
| [刷怪系统迭代 2026-04-11](WorkReports/SpawnSystem_WorkReport_20260411.md) | MobSpawner 白名单 / 类型上限 / 补刷 / 错开 / 计时触发 / 难度迁移 |
| [系统细化与交互优化 2026-04-11](WorkReports/SystemPolish_WorkReport_20260411.md) | 传送门 NeverOpen / 按 E 拾取 / 死亡碰撞清除 / 敌人朝向修复 |
| [商业计划书分析 2026-04-11](WorkReports/BPAnalysis_20260411.md) | 差异化分析 / 问题查漏 / 改进建议 / 吸引力评估 |
| [冲刺系统 + 近战清理 2026-04-12](WorkReports/Dash_MeleeCleanup_WorkReport_20260412.md) | GA_PlayerDash C++ 实现、越障算法、根运动位移、ANS_AttackRotate |
| [Bug 修复与冲刺重构 2026-04-12](WorkReports/BugFix_And_Dash_WorkReport_20260412.md) | AbilityData PIE 污染根治 / 冲刺根运动驱动 + 越障 / Debug 工具 |
| [刷怪系统扩展 2026-04-13](WorkReports/SpawnSystem_EnemyBuff_WorkReport_20260413.md) | FBuffEntry / 敌人专属 Buff 池 / MaxEnemiesPerWave / 主城房间 / 奖励配置 |
| [近战HitBox修复 + 冲刺越障重构 2026-04-13](WorkReports/MeleeAndDash_WorkReport_20260413.md) | Annulus InnerR补偿 / 冲刺步进越障算法 / WorldDynamic+Pawn通道 |
| [背包与符文激活系统 2026-04-14](WorkReports/BackpackSystem_WorkReport_20260414.md) | BackpackScreenWidget / 符文移动 / 自动入格 / FallbackLootPool |
| [符文升级系统 2026-04-14](WorkReports/RuneUpgrade_WorkReport_20260414.md) | 升级决策分析 / Lv.I→II→III 倍率 / NotifyRuneUpgraded / 满级奖励池过滤 |
| [相机系统 + 背包UI手柄适配 2026-04-16](WorkReports/CameraAndUI_WorkReport_20260416.md) | AYogCameraPawn 6状态相机 / 手柄背包操作 / 战斗日志 / 蓝图配置任务清单 |
| [CommonUI UI重构 + 背包金币系统 2026-04-16](WorkReports/UICommonUI_WorkReport_20260416.md) | BackpackScreenWidget/LootSelectionWidget迁移CommonUI / Gold迁移BackpackGridComponent |

---

## 文档规范

| 文档 | 内容 |
|---|---|
| [文档编写规则](DocWritingGuide.md) | 目录结构、命名规范、策划/程序文档写法 |

---

## 快速定位

**策划：我想……**
- 了解游戏整体循环设计 → [游戏主循环设计](Systems/MainLoop_Design.md)
- 了解目标用户和玩家诉求 → [目标用户与玩家声音调研](Systems/PlayerResearch_Design.md)
- 查看调研方法和访谈提纲 → [玩家调研报告 v0.1](PlayerResearch/PlayerResearch_Report_v0.1_20260415.md)
- 找问卷题目 / 访谈 Q&A → [问卷](PlayerResearch/PlayerResearch_Questionnaire_v0.1_20260415.md) · [Q&A 题库](PlayerResearch/PlayerResearch_QA_20260415.md)
- 查看当前开发进度 → [当前进展 2026-04-14](WorkReports/CurrentProgress_20260414.md)
- 查看后续开发计划 → [开发路线图 2026-04-16](WorkReports/DevRoadmap_20260416.md)
- 确认符文制作优先级 → [符文制作主指南](FeatureConfig/RuneMaster_ProductionGuide.md)
- 制作某个符文的详细 FA → [测试符文制作指南（1001-1016）](FeatureConfig/TestRune_CreationGuide.md) · [高感知符文（1017-1021）](FeatureConfig/TestRune_HighPerception_Guide.md)
- 配置攻击造成伤害 → [攻击伤害配置指南](FeatureConfig/AttackDamage_ConfigGuide.md)
- 攻击伤害的设计原理 → [攻击伤害系统设计说明](Systems/AttackDamage_Design.md)
- 配置敌人攻击行为 → [行为树攻击任务配置](FeatureConfig/BT_AttackTask_ConfigGuide.md)
- 配置死亡消解 → [死亡消解特效配置](FeatureConfig/DeathDissolve_ConfigGuide.md)
- 配置多段连击 → [敌人连击蒙太奇配置](FeatureConfig/EnemyCombo_ConfigGuide.md)
- 让某段攻击命中时触发击退/燃烧等效果 → [蒙太奇命中符文配置指南](FeatureConfig/MeleeCombo_NotifyRune_Guide.md)
- 配置敌人朝向修正 / 转身动画方案 → [敌人朝向修正配置指南](FeatureConfig/EnemyRotation_ConfigGuide.md)
- 不知道用什么 Tag → [Tag 情景使用指南](Tags/Tag_SituationalGuide.md)
- GA 里怎么填 Tag → [GA Tag 字段使用指南](Tags/GA_TagFields_Guide.md)
- 配置状态冲突规则 → [状态冲突规则表](StateConflict/StateConflict_TagBlock.md)
- 设计新符文 → [BuffFlow 符文工作流](BuffFlow/BuffFlow_RuneWorkflow.md)
- 配置传送门（含 NeverOpen 和按 E 拾取）→ [传送门配置指南](FeatureConfig/Portal_ConfigGuide.md)
- 制作背包 UI（UE5 零基础）→ [背包 UI 零基础制作手册](UI/BackpackUI_StepByStep.md)
- 制作背包 UI / 配置三选一符文池 → [背包系统蓝图制作指南](FeatureConfig/BackpackSystem_Guide.md)
- 配置关卡 Buff → [关卡 Buff 池配置指南](FeatureConfig/BuffPool_ConfigGuide.md)

**程序：我想……**
- 了解开发任务优先级 → [开发路线图 2026-04-16](WorkReports/DevRoadmap_20260416.md)
- 了解攻击伤害架构及改造方案 → [攻击伤害系统技术文档](Systems/AttackDamage_Technical.md)
- 了解 Tag 架构 → [GameplayTag 总体设计指南](Tags/GameplayTag_MasterGuide.md)
- 了解 StateConflict 实现 → [状态冲突系统技术文档](StateConflict/StateConflict_Technical.md)
- 接入 BuffFlow → [BuffFlow 程序接入指南](BuffFlow/BuffFlow_ProgrammerGuide.md)
- 了解背包系统架构和接口 → [背包与符文激活系统技术文档](Systems/BackpackSystem_Technical.md)
- 写新文档 → [文档编写规则](DocWritingGuide.md)
