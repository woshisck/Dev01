# 当前开发进展 2026-04-14

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 详细工作记录：[符文升级系统工作报告](RuneUpgrade_WorkReport_20260414.md)  
> 最后更新：2026-04-14

---

## 项目阶段

**背包 UI 基础框架完成；符文三选一自动入格就绪；符文升级系统（Lv.I→II→III）实现完毕，满级符文自动从奖励池过滤。**

---

## 今日完成（2026-04-14）

### 符文升级系统

- `FRuneInstance.UpgradeLevel`：0=Lv.I / 1=Lv.II / 2=Lv.III，`GetUpgradeMultiplier()` 内联辅助函数
- `BackpackGridComponent::FindRuneByName`：按符文名查找已放置符文
- `BackpackGridComponent::GetMaxLevelRuneNames`：返回满级符文名列表
- `BackpackGridComponent::NotifyRuneUpgraded`：重启 BuffFlow 使新倍率生效 + 广播 UI 刷新
- `PlayerCharacterBase::AddRuneToInventory`：拾取时先检查升级，再寻位放置，背包满则进待放置列表
- `YogGameMode::GenerateLootBatch`：满级符文从奖励候选池中过滤排除

### 背包 UI（前次完成，2026-04-14 第一版）

- WBP_BackpackScreen 基类 + 三选一自动入格
- FallbackLootPool 兜底符文池
- Tab 键绑定背包界面

---

## 已完成（累积）

### 战斗系统

- 近战攻击 + 连击系统（ComboWindow / EarlyExit / ClearBuffer）
- 伤害流（DamageExecution / EffectContainerMap / AN_MeleeDamage 命中判定）
- 近战命中框：Annulus InnerR 补偿 ✅ 2026-04-13
- 死亡流程（GA_Dead / 消解特效 / 竞态修复 / 死亡时禁用胶囊碰撞）
- 状态冲突（StateConflict DA / OnTagUpdated 守卫）
- 冲刺系统（GA_PlayerDash）✅ 完整版 2026-04-13

### 热度 & 符文系统

- 热度 Phase 0-3 升降阶、衰减计时、CanPhaseUp 精确控制
- BackpackGridComponent 激活区 / 永久符文 / Flow Graph 可视化编排
- **符文升级系统** ✅ 2026-04-14
- 满级符文奖励池过滤 ✅ 2026-04-14

### 背包 UI

- WBP_BackpackScreen 基类 ✅ 2026-04-14
- 三选一自动入格 ✅ 2026-04-14
- FallbackLootPool 兜底 ✅ 2026-04-14

### 关卡流程

- 波次刷怪：CampaignData + RoomDataAsset，预算算法，4 种触发条件
- 波次推进卡死修复 ✅ 2026-04-13
- 难度配置：FDifficultyConfig 内嵌 FFloorConfig，ActiveRoomBuffs 施加
- 结算流程：背包解锁、金币发放、符文选项生成（逻辑层完整）
- 切关 / 跨关状态继承（FRunState：HP / 金币 / 背包符文）
- 传送门 NeverOpen / 按 E 拾取 / 敌人死亡碰撞清除

---

## 未完成

### 立即需要（P0）

| 任务 | 说明 | 状态 |
| --- | --- | --- |
| 金币 HUD | `OnGoldChanged` 委托未绑定显示 | ⏳ |
| 背包 UI 升级标识 | WBP_BackpackScreen 需显示 Lv.II/III 标签 | ⏳ |

### 近期需要（P1）

| 任务 | 说明 |
| --- | --- |
| BP_GA_PlayerDash 子类 | 配置 Tag + AbilityDA 蒙太奇 |
| 敌人碰撞设置 | 胶囊对 DashTrace 通道改为 Ignore |
| 血量/伤害倍率施加 | `FDifficultyConfig` 倍率未应用到刷怪 GE |
| 受击系统 | 玩家受伤无反应动画 |
| 结算回血 | 过关后 HP 无回复 |
| 分支选关 UI | 结算后无法选择下一房间路径 |
