# 设计文档索引

> 项目：星骸降临 Dev01  
> 更新：2026-04-10  
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
| [行为树攻击任务配置](FeatureConfig/BT_AttackTask_ConfigGuide.md) | `BTTask_ActivateAbilityByTag` 用法、随机攻击、距离判断 | 策划 |
| [敌人连击蒙太奇配置](FeatureConfig/EnemyCombo_ConfigGuide.md) | 多段连击蒙太奇结构 + AN_EnemyComboSection 配置 | 策划 |
| [死亡消解特效配置](FeatureConfig/DeathDissolve_ConfigGuide.md) | GA_Dead + GameplayCue 消解粒子配置 | 策划 |
| [测试符文创建指南](FeatureConfig/TestRune_CreationGuide.md) | 快速创建可测试符文的完整流程 | 策划 |

---

## 子系统（Systems/）

| 文档 | 内容 | 适用人群 |
|---|---|---|
| [热度系统设计](Systems/HeatSystem_Design.md) | 热度阶段机制、衰减规则、符文联动 | 策划 + 程序 |
| [充能系统指南](Systems/SkillCharge_Guide.md) | SkillChargeComponent 配置和使用 | 策划 + 程序 |
| [关卡系统配置指南](Systems/LevelSystem_ConfigGuide.md) | 关卡/波次配置 | 策划 |
| [关卡系统技术文档](Systems/LevelSystem_ProgrammerDoc.md) | 关卡系统程序实现 | 程序 |

---

## 工作报告（WorkReports/）

| 文档 | 内容 |
|---|---|
| [热度系统工作报告 2026-04-06](WorkReports/HeatSystem_WorkReport_20260406.md) | 热度系统实现阶段总结 |

---

## 文档规范

| 文档 | 内容 |
|---|---|
| [文档编写规则](DocWritingGuide.md) | 目录结构、命名规范、策划/程序文档写法 |

---

## 快速定位

**策划：我想……**
- 配置敌人攻击 → [行为树攻击任务配置](FeatureConfig/BT_AttackTask_ConfigGuide.md)
- 配置死亡消解 → [死亡消解特效配置](FeatureConfig/DeathDissolve_ConfigGuide.md)
- 配置多段连击 → [敌人连击蒙太奇配置](FeatureConfig/EnemyCombo_ConfigGuide.md)
- 不知道用什么 Tag → [Tag 情景使用指南](Tags/Tag_SituationalGuide.md)
- GA 里怎么填 Tag → [GA Tag 字段使用指南](Tags/GA_TagFields_Guide.md)
- 配置状态冲突规则 → [状态冲突规则表](StateConflict/StateConflict_TagBlock.md)
- 设计新符文 → [BuffFlow 符文工作流](BuffFlow/BuffFlow_RuneWorkflow.md)

**程序：我想……**
- 了解 Tag 架构 → [GameplayTag 总体设计指南](Tags/GameplayTag_MasterGuide.md)
- 了解 StateConflict 实现 → [状态冲突系统技术文档](StateConflict/StateConflict_Technical.md)
- 接入 BuffFlow → [BuffFlow 程序接入指南](BuffFlow/BuffFlow_ProgrammerGuide.md)
- 写新文档 → [文档编写规则](DocWritingGuide.md)
