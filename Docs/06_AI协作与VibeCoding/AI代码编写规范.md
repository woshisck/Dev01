---
status: current
type: guide
owner: ai
updated: 2026-06-06
---

# AI 代码编写规范

## 基本原则

- 修改前先追踪真实运行路径，不凭文档标题、资产名或猜测判断来源。
- 优先使用项目已有架构，不新增平行系统。
- 代码改动必须配套验证方式：编译、自动化测试、PIE 检查或明确的人工验收步骤。
- 不把 `99_归档/` 或 `98_废弃/` 文档当作当前实现依据。
- 不修改无关文件，不重置用户已有改动。

## 必读规范

- [C++ Coding Style Guide](../01_长期系统文档/编码规范/CppCodingStyle_Guide.md)
- [GAS 规范](../01_长期系统文档/编码规范/GAS.md)
- [Widget 规范](../01_长期系统文档/编码规范/Widget.md)
- [DataAsset 规范](../01_长期系统文档/编码规范/DataAsset.md)
- [AnimNotify 规范](../01_长期系统文档/编码规范/AnimNotify.md)
- [GameplayTag 总指南](../01_长期系统文档/标签/GameplayTag_MasterGuide.md)

## 系统优先入口

- Buff、符文、卡牌效果：先查 [Rune 系统](../01_长期系统文档/系统/Rune/README.md)。
- 剧情、教程、规则触发：先查 [Story 系统](../01_长期系统文档/系统/Story/README.md)。
- 关卡、房间、传送门：先查 [Level 系统](../01_长期系统文档/系统/Level/README.md)。
- UI、HUD、WBP：先查 [UI 系统](../01_长期系统文档/系统/UI/) 和 Widget 规范。
- 存档、局外成长：先查 [Progression 系统](../01_长期系统文档/系统/Progression/README.md)。

## 代码提交前检查

- 是否确认运行时入口和资产来源。
- 是否遵守既有命名、Tag、GAS、Widget、DataAsset 规范。
- 是否避免了无关重构。
- 是否更新了相关文档。
- 是否给出可复现验证命令或测试步骤。

