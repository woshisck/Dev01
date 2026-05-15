# 节点制作红线

> 适用范围：所有 BuffFlow / Yog Flow 节点的新增与修改  
> 原则：**不到万不得已，不新增节点类**。现有 39 个节点覆盖绝大多数场景。  
> 最后更新：2026-05-09

---

## 一、制作前必须先做的事

1. **查 [BuffFlow_NodeReference.md](../系统/Rune/BuffFlow_NodeReference.md)** — 确认没有现成节点能覆盖需求。  
2. **查 [RuneEditor_UserGuide.md](../系统/Rune/RuneEditor_UserGuide.md) 中的常见 Flow 模板** — 确认组合用法不能解决问题。  
3. **和程序确认** — 你认为「缺节点」的场景，90% 都可以用现有节点组合 + 配置解决。

---

## 二、绝对禁止（Red Lines）

### 🔴 禁止为同一效果重复造节点
- 已有 `EffectApplyState` → 不再新增「施加燃烧」「施加冰冻」等具体状态节点。  
- 已有 `EffectDamage` → 不再新增「暴击伤害」「溅射伤害」专用节点（用条件分支 + 同一节点配置解决）。

### 🔴 禁止因「想要中文名」而新增节点
- 节点显示名可以在 `BuildNodeLibraryPanel()` 中改，不需要新建 C++ 类。

### 🔴 禁止把业务逻辑硬编码进节点类
- 节点类只提供「机制」（施加GE / 检查Tag / 生成投射物），**不包含具体数值或 Tag 字符串**。  
- 具体数值放数值表 Key，具体 Tag 通过节点属性配置。

### 🔴 禁止一个 Flow 图超过 12 个节点
- 超过 12 个节点说明这个符文承担了太多职责。  
- 解决方案：拆分成子流程（Child Flow）或用 `EffectApplyProfile` / `SpawnAreaProfile` 将一组固定配置封装成配置资产。

### 🔴 禁止在 Flow 图里做数值计算
- 所有数值来自 **数值表 Key**，通过 `GetRuneTuningValue()` 在 C++ 或 Blueprint 中读取。  
- Flow 节点只做「触发/条件/效果」，不做加减乘除。

### 🔴 禁止跳过 Tag 文档直接新建 Tag
- 新 Tag 前必须查 `Docs/Tags/Tag_Namespaces.md`，按命名空间规则添加，修改后同步文档。

---

## 三、强烈不推荐（Yellow Lines）

### 🟡 不推荐「只为一个符文」新增节点类
- 如果只有一个符文需要这个行为，优先用 `WaitGameplayEvent` + 自定义 GameplayEvent Tag 解决。  
- 通用性不足的节点是长期维护负担。

### 🟡 不推荐在同一 Flow 图里同时使用多个 Delay 节点
- 多 Delay 等价于在 Flow 里写定时器链，难以调试。  
- 改用 GE 的 Periodic / Duration 机制让引擎管理时序。

### 🟡 不推荐 DoOnce 嵌套在 Probability 里
- `DoOnce` 保证单次触发，`Probability` 控制触发概率，二者串联即可，不要嵌套。

### 🟡 不推荐用 AddTag 替代 ApplyState
- `AddTag` 只加 Loose Tag，没有 GE 的生命周期管理（持续时间/层数/移除回调）。  
- 需要持续时间或层数的状态一律用 `ApplyState` + Blueprint GE。

---

## 四、何时可以新增节点类（C++）

满足以下**全部条件**才考虑：

1. 需求已经在至少 **3 个不同符文** 的 Flow 中重复出现。  
2. 无法用现有节点的**属性配置**差异化解决。  
3. 和程序讨论后确认没有更轻量的替代方案（配置资产、GameplayEvent、子流程）。  
4. 新节点有清晰的**一句话职责描述**，且不超出「机制」范畴。

新增后必须：
- 在 `BuildNodeLibraryPanel()` 注册（填中文显示名 + 描述）。  
- 在 `BuffFlow_NodeReference.md` 补充一行速查记录。

---

## 五、现有节点能力速查

| 我想要… | 用这个节点 |
|---|---|
| 命中时触发效果 | `TriggerDamageDealt` |
| 施加 Burn/Poison/任意 GE 状态 | `EffectApplyState` |
| 判断目标是否有某个 Tag | `ConditionHasTag` |
| 临时给角色加一个 Tag（短时） | `EffectAddTag` + `LifecycleDelay` + `EffectRemoveTag` |
| 给角色一个带时限的状态 Tag | `EffectApplyState`（一个只带 Tag 的 GE，设 Duration） |
| 连招越深效果越强 | 数值表开启 **Combo Bonus**，不需要额外节点 |
| 生成多个投射物 | `SpawnRangedProjectiles`（支持 Combo Count） |
| 按概率触发 | `ConditionProbability` |
| 投射物命中时触发另一个效果 | `SpawnProjectileProfile` → 配置 HitFlow 回调 |
| 符文之间通信 | `WaitGameplayEvent` + `SendGameplayEvent`（目前用 Tag 约定） |
| 读取目标当前 GE 层数 | `Get Rune Info` 节点（输出 StackCount） |
