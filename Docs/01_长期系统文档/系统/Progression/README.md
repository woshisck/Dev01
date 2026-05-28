# 成长系统 — Progression

> 包含局外元进度（升级树、货币、功能解锁）和存档系统（Checkpoint、槽位管理）。

## 架构说明

| 文档 | 内容 |
| --- | --- |
| [MetaProgression_Architecture.md](MetaProgression_Architecture.md) | 元进度系统架构：货币/升级节点/功能解锁/结算接口 |
| [SaveSubsystem_Architecture.md](SaveSubsystem_Architecture.md) | 存档系统架构：3 槽位/Checkpoint/异步写盘/版本迁移 |

## 使用指南

### 功能解锁（剧情触发）

```cpp
// 导演调用：解锁局外功能，不花费货币
MetaProgression->UnlockFeature(FeatureTag);

// 查询是否已解锁
MetaProgression->IsFeatureUnlocked(FeatureTag);
```

### 教程结束（完整三步，不可拆分）

```cpp
SaveSubsystem->MarkFirstRunTutorialCompleted();
SaveSubsystem->ClearRunCheckpoint();
GI->ClearRunState();
```

### 货币操作

```cpp
MetaProgression->AddCurrency(CurrencyTag, Amount);     // 增加（可为负）
MetaProgression->SpendCurrency(CurrencyTag, Amount);   // 扣除，不足返回 false
```

## 关键约定

- 存档系统是**唯一合法的持久化入口**，其他系统通过接口写入，不直接操作存档
- MetaProgression DataTable 由 `UMetaProgressionSettings`（`DefaultGame.ini`）配置路径，在 `Initialize()` 加载，不在 GameInstance BP 赋值

## 关联系统

- [../Story/DirectorInterfaces.md](../Story/DirectorInterfaces.md) — 导演调用本系统的接口清单
