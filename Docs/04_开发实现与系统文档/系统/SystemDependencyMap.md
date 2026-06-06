# 系统依赖关系图

> 最后更新：2026-05-28
> 依据：Public/*.h 静态扫描 + 运行时调用链追踪

---

## 三层架构总览

```
╔══════════════════════════════════════════════════════════════════╗
║                   GAME INSTANCE 层（跨局全局）                    ║
║                                                                  ║
║  YogGameInstanceBase  ──────────────── 所有 Subsystem 宿主        ║
║  │                                                               ║
║  ├─ YogSaveSubsystem          存档读写唯一入口                    ║
║  │    └─ YogSaveGame          存档数据对象                        ║
║  │    └─ YogSettingsSave      全局设置存档                        ║
║  │                                                               ║
║  ├─ StoryEngineSubsystem      故事规则 / 事件 / 标志位             ║
║  │    └─ StoryEventManager    事件注册与触发                       ║
║  │    └─ StoryEncounterRuntimeSubsystem  遭遇图执行               ║
║  │                                                               ║
║  ├─ FirstRunTutorialDirectorSubsystem   教程导演（教程专属）       ║
║  │                                                               ║
║  ├─ UTutorialManager          弹窗/教程步骤管理                   ║
║  │                                                               ║
║  └─ YogMetaProgressionSubsystem  局外货币 / 升级节点 / 功能解锁   ║
║         └─ (持久化委托 SaveSubsystem)                            ║
╚══════════════════════════════════════════════════════════════════╝
                           │ 新局开始（关卡加载）
                           ▼
╔══════════════════════════════════════════════════════════════════╗
║                   WORLD / GAMEMODE 层（单局生命周期）             ║
║                                                                  ║
║  AYogGameMode                                                    ║
║  │  关卡流程：房间生成 → 刷怪 → 奖励发放 → 传送门                ║
║  │  读取 FStoryNextRoomPlan ◄── 导演写入                         ║
║  │                                                               ║
║  └─ YogWorldSubsystem         关卡流媒体加载 / LevelScript 管理  ║
╚══════════════════════════════════════════════════════════════════╝
                           │ Actor Spawn
                           ▼
╔══════════════════════════════════════════════════════════════════╗
║                   CHARACTER / COMPONENT 层（Actor 生命周期）      ║
║                                                                  ║
║  APlayerCharacterBase                                            ║
║  ├─ YogAbilitySystemComponent   能力激活枢纽                     ║
║  ├─ BuffFlowComponent           符文/Buff 执行引擎                ║
║  ├─ BackpackGridComponent       背包 + 热度                      ║
║  ├─ CombatDeckComponent         战斗卡组                         ║
║  ├─ PlayerActiveSkillComponent  主动技能                         ║
║  └─ SacrificeRuneComponent      献祭符文                         ║
║                                                                  ║
║  AEnemyCharacterBase                                             ║
║  ├─ YogAbilitySystemComponent                                    ║
║  └─ BuffFlowComponent                                            ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 主要依赖方向（A → B 表示 A 调用 B）

### GameInstance 层内部

| 调用方 | 被调用方 | 说明 |
| --- | --- | --- |
| FirstRunTutorialDirectorSubsystem | StoryEngineSubsystem | 广播故事事件、执行 Action |
| FirstRunTutorialDirectorSubsystem | YogGameInstanceBase | 写入 PendingStoryNextRoomPlan |
| FirstRunTutorialDirectorSubsystem | YogSaveSubsystem | 标记教程完成 |
| FirstRunTutorialDirectorSubsystem | AYogGameMode | 调用 StartForcedSurvivalEncounter |
| StoryEventManager | UTutorialManager | 触发教程弹窗 |
| YogMetaProgressionSubsystem | YogSaveSubsystem | 持久化委托 |

### GameInstance → World

| 调用方 | 被调用方 | 说明 |
| --- | --- | --- |
| AYogGameMode | YogGameInstanceBase | 读取 PendingStoryNextRoomPlan、切关状态保存 |
| AYogGameMode | YogSaveSubsystem | Checkpoint 触发 |
| AYogGameMode | YogMetaProgressionSubsystem | 结算时广播 RunEnded |

### World → Character

| 调用方 | 被调用方 | 说明 |
| --- | --- | --- |
| AYogGameMode | APlayerCharacterBase | 死亡处理 |
| AYogGameMode | AEnemyCharacterBase | 注册表管理、击杀统计 |

### Component 内部

| 调用方 | 被调用方 | 说明 |
| --- | --- | --- |
| BuffFlowComponent | YogAbilitySystemComponent | 能力触发 |
| BackpackGridComponent | YogAbilitySystemComponent | 能力激活 |
| BackpackGridComponent | YogMetaProgressionSubsystem | 隐藏被动激活 |
| CombatDeckComponent | YogAbilitySystemComponent | 卡牌激活 |

---

## 导演可调用的系统接口概览

> 详细接口规范见 [DirectorInterfaces.md](Story/DirectorInterfaces.md)

| 系统 | 已暴露给导演的调用点数量 | 文档 |
| --- | --- | --- |
| GameInstance（通过 GI） | 1（SetPendingStoryNextRoomPlan） | 本文 |
| AYogGameMode | 1（StartForcedSurvivalEncounter） | 待补：GameMode 接口文档 |
| StoryEngineSubsystem | 3（ExecuteAction / Broadcast / SetFlag） | [StoryEngine_Architecture.md](Story/StoryEngine_Architecture.md) |
| YogMetaProgressionSubsystem | 1（UnlockFeature） | [MetaProgression_Architecture.md](Progression/MetaProgression_Architecture.md) |
| YogSaveSubsystem | 1（MarkFirstRunTutorialCompleted） | [SaveSubsystem_Architecture.md](Progression/SaveSubsystem_Architecture.md) |

---

## 孤立系统（被依赖较少，需关注集成点）

| 系统 | 当前被依赖方 | 备注 |
| --- | --- | --- |
| YogWorldSubsystem | 仅 AYogGameMode | 关卡流媒体管理，无文档 |
| StoryEncounterRuntimeSubsystem | Story 系统内部 | 遭遇图执行，无文档 |
| YogInstanceSubSystem | 通用基类 | 容器级，职责不明确 |
