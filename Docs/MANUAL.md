# 星骸降临 开发者手册

> 最后更新：2026-06-06  
> 适用对象：参与本项目的程序、策划、TA，及 AI 助手（Codex/Claude）

## 一、快速上手

### 三步找到你要的文档

| 你想做什么 | 第一步 | 第二步 |
| --- | --- | --- |
| 了解某系统怎么运作 | [01_长期系统文档](01_长期系统文档/README.md) | 进入对应系统 README |
| 知道怎么配置某功能 | [03_配置与编辑器手册](03_配置与编辑器手册/README.md) | 找对应功能子目录 |
| 查当前开发任务 | [TASKS](05_项目管理与进度/PM/TASKS.md) | 查看验收和进度记录 |
| 查当前核心方案 | [02_版本计划与需求](02_版本计划与需求/README.md) | 进入核心方案目录 |
| 做 AI 辅助开发 | [06_AI协作与VibeCoding](06_AI协作与VibeCoding/README.md) | 先读边界、规范和测试要求 |
| 查历史方案 | [99_归档](99_归档/README.md) | 明确它不是当前依据 |
| 查废弃内容 | [98_废弃](98_废弃/README.md) | 不得作为实现依据 |

### 文档目录

```text
Docs/
├── INDEX.md
├── MANUAL.md
├── 00_入口与规范/              文档规则、迁移表、缺失引用
├── 01_长期系统文档/            长期技术文档、编码规范、系统接口
├── 02_版本计划与需求/          当前核心方案、需求拆解、协作接口
├── 03_配置与编辑器手册/        策划/TA 在编辑器中的操作手册
├── 04_调研与玩法设计/          玩法设计、系统草案、调研和剧情源
├── 05_项目管理与进度/          任务、进度、验收、FeatureLog
├── 06_AI协作与VibeCoding/      AI 写代码前的规范和交接入口
├── 90_自动生成报告/            Commandlet、脚本、生产流水线输出
├── 98_废弃/                    与当前方向冲突，不作为依据
└── 99_归档/                    历史过程、旧方案、旧需求
```

## 二、项目架构总览

### 三层运行时架构

```text
GameInstance 层（全局，跨关卡存活）
  ├── UStoryEngineSubsystem
  ├── UFirstRunTutorialDirectorSubsystem
  ├── UYogMetaProgressionSubsystem
  └── UYogSaveSubsystem

GameMode / World 层（单局，切关卡后重建）
  ├── AYogGameMode
  ├── UStoryRuleSetDA
  └── ALevelFlowActor

Character / Component 层（角色，每帧）
  ├── AYogCharacterBase
  ├── UBackpackGridComponent
  └── UBuffFlowComponent
```

完整依赖图：[SystemDependencyMap.md](01_长期系统文档/系统/SystemDependencyMap.md)

## 三、功能模块导航

| 模块 | 当前依据 | 说明 |
| --- | --- | --- |
| 3C / 摄像机 | [Camera](01_长期系统文档/系统/Camera/) | 摄像机、遮挡、相机体积 |
| 战斗 | [Combat](01_长期系统文档/系统/Combat/README.md) | 攻击、闪避、伤害、架势、技能 |
| 符文/卡牌 | [Rune](01_长期系统文档/系统/Rune/README.md) | BuffFlow、RuneDA、CombatDeck、连携 |
| 剧情/教程 | [Story](01_长期系统文档/系统/Story/README.md) | StoryEngine、Director、教程流程 |
| 关卡 | [Level](01_长期系统文档/系统/Level/README.md) | RoomData、传送门、奖励、跨关卡状态 |
| 成长/存档 | [Progression](01_长期系统文档/系统/Progression/README.md) | 元进度、存档、局外升级 |
| UI | [UI](01_长期系统文档/系统/UI/) | Widget、HUD、CommonUI、背包界面 |
| VFX | [VFX](01_长期系统文档/系统/VFX/) | 材质、特效、角色闪白 |
| 武器 | [Weapon](01_长期系统文档/系统/Weapon/) | 武器系统、远程武器、初始卡组 |

## 四、配置与制作入口

| 制作内容 | 文档入口 |
| --- | --- |
| 编辑器工具 | [03_配置与编辑器手册/编辑器](03_配置与编辑器手册/编辑器/README.md) |
| UI/WBP 配置 | [核心配置说明/UI](03_配置与编辑器手册/核心配置说明/UI/) |
| 符文/卡牌配置 | [核心配置说明/符文](03_配置与编辑器手册/核心配置说明/符文/) |
| 战斗配置 | [核心配置说明/战斗](03_配置与编辑器手册/核心配置说明/战斗/) |
| 敌人配置 | [核心配置说明/敌人](03_配置与编辑器手册/核心配置说明/敌人/) |
| 关卡配置 | [核心配置说明/关卡](03_配置与编辑器手册/核心配置说明/关卡/) |
| 剧情制作 | [核心配置说明/剧情](03_配置与编辑器手册/核心配置说明/剧情/) |
| 音频 SOP | [音频](03_配置与编辑器手册/核心配置说明/音频/) |

## 五、AI 开发流程

AI 助手执行代码任务前必须先读：

1. [AI 修改边界与禁区](06_AI协作与VibeCoding/AI修改边界与禁区.md)
2. [引擎版本与项目依据](06_AI协作与VibeCoding/引擎版本与项目依据.md)
3. [AI 代码编写规范](06_AI协作与VibeCoding/AI代码编写规范.md)
4. [测试与验证要求](06_AI协作与VibeCoding/测试与验证要求.md)

任务交接优先使用 [AI 任务交接模板](06_AI协作与VibeCoding/AI任务交接模板.md)。

## 六、文档状态规则

- `current`：当前依据，可用于实现和配置。
- `archived`：历史记录，只用于追溯。
- `deprecated`：废弃内容，不得作为当前依据。

引用归档或废弃文档时，必须在正文说明其状态。

