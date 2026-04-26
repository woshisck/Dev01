# Tag 命名空间速查

> 创建新 Tag 时判断归属命名空间的速查表。含各命名空间职责、是否挂 ASC、挂载时机、对应 .ini 文件。
>
> 来源：原 memory `reference_tag_namespaces.md`（2026-04-18 起沉淀）。已转写到本文件。
>
> 配套：[GameplayTag_MasterGuide](GameplayTag_MasterGuide.md)（命名规则）· [Tag_SituationalGuide](Tag_SituationalGuide.md)（"我想做 X 用哪个 Tag"）· [GA_TagFields_Guide](GA_TagFields_Guide.md)（GA 上 5 个 Tag 字段怎么用）

---

## 创建新 Tag 的决策树

```
要创建一个新 Tag？
│
├─ 触发某个 GA 激活（GameplayEvent，瞬时） ─────────────────→ Action.*
├─ 符文完成后通知其他符文（跨系统广播） ────────────────────→ Event.Rune.*
├─ 玩家正在执行某技能 / 动作（GA 激活期间持有） ────────────→ PlayerState.AbilityCast.*
├─ 敌人 GA 的身份标签（BT Task 查找用） ────────────────────→ Enemy.*
├─ 符文 / Buff 效果产生的运行时状态（挂 ASC） ──────────────→ Buff.Status.*
├─ 全局状态（死亡 / 对话 / 交互，不归属单一技能） ────────────→ State.*
├─ 这个符文是什么类型 / 稀有度（元数据） ────────────────────→ Buff.Rune.*
├─ GE 的行为标识（GE AssetTags） ───────────────────────────→ Buff.Effect.*
├─ 符文何时触发（OnHit / OnKill 等，贴在符文 GA AssetTags） ─→ Buff.Trigger.*
├─ SetByCaller 数值通道 Key ────────────────────────────────→ Buff.Data.*
├─ AttributeSet 字段的 Tag 化引用（SetByCaller 管道用） ─────→ Attribute.*
├─ StateConflict.BlockCategoryMap 的 Key ───────────────────→ Block.*（仅作 Key，不挂 ASC）
├─ 敌人 AI 行为状态机 ─────────────────────────────────────→ Enemy_Behavior.*
└─ 房间类型 / 层级（RoomDataAsset.RoomTags） ───────────────→ Room.*（不挂 ASC）
```

---

## 各命名空间速查表

| 命名空间 | 挂 ASC | 挂载时机 | .ini 文件 |
|---|---|---|---|
| `Action.*` | 不挂（Event）/ 临时挂（ANS 窗口） | GameplayEvent 或 ANS | `Config/Tags/BuffTag.ini` |
| `Attribute.*` | 不挂 | 仅作 Key | `Config/Tags/BuffTag.ini` |
| `Buff.Rune.*` | 不挂 | 贴 RuneDA 元数据 | `Config/Tags/BuffTag.ini` |
| `Buff.Effect.*` | 不挂 | GE AssetTags | `Config/Tags/BuffTag.ini` |
| `Buff.Trigger.*` | 不挂 | 符文 GA AssetTags | `Config/Tags/BuffTag.ini` |
| `Buff.Status.*` | **挂** | GA 激活期间 / GE 授予 | `Config/Tags/BuffTag.ini` |
| `Buff.Data.*` | 不挂 | SetByCaller Key | `Config/Tags/BuffTag.ini` |
| `Event.Rune.*` | 不挂 | SendGameplayEvent 广播 | `Config/Tags/BuffTag.ini` |
| `PlayerState.AbilityCast.*` | **挂** | GA Activation Owned Tags | `Config/Tags/PlayerTag.ini` |
| `Enemy.*` | **挂** | GA Ability Tags（激活期间） | `Config/Tags/EnemyTag.ini` |
| `Enemy_Behavior.*` | **挂** | BT / AI 系统管理 | `Config/Tags/EnemyTag.ini` |
| `State.*` | **挂** | StateConflict 冲突管理 | `Config/Tags/StateTag.ini` |
| `Block.*` | 不挂 | 仅 BlockCategoryMap Key | `Config/Tags/StateTag.ini` |
| `Room.*` | 不挂 | RoomDataAsset.RoomTags | `Config/Tags/RoomTag.ini` |

> 文件位置随 `Config/DefaultGameplayTags.ini` 中 `+TagSourceFiles` 配置，新增 .ini 需要同步登记。

---

## `Action.*` 的两种用法

1. **GameplayEvent（瞬时）**：`SendGameplayEventToActor` 触发 GA 或 FA，不挂 ASC
   - 示例：`Action.Dead`、`Action.Knockback`
2. **AnimNotifyState（窗口持续）**：ANS 期间 `AddLooseGameplayTag`，代码可 `HasMatchingGameplayTag` 轮询
   - 示例：`Action.Combo.LastHit`、`Action.Combo.DashSavePoint`

---

## StateConflict 配置要点

- `BlockCategoryMap` Key = `Block.AI` / `Block.Movement`（**不是**状态 Tag，是系统分类 Key）
- `BlockCategoryMap` Value = 触发该阻断的状态 Tag 列表（如 `Buff.Status.Dead`）
- `ConflictMap` ActiveTag = 任意层的状态 Tag（哪个 Tag 挂上来触发规则）
- `ConflictMap` BlockTags / CancelTags = 要屏蔽 / 取消的 GA 的 AbilityTags

详见 [StateConflict_TagBlock](StateConflict/StateConflict_TagBlock.md) · [StateConflict_Technical](StateConflict/StateConflict_Technical.md)
