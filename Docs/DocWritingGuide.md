# 文档编写规则

> 适用范围：Docs/Design/ 下所有文档  
> 原则：让不同角色的人能快速找到、快速看懂、快速上手

---

## 一、目录结构规则

### 1.1 文件夹分类

**一类文档放一个文件夹**，按系统/主题分，不按读者角色分：

```
Docs/Design/
├── INDEX.md              ← 总索引，所有文档入口
├── DocWritingGuide.md    ← 本文档
├── Tags/                 ← GameplayTag 系统全部文档
├── StateConflict/        ← 状态冲突系统
├── BuffFlow/             ← BuffFlow / 符文系统
├── FeatureConfig/        ← 功能配置指南（策划在编辑器里操作的）
├── Systems/              ← 独立子系统（热度、充能、关卡等）
└── WorkReports/          ← 工作报告 / 阶段总结
```

### 1.2 文件命名规则

```
格式：[系统名]_[文档类型].md

文档类型后缀：
  _Guide          → 使用/配置指南（策划向）
  _Technical      → 技术文档（程序向）
  _Reference      → 参考手册（可查询的索引型）
  _Design         → 设计文档（系统设计思路）
  _WorkReport     → 工作报告

示例：
  StateConflict_Technical.md    ✅
  GA_TagFields_Guide.md         ✅
  Tag_SituationalGuide.md       ✅
  buff系统说明.md                ❌（中文、无类型后缀）
  doc1.md                       ❌（无意义命名）
```

### 1.3 INDEX.md 维护规则

每新增或移动一个文档，必须同步更新 `INDEX.md`：
- 添加对应分类的链接
- 填写一句话描述
- 标注适用人群（策划 / 程序 / 两者）

---

## 二、文档头部规范

每个文档开头必须包含以下 metadata 块：

```markdown
# 文档标题

> 适用范围：XXX  
> 适用人群：策划 / 程序 / 两者  
> 配套文档：[相关文档](路径)（可选）  
> 最后更新：YYYY-MM-DD（WorkReport 类必填，其余可选）
```

---

## 三、策划文档（Guide 类）写作规则

> 目标读者：策划，不需要看代码，在编辑器里操作

### 3.1 结构模板

```
# 标题

> metadata

---

## 概述
一句话说清楚这个文档讲什么、解决什么问题

---

## 核心概念（可选，复杂系统才需要）
用类比/比喻解释，不要用技术术语

---

## 配置步骤 / 使用方法
分步骤，每步操作明确

---

## 示例
完整示例，从头到尾能跑通

---

## 注意事项
| 情况 | 行为 |（表格）

---

## 常见问题
Q & A 格式
```

### 3.2 表格优先原则

**能用表格表达的，不用段落文字**。表格适合：
- 参数说明
- 字段描述
- 对比（正确 vs 错误）
- 情况 → 行为 映射
- 速查表

```markdown
✅ 好的写法：
| 字段 | 类型 | 说明 |
|---|---|---|
| Montage | UAnimMontage | 死亡动画，留空则无死亡动画 |
| DissolveTag | FGameplayTag | 消解粒子 GC Tag，留空则无消解 |

❌ 差的写法：
Montage 字段填死亡动画资产，如果不需要死亡动画可以留空。
DissolveTag 填消解粒子的 GameplayCue Tag，如果留空则不会触发消解特效。
```

### 3.3 类比规则

复杂概念必须附一句类比，放在 `>` 引用块里：

```markdown
> 类比：AbilityData 就是角色的技能配置表，Tag 是行索引，Montage 是这一行的数据。
```

### 3.4 操作步骤写法

分步骤，每步单独一行，**动词开头**，注明在编辑器哪里操作：

```markdown
1. 打开 Content Browser → 找到角色的 `DA_Ability_XXX`
2. 展开 `PassiveMap` → 找到 Key 为 `Action.Dead` 的条目
3. 在 `DissolveGameplayCueTag` 里填写消解 GC Tag
4. 保存，无需重新编译
```

### 3.5 标记符号使用规范

| 符号 | 含义 | 使用场景 |
|---|---|---|
| ✅ | 正确做法 | 对比表格、正误示例 |
| ❌ | 错误做法 | 对比表格、常见错误 |
| ⚠️ | 注意事项 | 容易踩坑的地方 |
| ⭐ | 推荐/重点 | 关键信息强调 |

不要滥用，每页不超过 5 个。

### 3.6 常见问题格式

```markdown
**Q：Task 配置好了但 GA 没有激活？**  
A：检查以下几点：
1. ASC 上是否已 GiveAbility
2. AbilityData 中该 Tag 是否正确注册
3. GA 是否有 ActivationBlockedTags 阻止
```

---

## 四、程序文档（Technical 类）写作规则

> 目标读者：程序，需要看架构、接口、数据流

### 4.1 结构模板

```
# 标题

---

## 架构总览
ASCII 流程图 / 数据流图

---

## 核心机制
关键函数/类的说明，附代码片段

---

## 数据结构
关键数据结构字段说明（用表格）

---

## 接入方式
如何接入这个系统，新增功能时改哪里

---

## 注意事项 / 已知问题
技术细节、边界条件、性能考量
```

### 4.2 架构图规则

**优先用 ASCII 图表达数据流和调用链**，不要用纯文字描述：

```markdown
✅ 好的写法：
死亡事件
  └─ YogCharacterBase::Die()
       └─ SendGameplayEventToActor(Action.Dead)
            └─ GA_Dead::ActivateAbility()
                 ├─ 播放死亡蒙太奇
                 └─ ActivationOwnedTags 挂 Buff.Status.Dead
                      └─ OnTagUpdated()
                           ├─ PauseLogic()（AI 停止）
                           └─ DisableMovement()（移动停止）

❌ 差的写法：
死亡时调用 Die() 函数，Die() 会发送 Action.Dead 事件，GA_Dead 收到后激活，
激活后 Buff.Status.Dead 挂到 ASC，OnTagUpdated 触发，然后停止 AI 和移动。
```

### 4.3 代码块规则

- 用 `cpp` 语言标记
- 只展示关键逻辑，不贴完整函数（完整代码看源文件）
- 重要行加注释

```cpp
// 防递归保护
if (bProcessingConflict) return;

const FStateConflictRule* Rule = ConflictMap.Find(Tag);  // O(1) 查找
if (!Rule) return;

TGuardValue<bool> Guard(bProcessingConflict, true);  // 退出时自动还原
```

### 4.4 接口说明格式

```markdown
### `InitConflictTable()`

**调用时机**：`PostInitializeComponents()` 之后，角色 BeginPlay 期间自动调用  
**作用**：从 DevAssetManager 加载 StateConflictDataAsset，构建 O(1) 查找表  
**注意**：只需调用一次，重复调用会重置表但不会崩溃
```

---

## 五、参考手册（Reference 类）写作规则

> 目标读者：两者，用于查询而非学习

### 5.1 结构要求

- 以**表格为主**，不要段落
- 每条目必须有：名称、类型/位置、一句话说明
- 按字母或使用频率排序
- 可以有示例，但每条示例控制在 3 行内

### 5.2 示例

```markdown
## 节点速查

| 节点名 | 分类 | 说明 |
|---|---|---|
| `BFNode_ApplyGE` | 效果 | 对目标应用 GameplayEffect |
| `BFNode_SendGameplayEvent` | 通信 | 向目标发送 GameplayEvent（触发 GA）|
| `BFNode_WaitGameplayEvent` | 等待 | 阻塞等待某个 GameplayEvent 到达 |
| `BFNode_AddTag` | Tag | 向目标 ASC 添加 Loose Tag |
```

---

## 六、工作报告（WorkReport 类）写作规则

> 目标：记录做了什么、为什么这样做、遗留问题

### 6.1 结构模板

```
# [系统名] 工作报告 YYYY-MM-DD

---

## 本次完成
- 功能 A：一句话说明
- 功能 B：一句话说明

## 关键决策
记录"为什么这样做而不是那样做"，3 年后的自己能看懂

## 遗留问题
- [ ] 问题 A（优先级：高/中/低）
- [ ] 问题 B

## 下次计划
- 计划 A
```

### 6.2 日期格式

文件名用 `YYYYMMDD`（无连字符）：`HeatSystem_WorkReport_20260406.md`  
文档内日期用 `YYYY-MM-DD`：`2026-04-06`

---

## 七、通用格式规范

### 7.1 标题层级

```
# H1   → 文档标题（每文档只有一个）
## H2  → 主要章节（带编号，如"一、二、三"或"1. 2. 3."）
### H3 → 子节
#### H4 → 不推荐超过这一层，太深请拆文档
```

### 7.2 代码/路径引用格式

| 内容 | 格式 | 示例 |
|---|---|---|
| 函数名、变量名 | `反引号` | `InitConflictTable()` |
| 文件路径 | `反引号` | `Config/Tags/BuffTag.ini` |
| Tag | `反引号` | `Buff.Status.Dead` |
| 资产名 | `反引号` | `DA_Base_StateConflict_Initial` |
| 文档链接 | markdown 链接 | `[文档名](相对路径)` |

### 7.3 禁止事项

| 禁止 | 原因 |
|---|---|
| 段落超过 5 行不分段 | 难以扫读 |
| 表格第一列留空 | 不清晰 |
| 用"如上所述"、"如前文"等引用 | 文档顺序可能变化 |
| 文档中直接贴完整 cpp 文件 | 过长，应指向源文件路径 |
| 文档不写 metadata 头 | 找不到关联文档 |
| 新增文档不更新 INDEX.md | 文档孤岛 |

### 7.4 中英混排规范

- 中文和英文/数字之间加空格：`GA_Dead 激活时` ✅，`GA_Dead激活时` ❌
- 专有名词保留英文：`GameplayAbility`、`ASC`、`DataAsset`
- 中文括号用（），英文代码括号用 ()

---

## 八、文档类型对照速查

| 我要写的内容 | 文档类型 | 放哪个文件夹 |
|---|---|---|
| 策划在编辑器里怎么配置 | `_Guide` | `FeatureConfig/` |
| 某个系统的技术架构和接口 | `_Technical` | 对应系统文件夹 |
| 所有节点/Tag/字段的查询表 | `_Reference` | 对应系统文件夹 |
| 系统的设计思路和规则 | `_Design` | 对应系统文件夹 |
| 本周/本次做了什么 | `_WorkReport` | `WorkReports/` |
| Tag 相关的任何内容 | 按上述类型定 | `Tags/` |
| BuffFlow/符文系统 | 按上述类型定 | `BuffFlow/` |
| StateConflict 系统 | 按上述类型定 | `StateConflict/` |
