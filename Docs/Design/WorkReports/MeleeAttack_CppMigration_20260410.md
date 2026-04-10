# 近战攻击 GA 系统 C++ 迁移工作报告

> 日期：2026-04-10  
> 类型：系统重构  
> 影响范围：攻击判定、连击系统、命中框检测、调试可视化

---

## 背景

原近战攻击系统存在多个问题：
- 攻击 GA 逻辑全部在 Blueprint，策划修改困难，版本管理混乱
- B_TT（Blueprint TargetType）使用已废弃 API `GetCurrentAbilityInstance()`，多 GA 并发时随机返回错误实例
- `EffectContainerMap` 挂在 ASC 上，每个角色需独立配置，漏配后静默失败无任何提示
- 连击系统逻辑错误：首次触发时所有段同时激活，蒙太奇级联导致第 3 段优先完成
- `EventTags` 为空容器，`OnEventReceived` 永不触发，攻击无伤害

---

## 完成内容

### 1. C++ GA 基类（`GA_MeleeAttack`）

封装完整近战攻击流程，Blueprint 侧无需编写任何逻辑节点：

- `ActivateAbility`：CommitAbility → 施加攻击前摇 GE（含 4 个 SetByCaller Magnitude）→ 播放蒙太奇 + 注册伤害事件监听
- `EndAbility`：移除前摇 GE；若正常结束（非取消/打断）则施加后摇 GE
- `OnEventReceived`：将 `this` 写入 `EventData.OptionalObject` → `ApplyEffectContainer`
- `GetAbilityActionData_Implementation`：从角色 `CharacterData->AbilityData->AbilityMap` 按 AbilityTags 查表

### 2. 玩家连击 GA（`GA_PlayerMeleeAttack` 系列）

共 9 个 C++ GA 类：`LightAtk1~4`、`HeavyAtk1~4`、`DashAtk`

正确实现 GAS 连击链机制：
```
Combo2 的 ActivationOwnedTags = {LC1, LC2}   — 激活时持有前两段标记
Combo2 的 ActivationRequiredTags = {CanCombo, LC1}  — 要求前一段已激活且在连招窗口
Combo2 的 ActivationBlockedTags = {LC2}        — 防止自身重复激活
```

`GE_StatBeforeATK` / `GE_StatAfterATK` 在 `UGA_PlayerMeleeAttack` 构造函数中自动绑定，策划无需手动填写。

### 3. 敌人攻击 GA（`GA_EnemyMeleeAttacks` 系列）

共 8 个 C++ GA 类：`LAtk1~4`、`HAtk1~4`

仅设置 AbilityTags 和 Dead 阻断，其余全继承 `GA_MeleeAttack`。

### 4. C++ 命中框检测（`UYogTargetType_MeleeBase` 系列）

完全替代 `B_TT_Enemy` / `B_TT_Player` Blueprint：

| 类 | 功能 |
|---|---|
| `UYogTargetType_MeleeBase` | Annulus / Triangle / 球形兜底检测逻辑 |
| `UYogTargetType_Enemy` | 对场景内 `APlayerCharacterBase` 检测（敌人用） |
| `UYogTargetType_Player` | 对场景内 `AEnemyCharacterBase` 检测（玩家用） |

ActionData 通过 `EventData.OptionalObject`（精确 GA 引用）读取，消除 CDO 无状态问题。

调试可视化按实际命中框形状绘制（Annulus 扇区 / Triangle 扇面 / 全圆兜底）。

### 5. Bug 修复

| Bug | 修复 |
|---|---|
| 连击首段同时激活所有段 | 实现正确的 ActivationOwnedTags 累积链 |
| 攻击无伤害（OnEventReceived 不触发） | EventTags 明确传入 `GameplayEffect.DamageType.GeneralAttack` |
| GE_StatBeforeATK SetByCaller 报错 | 读取 ActionData 后设置全部 4 个 Magnitude |
| Blueprint 保存触发编辑器崩溃 | `check()` 改为 `ensure()` |
| 敌人 Debug 显示圆圈（命中框未读到） | ActionData 路由从 OptionalObject 精确获取 GA 引用 |

---

## 遗留问题 / 后续工作

| 项目 | 状态 | 说明 |
|---|---|---|
| `AN_MeleeDamage` vs `AN_Dmg_GeneralAttack` | ⚠️ 待清理 | 两个 AnimNotify 并存，旧版 Blueprint 动画资产仍用旧 AN |
| 玩家 HeavyAtk 蒙太奇 | ❌ 未配置 | AbilityData 中重攻击行无 Montage，GA 立即结束（正常行为，待策划填写） |
| B_TT Blueprint 资产 | 🗑️ 可删除 | 已被 C++ 替代，但 Blueprint 资产仍存在，可在确认无引用后删除 |

---

## 配置变更说明（对策划）

**原有配置方式**（已废弃）：
- 角色 BP → AbilitySystemComponent → Buffer → Effect Container Map 手动填写

**新配置方式**：
1. 角色父类 BP：`DefaultMeleeTargetType` + `DefaultMeleeDamageEffect`（一次配置，全族继承）
2. `DA_AbilityData`：每攻击行填写 Montage + hitboxTypes + 攻击参数
3. 蒙太奇：攻击帧放 `AN_MeleeDamage` 通知

详见：[攻击伤害配置指南](../FeatureConfig/AttackDamage_ConfigGuide.md)
