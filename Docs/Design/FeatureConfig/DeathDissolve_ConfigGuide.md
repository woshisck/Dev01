# 死亡消解特效配置指南

**功能模块**：`GA_Dead` + `GameplayCue`  
**适用对象**：所有使用 `YogCharacterBase` 的角色（玩家、敌人通用）  
**状态**：已实现，可直接配置

---

## 概述

角色死亡后，在死亡动画播放结束时自动触发一个 GameplayCue，用于驱动材质消解、粒子消散等视觉效果。  
消解特效在角色仍然存活（未 Destroy）的状态下触发，角色会等待 **2 秒**后再被销毁，确保消解动画有足够时间完整播放。

**`DissolveGameplayCueTag` 为可选字段**：不填则跳过 2 秒等待，死亡动画结束后立即销毁角色。

---

## 时序说明

### 有 DissolveGameplayCueTag

```
死亡触发
  └─ 播放死亡蒙太奇（可选）
       └─ BlendOut 时立即触发消解 GC  ← 角色此时仍存活，材质消解可正常工作
            └─ 等待 2 秒
                 └─ 角色 Destroy
```

### 无 DissolveGameplayCueTag

```
死亡触发
  └─ 播放死亡蒙太奇（可选）
       └─ BlendOut
            └─ 角色立即 Destroy（无延迟）
```

### 无蒙太奇 + 有 DissolveGameplayCueTag

```
死亡触发
  └─ 立即触发消解 GC
       └─ 等待 2 秒
            └─ 角色 Destroy
```

---

## 配置步骤

### 第一步：在 AbilityData 里填写 DissolveGameplayCueTag

找到角色对应的 `AbilityData` 资产（通常在 `Content/Docs/Data/` 下角色子目录内）。

打开后定位到：
```
PassiveMap
  └─ Key: Action.Dead
       ├─ Montage：死亡动画（可为空）
       └─ DissolveGameplayCueTag：填写消解 GC Tag（见下）
```

Tag 命名规范：`GameplayCue.Character.Dissolve.XXX`  
例：`GameplayCue.Character.Dissolve.Humanoid`

> 如果不需要消解效果，`DissolveGameplayCueTag` **留空**即可，角色在动画结束后立即销毁。

---

### 第二步：创建 GameplayCue Blueprint

1. 在 Content Browser 中右键 → `Blueprint Class`  
2. 父类选择：
   - **`GameplayCueNotify_Static`**：适合纯粒子/音效，不需要 Tick，性能更好（推荐）
   - **`GameplayCueNotify_Actor`**：适合需要持续 Tick 或 Spawn Actor 的复杂效果
3. 命名与 Tag 对应，例如：`GC_Character_Dissolve_Humanoid`
4. 在 Blueprint 的 `Details` 面板中，将 `GameplayCueTag` 设为第一步填写的 Tag

---

### 第三步：在 GameplayCue Blueprint 中实现消解逻辑

在 `OnExecute`（Static）或 `WhileActive` 事件中实现效果。

#### 材质消解（推荐方式）

材质消解依赖角色 Mesh 的动态材质实例，**角色存活时才有效**，GA_Dead 已确保触发时角色仍存活。

```
OnExecute(EventReference)
  ├─ Get Actor from EventReference → 获取目标角色
  ├─ Get Mesh Component
  ├─ Create Dynamic Material Instance（Slot 0 / 1 / ...）
  └─ Timeline: 驱动 Dissolve 参数从 0 → 1（持续 2 秒）
```

消解材质参数名建议统一命名为 `DissolveAmount`，方便在多个材质中复用逻辑。

#### 世界坐标粒子（Niagara）

若使用 Niagara 粒子，需要在**世界坐标**生成，而非附加到角色上，避免角色 Destroy 后粒子也被删除。

```
OnExecute(EventReference)
  ├─ Get Location from EventReference（已包含 Actor 位置）
  └─ Spawn Niagara System at Location（不要 Attach）
```

> ⚠️ **不要**使用 `Spawn Attached`，否则 Actor 销毁后粒子立即消失。

---

## 注意事项

| 情况 | 行为 |
|---|---|
| `DissolveGameplayCueTag` 留空 | 动画结束后立即销毁，无延迟 |
| 无死亡蒙太奇 + 有 Tag | GC 立即触发，等 2s 后销毁 |
| 蒙太奇被取消（异常情况） | 跳过 2s 等待，立即销毁 |
| `EnemyCharacterBase` 的 30s 兜底计时器 | 仅在 GA_Dead 完全未激活时触发，正常情况不会触及 |

---

## 常见问题

**Q：消解动画没播完角色就消失了？**  
A：检查 `DissolveGameplayCueTag` 是否正确填写，且 Tag 名称与 GC Blueprint 的 `GameplayCueTag` 完全一致。

**Q：材质看起来没有消解，但粒子正常？**  
A：确认材质里有 `DissolveAmount` 参数节点，且 GC 里创建的是动态材质实例（不是直接修改静态材质）。

**Q：我想在动画特定帧开始消解，而不是动画结束时？**  
A：在死亡蒙太奇中放置一个自定义 `UAnimNotify` 子类（C++ 或 BP），在 `Notify()` 里直接从角色 ASC 调用 `ExecuteGameplayCue`，同时将 `DissolveGameplayCueTag` **留空**（避免重复触发）。  
这样消解由蒙太奇帧精确控制，GA_Dead 仍负责 2s 后销毁角色。
