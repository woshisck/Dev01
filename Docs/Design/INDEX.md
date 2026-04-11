# 设计文档索引

> 项目：星骸降临 Dev01  
> 更新：2026-04-11（新增：传送门 NeverOpen / 按 E 拾取 / 死亡碰撞 / 系统细化工作报告 / 进展路线图更新 / 玩家调研 / 商业计划分析）  
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
| [攻击伤害配置指南](FeatureConfig/AttackDamage_ConfigGuide.md) | 配置 EffectContainerMap 使攻击造成伤害，含多段连击示例 | 策划 |
| [行为树攻击任务配置](FeatureConfig/BT_AttackTask_ConfigGuide.md) | `BTTask_ActivateAbilityByTag` 用法、随机攻击、距离判断 | 策划 |
| [敌人连击蒙太奇配置](FeatureConfig/EnemyCombo_ConfigGuide.md) | 多段连击蒙太奇结构 + AN_EnemyComboSection 配置 | 策划 |
| [死亡消解特效配置](FeatureConfig/DeathDissolve_ConfigGuide.md) | GA_Dead + GameplayCue 消解粒子配置 | 策划 |
| [测试符文创建指南](FeatureConfig/TestRune_CreationGuide.md) | 快速创建可测试符文的完整流程 | 策划 |
| [传送门配置指南](FeatureConfig/Portal_ConfigGuide.md) | 编辑器配置 APortal Index / PortalDestinations / RewardPickupClass / NeverOpen BP 实现 | 策划 |
| [关卡 Buff 池配置指南](FeatureConfig/BuffPool_ConfigGuide.md) | 创建 BuffDataAsset + 在 DA_Room 中配置 BuffPool | 策划 |
| [敌人朝向修正配置指南](FeatureConfig/EnemyRotation_ConfigGuide.md) | BTT_RotateCorrect 配置（Interp Speed）+ 转身动画方案设计 | 策划 + 程序 |

---

## 子系统（Systems/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [游戏主循环设计](Systems/MainLoop_Design.md) | 核心循环流程、房间类型、奖励系统、章节结构、局外成长 | 策划 + 程序 |
| [攻击伤害系统 — 设计说明](Systems/AttackDamage_Design.md) | 攻击判定流程、伤害容器设计原理、与其他系统关系 | 策划 |
| [攻击伤害系统 — 技术文档](Systems/AttackDamage_Technical.md) | 架构图、核心函数、CDO 问题、改造方案（移 GA / C++ TargetType） | 程序 |
| [热度系统设计](Systems/HeatSystem_Design.md) | 热度阶段机制、衰减规则、符文联动 | 策划 + 程序 |
| [充能系统指南](Systems/SkillCharge_Guide.md) | SkillChargeComponent 配置和使用 | 策划 + 程序 |
| [关卡系统配置指南](Systems/LevelSystem_ConfigGuide.md) | DA_Room / DA_Campaign 配置（敌人池 / 难度 / MobSpawner 白名单 / 传送门）| 策划 |
| [关卡系统技术文档](Systems/LevelSystem_ProgrammerDoc.md) | 波次生成算法 / 补刷 / 错开刷新 / 切关流程 / Timer 汇总 | 程序 |
| [传送门与关卡奖励系统设计](Systems/Portal_Design.md) | 传送门多分支切关 + 奖励拾取物设计原理（含 NeverOpen + 按 E 拾取）| 策划 + 程序 |
| [跨关状态持久化技术文档](Systems/CrossLevelState_Technical.md) | FRunState 数据流、存储/恢复流程、调试方法 | 程序 |
| [目标用户与玩家声音调研](Systems/PlayerResearch_Design.md) | 核心用户画像 / 玩家诉求 / 竞品分析 / 设计建议 / 市场估算 | 策划 |

---

## 工作报告（WorkReports/）

| 文档 | 内容 |
|---|---|
| [近战攻击 C++ 迁移 2026-04-10](WorkReports/MeleeAttack_CppMigration_20260410.md) | GA_MeleeAttack / TargetType_Melee / 连击链 / 命中框调试迁移到 C++ 的完整记录 |
| [当前进展 2026-04-10](WorkReports/CurrentProgress_20260410.md) | 项目阶段概述、已完成 / 未完成功能速览、本周目标 |
| [开发路线图 2026-04-10](WorkReports/DevRoadmap_20260410.md) | 详细任务规划（P0-P3）、各任务步骤、里程碑时间线 |
| [主循环开发状态 2026-04-10](WorkReports/MainLoop_WorkReport_20260410.md) | 主循环实现进度、已确认设计决策、设计遗漏分析 |
| [主循环工作报告 2026-04-10（第二版）](WorkReports/MainLoop_WorkReport_20260410b.md) | 传送门/跨关状态完成记录、架构变更、下阶段计划 |
| [刷怪系统迭代 2026-04-11](WorkReports/SpawnSystem_WorkReport_20260411.md) | MobSpawner 白名单 / 类型上限 / 补刷 / 错开 / 计时触发 / 难度迁移 |
| [系统细化与交互优化 2026-04-11](WorkReports/SystemPolish_WorkReport_20260411.md) | 传送门 NeverOpen / 按 E 拾取 / 死亡碰撞清除 / 敌人朝向修复 |
| [当前进展 2026-04-11](WorkReports/CurrentProgress_20260411.md) | 项目阶段概述、已完成 / 未完成功能速览、本周目标（最新）|
| [开发路线图 2026-04-11](WorkReports/DevRoadmap_20260411.md) | 详细任务规划（P0-P3）、各任务步骤、里程碑时间线（最新）|
| [商业计划书分析 2026-04-11](WorkReports/BPAnalysis_20260411.md) | 差异化分析 / 问题查漏 / 改进建议 / 吸引力评估 |
| [热度系统工作报告 2026-04-06](WorkReports/HeatSystem_WorkReport_20260406.md) | 热度系统实现阶段总结 |

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
- 查看当前开发进度和优先级 → [当前进展 2026-04-11](WorkReports/CurrentProgress_20260411.md)
- 配置攻击造成伤害 → [攻击伤害配置指南](FeatureConfig/AttackDamage_ConfigGuide.md)
- 攻击伤害的设计原理 → [攻击伤害系统设计说明](Systems/AttackDamage_Design.md)
- 配置敌人攻击行为 → [行为树攻击任务配置](FeatureConfig/BT_AttackTask_ConfigGuide.md)
- 配置死亡消解 → [死亡消解特效配置](FeatureConfig/DeathDissolve_ConfigGuide.md)
- 配置多段连击 → [敌人连击蒙太奇配置](FeatureConfig/EnemyCombo_ConfigGuide.md)
- 配置敌人朝向修正 / 转身动画方案 → [敌人朝向修正配置指南](FeatureConfig/EnemyRotation_ConfigGuide.md)
- 不知道用什么 Tag → [Tag 情景使用指南](Tags/Tag_SituationalGuide.md)
- GA 里怎么填 Tag → [GA Tag 字段使用指南](Tags/GA_TagFields_Guide.md)
- 配置状态冲突规则 → [状态冲突规则表](StateConflict/StateConflict_TagBlock.md)
- 设计新符文 → [BuffFlow 符文工作流](BuffFlow/BuffFlow_RuneWorkflow.md)
- 配置传送门（含 NeverOpen 和按 E 拾取）→ [传送门配置指南](FeatureConfig/Portal_ConfigGuide.md)
- 配置关卡 Buff → [关卡 Buff 池配置指南](FeatureConfig/BuffPool_ConfigGuide.md)

**程序：我想……**
- 了解开发任务优先级 → [开发路线图 2026-04-11](WorkReports/DevRoadmap_20260411.md)
- 了解攻击伤害架构及改造方案 → [攻击伤害系统技术文档](Systems/AttackDamage_Technical.md)
- 了解 Tag 架构 → [GameplayTag 总体设计指南](Tags/GameplayTag_MasterGuide.md)
- 了解 StateConflict 实现 → [状态冲突系统技术文档](StateConflict/StateConflict_Technical.md)
- 接入 BuffFlow → [BuffFlow 程序接入指南](BuffFlow/BuffFlow_ProgrammerGuide.md)
- 写新文档 → [文档编写规则](DocWritingGuide.md)
