# 中毒 GameplayEffect 配置说明

本说明用于 `GE_Poison` 和 `GE_PoisonSplash`。当前不再使用旧的 `B_MaxHealthDamage` Modifier，统一改为 `GEExec_PoisonDamage` 执行计算，避免资产缺失导致中毒不生效。

## 1. 资产路径

| 资产 | 路径 | 用途 |
| --- | --- | --- |
| `GE_Poison` | `/Game/Code/GAS/Abilities/Shared/GE_Poison` | 主目标中毒，月光毒素命中后叠 3 层 |
| `GE_PoisonSplash` | `/Game/Docs/BuffDocs/Playtest_GA/DeathPoison/GE_PoisonSplash` | 次级敌人扩散中毒，默认 1 层并带小额伤害 |

## 2. GE 配置

`GE_Poison`：

| 字段 | 值 |
| --- | --- |
| `Duration Policy` | `Has Duration` |
| `Duration Magnitude` | `5.0` |
| `Period` | `1.0` |
| `Execute Periodic Effect On Application` | 关闭 |
| `Executions` | `GEExec_PoisonDamage` |
| `Modifiers` | 空 |
| `Components` | 添加 `Grant Tags to Target Actor`，其中 `Add Tags` 填 `Buff.Status.Poisoned` |
| `Stacking Type` | `Aggregate By Target` |
| `Stack Limit Count` | `20` |
| `Stack Duration Refresh Policy` | `Refresh On Successful Application` |
| `Stack Period Reset Policy` | `Reset On Successful Application` |
| `Stack Expiration Policy` | `Clear Entire Stack` |

`GE_PoisonSplash`：

| 字段 | 值 |
| --- | --- |
| `Duration Policy` | `Has Duration` |
| `Duration Magnitude` | `3.0` |
| `Period` | `1.0` |
| `Execute Periodic Effect On Application` | 开启 |
| `Executions` | `GEExec_PoisonDamage` |
| `Modifiers` | 空 |
| `Components` | 添加 `Grant Tags to Target Actor`，其中 `Add Tags` 填 `Buff.Status.Poisoned` |
| `Stacking Type` | `Aggregate By Target` |
| `Stack Limit Count` | `10` |

## 3. 毒伤计算

每次 Tick：

```text
生命毒伤 = MaxHealth * 每层百分比 * 当前毒层数 + Data.Damage
护甲毒伤 = MaxHealth * 每层护甲百分比 * 当前毒层数
```

默认值：

| 参数 | 默认 |
| --- | --- |
| `Data.Poison.PercentPerStack` | 不填时为 `0.02`，每层每秒造成最大生命 2% |
| `Data.Poison.ArmorPercentPerStack` | 不填时为 `0.08`，有护甲时每层每秒造成最大生命 8% 的护甲伤害 |
| `Data.Damage` | 不填时为 `0`，用于次级扩散的小额固定伤害 |

## 4. 月光毒素 FA 用法

主目标节点：

| 节点 | 字段 | 值 |
| --- | --- | --- |
| `ApplyEffect` | `Effect` | `GE_Poison` |
| `ApplyEffect` | `Target` | `LastDamageTarget` |
| `ApplyEffect` | `Application Count` | `3` |

次级扩散节点：

| 节点 | 字段 | 值 |
| --- | --- | --- |
| `ApplyGEInRadius` | `Effect` | `GE_PoisonSplash` |
| `ApplyGEInRadius` | `Radius` | `300` |
| `ApplyGEInRadius` | `Exclude Location Source Actor` | 勾选 |
| `ApplyGEInRadius` | `Max Targets` | `3` |
| `ApplyGEInRadius` | `Application Count` | `1` |
| `ApplyGEInRadius` | `SetByCallerTag1` | `Data.Damage` |
| `ApplyGEInRadius` | `SetByCallerValue1` | `5` |

## 5. 自动修复

运行批量工具会自动修复这两个 GE 资产：

```powershell
& "Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "X:\Project\Dev01\DevKit.uproject" -run=RuneCardBatchGenerator -Apply -unattended -nop4 -nosplash
```

修复后 `Saved/512RuneCardBatchReport.md` 中会出现 `Poison GameplayEffect assets` 段落。
