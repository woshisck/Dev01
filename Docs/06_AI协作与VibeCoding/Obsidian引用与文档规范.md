---
status: current
type: guide
owner: ai
updated: 2026-06-06
---

# Obsidian 引用与文档规范

## 链接格式

优先使用相对 Markdown 链接：

```md
[文档名](../04_开发实现与系统文档/系统/Rune/README.md)
```

可以使用 Obsidian Backlinks、Outgoing Links 和 Graph 查看关系，但正文链接保持 GitHub 和编辑器兼容。

## 状态标记

关键入口文档建议使用 YAML frontmatter：

```yaml
---
status: current
type: guide
owner: engineering
updated: 2026-06-06
---
```

状态含义：

- `current`：当前依据。
- `archived`：历史记录，仅追溯。
- `deprecated`：废弃内容，不作为依据。

## 移动文档必须做

1. 更新 [路径迁移表](../00_入口与规范/路径迁移表.md)。
2. 修复所有能自动确定的新路径链接。
3. 无法确定的链接写入 [缺失引用记录](../00_入口与规范/缺失引用记录.md)。
4. 如果文档被归档或废弃，在正文开头标注状态。

