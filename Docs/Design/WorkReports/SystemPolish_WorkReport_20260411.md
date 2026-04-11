# 系统细化与交互优化 工作报告 2026-04-11

> 适用范围：关卡流程 / 传送门 / 拾取交互 / 敌人 AI  
> 适用人群：策划 + 程序  
> 最后更新：2026-04-11

---

## 本次完成内容

### 1. 传送门永不开启状态（NeverOpen）

**问题**：之前所有不在 `PortalDestinations` 中的门只是"不会被开启"，但没有专门的视觉/碰撞处理，玩家可能误入已关闭状态的门。

**方案**：在 `APortal` 新增 `NeverOpen()` BlueprintImplementableEvent 和 `bWillNeverOpen` 属性。GameMode 在 `StartLevelSpawning()` 时遍历场景所有 Portal，凡是未登记在本关 `PortalDestinations` 中的门，调用 `NeverOpen()`。

**效果**：
- BP_Portal 中可实现"永不开启"门的专属视觉（去除门效果、换为静态装饰、关闭碰撞）
- `bWillNeverOpen` 为 true 的门不受 `ActivatePortals()` 影响（即使哪怕勾选到也不会意外开启）
- 调用时机在关卡开始时确定，不会后期改变

**修改文件**：`Source/DevKit/Public/Map/Portal.h`、`Private/GameModes/YogGameMode.cpp`

---

### 2. 奖励拾取物改为按 E 拾取（Press-E Pickup）

**问题**：原逻辑是玩家走入 `ARewardPickup` 碰撞范围自动触发战利品界面，体验不直觉：玩家可能在清场路上无意触发。

**方案**：
- `ARewardPickup` 改为：进入范围时将引用存储到 `PlayerCharacterBase.PendingPickup`，离开时清除
- 玩家按 E 键（现有 `IA_Interact` + `Input_Interact`）时，Controller 检查 `PendingPickup` 是否有效，有则调用 `TryPickup()`
- `TryPickup()` 执行实际拾取并通知 GameMode

**效果**：玩家可以在击杀最后一个敌人后自由走动，主动靠近发光拾取物再按 E 确认选符文，减少误触。

**修改文件**：`Public/Map/RewardPickup.h`、`Private/Map/RewardPickup.cpp`、`Public/Character/PlayerCharacterBase.h`、`Private/Character/YogPlayerControllerBase.cpp`

---

### 3. 敌人死亡时禁用胶囊碰撞

**问题**：敌人死亡后尸体仍有胶囊碰撞，会阻挡玩家移动和敌人寻路。

**方案**：在 `AYogCharacterBase::Die()` 开始时立即调用：
```cpp
if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
```

此时死亡动画尚未开始播放（GA_Dead 随后触发），碰撞已被关闭，不影响动画根运动。

**效果**：尸体动画完整播放，但胶囊碰撞即时关闭，玩家和其他敌人可以正常穿越尸体。

**修改文件**：`Private/Character/YogCharacterBase.cpp`

---

### 4. 敌人瞬间朝向修复（BTT_RotateCorrect）

**问题**：敌人攻击后需要面向玩家时，使用 `RInterpConstantTo(Speed=0)` 会直接返回 Target（即瞬间朝向），体验生硬。

**方案**：在 BP_BTT_RotateCorrect 中将 Interp Speed 从 0 改为 270（约 0.7 秒内完成 ±180°），并设置朝向误差阈值（Tolerance = 10°）。

**效果**：敌人在攻击前/后会平滑旋转面向玩家，视觉不再跳帧。

**相关文档**：[敌人朝向修正配置指南](../FeatureConfig/EnemyRotation_ConfigGuide.md)

---

## 关键决策

| 决策点 | 选择 | 原因 |
|---|---|---|
| NeverOpen 调用时机 | `StartLevelSpawning()` 而非 `BeginPlay()` | GameMode 此时已持有 ActiveRoomData，可正确比对 PortalDestinations |
| NeverOpen vs 仅关闭碰撞 | BlueprintImplementableEvent | 视觉实现由策划/美术在 BP_Portal 蓝图侧自由决定，程序不硬编码 |
| 拾取改为按 E | 保留现有 IA_Interact，扩展 Interact() | 已有 Input_Interact 绑定，直接复用，不需要新 InputAction 资产 |
| 死亡碰撞关闭时机 | Die() 函数最开始，GA_Dead 激活之前 | 确保无论死亡动画时长多久，碰撞已经及时关闭 |

---

## 遗留问题

- [ ] **BP_Portal 需要实现 `Event Never Open`**（策划/程序）：隐藏门效果、关闭 CollisionVolume、显示静态装饰网格
- [ ] **拾取物 UI 提示**（P1）：玩家进入范围时屏幕应该显示"按 E 拾取"提示
- [ ] **NeverOpen 与 PortalDestinations 最少开门数量约束**（P1）：当前传送门最多开门无上限配置（见 Portal_Design.md 4.5 待完善）

---

## 下次计划

- 符文三选一 Widget（P0-4）
- 金币 HUD（P0-5）
- 受击系统（P1-1）
