# Montage 播放速率曲线使用指南

> 项目：星骸降临
> 关联文档：[MeleeCombo_NotifyRune_Guide.md](MeleeCombo_NotifyRune_Guide.md) · [../../编码规范/AnimNotify.md](../../编码规范/AnimNotify.md)
> 更新日期：2026-07-07

---

## 一、概述

Montage 播放过程中可以通过一条 **内嵌浮点曲线** 动态修改播放速率（play rate），
用于「起手慢、命中快」等非线性节奏，无需拆分 Section 或额外蓝图逻辑。

关键约定：**曲线名硬编码为 `PlayRate`（大小写敏感）**。

| 情况 | 行为 |
|------|------|
| Montage 内含名为 `PlayRate` 的曲线 | 每帧按当前播放位置采样曲线值，实时设置播放速率 |
| Montage 无该曲线 | 播放速率保持传入的 `Rate`（默认 `1.0`），不做任何改动 |

曲线值作为 **倍率** 叠加在起始 `Rate` 之上：`最终速率 = Rate × 曲线值`（下限 `0.01`）。

## 二、生效范围（C++）

两个播放 Montage 的 AbilityTask 均已支持（构造函数开启 `bTickingTask`，在 `TickTask` 中采样）：

| Task | 使用方 |
|------|--------|
| `UYogTask_PlayMontageAbility` | `GA_PlayMontage`（武器/近战技能，如剑攻击） |
| `UYogAbilityTask_PlayMontageAndWaitForEvent` | `GA_PlayerDash` 等 |

采样逻辑（伪代码）：

```
if (Montage->HasCurveData("PlayRate"))
{
    Pos   = AnimInstance->Montage_GetPosition(Montage);
    Value = Montage->EvaluateCurveData("PlayRate", FAnimExtractContext(Pos));
    AnimInstance->Montage_SetPlayRate(Montage, max(Rate * Value, 0.01));
}
```

> 曲线名常量定义在各 Task 的 .cpp 匿名 namespace 中（`YogMontage_PlayRateCurveName`）。
> 如需改名或改为按调用可配置，需同时修改两处并（可选）透传到工厂函数。

## 三、策划配置步骤

1. 打开目标 Montage → 底部 **Curves** 轨道 → **Add Curve**。
2. 曲线名必须精确填写 **`PlayRate`**。
3. 按时间轴打关键帧，Y 值即该时刻的目标速率倍率，例如：

   | 时间 | 值 | 效果 |
   |------|----|------|
   | 0.0 | 1.0 | 正常 |
   | 0.3 | 0.4 | 起手放慢 |
   | 0.6 | 2.0 | 出招加速 |
   | 1.0 | 1.0 | 收尾正常 |

4. 保存。无需在编辑器里勾选任何字段——曲线名 `PlayRate` 即约定。

## 四、与 Time Stretch Curve 的区别

- 引擎内建的 **Time Stretch Curve** 只在播放速率 ≠ 1.0 时重新分配拉伸量，速率为 1.0 时无效果。
- 本 `PlayRate` 曲线是 **直接设置速率**，在速率 1.0 下即可生效，二者互不冲突。
