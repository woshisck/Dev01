# 待定设计决策

> 此文件记录已暂定但需要后续重新评审的设计决策。
> 每条记录暂定方案 + 触发条件 + 评审责任人。

---

## DEC-001 — 背包详情卡的通用效果子窗默认展开

**当前决策**：在背包（`WBP_BackpackScreen`）中点开符文 → `RuneInfoCard` 的 `GenericEffectList` **默认展开**显示。

**待重新考虑的原因**：
- 背包符文较多时，每张卡都展开右侧通用效果小窗会占用屏幕空间
- 玩家是否真的每次都需要看到通用效果说明？还是只在第一次/学习阶段需要？
- 默认展开 vs 默认折叠（按某键展开）vs 仅鼠标悬停展开，三种方案各有取舍

**触发评审条件**（满足任一）：
- 玩家测试反馈"信息过载"或"看不清符文"
- 背包同时显示多个 RuneInfoCard 的场景出现（目前只 1 张）
- 通用效果库扩展到 10+ 种后

**关联代码**：
- [URuneInfoCardWidget::SetGenericEffectsExpanded](../../Source/DevKit/Public/UI/RuneInfoCardWidget.h)
- 默认值：`bGenericEffectsExpanded = true`

**评审责任人**：龚正昂

**记录日期**：2026-04-26

---

---

## DEC-002 — ~~YogHUD::Tick 暂停 fade 完成后早返回挡 Portal/Blackout 更新~~ ✅ 已修复

**状态**：2026-04-26 修复 — `TickPortalPreview` / `TickBlackoutFade` 已移到 `AYogHUD::Tick` 顶部，所有早返回之前。Portal 引导和 Blackout 在常规非暂停态下也能更新。

---

## 模板（新增决策时复制）

```markdown
## DEC-XXX — [简短描述]

**当前决策**：

**待重新考虑的原因**：

**触发评审条件**：

**关联代码/文档**：

**评审责任人**：

**记录日期**：YYYY-MM-DD
```
