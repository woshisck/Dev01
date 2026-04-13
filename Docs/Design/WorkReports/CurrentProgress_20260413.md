# 当前开发进展 2026-04-13

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 详细工作记录：[近战HitBox修复+冲刺越障重构报告](MeleeAndDash_WorkReport_20260413.md)  
> 最后更新：2026-04-13

---

## 项目阶段

**战斗系统底层稳定，命中判定精度修复完成；冲刺越障逻辑对齐蓝图原型，无敌帧/穿透/碰撞全部就绪。**

---

## 今日完成（2026-04-13）

### 近战命中框修复

- `IsInAnnulus`：`bAutoOffset=true` 时新增 `EffectiveOuterRadius = OuterRadius + InnerR`，前向最远触达距离恢复为 `ActRange`（配表直觉一致）
- `DrawHitboxDebug`：Debug 绘制同步对齐，可视化与判定逻辑一致

### 冲刺系统重构（GA_PlayerDash）

- **越障算法**：旧双向 SphereTrace + SetActorLocation 传送 → 新终点步进法（6×50cm，严格还原蓝图 `GetFurthesValidLocationAlongRoute`）
- **碰撞通道补齐**：新增 WorldDynamic 和 Pawn → Overlap（与蓝图对齐）
- **移除** `MaxTraversableWallThickness` 配置项
- 全程无 SetActorLocation，Capsule Overlap + 根运动 AnimScale>1 驱动穿越

### 波次系统 Bug 修复（YogGameMode）

- `SetupWaveTrigger`（AllEnemiesDead）刷怪队列清空后主动调用 `CheckWaveTrigger()`，修复全批次刷出失败时流程卡死问题
- `PercentKilled_50 / _20`：`TotalSpawnedInWave==0` 早退检查移至各分支内（避免误拦截 AllEnemiesDead 路径）

---

## 已完成（累积）

### 战斗系统

- 近战攻击 + 连击系统（ComboWindow / EarlyExit / ClearBuffer）
- 伤害流（DamageExecution / EffectContainerMap / AN_MeleeDamage 命中判定）
- 近战命中框：Annulus InnerR 补偿 ✅ 2026-04-13
- 死亡流程（GA_Dead / 消解特效 / 竞态修复 / 死亡时禁用胶囊碰撞）
- 状态冲突（StateConflict DA / OnTagUpdated 守卫）
- **冲刺系统（GA_PlayerDash）** ✅ 完整版 2026-04-13

### 热度 & 符文系统

- 热度 Phase 0-3 升降阶、衰减计时、CanPhaseUp 精确控制
- BackpackGridComponent 激活区 / 永久符文 / Flow Graph 可视化编排

### 关卡流程

- 波次刷怪：CampaignData + RoomDataAsset，预算算法，4 种触发条件
- 波次推进卡死修复 ✅ 2026-04-13
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
