> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# 开发方案：系统文档整理与导演接口汇总

## 需求描述

项目已有导演系统（`FirstRunTutorialDirectorSubsystem`）调用各子系统接口驱动故事流程的架构雏形，但：

- 各系统的对外接口未统一汇总，导演需要翻代码才能知道能调什么
- 多个核心系统（存档、元进度、故事引擎）缺乏架构级文档
- 没有全局系统依赖关系图，开发新功能前不清楚改动影响范围

**本轮目标**：纯文档整理（不改代码），补齐无文档系统，建立"导演可调用接口清单"，为后续接口扩充提供决策依据。

---

## 方案设计

### 设计原则

- 只记录现有事实，不设计未来：文档描述已有接口，不预设未实现的
- 接口文档优先于架构文档：从导演视角出发，先记录"能调什么"
- 按使用频率排序：导演最常用的系统优先

### 文档三层结构

```
Layer 1：全局架构图       → Docs/04_开发实现与系统文档/系统/SystemDependencyMap.md
Layer 2：系统接口卡片     → Docs/04_开发实现与系统文档/系统/{System}/*_Architecture.md
Layer 3：导演接口汇总     → Docs/04_开发实现与系统文档/系统/Director/DirectorInterfaces.md  ← 核心输出
```

---

## 实现步骤

### Step 1：补全三个无文档的核心系统

**1a. 存档系统** → `Docs/04_开发实现与系统文档/系统/Save/SaveSubsystem_Architecture.md`

- 职责：多槽位管理（0-2）、Checkpoint 触发/清除、异步写盘并发保护、统计记录、版本迁移
- 关键分组：槽位管理 / 存档点 / 快速存档 / 设置 / 统计
- 数据流：`TriggerCheckpoint` → `PopulateCheckpointFromRunState` → `DoAsyncSave` → 回调

**1b. 元进度系统** → `Docs/04_开发实现与系统文档/系统/MetaProgression/MetaProgression_Architecture.md`

- 职责：局外货币增减、升级节点购买（前置检查+DataTable）、功能解锁、神秘侧等级、打造存档
- 核心约定：MetaProgression 只做业务逻辑，持久化完全委托 SaveSubsystem
- 数据表：`MetaUpgradeNodeTable` / `MetaCurrencyRuleTable`（在 GameInstance BP 中赋值）

**1c. 故事引擎** → `Docs/04_开发实现与系统文档/系统/Story/StoryEngine_Architecture.md`

- 职责：RuleSet 管理、事件广播队列（防重入）、条件求值、动作执行、任务状态、标志位
- 三类标志位作用域：Run（局内）/ Session（本次启动）/ Map（本关）及存活周期
- 与导演的关系：导演是故事引擎的"主要调用者"，不是"被管理者"

---

### Step 2：全局系统依赖关系图

→ `Docs/04_开发实现与系统文档/系统/SystemDependencyMap.md`

内容：

- ASCII 三层架构图（GameInstance / World / Actor-Component）
- 各系统间依赖箭头（调用方向）
- 标注每个系统向导演暴露的调用点

---

### Step 3：导演接口汇总（核心输出）

→ `Docs/04_开发实现与系统文档/系统/Director/DirectorInterfaces.md`

**3a. 已实现的导演接口**

| 接口 | 所在系统 | 调用时机 | 效果 |
|---|---|---|---|
| `GI->SetPendingStoryNextRoomPlan(Plan)` | GameInstance | Arrangement 前 | 覆盖下一关关卡/掉落/Buff |
| `GameMode->StartForcedSurvivalEncounter()` | GameMode | 运行时直接指令 | 强制触发特定遭遇战 |
| `StoryEngine->ExecuteStoryAction()` | StoryEngine | 任意时 | 弹窗/标志位/任务等 |
| `StoryEngine->BroadcastStoryEventWithPayload()` | StoryEngine | 任意时 | 推进叙事规则 |
| `StoryEngine->SetStoryFlag()` | StoryEngine | 任意时 | 设置 Run/Session/Map 标志 |
| `MetaProgression->UnlockFeature()` | MetaProgression | 剧情触发 | 解锁局外功能（不花货币） |
| `SaveSubsystem->MarkFirstRunTutorialCompleted()` | SaveSubsystem | 教程结束 | 标记教程完成并写盘 |

**3b. 当前缺口分析**（导演想控但没有接口的）

| 想控制的效果 | 当前状态 | 评估 |
|---|---|---|
| 刷特定敌人（指定波次） | 只能通过 RoomData 间接控制 | 需要 `GameMode::OverrideNextSpawnWave()` |
| 触发实时对话气泡 | 无对话系统 | 需先设计对话系统 |
| 锁定/解锁传送门 | 无接口 | Portal 上加 `SetLocked(bool)` 即可 |
| 强制指定关卡类型 | `FStoryNextRoomPlan.RoomDataOverride` 已有 | ✓ 已覆盖 |

---

### Step 4：文档规范补充

→ `Docs/00_入口与规范/DocConventions.md`

约定：

- 新系统上线时必须同步写接口卡片（接口名/调用时机/参数/副作用/是否已实现）
- 导演新增调用点时必须同步更新 `DirectorInterfaces.md`

---

## 涉及文件

**新建（6 个纯文档，不改任何代码）**

- `Docs/04_开发实现与系统文档/系统/SystemDependencyMap.md` — 全局系统依赖图
- `Docs/00_入口与规范/DocConventions.md` — 文档维护规范
- `Docs/04_开发实现与系统文档/系统/Save/SaveSubsystem_Architecture.md` — 存档系统架构
- `Docs/04_开发实现与系统文档/系统/MetaProgression/MetaProgression_Architecture.md` — 元进度系统架构
- `Docs/04_开发实现与系统文档/系统/Story/StoryEngine_Architecture.md` — 故事引擎架构
- `Docs/04_开发实现与系统文档/系统/Director/DirectorInterfaces.md` — 导演接口汇总（核心输出）

---

## 潜在风险

- **文档立即过时**：写完后代码若改动未同步，文档失真。Step 4 的规范约束可缓解，但需要开发习惯支撑。
- **BP 层调用点遗漏**：本次只扫描了 Public/*.h，蓝图中可能有额外导演调用点未被发现，初版接口汇总可能不完整。

---

## 已确认决策（2026-05-28）

1. **导演边界**：后续通用剧情复用 StoryEngine RuleSet，不新建 StoryDirectorSubsystem。`FirstRunTutorialDirectorSubsystem` 是教程专属，保持现状。
2. **对话系统**：非近期需求，但是硬性需求——接口汇总中记录为"未来必做"缺口，保留占位。
3. **刷怪接口颗粒度**：需要支持"指定特定敌人"+ "特定时机追加一波"两种场景，接口粒度设计时需同时覆盖。


