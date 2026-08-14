---
status: current
type: guide
owner: production
updated: 2026-08-14
---

# 文档维护规范

> 最后更新：2026-08-14

本文规定项目文档的维护原则，重点面向系统接口文档、导演接口汇总、配置说明和 AI 协作入口的同步要求。

---

## 触发文档更新的时机

| 开发事件 | 必须同步更新的文档 |
| --- | --- |
| 新建 Subsystem / Manager / 核心 Component | 在 `Docs/04_开发实现与系统文档/系统/{System}/` 下建立或更新接口文档 |
| 导演新增对某系统的调用 | `Docs/04_开发实现与系统文档/系统/Story/DirectorInterfaces.md` |
| 现有接口签名变更 | 对应系统的接口文档 |
| 系统新增对另一系统的依赖 | `Docs/04_开发实现与系统文档/系统/SystemDependencyMap.md` |
| 新增 DataAsset / WBP / GameplayTag / Commandlet 配置 | `Docs/03_策划配置与制作手册/` 或对应系统配置说明 |
| AI 辅助代码任务启动 | `Docs/06_AI协作与VibeCoding/AI任务交接模板.md` |

**原则：不要等整理期再补，改代码时同步改文档。**

---

## 接口文档格式（系统接口卡片）

每个对外暴露的接口按以下格式记录：

```text
### 接口名（函数签名）

- **调用时机**：Arrangement 前 / 运行时 / 任意时
- **参数**：逐参数说明
- **副作用**：调用后会改变哪些状态（存档、标志位、Actor 状态等）
- **实现状态**：已实现 / 未实现（占位）
- **注意**：特殊约束或已知问题
```

---

## 导演接口汇总维护规则

[DirectorInterfaces.md](../04_开发实现与系统文档/系统/Story/DirectorInterfaces.md) 是导演系统的唯一接口索引。

- 已实现接口记入“已实现”表。
- 有需求但未实现的接口记入“缺口”表，注明是否为硬性需求。
- 不记录正在开发中的接口；等合入后再写入长期系统文档。

---

## 书写语言 / Writing language

**Write `.md` body text in English by default.** This applies to prose, headings, table cells, and
bullet lists in any newly authored or rewritten document.

Keep the following in Chinese, verbatim:

| Keep in Chinese | Reason |
| --- | --- |
| Folder names (`03_策划配置与制作手册/`) | Renaming would break every existing link |
| Existing document filenames | Same — links and the index depend on them |
| Row labels in [Docs/INDEX.md](../INDEX.md) and folder `README.md` entry tables | The index is scanned by Chinese-reading staff; keep it consistent |
| GameplayTag paths, asset names, DataAsset field names | They are identifiers, not prose |

Additional rules:

- Do **not** mass-translate existing Chinese documents. Convert a document's body to English only when
  you are already rewriting it for other reasons.
- A Chinese filename with an English body is acceptable and expected during the transition.
- When mixing CJK and Latin in the same line, put a space between the character sets
  (`GA_Dead 激活时`, not `GA_Dead激活时`).
- Code comments are English only — see the code style rules in `CLAUDE.md`.

---

## 文档目录约定

```text
Docs/
├── 00_入口与规范/             入口、分类规则、迁移表、缺失引用记录
├── 01_策划需求与版本方案/      策划需求、版本方案、协作接口、验收要求
├── 02_玩法设计与调研/          玩法设计、调研、故事源内容
├── 03_策划配置与制作手册/      引擎内配置、编辑器操作、制作手册
├── 04_开发实现与系统文档/      长期有效的系统、标签、编码规范
│   ├── 编码规范/              C++/GAS/WBP/DA 等工程规范
│   └── 系统/                  Combat、Rune、Story、Save、UI 等系统依据
├── 05_完成记录与任务看板/      任务、完成记录、策划配置清单、验收报告
├── 06_AI协作与VibeCoding/     AI 代码协作规范与任务交接入口
├── 90_自动生成报告/           Commandlet、StoryPipeline、ProductionGraph 输出
├── 98_废弃/                   与当前方向冲突的历史依据
└── 99_归档/                   仅供历史追溯，不作为当前实现依据
```

---

## 不需要文档化的内容

以下内容不写进长期文档，因为有更权威的来源：

- 代码实现细节 -> 直接读代码。
- Git 历史和改动原因 -> commit message。
- 正在开发中的临时方案 -> `01_策划需求与版本方案/` 或 `99_归档/WorkSession/` 中对应记录。
- 已废弃的接口 -> 移入 `98_废弃/` 或在原归档文档顶部标注废弃状态。
