# 当前开发进展 2026-04-11

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 详细任务清单：[开发路线图](DevRoadmap_20260411.md)  
> 最后更新：2026-04-11

---

## 项目阶段

**关卡流程闭环基本打通，正在完善体验细节并推进 UI 层。**

战斗底层已可运行，P0-1（跨关状态继承）、P0-2（测试数据资产）、P0-3（MobSpawner 放置）均已完成。本日完成了关卡流程的多项体验细化：传送门永不开启状态、按 E 拾取奖励、死亡碰撞清除。P0 剩余任务集中在 UI 层（符文三选一、金币 HUD）。

---

## 已完成

### 战斗系统

- 近战攻击 + 连击系统（ComboWindow / EarlyExit / ClearBuffer 防堆积）
- 伤害流（DamageExecution / EffectContainerMap / AN_MeleeDamage 命中判定）
- 死亡流程（GA_Dead / 死亡蒙太奇 / 消解特效 / 竞态修复）
- **死亡时立即禁用胶囊碰撞**（防止尸体阻挡移动/寻路）✅ 2026-04-11 新增
- 状态冲突（StateConflict DA / OnTagUpdated 守卫）

### 热度 & 符文系统

- 热度 Phase 0-3 升降阶，含衰减计时和 CanPhaseUp 精确控制
- BackpackGridComponent 激活区 / 永久符文 / Flow Graph 可视化编排
- BFNode 节点集（OnPhaseUpReady / IncrementPhase / DecrementPhase / PhaseDecayTimer / OnHeatReachedZero）

### 关卡流程

- 波次刷怪：CampaignData + RoomDataAsset，预算算法，4 种触发条件，Wave / OneByOne 两种刷怪方式
- MobSpawner 白名单过滤 / 同类上限 / 关卡总上限 / 补刷 / 错开刷出 / 计时触发可配置
- 难度配置迁移至 DA_Campaign（FDifficultyConfig 内嵌 FFloorConfig）
- 关卡 Buff：刷怪时对敌人施加 ActiveRoomBuffs
- 结算流程：背包解锁、金币发放、符文选项生成（逻辑层完整，UI 待接入）
- **奖励拾取物改为按 E 拾取**（ARewardPickup + PendingPickup 引用）✅ 2026-04-11 新增
- 切关：OpenLevel + GameInstance 传递楼层编号
- 跨关状态继承：FRunState 存储 HP / 金币 / 背包符文，BeginPlay 时恢复
- **传送门永不开启（NeverOpen）**：未登记门关卡开始时调用 NeverOpen()，BP 可实现纯装饰状态 ✅ 2026-04-11 新增

### 角色 & AI

- 玩家：LastInputDirection 输入方向、冲刺 GA
- 敌人：行为树 / 巡逻 / 攻击 / 多段连击蒙太奇
- **敌人朝向修复**：BTT_RotateCorrect Interp Speed = 270，消除瞬间朝向 ✅ 2026-04-11 新增

---

## 未完成

### 立即需要（P0）

| 任务 | 说明 | 状态 |
|---|---|---|
| **整理阶段 UI** | 符文三选一界面未实现，当前无法选符文 | ⏳ 待做 |
| 金币 HUD | 金币有逻辑无显示 | ⏳ 待做 |

### 近期需要（P1）

| 任务 | 说明 |
|---|---|
| 受击系统 | 玩家受伤无反应动画 |
| 完美闪避判定 | 无敌帧检测未实现 |
| 分支选关 UI | 结算后无法选择下一房间路径 |
| 结算回血 | 过关后 HP 无回复 |
| 血量/伤害倍率 | DA_Room 难度配置的倍率未应用到敌人 |

---

## 最近三次 Commit 主要内容

| Commit | 主要内容 |
|---|---|
| `288a5460` | 传送门 NeverOpen + 奖励按 E 拾取 + 死亡碰撞关闭（本日）|
| `f96f4149` | RoomTag 体系重构 + bEliteOnly 移除 + L1/L2/L3 层级支持 |
| `a74a3430` | BTT_RotateCorrect 朝向修复 + 连击/朝向/关卡系统文档补全 |

---

## 本周目标（4.11 剩余）

```
① P0-4  WBP_LootSelection（符文三选一 UI）— 最高优先
② P0-5  金币 HUD
--- P0 验收通过后继续 ---
③ P1-1  受击系统
④ P1-2  冲刺朝向细化
```
