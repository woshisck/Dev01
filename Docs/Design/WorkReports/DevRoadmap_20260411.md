# 开发路线图 & 当前进展 2026-04-11

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 配套文档：[主循环设计](../Systems/MainLoop_Design.md)、[当前进展快照](CurrentProgress_20260411.md)  
> 最后更新：2026-04-11

---

## 一、当前进展概述

**项目阶段：关卡闭环基础已打通，剩余 P0 任务集中在 UI 层。**

P0-1（跨关状态继承）、P0-2（测试数据资产）、P0-3（MobSpawner）均已完成。

今日完成：
- 传送门永不开启状态（NeverOpen BP 事件接口）
- 奖励拾取物改为按 E 主动拾取
- 敌人死亡时立即关闭胶囊碰撞
- 敌人朝向修复（BTT_RotateCorrect Interp Speed）

**剩余 P0 任务：** 符文三选一 UI（P0-4）+ 金币 HUD（P0-5）

---

## 二、已完成功能

| 模块 | 功能 | 状态 |
|---|---|---|
| **GAS 框架** | AbilitySystemComponent / AttributeSet / GameplayEffect 基础架构 | ✅ |
| **伤害系统** | DamageExecution / EffectContainerMap / 近战判定 / AN_MeleeDamage | ✅ |
| **连击系统** | 连击缓存 / ComboWindow / EarlyExit / ClearBuffer 防堆积 | ✅ |
| **死亡流程** | GA_Dead / 死亡蒙太奇 / 死亡消解特效 / 竞态修复 | ✅ |
| **死亡碰撞清除** | Die() 时立即 SetCollisionEnabled(NoCollision) | ✅ 4.11 |
| **热度系统** | Phase 0-3 升降阶 / 衰减计时 / CanPhaseUp 精确控制 / BFNode 节点 | ✅ |
| **符文系统** | BackpackGridComponent / 激活区 / Flow Graph / 永久符文 | ✅ |
| **关卡刷怪** | CampaignData / RoomDataAsset / 波次预算算法 / 4 种触发条件 | ✅ |
| **刷怪高级功能** | 白名单过滤 / 类型上限 / 关卡总上限 / 补刷 / 错开刷出 / 计时触发 | ✅ 4.11 |
| **难度配置** | FDifficultyConfig 内嵌 FFloorConfig（DA_Campaign 配置难度） | ✅ 4.11 |
| **关卡 Buff** | SpawnEnemyFromPool 刷出时对敌人施加 ActiveRoomBuffs | ✅ |
| **结算流程** | EnterArrangementPhase / 背包解锁 / 符文选项生成 / SelectLoot | ✅ |
| **奖励拾取物** | ARewardPickup 按 E 主动拾取（PendingPickup 引用机制） | ✅ 4.11 |
| **金币系统** | AddGold / OnGoldChanged / 结算发放金币 | ✅ |
| **切关逻辑** | ConfirmArrangementAndTransition / OpenLevel / GI 传递楼层编号 | ✅ |
| **跨关状态继承** | FRunState：HP / 金币 / 背包符文跨关保留 | ✅ |
| **状态冲突** | StateConflict DA / OnTagUpdated 守卫 | ✅ |
| **传送门系统** | APortal / ActivatePortals / Fisher-Yates 随机开门 / 房间类型骰子 | ✅ |
| **传送门 NeverOpen** | 未登记门关卡开始标记 bWillNeverOpen，调用 NeverOpen() | ✅ 4.11 |
| **敌人 AI** | 行为树 / 巡逻 / 攻击 / BTTask_ActivateAbilityByTag | ✅ |
| **敌人连击** | 多段连击蒙太奇 / AN_EnemyComboSection | ✅ |
| **敌人朝向** | BTT_RotateCorrect Interp Speed 修复（消除瞬间朝向）| ✅ 4.11 |
| **输入方向** | LastInputDirection 字段 / Controller.Move() 更新 | ✅ |
| **刷怪特效接口** | AMobSpawner::OnEnemySpawned BlueprintImplementableEvent | ✅ 4.11 |

---

## 三、P0 — 最小可运行闭环

> 目标：PIE 内跑通"战斗 → 结算 → 选符文 → 切关 → 状态保留 → 继续战斗"。

| 任务 | 状态 | 说明 |
|---|---|---|
| P0-1 跨关状态继承 | ✅ 完成 | FRunState HP/金币/符文均已实现 |
| P0-2 创建测试数据资产 | ✅ 完成 | DA_Room + DA_Campaign 已配置 |
| P0-3 场景放置 MobSpawner | ✅ 完成 | 测试场景已放置，日志确认刷怪正常 |
| **P0-4 整理阶段 UI** | ⏳ 待做 | 符文三选一 Widget 未实现 |
| **P0-5 金币 HUD** | ⏳ 待做 | 金币逻辑已有，HUD 显示未做 |

### 任务 P0-4：整理阶段 UI（符文三选一）

| 步骤 | 操作 | 说明 |
|---|---|---|
| 1 | 新建 `WBP_LootSelection` Widget Blueprint | 包含 3 个符文卡牌槽 |
| 2 | 在 HUD 蓝图中绑定 `GameMode.OnLootGenerated` | 收到事件时显示 Widget，填充 3 个 `FLootOption` |
| 3 | 每张卡牌显示符文名称 + 稀有度颜色 | 点击调用 `GameMode->SelectLoot(Index)` |
| 4 | 选择后隐藏 Widget，显示"确认进入下一关"按钮 | 点击调用 `GameMode->ConfirmArrangementAndTransition()` |

### 任务 P0-5：金币 HUD

| 步骤 | 操作 |
|---|---|
| 1 | 在 HUD Widget 中添加金币文本控件 |
| 2 | 获取 PlayerCharacterBase，绑定 `OnGoldChanged` 委托 |
| 3 | 委托触发时更新文本显示 `NewGold` 值 |

---

## 四、P1 — 核心体验完整

> 目标：战斗手感完整，玩家可以完整体验一局。

| 任务 | 状态 | 说明 |
|---|---|---|
| P1-1 受击系统 | ⏳ 待做 | 玩家受伤无反应动画 |
| P1-2 冲刺朝向 | ⏳ 待做 | 冲刺方向已读取 LastInputDirection，GA 侧接入待完成 |
| P1-3 完美闪避判定 | ⏳ 待做 | 无敌帧检测未实现 |
| P1-4 结算小量回血 | ⏳ 待做 | 过关后 HP 无回复 |
| P1-5 分支选关 UI | ⏳ 待做 | 结算后无法选择下一房间路径 |
| P1-6 血量/伤害倍率 | ⏳ 待做 | DA_Room 难度倍率未应用到敌人属性 |
| P1-7 真实多关切关 | ⏳ 待做 | 需要至少 2 个不同战斗场景 |

**P1 任务步骤参见：[DevRoadmap_20260410.md](DevRoadmap_20260410.md)（步骤未变，仅状态更新）**

---

## 五、P2 — 丰富玩法

| # | 任务 | 依赖 | 说明 |
|---|---|---|---|
| P2-1 | 房间类型扩展 | P1-5 | ERoomType 枚举；战斗 / 精英 / 商店 / 休息 / 事件 |
| P2-2 | 休息房间 | P2-1 | 进入时施加大量回血 GE，无战斗 |
| P2-3 | 商店系统 | P2-1 | 金币消耗 / 符文购买 / 净化 / 被动赐福 |
| P2-4 | 局外成长资源 | P0-1 | DA_Room 资源掉落 / 写入 SaveGame / 局外解锁树 |
| P2-5 | 精英房 | P2-1 | bIsEliteRoom / 精英专属怪 / 额外奖励 |
| P2-6 | 关卡词条系统 | P2-1 | 词条列表设计 + 系统实现 |
| P2-7 | Phase 3 满阶特效 | — | 角色光环粒子 + UI 状态标识 |
| P2-8 | 热度 UI | — | Phase 0-3 进度条 / 激活区视觉反馈 |
| P2-9 | 升降阶特效/音效 | — | Phase UP / DOWN 视听反馈 |
| P2-10 | 分支节点地图 UI | P1-5 | 可视化节点图替代原型按钮 |
| P2-11 | Boss 系统 | Boss 设计文档 | Boss 房间 / 多阶段攻击 |
| P2-12 | 传送门最少/最多开门约束 | — | MinOpenPortals / MaxOpenPortals 配置（当前固定规则）|
| P2-13 | 转身动画 | 动画资产 | Blend Space 1D，YawRate 驱动 TurnLeft/TurnRight |
| P2-14 | 按 E 拾取 UI 提示 | — | 玩家进入范围时显示"按 E 拾取"HUD 提示 |

---

## 六、P3 — 后期内容

| # | 任务 | 前置条件 |
|---|---|---|
| P3-1 | 满阶专属动作集 | Phase 3 特效完成；需动画资产 |
| P3-2 | 局外大厅界面 | 元进度系统完成 |
| P3-3 | 武器选择界面 | 多把武器资产完成 |
| P3-4 | 消耗道具系统 | 需道具设计文档 |
| P3-5 | 事件房间 | 需事件内容设计文档 |
| P3-6 | 祭坛房间 | 需祭坛规则设计文档 |
| P3-7 | 深渊模式 | 需深渊规则设计文档 |
| P3-8 | 多结局系统 | 需支线 / 结局设计文档 |

---

## 七、里程碑时间线

```
2026-04-11 ──────────────────────────────────────────────▶
            │
            P0 完成（剩余 UI 层 P0-4 + P0-5）
            │  战斗→结算→选符文→切关 可跑通
            │  跨关状态不丢失 ✅
            │
            P1 完成
            │  受击 / 冲刺 / 完美闪避 / 分支选关 / 多关切换
            │  单局体验完整，可内部测试
            │
            P2 完成
            │  商店 / 多房间类型 / 元进度 / Phase3 表现
            │  游戏内容基本可玩，可进行外部小范围测试
            │
            P3 完成（目标 2027 EA）
            │  大厅 / 武器选择 / 深渊 / 多结局
```
