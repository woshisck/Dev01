# 状态冲突规则表 — 策划配置指南

> **文件位置**：`Content/Docs/GlobalSet/CharacterBaseSet/DA_Base_StateConflict_Initial.uasset`（DataAsset，类型 `UStateConflictDataAsset`）  
> **加载方式**：由代码通过 `DevAssetManager` 全局自动加载，**无需在角色蓝图中手动赋值**  
> **生效范围**：所有挂载 `YogAbilitySystemComponent` 的角色  
> **更新方式**：编辑器直接修改并保存，无需重新编译

---

## 核心概念

本 DA 包含两个独立但互补的配置区域：

```
DA_Base_StateConflict_Initial
├── Rules[]           → 管"哪些 GA 被阻断/取消"（GA 级别）
└── BlockCategoryMap  → 管"移动组件/AI系统被阻断"（系统级别）
```

---

## 区域一：Rules（GA 冲突规则）

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

### 当前冲突规则表

#### 动作类互斥

| ActiveTag | BlockTags | CancelTags | Priority | 说明 |
|-----------|-----------|------------|----------|------|
| `Buff.Status.HitReact` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 20 | 受击硬直打断并阻止所有主动动作 |
| `Buff.Status.Dead` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 999 | 死亡禁止一切行为，优先级最高 |
| `Buff.Status.Knockback` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 50 | 击退期间取消全部玩家动作 |

#### 符文类互斥

| ActiveTag | BlockTags | CancelTags | Priority | 说明 |
|-----------|-----------|------------|----------|------|
| `Buff.Rune.Type.Attack` | `Buff.Rune.Type.Attack` | — | -1 | 同类攻击符文只能激活一个 |
| `Buff.Rune.Type.Defense` | `Buff.Rune.Type.Defense` | — | -1 | 同类防御符文只能激活一个 |

> `Priority = -1` 表示符文互斥不参与优先级计算，纯粹阻止重复激活。

---

## 区域二：BlockCategoryMap（系统级阻断）

控制**移动组件**和 **AI 行为树**等底层系统的开关，与 GA 级别的 Block 互补。

### 当前支持的阻断类别

| Key（阻断类别） | 说明 |
|----------------|------|
| `Block.Movement` | 停止角色移动组件（`DisableMovement`） |
| `Block.AI` | 暂停 AI 行为树逻辑（`PauseLogic`），对玩家角色无效 |

### 当前配置

| Key | Value（触发阻断的状态 Tag） |
|-----|--------------------------|
| `Block.Movement` | `Buff.Status.HitReact`, `Buff.Status.Dead`, `Buff.Status.Knockback` |
| `Block.AI` | `Buff.Status.HitReact`, `Buff.Status.Dead`, `Buff.Status.Knockback` |

> Value 中任意一个 Tag 挂到 ASC 上时阻断生效；所有 Tag 都消失且角色存活时自动恢复。

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
| Rules 的 BlockTags 填状态 Tag（`Buff.Status.HitReact`） | BlockTags 填 GA 的 AbilityTag（`Action.Attack`） |
| BlockCategoryMap 的 Value 填 GA AbilityTag | Value 填挂在 ASC 上的状态 Tag（ActivationOwnedTags） |
| 同一条规则的 BlockTags 和 CancelTags 完全一样且无必要 | Cancel 用于"立即打断"，Block 用于"防止新激活"，按需分开 |
| ActiveTag 留空 | 必须填有效 Tag，否则规则会被跳过并输出 Warning |

---

## 新增状态互斥的流程

1. **在 ini 里注册新 Tag**（例如 `PlayerState.AbilityCast.Dash`）
2. **GA 的 `AbilityTags` 填写该 Tag**
3. **在 Rules 新增一行**，填写该状态的冲突规则
4. 如需同时阻断移动/AI，在 `BlockCategoryMap` 对应 Value 中添加触发 Tag
5. 保存，热重载即生效

---

## 与其他文档的关系

| 文档 | 职责 |
|------|------|
| `Buff_Tag_Spec.md` | 所有 Tag 的命名规范和分层定义 |
| `StateConflict_TagBlock.md`（本文档） | 策划填表规范和当前规则列表 |
| `StateConflict_Technical.md` | 程序实现细节和接入方式 |
