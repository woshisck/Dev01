# 敌人连击蒙太奇配置指南

**功能模块**：`UAN_EnemyComboSection` AnimNotify + 攻击 GA  
**适用对象**：使用单一蒙太奇完成多段连击的敌人  
**状态**：已实现，可直接在编辑器使用

---

## 概述

通过在攻击蒙太奇中使用 `AN_EnemyComboSection` Notify，敌人可以在**一个蒙太奇资产**内完成多段连击动作，每段攻击可以有独立的打击判定帧和伤害配置。

Notify 触发时直接调用 `AnimInstance->Montage_JumpToSection`，将播放头跳到下一段，整个连击对 GA 和 BT 透明——GA 只负责开始播放和等待结束，BT Task 只负责等待 GA 结束。

---

## 蒙太奇结构设计

### 创建 Section

在 Unreal 蒙太奇编辑器顶部 Timeline 中，右键添加 Section。命名建议：
```
Atk1  Atk2  Atk3
```
也可以用语义化名称，例如：
```
Slash1  Slash2  Slash3_Finisher
```

Section 的顺序在 Timeline 上会显示为：
```
[Atk1][Atk2][Atk3]
```

> ⚠️ 默认情况下，Unreal 蒙太奇各 Section 之间**不会自动衔接**（播放完当前 Section 就停）。这正是我们想要的行为——跳节完全由 Notify 控制。

---

## 添加 AN_EnemyComboSection Notify

### 步骤

1. 在蒙太奇编辑器的 **Notifies** 轨道中，右键 → `Add Notify`
2. 搜索 `AN_EnemyComboSection`，选中添加
3. 将 Notify 拖到合适的时间点（建议放在当前段最后 1~3 帧处）
4. 在右侧 `Details` 面板中填写 `NextSection` 字段

### 每段配置示例

| Section | Notify 放置位置 | NextSection 填写 |
|---|---|---|
| `Atk1` | Atk1 倒数第 2 帧 | `Atk2` |
| `Atk2` | Atk2 倒数第 2 帧 | `Atk3` |
| `Atk3` | ❌ 不放 Notify | （留空）→ 蒙太奇自然结束 |

最后一段不放 Notify，蒙太奇播完后自然触发 `OnCompleted`，GA 结束，BT Task 返回 Succeeded。

### Notify 参数说明

| 参数 | 默认值 | 说明 |
|---|---|---|
| `NextSection` | 空 | 跳转目标 Section 名称。填写后触发时立即跳节；留空则不跳 |
| `BlendOutTime` | `0.2` | 连招结束时的混出时间（秒）。**仅在 NextSection 为空时生效** |
| `bRequireHit` | `false` | 开启后仅在本段命中目标时才继续连招，未命中则停止连击链 |

> ⭐ **优先级规则**：`NextSection` 优先于 `BlendOutTime`。若同时填写了 `NextSection` 和 `BlendOutTime > 0`，以 `NextSection` 为准（跳节继续连击），`BlendOutTime` 只在 `NextSection` 为空时触发平滑结束。

### 连击结束行为速查

| NextSection | BlendOutTime | 行为 |
|---|---|---|
| 填写（如 `Atk2`）| 任意值 | 跳转到 `Atk2`，忽略 `BlendOutTime` |
| 空 | `> 0`（如 `0.2`）| 平滑混出蒙太奇，GA 收到 `OnBlendOut` |
| 空 | `0` | 蒙太奇自然播完，GA 收到 `OnCompleted` |

---

## 完整配置流程示例

### 示例：腐烂守卫三段连击

**蒙太奇**：`AM_RottenGuard_Combo3Hit`

```
Timeline:
  [0.0s ───── Atk1 ─────] [0.8s ──── Atk2 ────] [1.6s ────── Atk3 ──────] 2.8s
                        ↑ Notify: NextSection=Atk2   ↑ Notify: NextSection=Atk3
                       0.75s                         1.55s
```

**AbilityData 配置**（`DA_Ability_RottenGuard` → `AbilityMap`）：

| Key（Tag） | Montage | 备注 |
|---|---|---|
| `Enemy.Atk.Combo` | `AM_RottenGuard_Combo3Hit` | 三段连击 |

**行为树**：`BTTask_ActivateAbilityByTag`，Tag 填 `Enemy.Atk.Combo`

---

## 与打击判定的配合

每个 Section 内通常有自己的打击判定 Notify（如 `AN_HitBox` 或自定义伤害 Notify），各段独立触发，互不干扰。

```
Atk1:  [─────打击判定帧──] [AN_EnemyComboSection→Atk2]
Atk2:  [─────打击判定帧──] [AN_EnemyComboSection→Atk3]
Atk3:  [─────打击判定帧──]  （蒙太奇结束）
```

---

## 注意事项

| 情况 | 行为 |
|---|---|
| `NextSection` 留空 | 触发时不跳节；若 `BlendOutTime > 0` 则平滑结束，否则等蒙太奇自然结束 |
| `NextSection` 和 `BlendOutTime` 同时填写 | 以 `NextSection` 为准，`BlendOutTime` 被忽略 |
| Notify 时间点太晚（超过 Section 末尾） | 会超出段范围，建议放在距段末尾 2~3 帧内 |
| GA 被外部打断（受击/死亡等 State Conflict） | 蒙太奇中断，GA 结束，连击自然停止，无需额外处理 |
| `bRequireHit` 开启但本段未命中 | Notify 提前返回，不跳节不结束，蒙太奇继续播到当前 Section 自然结束 |
| 想要让 AI 决策是否继续连击 | 参见《行为树攻击任务配置指南》中的 Selector 模式 |

---

## 常见问题

**Q：动画跳节了但感觉有一帧卡顿？**  
A：Notify 放置的时间点偏早，与上一段的动画还没完全过渡，适当往后移 1~2 帧。

**Q：只想让前两段连击，第三段作为可选（AI 判断）？**  
A：在 Atk2 的 Notify 改为触发一个 Gameplay Event，GA 监听该 Event 后根据 BT 的黑板变量决定是否跳到 Atk3。这属于进阶用法，当前 `AN_EnemyComboSection` 实现的是确定性自动跳节。

**Q：我想要不同的连击概率（50% 打两段，50% 打三段）？**  
A：在 BT 里用两个不同的蒙太奇配置（二段连击蒙太奇 / 三段连击蒙太奇），通过 `BTTask_ActivateAbilityByTag` 的父 Tag 随机选择，详见《行为树攻击任务配置指南》。
