# YogSaveSubsystem 存档系统架构

> 最后更新：2026-05-28
> 源文件：`Source/DevKit/Public/SaveGame/YogSaveSubsystem.h`

---

## 职责

`UYogSaveSubsystem` 是 GameInstance 级 Subsystem，是项目中**唯一合法的存档读写入口**。

- 管理 3 个独立存档槽位（索引 0-2）
- 维护 Checkpoint（存档点）机制，支持断线恢复
- 异步写盘，防止主线程卡顿
- 存档版本迁移（顺序逐版本升级）
- 记录局内统计数据（击杀数、死亡数、金币等）

不在本系统中处理的职责：

- 局外成长数据（货币/升级节点）→ 委托 `YogMetaProgressionSubsystem`
- 全局设置（音量/画质）→ 使用独立的 `YogSettingsSave`（不绑定槽位）

---

## 数据结构

```
存档槽位 (SlotIndex 0/1/2)
└── YogSaveGame
    ├── MetaProgressionData    局外成长数据（节点/货币/功能解锁）
    ├── RunCheckpointData      当前局存档点（bIsValid = 是否有存档）
    ├── Statistics             累计统计（跨局不清零）
    └── FirstRunTutorialState  新手教程完成状态

独立存档（不绑定槽位）
└── YogSettingsSave            全局设置（音量/画质/键位）
```

---

## 核心流程

### Checkpoint 写盘流程

```
触发点（三个）：
  1. 进入关卡前
  2. 清关后

TriggerCheckpoint(CurrentFloor)
  → PopulateCheckpointFromRunState(OutCheckpoint, Floor)
  → DoAsyncSave()
      → bAsyncSavePending 并发保护（已有写盘则 queue）
      → AsyncSaveGameToSlot
      → OnAsyncSaveComplete() 回调
```

### 断线恢复流程

```
Continue 按钮
  → TryRestoreRunCheckpoint()
      → 检查 RunCheckpoint.bIsValid
      → 若有效：RestoreRunStateFromCheckpoint() → GI->PendingRunState
      → 返回 bool（UI 据此决定是否跳转关卡）
```

### 死亡/结束流程

```
玩家死亡 / 教程脚本死亡
  → ClearRunCheckpoint()
      → 清除 bIsValid
      → DoAsyncSave()
```

---

## 对外接口分组

### 槽位管理

| 接口 | 调用时机 | 说明 |
| --- | --- | --- |
| `SelectSlot(int32)` | 选档后 | 激活并加载指定槽位 |
| `DeleteSlot(int32)` | 删档 | 删除存档文件 |
| `ResetSlotForNewGame(int32)` | 新游戏 | 清空 MetaProgression + Checkpoint，保留 Statistics |
| `RequestSlotPreview(int32, Callback)` | 选档 UI | 异步读取槽位预览，不加载全部数据 |
| `GetSlotName(int32)` | 任意 | 返回 "SaveSlot_0/1/2" |

### 存档点

| 接口 | 调用时机 | 说明 |
| --- | --- | --- |
| `TriggerCheckpoint(int32 Floor)` | 进关前 / 清关后 | 序列化 RunState 并异步写盘 |
| `ClearRunCheckpoint()` | 玩家死亡 / 结局 | 清除存档点并写盘 |
| `TryRestoreRunCheckpoint()` | Continue 按钮 | 恢复 RunState，返回是否成功 |
| `QuickSave()` | 背包 UI 关闭 | 快速异步写盘（主线程无感知） |

### 教程状态

| 接口 | 调用时机 | 说明 |
| --- | --- | --- |
| `IsFirstRunTutorialActive()` | 任意（导演查询） | 是否处于新手教程局 |
| `IsFirstRunTutorialCompleted()` | 任意 | 教程是否已完成 |
| `MarkFirstRunTutorialCompleted()` | 教程结束（导演调用） | 标记完成并写盘 |

### 统计记录（由各事件钩子调用，不供外部主动调用）

| 接口 | 调用方 |
| --- | --- |
| `RecordRunStarted()` | GameMode.StartNewRunFromFrontend |
| `RecordEnemyKilled(int32)` | GameMode.UpdateFinishLevel |
| `RecordPlayerDeath()` | GameMode.HandlePlayerDeath |
| `RecordGoldEarned(int32)` | BackpackGridComponent.AddGold |

---

## 导演可用接口

导演系统（`FirstRunTutorialDirectorSubsystem`）当前使用：

- `IsFirstRunTutorialActive()` — 判断是否需要执行教程逻辑
- `MarkFirstRunTutorialCompleted()` — 教程结束时调用

其余接口（槽位管理/存档点）由 GameMode 和 UI 调用，导演不直接使用。

---

## 并发保护机制

```cpp
bool bAsyncSavePending = false;   // 当前是否有写盘进行中
bool bAsyncSaveQueued  = false;   // 写盘进行中时是否有 pending 请求
```

调用 `DoAsyncSave()` 时：

- 若 `bAsyncSavePending == false`：直接开始写盘
- 若 `bAsyncSavePending == true`：设 `bAsyncSaveQueued = true`，回调完成后自动触发队列中的写盘

---

## 版本迁移

`MigrateSaveGame(Save, FromVersion, ToVersion)` 在 `LoadSaveGame` 时调用，顺序逐版本升级。新增字段时在此函数补迁移逻辑，不要直接修改旧存档结构。
