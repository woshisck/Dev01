# 状态冲突规则表 — 策划配置指南

> **文件位置**：`Content/Data/DA_StateConflictTable.uasset`（DataAsset，类型 `UStateConflictDataAsset`）  
> **生效范围**：所有挂载 `YogAbilitySystemComponent` 且引用了本表的角色  
> **更新方式**：编辑器直接修改并保存，无需重新编译

---

## 核心概念

### Tag 三角关系

```
ActiveTag ──触发──→ 规则生效
              ├── BlockTags  → 让带这些 Tag 的 GA 无法激活（可恢复）
              └── CancelTags → 立即打断正在运行的 GA（不可撤销）
```

- **ActiveTag**：出现在角色 ASC 上时触发规则，Tag 消失时自动解除 Block
- **BlockTags**：填写对应 GA 的 `AbilityTags`，不是状态 Tag
- **CancelTags**：填写对应 GA 的 `AbilityTags`，不是状态 Tag

### GA 需要配置的唯一字段

每个 GA 只需要在 `AbilityTags` 里填自己的身份 Tag，**不需要配置任何互斥逻辑**：

```
GA_Attack   → AbilityTags: Action.Attack
GA_Move     → AbilityTags: Action.Move
GA_Interact → AbilityTags: Action.Interact
GA_Dialogue → AbilityTags: Action.Dialogue
```

---

## 当前冲突规则表

### 动作类互斥

| ActiveTag | BlockTags | CancelTags | Priority | 说明 |
|-----------|-----------|------------|----------|------|
| `Action.Attack` | `Action.Move` | `Action.Move` | 10 | 攻击时阻止并打断移动 |
| `Action.Move` | — | — | 5 | 移动不主动阻止其他行为 |
| `Status.HitStun` | `Action.Attack`, `Action.Move` | `Action.Attack`, `Action.Move` | 20 | 受击硬直打断攻击和移动 |
| `Status.Interact` | `Action.Attack` | — | 15 | 交互期间不能攻击（移动由蒙太奇决定） |
| `Status.Dialogue` | `Action.Attack`, `Action.Move`, `Action.Interact` | `Action.Attack`, `Action.Move` | 30 | 对话期间禁止大部分行为 |
| `Status.Dead` | `Action.Attack`, `Action.Move`, `Action.Interact` | `Action.Attack`, `Action.Move` | 100 | 死亡禁止一切行为 |

### 符文类互斥

| ActiveTag | BlockTags | CancelTags | Priority | 说明 |
|-----------|-----------|------------|----------|------|
| `Buff.Rune.Type.Attack` | `Buff.Rune.Type.Attack` | — | -1 | 同类攻击符文只能激活一个 |
| `Buff.Rune.Type.Defense` | `Buff.Rune.Type.Defense` | — | -1 | 同类防御符文只能激活一个 |

> `Priority = -1` 表示符文互斥不参与优先级计算，纯粹阻止重复激活。

---

## 填表规则

### Priority（优先级）说明

| 数值 | 含义 |
|------|------|
| `-1` | 不参与优先级（仅用于同类阻止） |
| `0–9` | 低优先级（普通行为） |
| `10–19` | 中优先级（战斗行为） |
| `20–49` | 高优先级（受击/控制） |
| `50+` | 最高优先级（死亡/剧情） |

> **注意**：当前版本优先级字段为预留，系统尚未实现高优先级打断低优先级的逻辑，
> 仅做记录用途，方便后续扩展。

### 常见填法错误

| ❌ 错误 | ✅ 正确 |
|--------|--------|
| BlockTags 填状态 Tag（`Status.HitStun`） | BlockTags 填 GA 的 AbilityTag（`Action.Attack`） |
| 同一条规则的 BlockTags 和 CancelTags 完全一样且无必要 | Cancel 用于"立即打断"，Block 用于"防止新激活"，按需分开 |
| ActiveTag 留空 | 必须填有效 Tag，否则规则会被跳过并输出 Warning |

---

## 新增状态的流程

1. **在 `BuffTag.ini` 里注册新 Tag**（例如 `Action.Dash`）
2. **GA 的 `AbilityTags` 填写新 Tag**
3. **在本表新增一行**，填写该状态的冲突规则
4. 保存，热重载即生效

---

## 与其他文档的关系

| 文档 | 职责 |
|------|------|
| `Buff_Tag_Spec.md` | 所有 Tag 的命名规范和分层定义 |
| `StateConflict_TagBlock.md`（本文档） | 策划填表规范和当前规则列表 |
| `StateConflict_Technical.md` | 程序实现细节和接入方式 |
