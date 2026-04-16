# 符文升级系统 工作报告 2026-04-14

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 最后更新：2026-04-14

---

## 背景 & 设计决策

### 问题：拿到相同符文时该怎么办？

玩家测试反馈显示，出现相同符文时玩家无所适从（拿了放不下 / 格子浪费）。  
最初备选方案有两个：

| 方案 | 描述 | 问题 |
| --- | --- | --- |
| A. 堆叠 | 同名符文叠加数量，效果 ×N | 退化成"数字堆叠"，背包空间决策失去意义 |
| B. 升级 | 同名符文不占新格子，就地升级 | 维持空间策略，给长局奖励感 |

**决策：选 B（升级）。** 维持背包的空间权衡是核心玩法支柱，升级方案在不破坏格子系统的前提下给玩家正向反馈。

### 升级规则

| 等级 | UpgradeLevel 值 | 效果倍率 |
| --- | --- | --- |
| Lv.I（初始） | 0 | ×1.0 |
| Lv.II | 1 | ×1.5 |
| Lv.III（满级） | 2 | ×2.0 |

- 满级（Lv.III）后，该符文从三选一奖励池中**过滤排除**，不再出现
- 满级过滤仅影响奖励池生成，已放置的满级符文本身不受影响

---

## 实现概述

### 修改文件（4 个 C++ 文件）

#### 1. `RuneDataAsset.h` — UpgradeLevel 字段（已在前置提交实现）

在 `FRuneInstance` 中新增：

```cpp
/** 升级等级：0=Lv.I, 1=Lv.II, 2=Lv.III（满级）*/
UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
int32 UpgradeLevel = 0;

float GetUpgradeMultiplier() const { return 1.0f + (UpgradeLevel * 0.5f); }
```

#### 2. `BackpackGridComponent.h / .cpp` — 三个新接口

| 函数 | 用途 |
| --- | --- |
| `FindRuneByName(FName)` | 按符文名查找已放置符文，升级时使用 |
| `GetMaxLevelRuneNames()` | 返回满级符文名列表，供奖励池过滤 |
| `NotifyRuneUpgraded(FGuid)` | 升级后重启 BuffFlow 使新倍率生效，并广播 UI 刷新事件 |

`NotifyRuneUpgraded` 内部执行 `DeactivateRune → ActivateRune` 重启 BuffFlow，确保 FlowAsset 读取到最新的 `UpgradeLevel`。

#### 3. `PlayerCharacterBase.cpp` — AddRuneToInventory 升级逻辑

流程：

```
拿到符文
  ↓
背包中有同名符文？
  是 → UpgradeLevel < 2？
           是 → UpgradeLevel++ → NotifyRuneUpgraded → return
           否 → return（理论上奖励池已过滤，不会到这里）
  否 → 自动寻位放置（从左上角逐行扫描）
          失败 → 进 PendingRunes 待放置列表
```

#### 4. `YogGameMode.cpp` — GenerateLootBatch 满级过滤

在奖励候选池构建完成后，读取玩家背包的满级符文列表，用 `FilterByPredicate` 排除：

```cpp
TSet<FName> MaxLevelSet(Player->BackpackGridComponent->GetMaxLevelRuneNames());
Pool = Pool.FilterByPredicate([&](URuneDataAsset* DA) {
    return DA && !MaxLevelSet.Contains(DA->RuneInfo.RuneConfig.RuneName);
});
```

---

## 关键设计考量

### 为什么用 NotifyRuneUpgraded 而不是直接调 RefreshAllActivations？

- `RefreshAllActivations`、`ActivateRune`、`DeactivateRune` 都是 `BackpackGridComponent` 的私有方法
- 从外部（`PlayerCharacterBase`）直接调用需要破坏封装性
- `NotifyRuneUpgraded` 作为公开接口，内部封装"重启 BuffFlow + 广播 UI 事件"的完整逻辑，语义更清晰

### UpgradeLevel 与 Level 字段的区别

- `Level`（int32，默认 1）：预留字段，暂无功能，保留不动
- `UpgradeLevel`（int32，默认 0）：实际控制升级状态，FlowAsset 通过 BlueprintReadWrite 直接读取

### UI 显示

`OnRuneActivationChanged` 广播后，背包 UI 应刷新对应格子，显示升级标识（Lv.II / Lv.III 标签）。  
具体 UI 实现由 WBP_BackpackScreen 负责（本次工作报告不涉及）。

---

## 验证方法

1. 编译通过无报错
2. PIE 中拾取同一符文两次：第二次不新增格子，`UpgradeLevel` 从 0 变为 1
3. 第三次拾取：`UpgradeLevel` 变为 2
4. 第四次尝试：三选一中该符文不再出现
5. Log 确认：`AddRuneToInventory: XX符文 升级到 Lv.2`
