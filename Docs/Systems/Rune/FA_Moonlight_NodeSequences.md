# 月光卡 FA 节点流程文档

> 项目：星骸降临
> 最后更新：2026-05-09
> 关联文档：[BuffFlow_NodeReference.md](BuffFlow_NodeReference.md) · [RuneEditor_UserGuide.md](RuneEditor_UserGuide.md)

---

## 概览

月光卡共涉及以下 FlowAsset：

| FA 资产名 | 路径 | 用途 |
|---|---|---|
| `FA_Rune_Moonlight` | `/Game/YogRuneEditor/Flows/` | 月光卡打出时的基础效果（投射物 + 命中回调） |
| `FA_Rune_MoonlightBonus` | `/Game/YogRuneEditor/Flows/` | 正向连携共用：额外多发投射物 |
| `FA_Rune_Moonlight_FwdBurn` | `/Game/YogRuneEditor/Flows/` | 月焰连携（Fire→Moon 正向）LinkFlow |
| `FA_Rune_Moonlight_FwdPoison` | `/Game/YogRuneEditor/Flows/` | 蚀月连携（Poison→Moon 正向）LinkFlow |

**连携配方汇总（月光 DA 的 `CombatCardInfo.LinkRecipes` 字段）：**

| 连携名 | 方向 | NeighborTag | ForwardFlow | BackwardFlow | 倍率 |
|---|---|---|---|---|---|
| 月焰 | Fire 在前 | `RuneTag.Fire` | `FA_Rune_Moonlight_FwdBurn` | — | 1.5 |
| 蚀月 | Poison 在前 | `RuneTag.Poison` | `FA_Rune_Moonlight_FwdPoison` | — | 1.0 |
| 月照 | Moon 在前（→Fire） | `RuneTag.Fire` | — | — | 1.3 |
| 月毒 | Moon 在前（→Poison） | `RuneTag.Poison` | — | — | 1.4 |

> 月照、月毒只有倍率，无 LinkFlow。反向连携不触发多发效果。

---

## 已注册 Gameplay Tag

> 均已写入 `Config/DefaultGameplayTags.ini`，无需重复注册。

| Tag | 用途 |
|---|---|
| `Event.Moonlight.Hit` | 主流程投射物命中时发给源 ASC，触发命中回调 |
| `Event.Moonlight.FwdBurn.Hit` | 月焰 LinkFlow 投射物命中时发给源 ASC（预留，暂未使用） |
| `Event.Moonlight.FwdPoison.Hit` | 蚀月 LinkFlow 投射物命中时发给源 ASC，触发附加中毒 |
| `State.Combo.ChainActive` | 连击链激活中，供 RequiredComboTags 读取（冲刺保留） |

---

## FA 1：FA_Rune_Moonlight（月光主流程）

### 定位

月光战斗卡打出时由 `CombatDeckComponent` 启动的主 FA。负责：
- 发射月光投射物（伤害已由投射物的 DamageGE 处理）
- 投射物命中后回调：播放命中 VFX，再结束 FA

### 节点图

```
[Start]──→ [生成远程弹幕]──→ [等待事件]──→ [特效表现: 命中 VFX]──→ [结束符文]
```

> `生成远程弹幕` 的 Out 引脚在投射物生成后**立即**触发（不等命中），`等待事件` 随即开始异步监听命中事件。投射物飞行与 FA 等待是并发的，图里是串行连线。

### 节点配置

#### 节点 A：生成远程弹幕（SpawnRangedProjectiles）

| 字段 | 值 | 说明 |
|---|---|---|
| `bPreferCombatCardProjectileClass` | ✅ | 使用卡牌 DA 上配置的投射物类 |
| `bPreferCombatCardDamageEffectClass` | ✅ | 使用卡牌 DA 上配置的伤害 GE |
| `bUseCombatCardAttackDamage` | ✅ | 伤害值来自战斗卡攻击力 |
| `bShareAttackInstanceGuid` | ✅ | 同一攻击实例，不额外消耗卡牌 |
| `bRequireRangedWeaponTag` | ✅ | 需要装备远程武器（保留默认） |
| `ProjectileCount` | 1 | 基础单发 |
| `ProjectileConeAngleDegrees` | 0 | 无散射，直线飞行 |
| `HitGameplayEventTag` | `Event.Moonlight.Hit` | 命中后向源 ASC 发送事件 |
| `bSendHitGameplayEventToSourceASC` | ✅ | 事件发给攻击者本人（玩家），供同 FA 内 WaitGameplayEvent 接收 |
| `bUseDamageAsHitGameplayEventMagnitude` | ✅ | 事件 Magnitude = 实际伤害量（供下游读取） |
| **Out 引脚** | 不连接 | WaitGameplayEvent 负责 FA 结束时机 |

#### 节点 B：等待事件（WaitGameplayEvent）

| 字段 | 值 |
|---|---|
| `EventTag` | `Event.Moonlight.Hit` |
| `Target` | BuffOwner（玩家自身） |

- **Out** → 节点 C

#### 节点 C：特效表现（Play VFX / PlayNiagara）

> 播放月光命中特效，具体 Niagara 资产由美术指定。

| 字段 | 值 |
|---|---|
| Target | LastDamageTarget（被命中敌人） |
| VFX 资产 | *(待填写：月光碎裂特效)* |

- **Out** → 节点 D

#### 节点 D：结束符文（FinishBuff）

- 无配置，直接终止 FA。

---

## FA 2：FA_Rune_MoonlightBonus（正向连携多发）

### 定位

当任意正向连携（Fire→Moon 或 Poison→Moon）激活时，若直接作为共用 LinkFlow 使用（不带额外效果时），本 FA 只补发额外投射物。

> **注意**：月焰和蚀月实际各自使用独立的 LinkFlow（`FwdBurn` / `FwdPoison`），它们内部已包含多发逻辑。  
> `FA_Rune_MoonlightBonus` 作为备用公共多发 FA，供日后新增不带特殊效果的正向连携复用。

### 节点图

```
[Start]──→ [生成远程弹幕]──→ [结束符文]
```

### 节点配置

#### 节点 A：生成远程弹幕（SpawnRangedProjectiles）

| 字段 | 值 | 说明 |
|---|---|---|
| `bPreferCombatCardProjectileClass` | ✅ | 复用月光卡的投射物外观 |
| `bPreferCombatCardDamageEffectClass` | ✅ | 复用月光卡的伤害 GE |
| `bUseCombatCardAttackDamage` | ✅ | 伤害继承卡牌攻击力 |
| `bShareAttackInstanceGuid` | ✅ | 与主流程共享攻击 GUID |
| `bRequireRangedWeaponTag` | ❌ | Link Flow 无需武器 Tag 检查 |
| `ProjectileCount` | **2** | 额外多发 2 枚（可调） |
| `ProjectileConeAngleDegrees` | 20 | 轻微散射，视觉上区别于主投射物 |
| `HitGameplayEventTag` | *(留空)* | 多发不需要命中回调 |

- **Out** → 节点 B

#### 节点 B：结束符文（FinishBuff）

---

## FA 3：FA_Rune_Moonlight_FwdBurn（月焰连携 LinkFlow）

### 定位

月焰正向连携（Fire 在前 → 月光）激活时作为 LinkFlow 运行。  
效果：额外发射 2 枚月光投射物 + 播放火焰融合 VFX。  
倍率 1.5 已由 `LinkRecipes.Multiplier` 在伤害计算层处理，本 FA 只负责多发和表现。

### 节点图

```
[Start]
   ├──→ [生成远程弹幕]──→ [结束符文]
   │
   └──→ [特效表现: 月焰连携 VFX]
```

### 节点配置

#### 节点 A：生成远程弹幕（SpawnRangedProjectiles）

| 字段 | 值 |
|---|---|
| `bPreferCombatCardProjectileClass` | ✅ |
| `bPreferCombatCardDamageEffectClass` | ✅ |
| `bUseCombatCardAttackDamage` | ✅ |
| `bShareAttackInstanceGuid` | ✅ |
| `bRequireRangedWeaponTag` | ❌ |
| `ProjectileCount` | **2** |
| `ProjectileConeAngleDegrees` | 15 |
| `HitGameplayEventTag` | *(留空)* |

- **Out** → 节点 C（结束符文）

#### 节点 B：特效表现（PlayNiagara，与节点 A 并行）

> 播放月焰连携激活瞬间的融合特效（火焰 + 月光叠加）。具体资产待美术提供。

| 字段 | 值 |
|---|---|
| Target | BuffOwner（玩家自身） |
| VFX 资产 | *(待填写：月焰连携激活特效)* |

- **Out** → *(不连接)*，特效播放后 FA 已由节点 C 结束

#### 节点 C：结束符文（FinishBuff）

---

## FA 4：FA_Rune_Moonlight_FwdPoison（蚀月连携 LinkFlow）

### 定位

蚀月正向连携（Poison 在前 → 月光）激活时作为 LinkFlow 运行。  
效果：额外发射 2 枚月光投射物，且每枚命中时对目标附加中毒 GE。  
倍率 1.0 由 LinkRecipes.Multiplier 处理（本连携的核心价值在附加中毒效果而非倍率加成）。

> **条件无需在 FA 内判断**：LinkRecipes 系统已确保只有"Poison 在前 → Moon"的顺序才会激活此 FA。FA 内部不需要条件检查。

### 节点图

```
[Start]
   ├──→ [生成远程弹幕]
   │         HitGameplayEventTag = Event.Moonlight.FwdPoison.Hit
   │         (Out 不连接)
   │
   └──→ [等待事件: Event.Moonlight.FwdPoison.Hit]
              Out──→ [施加状态: GE_Poison]──→ [结束符文]
```

### 节点配置

#### 节点 A：生成远程弹幕（SpawnRangedProjectiles）

| 字段 | 值 | 说明 |
|---|---|---|
| `bPreferCombatCardProjectileClass` | ✅ | |
| `bPreferCombatCardDamageEffectClass` | ✅ | |
| `bUseCombatCardAttackDamage` | ✅ | |
| `bShareAttackInstanceGuid` | ✅ | |
| `bRequireRangedWeaponTag` | ❌ | |
| `ProjectileCount` | **2** | 额外多发 2 枚 |
| `ProjectileConeAngleDegrees` | 15 | |
| `HitGameplayEventTag` | `Event.Moonlight.FwdPoison.Hit` | 命中后向源 ASC 发事件 |
| `bSendHitGameplayEventToSourceASC` | ✅ | 事件发给玩家 ASC，让同 FA 内的 WaitGameplayEvent 接收 |
| `bUseDamageAsHitGameplayEventMagnitude` | ❌ | Magnitude 无需携带伤害量 |

- **Out** → *(不连接)*

#### 节点 B：等待事件（WaitGameplayEvent）

| 字段 | 值 |
|---|---|
| `EventTag` | `Event.Moonlight.FwdPoison.Hit` |
| `Target` | BuffOwner |

> 每枚投射物命中时都会触发一次，因此如果发射 2 枚，本节点最多触发 2 次。  
> 如需限制为只附加一次中毒，可在 GE_Poison 层设置 `Stackable + StackLimit = 1`，或改用 `条件节点→只执行一次` 包裹。

- **Out** → 节点 C

#### 节点 C：施加状态（Apply Gameplay Effect Class）

| 字段 | 值 | 说明 |
|---|---|---|
| `GEClass` | `GE_Moonlight_Poison` | 中毒 GE（Blueprint GE，需单独创建） |
| `Target` | LastDamageTarget | WaitGameplayEvent 命中后，目标写入此选择器 |
| **SetByCaller** | *(可选)* | 如需调参则填写，默认由 GE 内部配置 |

- **Out** → 节点 D

#### 节点 D：结束符文（FinishBuff）

---

## 配套 GE：GE_Moonlight_Poison

蚀月连携需要专门创建一个 Blueprint GE。

| 字段 | 建议值 |
|---|---|
| Duration Policy | Has Duration |
| Duration Magnitude | 4.0 秒（可调） |
| Modifier: Attribute | `BaseAttributeSet.Health` |
| Modifier: Modifier Op | Additive |
| Modifier: Magnitude | −5.0 / 秒（Period = 1.0） |
| Period | 1.0 |
| Stack Policy | Aggregate by Source（多次蚀月叠层） |
| Stack Limit Count | 3（防止无限叠加） |
| Asset Tags | `Buff.Status.Poison`（用于视觉和条件判断） |

> GE 存放路径建议：`/Game/YogRuneEditor/GEs/GE_Moonlight_Poison`

---

## 创建检查清单

### FA_Rune_Moonlight

- [ ] 在 `/Game/YogRuneEditor/Flows/` 新建 YogRuneFlowAsset，命名 `FA_Rune_Moonlight`
- [ ] 添加节点 A（生成远程弹幕），按表格填写配置
- [ ] 添加节点 B（等待事件，EventTag = `Event.Moonlight.Hit`）
- [ ] 添加节点 C（特效表现，VFX 资产待填）
- [ ] 添加节点 D（结束符文）
- [ ] 连线：Start → A（Out 不连）；Start → B → C → D
- [ ] 将此 FA 填入月光 DA 的 `RuneInfo.Flow.FlowAsset`

### FA_Rune_MoonlightBonus

- [ ] 新建 `FA_Rune_MoonlightBonus`
- [ ] 添加节点 A（生成远程弹幕，ProjectileCount = 2，HitTag 留空，bRequireRangedWeaponTag = ❌）
- [ ] 添加节点 B（结束符文）
- [ ] 连线：Start → A → B
- [ ] （备用 FA，供无额外效果的正向连携使用）

### FA_Rune_Moonlight_FwdBurn

- [ ] 新建 `FA_Rune_Moonlight_FwdBurn`
- [ ] 添加节点 A（生成远程弹幕，同上，ProjectileCount = 2）
- [ ] 添加节点 B（特效表现，VFX 资产待填）
- [ ] 添加节点 C（结束符文）
- [ ] 连线：Start → A → C；Start → B（Out 不连）
- [ ] 在月光 DA LinkRecipes 中将 `月焰 ForwardFlowIdx` 指向此 FA

### FA_Rune_Moonlight_FwdPoison

- [ ] 先创建 `GE_Moonlight_Poison`（Blueprint GE）
- [ ] 新建 `FA_Rune_Moonlight_FwdPoison`
- [ ] 添加节点 A（生成远程弹幕，HitGameplayEventTag = `Event.Moonlight.FwdPoison.Hit`，ProjectileCount = 2）
- [ ] 添加节点 B（等待事件，EventTag = `Event.Moonlight.FwdPoison.Hit`）
- [ ] 添加节点 C（施加状态：GE_Moonlight_Poison，Target = LastDamageTarget）
- [ ] 添加节点 D（结束符文）
- [ ] 连线：Start → A（Out 不连）；Start → B → C → D
- [ ] 在月光 DA LinkRecipes 中将 `蚀月 ForwardFlowIdx` 指向此 FA

---

## 连携 DA 数值配置汇总

月光 DA（`RuneDA_Moonlight`）的 `CombatCardInfo.LinkRecipes` 需按下表填写：

| # | NeighborTag | bUseEffectTag | ForwardFlow | ForwardMultiplier | BackwardFlow | BackwardMultiplier |
|---|---|---|---|---|---|---|
| 0 | `RuneTag.Fire` | ✅ | `FA_Rune_Moonlight_FwdBurn` | 1.5 | — | 1.3 |
| 1 | `RuneTag.Poison` | ✅ | `FA_Rune_Moonlight_FwdPoison` | 1.0 | — | 1.4 |

> **注**：月照（Moon→Fire，Multiplier 1.3）和月毒（Moon→Poison，Multiplier 1.4）均无 LinkFlow，只填倍率。
