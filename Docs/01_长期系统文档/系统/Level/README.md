# 关卡系统 — Level

> 房间数据、传送门（Portal）、Buff 池、跨关卡状态保持。

## 架构说明

| 文档 | 内容 |
| --- | --- |
| [LevelSystem_ProgrammerDoc.md](LevelSystem_ProgrammerDoc.md) | 关卡系统程序文档（数据流 + 生命周期） |
| [CrossLevelState_Technical.md](CrossLevelState_Technical.md) | 跨关卡状态技术说明 |

## 使用指南 & 配置

| 文档 | 内容 |
| --- | --- |
| [LevelSystem_ConfigGuide.md](LevelSystem_ConfigGuide.md) | 关卡系统配置说明 |
| [Portal_ConfigGuide.md](Portal_ConfigGuide.md) | 传送门配置 |
| [WBP_PortalPreview_Layout.md](WBP_PortalPreview_Layout.md) | 传送门预览 Widget 布局 |
| [BuffPool_ConfigGuide.md](BuffPool_ConfigGuide.md) | 关卡 Buff 池配置 |

## 导演控制关卡（Arrangement 阶段）

通过 `FStoryNextRoomPlan` 写入 GameInstance，由 GameMode 在 Arrangement 读取：

| 需求 | 字段 |
| --- | --- |
| 指定关卡资产 | `RoomDataOverride` |
| 指定奖励内容 | `RewardOptionsOverride` |
| 指定 Buff 池 | `BuffsOverride` |
| 强制单传送门 | `bForceSinglePortal` |
| 抑制清房奖励 | `bSuppressRoomClearRewardPickup` |

完整字段列表见 [../Story/DirectorInterfaces.md](../Story/DirectorInterfaces.md)

## 关联系统

- [../Story/](../Story/) — 导演在 HandleArrangementPhase 写入关卡 Plan
- [../Mob/MobSpawner_Technical.md](../Mob/MobSpawner_Technical.md) — 刷怪器
