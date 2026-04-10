# 当前开发进展 2026-04-10

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 详细任务清单：[开发路线图](DevRoadmap_20260410.md)

---

## 项目阶段

**核心战斗系统完成，正在进入关卡流程闭环阶段。**

战斗底层已可运行，单关卡可以走通"进场 → 战斗 → 触发结算"。当前主要任务是打通多关卡循环，让玩家状态在关卡间正确继承，并补上整理阶段的 UI 交互。

---

## 已完成

### 战斗系统

- 近战攻击 + 连击系统（ComboWindow / EarlyExit / ClearBuffer 防堆积）
- 伤害流（DamageExecution / EffectContainerMap / AN_MeleeDamage 命中判定）
- 死亡流程（GA_Dead / 死亡蒙太奇 / 消解特效 / 竞态修复）
- 状态冲突（StateConflict DA / OnTagUpdated 守卫）

### 热度 & 符文系统

- 热度 Phase 0-3 升降阶，含衰减计时和 CanPhaseUp 精确控制
- BackpackGridComponent 激活区 / 永久符文 / Flow Graph 可视化编排
- BFNode 节点集（OnPhaseUpReady / IncrementPhase / DecrementPhase / PhaseDecayTimer / OnHeatReachedZero）

### 关卡流程

- 波次刷怪：CampaignData + RoomDataAsset，预算算法，4 种触发条件，Wave / OneByOne 两种刷怪方式
- 关卡 Buff：刷怪时对敌人施加 ActiveRoomBuffs
- 结算流程：背包解锁、金币发放、符文选项生成（逻辑层完整，UI 未做）
- 切关：OpenLevel + GameInstance 传递楼层编号

### 角色 & AI

- 玩家：LastInputDirection 输入方向、冲刺 GA（方向待接入）
- 敌人：行为树 / 巡逻 / 攻击 / 多段连击蒙太奇

---

## 未完成

### 立即需要（P0）

| 任务 | 说明 |
|---|---|
| ⚠️ **跨关状态继承** | 切关后 HP / 金币 / 背包符文全部丢失，需写入 GameInstance |
| 测试数据资产 | DA_Room + DA_Campaign 未配置，无法测试关卡流程 |
| 场景 MobSpawner | 测试场景未放置出生点 |
| **整理阶段 UI** | 符文三选一界面未实现，当前无法选符文 |
| 金币 HUD | 金币有逻辑无显示 |

### 近期需要（P1）

| 任务 | 说明 |
|---|---|
| 受击系统 | 玩家受伤无反应动画 |
| 冲刺朝向 | 冲刺方向未读取摇杆输入 |
| 完美闪避判定 | 无敌帧检测未实现 |
| 分支选关 UI | 结算后无法选择下一房间路径 |
| 结算回血 | 过关后 HP 无回复 |
| 血量/伤害倍率 | DA_Room 配置的难度倍率未应用到敌人 |

---

## 最近三次 Commit 主要内容

| Commit | 主要内容 |
|---|---|
| `446f92fa` | 综合更新（内容见 git log）|
| `c786ef5b` | 设计文档目录结构重组 + 新增配置指南 |
| `db88ce51` | EnemyTag 补充 + 敌人攻击能力 / StateConflict 资产更新 |

---

## 本周目标（4.10-4.11）

```
① 跨关状态继承 → 最高优先，其他 P0 任务依赖它
② 创建 DA_Room + DA_Campaign 测试资产
③ 场景放置 MobSpawner
④ 整理阶段 UI（符文三选一）
⑤ 金币 HUD

完成以上 5 项后，可跑通完整单局闭环。
```
