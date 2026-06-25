# HitStop 打击感配置说明

## 作用

用于配置近战命中后的停顿或慢动作反馈，让 Attack、WeaponSkill、Finisher 有不同打击手感。

## 配置位置

在 `MontageAttackDataAsset` 中配置，字段分类为 `HitStop`。

## 字段说明

| 字段 | 说明 | 推荐 |
| --- | --- | --- |
| `HitStopMode` | 打击停顿模式：`None`、`Freeze`、`Slow` | 普通命中用 `Freeze`，WeaponSkill / Finisher 可用 `Slow` |
| `HitStopFrozenDuration` | `Freeze` 模式冻结时长 | Attack `0.03-0.06`，WeaponSkill `0.06-0.10` |
| `HitStopSlowDuration` | `Slow` 模式持续时长 | WeaponSkill / Finisher `0.10-0.18` |
| `HitStopSlowRate` | 慢动作速度倍率，越低越慢 | `0.2-0.4` |
| `HitStopCatchUpRate` | 慢动作结束后的追帧倍率 | `1.5-2.5` |

## 推荐配置

| 攻击类型 | HitStopMode | HitStopFrozenDuration | HitStopSlowDuration | HitStopSlowRate | HitStopCatchUpRate |
| --- | --- | --- | --- | --- | --- |
| Attack | `Freeze` | `0.04` | 不填 | 不填 | 不填 |
| WeaponSkill | `Freeze` | `0.08` | 不填 | 不填 | 不填 |
| Finisher | `Slow` | 不填 | `0.14` | `0.3` | `2.0` |

## 配置示例

`DA_Attack_Sword_L3`：

- `HitStopMode = Freeze`
- `HitStopFrozenDuration = 0.06`


- `HitStopMode = Slow`
- `HitStopSlowDuration = 0.14`
- `HitStopSlowRate = 0.3`
- `HitStopCatchUpRate = 2.0`

## 验收方式

1. Attack 命中时有短暂停顿，但不影响后续输入。
2. WeaponSkill 命中时停顿更明显。
3. 空挥不触发 HitStop。
4. 多目标同时命中时，不出现长时间重复卡顿。
5. Finisher 的 HitStop 明显强于普通 Attack。

## 注意事项

- `HitStopFrozenDuration` 不建议超过 `0.10`，否则会影响连招输入手感。
- `HitStopSlowRate` 不建议低于 `0.2`，否则画面会显得卡死。
- HitStop 参数跟随 `MontageAttackDataAsset`，同一蒙太奇在不同动作或候选 AttackData 中可以得到不同打击感。
