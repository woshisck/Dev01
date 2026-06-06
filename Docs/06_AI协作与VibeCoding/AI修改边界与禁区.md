---
status: current
type: guide
owner: ai
updated: 2026-06-06
---

# AI 修改边界与禁区

## 禁止事项

- 禁止重置、覆盖或删除用户未要求处理的改动。
- 禁止把归档文档或废弃文档当成当前实现依据。
- 禁止凭猜测修改资产路径、GameplayTag、DataTable、DataAsset 或蓝图引用。
- 禁止绕过现有核心系统另写一套平行实现。
- 禁止为了通过测试而删除测试、降低断言或隐藏错误。
- 禁止在未确认运行时真源前改文案、配置或系统行为。

## 高风险系统

- GAS：遵守 Ability、Effect、Tag、OwnedTag、BlockedTag 的既有规则。
- BuffFlow/Rune：不得新增独立 Buff 执行链。
- SaveSubsystem：存档只能通过唯一持久化入口。
- StoryEngine/Director：剧情触发先确认规则集、事件 Tag 和 Director 调用链。
- UI/CommonUI：输入焦点、手柄导航、Widget 生命周期必须按现有规范处理。

## 必须说明

每次实现完成后，AI 必须说明：

- 改了哪些行为。
- 哪些文件是核心改动。
- 如何验证。
- 哪些风险仍存在。

