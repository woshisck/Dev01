# 4.15 可玩版本开发任务

> 创建日期：2026-04-05  
> 截止：2026-04-15  
> 目标：可演示的完整一局流程（战斗 → 整理 → 下一关）

---

## 背景

### 已完成的基础设施（截至 2026-04-05）

| 系统 | 状态 | 说明 |
|---|---|---|
| RuneDataAsset（DA 数据层） | ✅ 完成 | RuneConfig（DurationType/UniqueType/StackType/Period）+ Effects Fragment |
| BuffFlow 节点库 | ✅ 完成 | ApplyRuneGE / ApplyAttributeModifier / GrantGA / GetRuneInfo 等全套节点，旧节点已清理 |
| BackpackGrid 组件 | ✅ 完成 | 放置/移动/热度/激活区，GE 生命周期已移交 FA（ApplyRuneGE） |
| GAS 基础 | ✅ 完成 | ASC + AttributeSet（Base/Player/Enemy/Rune/Damage） |
| 关卡流程 C++ 框架 | ✅ 完成 | ELevelPhase / LevelSequenceDataAsset / LootSelectionWidget 基类 |
| 指南文档 | ✅ 完成 | BuffFlow_Guide.md (rev5) + RuneDataAsset_Guide.md (rev3) |

### 4.15 Demo 必须包含

1. 玩家基础战斗（移动 / 攻击 / 敌人死亡）
2. 背包网格 + 热度 + 激活区 — **核心差异化，最高优先级**
3. 关卡流程（刷怪 → 清空 → 调整阶段选符文 → 下一关）
4. 两阶段切换（战斗期锁背包 / 调整期可放入移动符文）

---

## P0 — 可跑通的战斗循环（截止 4/12）

> 没有这部分，Demo 无法演示。

### P0-1 验证基础战斗链路
**负责：程序**

确认以下链路完整无断点：
```
玩家攻击输入
  → GA_Attack（蒙太奇 + 命中检测）
  → DamageAttributeSet 写入伤害
  → BaseAttributeSet.PostGameplayEffectExecute → Health 扣减
  → Health <= 0 → SourceASC.OnKilledTarget.Broadcast
  → 敌人死亡动画 + 移除
```

验收：能对着一个敌人连续攻击并击杀，无报错。

---

### P0-2 热度 Attribute 接入 BackpackGrid
**负责：程序**

热度变化触发激活区扩展是核心差异化玩法，必须跑通：
```
战斗中造成/受到伤害 → Heat Attribute 变化
  → BaseAttributeSet.PostAttributeChange(Heat)
  → 计算 HeatPercent = Heat / MaxHeat
  → BackpackGrid.OnHeatPercentChanged(HeatPercent)
  → 激活区 Tier 升级（Tier1→2→3）
  → RefreshAllActivations()（新进入激活区的符文自动激活）
```

验收：战斗中热度上升后，背包激活区可见变大，新区域内符文的 GE 生效。

---

### P0-3 GameMode 关卡流程打通
**负责：程序**

```
[Combat 阶段]
  MobSpawner 刷怪
  玩家击杀 → KillCount++
  KillCount >= KillTarget → 广播 OnPhaseChanged(Arrangement)

[Arrangement 阶段]
  BackpackGrid.SetLocked(false)
  GameMode 从 LootPool 随机选 3 个 DA_Rune → 广播 OnLootOptionsGenerated
  LootSelectionWidget 显示三选一
  玩家选择 → 将 RuneInstance 放入背包（TryPlaceRune）
  玩家点击"确认" → ConfirmAndTransition()

[Transitioning]
  BackpackGrid.SetLocked(true)
  加载 LevelSequenceDA.NextLevelName
```

验收：击满 10 只怪后自动切到整理阶段，选完符文确认后加载下一关。

---

### P0-4 两阶段 BackpackGrid 锁定接入
**负责：程序**

- `OnPhaseChanged(Combat)` → `BackpackGrid.SetLocked(true)`
- `OnPhaseChanged(Arrangement)` → `BackpackGrid.SetLocked(false)`
- UI 层在战斗阶段灰化/隐藏背包操作按钮

验收：战斗中无法拖动符文，整理阶段可以操作。

---

## P1 — 核心玩法可体验（截止 4/13）

> 有战斗循环后，让符文系统真正产生游戏感。

### P1-5 创建 3 个可用 DA_Rune 资产
**负责：策划 + 程序**

覆盖三种基础类型，验证不同配置路径：

| 资产名 | 类型 | 配置要点 |
|---|---|---|
| `DA_Rune_AttackUp` | 永久被动，纯数值 | Duration=永久，Effects: Attack +20，无 FA |
| `DA_Rune_Berserk` | 命中叠层，有 FA | Duration=有时限(5s)，Stack=叠加(5层)，FA 里 OnDamageDealt→ApplyRuneGE→GetRuneInfo 判断层数触发爆发 |
| `DA_Rune_Poison` | DoT | Duration=有时限(8s)，Period=1.0，Effects: Health -15，FA 做周期视觉反馈 |

> **参考文档**：BuffFlow_Guide.md 五、完整使用案例

---

### P1-6 为 DA_Rune 搭配 FA Flow Graph
**负责：策划（主）+ 程序（示例）**

程序先做一个完整示例 FA（`FA_Rune_Knockback`，对应 BuffFlow 指南案例 2），策划参照搭其余 FA。

**验证清单：**
- [ ] 符文进入激活区 → FA 启动 → `[Start]→[ApplyRuneGE]` → GE 生效（属性变化可见）
- [ ] 符文移出激活区 → FA 停止 → `ApplyRuneGE.Cleanup()` → GE 移除（属性恢复）
- [ ] 叠层符文 → 每次命中叠一层，FA 停止时所有层清除

---

### P1-7 背包 UI 接入 BackpackGrid 新架构
**负责：程序**

| UI 功能 | 接口 |
|---|---|
| 显示所有格子和已放置符文 | `GetAllPlacedRunes()` |
| 高亮激活区 | `GetActivationZoneCells()` |
| 激活区随热度扩展 | 绑 `OnHeatTierChanged` 委托 |
| 符文激活/未激活视觉区分 | 绑 `OnRuneActivationChanged` 委托 |
| 拖拽放置 / 移动 | `TryPlaceRune()` / `MoveRune()` |

---

### P1-8 WBP_LootSelection BP 实现
**负责：程序 / 美术**

基类 `ULootSelectionWidget` 已定义好接口，BP 里实现：
- `OnLootOptionsReady` → 填充 3 张卡片的图标（`RuneAsset.RuneIcon`）和名称（`RuneAsset.RuneName`）
- `OnLevelPhaseChanged` → Arrangement 时 `SetVisibility(Visible)`，其他时 `Hidden`
- 3 张卡片的按钮绑 `SelectRuneLoot(0/1/2)`
- 确认按钮绑 `ConfirmAndTransition()`

---

### P1-9 关卡序列 DA 配置
**负责：策划**

创建 `DA_LevelSequence_Run01`：
```
NextLevelName  = <第二关关卡资产名>
KillTarget     = 10
LootPool       = [DA_Rune_AttackUp, DA_Rune_Berserk, DA_Rune_Poison, ...]
```

---

## P2 — 体验完善（4/14，有余力再做）

| # | 任务 | 说明 |
|---|---|---|
| P2-10 | 符文激活视觉反馈 | PlayNiagara + 音效，符文进入激活区时有"点亮"效果 |
| P2-11 | 关卡过渡动画 | Arrangement→加载下一关的淡出/加载画面 |
| P2-12 | 敌人 AI 完善 | 追击 + 近战攻击 + 死亡动画流畅无卡顿 |
| P2-13 | 热度 UI 显示 | 热度条 + 激活区 Tier 升级提示 |

---

## 风险与注意事项

### ⚠️ 风险 1：FA 内容是关键路径
架构今日才定型，策划第一次使用新节点体系。  
**缓解**：程序先完成 `FA_Rune_Knockback` 示例，策划照着改；遇到问题查 BuffFlow_Guide.md。

### ⚠️ 风险 2：背包 UI 状态未知
不确定现有背包 UI 是否已接入 BackpackGrid 新架构（GE 由 FA 管理，激活区委托）。  
**缓解**：P1-7 最先确认，有问题立即拉程序。

### ⚠️ 风险 3：GameMode 流程代码状态
`LevelFlowTypes.h` 定义存在，但 `YogGameMode.cpp` 里的状态切换和 LootOptions 广播是否实现需确认。  
**缓解**：P0-3 第一个确认，缺的部分程序补全。

---

## 每日节点

| 日期 | 目标 |
|---|---|
| 4/05（今天）| 架构清理完成，文档更新，任务分发 |
| 4/07 | P0-1、P0-2 完成；P0-3 开始 |
| 4/09 | P0 全部完成（可跑通一局） |
| 4/11 | P1-5、P1-6 完成（3 个符文可用） |
| 4/13 | P1 全部完成（核心玩法可演示） |
| 4/14 | P2 能做多少做多少，Buffer 日 |
| 4/15 | Demo 冻结，测试 + 录制 |
