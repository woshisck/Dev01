# 符文系统 — Rune

> BuffFlow 是本项目符文（Rune）和 Buff 的统一实现体系。  
> "符文"和"Buff"在本项目中均指 RuneDA + BuffFlow，不是两套东西。

## 架构说明

| 文档 | 内容 |
| --- | --- |
| [BuffFlow_Architecture.md](BuffFlow_Architecture.md) | BuffFlow 整体架构：FA/FlowGraph/BackpackGrid 关系 |
| [FA_UniversalArchitecture.md](FA_UniversalArchitecture.md) | FA 节点通用架构（所有符文的 FA 共享基础） |
| [FA_Moonlight_NodeSequences.md](FA_Moonlight_NodeSequences.md) | 月光卡 4 个 FA 的完整节点配置 + 数值表 |
| [YogRuneEditor_Technical.md](YogRuneEditor_Technical.md) | RuneEditor 程序技术文档 |

## 使用指南

| 文档 | 内容 |
| --- | --- |
| [RuneLogic_Complete_Guide.md](RuneLogic_Complete_Guide.md) | **新功能先读** — 完整开发流程 + 决策树 |
| [RuneEditor_UserGuide.md](RuneEditor_UserGuide.md) | RuneEditor 操作手册（策划/TA 用） |
| [BuffFlow_ProgrammerGuide.md](BuffFlow_ProgrammerGuide.md) | 程序实现指南 |
| [RuneMaster_ProductionGuide.md](RuneMaster_ProductionGuide.md) | 符文制作流程（综合） |
| [EnemyBuff_ProductionGuide.md](EnemyBuff_ProductionGuide.md) | 敌人 Buff 制作 |
| [TestRune_*.md](.) | 测试符文说明 |

## 接口参考

| 文档 | 内容 |
| --- | --- |
| [BuffFlow_NodeReference.md](BuffFlow_NodeReference.md) *(或 Usage)* | FA 节点清单 + 用法 |
| [RuneComboTuning_Requirements.md](RuneComboTuning_Requirements.md) | 连携/协同调参需求 |

## 关键规则

- 所有 buff/effect 逻辑必须在 FlowGraph 中可视化实现，不写死在 C++ 里
- 优先用 FA 动态 grant 或纯 GE，不预授予 GA 到角色蓝图
- 符文不允许重复格子；满足重复条件时自动升级（Lv.I×1.0 / II×1.5 / III×2.0）

## 配置手册（策划操作）

见 [03_配置与编辑器手册/编辑器/符文编辑器/](../../../03_配置与编辑器手册/编辑器/符文编辑器/)

## 关联系统

- [../Combat/MeleeCombo_NotifyRune_Guide.md](../Combat/MeleeCombo_NotifyRune_Guide.md) — 近战 AN 钩子与符文联动
- [../Story/](../Story/) — 符文获取可触发剧情事件
