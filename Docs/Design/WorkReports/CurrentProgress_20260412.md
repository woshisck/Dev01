# 当前开发进展 2026-04-12

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 详细工作记录：[冲刺 + 近战清理报告](Dash_MeleeCleanup_WorkReport_20260412.md)  
> 最后更新：2026-04-12

---

## 项目阶段

**战斗系统底层稳定，冲刺系统完成 C++ 实现；关卡流程逻辑闭环，UI 层待接入。**

---

## 今日完成（2026-04-12）

### 冲刺系统（GA_PlayerDash）

- `GA_PlayerDash` C++ 类完整实现（方向 / 位移 / 越障 / 穿透 / 无敌帧 / 充能/CD / Debug）
- 位移改为 `AnimRootMotionTranslationScale` 动态缩放驱动，修复双重位移 bug
- 越障算法：前向 + 后向双 SphereTrace，前半段薄墙（≤ 4m）自动穿越
- 新增碰撞通道 `DashThrough`（ECC_GameTraceChannel5）
- 新增 Tag `Buff.Status.DashInvincible`
- `DamageAttributeSet` 物理伤害和纯粹伤害管道均接入冲刺无敌帧检查

### 近战系统清理

- `AN_MeleeDamage` 移除 `ActRotateSpeed / JumpFrameTime / FreezeFrameTime`
- `AdditionalTargetEffects` 重命名为 `AdditionalRuneEffects`，类型改为 `TArray<URuneDataAsset*>`
- `YogCharacterBase`：`PendingAdditionalHitEffects` → `PendingAdditionalHitRunes`；新增 `ReceiveOnHitRune` BlueprintNativeEvent
- 新建 `ANS_AttackRotate` AnimNotifyState（独立控制攻击旋转窗口）
- `GA_MeleeAttack`：蒙太奇改从独立 MontageMap 读取；攻击参数直接从 `AN_MeleeDamage` Notify 读取（移除 FActionData 中转）

---

## 已完成（累积）

### 战斗系统

- 近战攻击 + 连击系统（ComboWindow / EarlyExit / ClearBuffer）
- 伤害流（DamageExecution / EffectContainerMap / AN_MeleeDamage 命中判定）
- 死亡流程（GA_Dead / 消解特效 / 竞态修复 / 死亡时禁用胶囊碰撞）
- 状态冲突（StateConflict DA / OnTagUpdated 守卫）
- **冲刺系统（GA_PlayerDash）** ✅ 2026-04-12 新增

### 热度 & 符文系统

- 热度 Phase 0-3 升降阶、衰减计时、CanPhaseUp 精确控制
- BackpackGridComponent 激活区 / 永久符文 / Flow Graph 可视化编排

### 关卡流程

- 波次刷怪：CampaignData + RoomDataAsset，预算算法，4 种触发条件
- 难度配置：FDifficultyConfig 内嵌 FFloorConfig，ActiveRoomBuffs 施加
- 结算流程：背包解锁、金币发放、符文选项生成（逻辑层完整，UI 待接入）
- 切关 / 跨关状态继承（FRunState：HP / 金币 / 背包符文）
- 传送门 NeverOpen / 按 E 拾取 / 敌人死亡碰撞清除

---

## 未完成

### 立即需要（P0）

| 任务 | 说明 | 状态 |
|---|---|---|
| 符文三选一 UI | `OnLootGenerated` 委托未绑定界面 | ⏳ |
| 金币 HUD | `OnGoldChanged` 委托未绑定显示 | ⏳ |

### 近期需要（P1）

| 任务 | 说明 |
|---|---|
| BP_GA_PlayerDash 子类 | 配置 Tag + AbilityDA 蒙太奇 |
| 敌人碰撞设置 | 胶囊对 DashTrace 通道改为 Ignore |
| 血量/伤害倍率施加 | `FDifficultyConfig` 倍率未应用到刷怪 GE |
| 受击系统 | 玩家受伤无反应动画 |
| 结算回血 | 过关后 HP 无回复 |
| 分支选关 UI | 结算后无法选择下一房间路径 |
