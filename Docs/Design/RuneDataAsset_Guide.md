# RuneDataAsset 使用指南

> 更新日期：2026-04-05  
> 对象：策划 + 程序  
> 详细的 BuffFlow FA 节点说明见 [BuffFlow_Guide.md](BuffFlow_Guide.md)

---

## 一、概念说明

**DA_Rune（RuneDataAsset）是游戏中"符文 / Buff / 卡牌"的数据层。**

DA 只负责：
- 展示信息（名称/图标/描述）
- GE 行为规则（Duration/Stack）
- 数值效果（Effects[]：属性修改 / Tag / 被动GA）
- 背包格子形状
- 指向哪个 FA（逻辑由 FA 负责，DA 不写逻辑）

---

## 二、DA 结构一览

```
DA_Rune_XXX
  ├── Rune Name / Icon / Description / Buff Type   ← 展示信息（顶层平铺）
  ├── Shape                                         ← 背包格子形状
  ├── ▼ Rune Config                                 ← GE 行为 + 效果列表
  │     ├── Buff ID           (数值 ID，策划表引用)
  │     ├── Buff Tag          (GameplayTag，唯一标识此 Buff)
  │     ├── Buff Duration     (0=瞬发, -1=永久, >0=秒数)
  │     ├── Stack Type        (None/Refresh/Stack)
  │     ├── Max Stack         (仅 Stack 模式，最大层数)
  │     ├── Stack Reduce Type (All=全部移除 / One=逐层移除)
  │     └── Effects[]         ← 效果片段，点击 + 添加
  └── ▼ Flow
        └── Buff Flow Asset   ← FA 资产（符文激活时启动，卸下时停止）
```

---

## 三、展示信息

| 字段 | 类型 | 说明 |
|---|---|---|
| `Rune Name` | FName | 符文名称 |
| `Rune Icon` | Texture2D | 背包 UI 图标 |
| `Rune Description` | FText | Tooltip 描述 |
| `Buff Type` | Enum | **增益 / 减益 / 无**，供 UI 显示颜色/图标方向 |
| `Shape` | FRuneShape | 背包格子形状，Cells 填相对于锚点的格子偏移列表 |

---

## 四、Rune Config 详解

### Duration（持续时间）

| BuffDuration 值 | GE 类型 | 说明 |
|---|---|---|
| `0` | Instant | 瞬发，立即触发一次，不留存 |
| `-1` | Infinite | 永久，需手动移除（装备类符文） |
| `> 0` | HasDuration | 持续指定秒数后自动移除 |

### Stack Type（堆叠方式）

| 值 | 含义 |
|---|---|
| `None` | 不叠加，不刷新时间（同时只能有一个） |
| `Refresh` | 不增加层数，但每次施加时重置计时 |
| `Stack` | 每次施加 +1 层（至 MaxStack），刷新计时 |

### Stack Reduce Type（到期减层）

仅在 `Stack Type != None` 且 `Duration > 0` 时有意义。

| 值 | 含义 |
|---|---|
| `All` | 时间到期时一次性移除所有层数 |
| `One` | 时间到期时只移除 1 层并刷新剩余层的计时 |

### 常用配置组合

```
永久被动（装备时持续）：
  Duration = -1（永久）, Stack = None

叠加 Buff（狂暴：每次命中+1层，最多10层，逐层消退）：
  Duration = 5.0s
  Stack Type = Stack, Max Stack = 10
  Stack Reduce Type = One

刷新 Buff（护盾：再次触发重置计时）：
  Duration = 3.0s
  Stack Type = Refresh

一次性 Buff（爆发：施加即生效）：
  Duration = 0（Instant）
```

---

## 五、Effects[] 效果片段

点击 `+` 选择类型，同一 DA 可叠加多个：

### Add Attribute Modifier（属性修改）

最常用。直接在编辑器里选属性、操作符、填数值。

| 子字段 | 说明 |
|---|---|
| `Attribute` | 要修改的属性（下拉选择，来自各 AttributeSet） |
| `Mod Op` | Additive（加）/ Multiplicative（乘）/ Override（覆盖） |
| `Value` | 数值 |

**示例：**
```
Attack +20         → Attribute=Attack,        ModOp=Additive,        Value=20
攻速 ×1.1         → Attribute=AttackSpeed,   ModOp=Multiplicative,  Value=0.1
击退力 +600        → Attribute=KnockbackForce, ModOp=Additive,       Value=600
```

### Add Gameplay Tags（授予 Tag）

GE 激活期间给目标授予 GameplayTag，GE 移除时自动撤销。

```
GrantedTags: State.Berserk.Active
→ 其他 GA/GE 可通过 Has Tag(State.Berserk.Active) 判断当前状态
```

### Trigger Gameplay Ability（被动 GA）

符文装备时授予一个被动 GA，卸下时自动清除。

配置：选择 `AbilityClass`

**GA 读取数值的正确方式：**
- GA 执行时从 AttributeSet 读取属性值，而非从 DA 读 Params
- 例：`GA_Knockback` 读 `KnockbackForce` 属性并施加冲量

### Advanced Modifier / Gameplay Cue / Execution Calculation

高级用法，不熟悉 GAS 的策划保持空，联系程序配置。

---

## 六、Flow（逻辑层）

| 字段 | 说明 |
|---|---|
| `Buff Flow Asset` | FA 资产（FlowAsset）。符文激活时自动启动，卸下时自动停止 |

FA 里的逻辑**不读取 DA 的配置字段**。  
FA 通过 `Get GE Info` 节点查询 ASC 上 GE 的运行时状态（层数、剩余时间等）。

详见 [BuffFlow_Guide.md](BuffFlow_Guide.md)。

---

## 七、常见符文配置速查

| 效果类型 | 配置位置 |
|---|---|
| 纯数值加成（Attack +20） | Effects → Add Attribute Modifier |
| 击退力增强 | Effects → Add Attribute Modifier（KnockbackForce） |
| 装备时授予 GA（如击退GA） | Effects → Trigger Gameplay Ability |
| 激活期间标记状态 | Effects → Add Gameplay Tags |
| 叠加 Buff（振奋） | RuneConfig（Stack, MaxStack=5, One） |
| 可视化条件逻辑 | Flow → Buff Flow Asset |
| 持续特效（随层数变化） | Flow → Buff Flow Asset（FA 里用 GetGEInfo + PlayNiagara） |

---

## 八、变更记录（2026-04-05）

### 主要重构
- `FRuneBuffConfig` → `FRuneConfig`（Duration 改为 float，Stack 改为简化枚举）
- `FRuneValues`（FRuneAttributeModifier + FRuneCalcSpec + Params）→ 全部迁移到 `Effects[]`
- `FRuneFlowConfig.PassiveAbilityClass` → 迁移到 `Effects → Trigger GA Fragment`
- `URuneDataAsset.Effects[]`（顶层）→ 移入 `RuneConfig.Effects[]`

### 新增
- `KnockbackForce` 属性（BaseAttributeSet）：攻击方施加的击退冲量
- `EnemyAttributeSet.KnockBackDist` 已存在：敌方对击退的承受幅度（保留）

### 策划迁移清单
- [ ] 所有 DA_Rune：`BuffConfig → RuneConfig`，Duration 改 float
- [ ] 所有 DA_Rune：`Values → Attribute Modifiers` 迁入 `Effects[]`
- [ ] 所有 DA_Rune：`Flow → Passive Ability Class` 迁入 `Effects → Trigger GA`
- [ ] Flow Graph 中的 `CompareFloat` 节点：A/B 重新连线（类型已变为数据引脚）
