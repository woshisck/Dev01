# 开发路线图 & 当前进展 2026-04-10

> 适用范围：星骸降临 Dev01  
> 适用人群：策划 + 程序  
> 配套文档：[主循环设计](../Systems/MainLoop_Design.md)、[主循环工作报告](MainLoop_WorkReport_20260410.md)  
> 最后更新：2026-04-10

---

## 一、当前进展概述

**项目阶段：核心战斗系统完成，准备进入关卡流程闭环阶段。**

战斗底层（GAS / 伤害 / 热度 / 连击 / 符文 Flow Graph）已基本成型，可以实现"进入场景 → 战斗 → 敌人死亡 → 触发结算"的单关卡流程。

当前主要缺口：
- 切关时玩家状态（HP / 金币 / 符文）会丢失，跨关继承未实现
- 整理阶段没有 UI，玩家无法真正"选符文"
- 没有真正的分支选关系统，关卡只能线性推进
- 局外大厅、武器选择、元进度系统均未开始

**下一个里程碑目标：** 完成 P0，在 PIE 内跑通一次完整的"战斗 → 结算 → 选符文 → 切关 → 状态保留 → 继续战斗"闭环。

---

## 二、已完成功能

| 模块 | 功能 | 状态 |
|---|---|---|
| **GAS 框架** | AbilitySystemComponent、AttributeSet、GameplayEffect 基础架构 | ✅ |
| **伤害系统** | DamageExecution / EffectContainerMap / 近战判定 / AN_MeleeDamage | ✅ |
| **连击系统** | 连击缓存 / ComboWindow / EarlyExit / ClearBuffer 防堆积 | ✅ |
| **死亡流程** | GA_Dead / 死亡蒙太奇 / 死亡消解特效 / 竞态修复 | ✅ |
| **热度系统** | Phase 0-3 升降阶 / 衰减计时 / CanPhaseUp 精确控制 / BFNode 节点 | ✅ |
| **符文系统** | BackpackGridComponent / 激活区 / Flow Graph / 永久符文 | ✅ |
| **关卡刷怪** | CampaignData / RoomDataAsset / 波次预算算法 / 4 种触发条件 / Wave + OneByOne | ✅ |
| **关卡 Buff** | SpawnEnemyFromPool 刷出时对敌人施加 ActiveRoomBuffs | ✅ |
| **结算流程** | EnterArrangementPhase / 背包解锁 / 符文选项生成 / SelectLoot | ✅ |
| **金币系统** | AddGold / OnGoldChanged / 结算发放金币 | ✅ |
| **切关逻辑** | ConfirmArrangementAndTransition / OpenLevel / GI 传递楼层编号 | ✅ |
| **状态冲突** | StateConflict DA / OnTagUpdated 守卫 | ✅ |
| **蒙太奇配置** | MontageConfigDA / AN_MeleeDamage / AnimNotifyState_AddGameplayTag | ✅ |
| **敌人 AI** | 行为树 / 巡逻 / 攻击 / BTTask_ActivateAbilityByTag | ✅ |
| **敌人连击** | 多段连击蒙太奇 / AN_EnemyComboSection | ✅ |
| **输入方向** | LastInputDirection 字段 / Controller.Move() 更新 | ✅ |

---

## 三、P0 — 最小可运行闭环

> 目标：PIE 内跑通"战斗 → 结算 → 选符文 → 切关 → 状态保留 → 下一关战斗"。  
> 全部完成前不进入 P1。

### 任务 P0-1：跨关状态继承 ⚠️ 最高优先级

**问题：** 当前 `OpenLevel` 后 Character 重建，HP / 金币 / 背包符文全部丢失。

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 在 `YogGameInstanceBase` 新增 `FRunState` 结构体，存储 CurrentHP / CurrentGold / PlacedRunes | `YogGameInstanceBase.h` |
| 2 | `ConfirmArrangementAndTransition` 切关前将玩家状态写入 `GI->RunState` | `YogGameMode.cpp` |
| 3 | 新关卡 `APlayerCharacterBase::BeginPlay` 时从 `GI->RunState` 恢复 HP / 金币 / 符文 | `PlayerCharacterBase.cpp` |
| 4 | 死亡时调用 `GI->ClearRunState()`，清空局内数据 | `YogGameInstanceBase.cpp` |

**验收：** 切关后打开角色面板，HP / 金币 / 背包符文与上一关结算后一致。

---

### 任务 P0-2：创建测试用数据资产

| 步骤 | 操作 |
|---|---|
| 1 | 创建 `DA_Room_Prison_Normal`：填 Enemy Pool（Rat × 1，Score=3）/ Loot Pool（3 个测试符文）/ Low Config（2 波，预算 10）|
| 2 | 创建 `DA_Campaign_TestRun`：FloorTable 填 2 条，Floor1 / Floor2 均指向上面的 DA_Room |
| 3 | 在 GameMode BP Details 面板 → Campaign 分类 → 将 `Campaign Data` 指向 `DA_Campaign_TestRun` |

参见 [关卡系统配置指南](../Systems/LevelSystem_ConfigGuide.md) 完整步骤。

---

### 任务 P0-3：场景放置 MobSpawner

| 步骤 | 操作 |
|---|---|
| 1 | 在测试战斗场景中放置 3-6 个 `AMobSpawner` Actor |
| 2 | 分布在地图各处，避免扎堆 |
| 3 | PIE 后搜索日志 `SpawnEnemyFromPool` 确认刷怪正常 |

---

### 任务 P0-4：整理阶段 UI（符文三选一）

| 步骤 | 操作 | 说明 |
|---|---|---|
| 1 | 新建 `WBP_LootSelection` Widget Blueprint | 包含 3 个符文卡牌槽 |
| 2 | 在 HUD 蓝图中绑定 `GameMode.OnLootGenerated` | 收到事件时显示 Widget，填充 3 个 `FLootOption` |
| 3 | 每张卡牌显示符文名称 + 稀有度颜色 | 点击调用 `GameMode->SelectLoot(Index)` |
| 4 | 选择后隐藏 Widget，显示"确认进入下一关"按钮 | 点击调用 `GameMode->ConfirmArrangementAndTransition()` |

**验收：** 击杀所有敌人后 Widget 弹出，选完符文点确认后切关。

---

### 任务 P0-5：金币 HUD

| 步骤 | 操作 |
|---|---|
| 1 | 在 HUD Widget 中添加金币文本控件 |
| 2 | 获取 PlayerCharacterBase，绑定 `OnGoldChanged` 委托 |
| 3 | 委托触发时更新文本显示 `NewGold` 值 |

---

## 四、P1 — 核心体验完整

> 目标：战斗手感完整，玩家可以完整体验一局（战斗 + 分支选关 + 多关推进）。

### 任务 P1-1：受击系统

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 在 `PlayerCharacterBase` 绑定 `ASC.ReceivedDamage` 委托 | `PlayerCharacterBase.cpp` |
| 2 | 收到伤害时发送 GameplayEvent `Action.GetHit` | — |
| 3 | 创建 `GA_PlayerGetHit`：播放受击蒙太奇，短暂打断普通攻击 | Blueprint |
| 4 | 配置受击蒙太奇：至少 4 方向（前后左右）根据伤害来源方向选择 | — |

---

### 任务 P1-2：冲刺朝向

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 在 `GA_PlayerDash.ActivateAbility` 里读取 `PlayerCharacterBase->LastInputDirection` | Blueprint |
| 2 | 将 `LastInputDirection` 转为旋转，调用 `SetActorRotation` | — |
| 3 | 若 `LastInputDirection` 为零向量，则朝角色当前面朝方向冲刺 | — |

---

### 任务 P1-3：完美闪避判定

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 在闪避 GA 的无敌帧期间，设置 Actor 上的自定义 Bool `bInvincible = true` | Blueprint / `PlayerCharacterBase.h` |
| 2 | 在 `DamageExecution` 或受击前检查 `bInvincible`：为 true 时 = 完美闪避 | `DamageExecution.cpp` |
| 3 | 完美闪避时向玩家 ASC 授予 `Action.Heat.CanPhaseUp` Tag（持续 0.3s）| — |
| 4 | 同时阻断本次伤害（不扣血）| — |

---

### 任务 P1-4：结算小量回血

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 创建 `GE_SettlementHeal`：Instant，回复固定量 HP（初始建议 10-15%）| Blueprint |
| 2 | 在 `EnterArrangementPhase` 末尾对玩家施加 `GE_SettlementHeal` | `YogGameMode.cpp` |

---

### 任务 P1-5：分支选择系统（原型）

**设计规则：** 显示房间类型 + 词条 + 奖励稀有度，不预告敌人。

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 在 `FloorEntry` 添加 `DisplayName`（房间类型文本）和 `PreviewRewardRarity`（枚举）字段 | `CampaignDataAsset.h` |
| 2 | `GameMode` 新增 `GetNextBranchOptions()`，返回当前楼层可选的 2-3 个 `FFloorEntry` | `YogGameMode.h/.cpp` |
| 3 | 新建 `WBP_BranchSelection` Widget：显示 2-3 个按钮，各自标注房间类型和奖励稀有度 | Blueprint |
| 4 | 玩家选择后调用 `GameMode->SetSelectedBranch(Index)`，再调用 `ConfirmArrangementAndTransition` | — |

---

### 任务 P1-6：血量/伤害倍率应用

| 步骤 | 操作 | 文件 |
|---|---|---|
| 1 | 创建 `GE_DifficultyModifier`：用 Attribute-based Modifier 修改 MaxHealth 和 Damage | Blueprint |
| 2 | `SpawnEnemyFromPool` 刷出敌人后，根据 `ActiveDifficultyConfig.HealthMultiplier / DamageMultiplier` 计算 Magnitude | `YogGameMode.cpp` |
| 3 | 对刷出的 `EnemyASC` 施加 `GE_DifficultyModifier` | — |

---

### 任务 P1-7：真实多关切关

| 步骤 | 操作 |
|---|---|
| 1 | 创建至少 2 个不同的战斗场景（`L_Prison_01`、`L_Prison_02`）|
| 2 | 在 `DA_Campaign_TestRun` 的 FloorTable 各条目填写对应 `LevelName` |
| 3 | 验证 `OpenLevel` 后楼层编号正确、玩家状态保留（依赖 P0-1）|

---

## 五、P2 — 丰富玩法

> 目标：局内循环有足够的内容深度，商店、多房间类型、元进度均可体验。

| # | 任务 | 依赖 | 说明 |
|---|---|---|---|
| P2-1 | 房间类型扩展 | P1-5 | `FloorEntry` 加 `ERoomType` 枚举；战斗 / 精英 / 商店 / 休息 / 事件 |
| P2-2 | 休息房间 | P2-1 | 进入时施加大量回血 GE，无战斗 |
| P2-3 | 商店系统 | P2-1 | 金币消耗 / 符文购买 / 符文净化 / 被动赐福（需先完成商店设计文档）|
| P2-4 | 局外成长资源 | P0-1 | DA_Room 配置资源掉落 / 写入 SaveGame / 局外解锁树 |
| P2-5 | 精英房 | P2-1 | `bIsEliteRoom = true` / 精英专属怪 / 额外奖励 |
| P2-6 | 关卡词条系统 | P2-1 | 需先设计词条列表，再做系统实现 |
| P2-7 | Phase 3 满阶特效 | — | 角色光环粒子 + UI 状态标识 |
| P2-8 | 热度 UI | — | Phase 0-3 进度条 / 激活区视觉反馈 |
| P2-9 | 升降阶特效/音效 | — | Phase UP / DOWN 视听反馈 |
| P2-10 | 分支节点地图 UI | P1-5 | 可视化节点图替代原型按钮 |
| P2-11 | Boss 系统 | 需 Boss 设计文档 | Boss 房间 / 多阶段攻击 |
| P2-12 | `GA_PlayMontage` Timer 驱动 | — | MontageConfigDA 替代 AnimNotifyState（遗留任务）|
| P2-13 | `RuneDA` IdentityTags | — | `FGameplayTagContainer` 供 UI 过滤（遗留任务）|

---

## 六、P3 — 后期内容

> 优先级最低，部分需要先完成设计文档再开发。

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
当前 ────────────────────────────────────────────────────────▶
      │
      P0 完成
      │  战斗→结算→选符文→切关 可跑通
      │  跨关状态不丢失
      │
      P1 完成
      │  受击 / 冲刺 / 完美闪避 / 分支选关 / 多关切换
      │  单局体验完整，可内部测试
      │
      P2 完成
      │  商店 / 多房间类型 / 元进度 / Phase3 表现
      │  游戏内容基本可玩，可进行外部小范围测试
      │
      P3 完成
      │  大厅 / 武器选择 / 深渊 / 多结局
      │  游戏功能完整，进入 EA 准备阶段（目标 2027）
```

---

## 八、当前最近任务（本周 4.10-4.11）

按顺序执行：

```
① P0-1  跨关状态继承（YogGameInstanceBase + PlayerCharacterBase）
② P0-2  创建 DA_Room_Prison_Normal + DA_Campaign_TestRun
③ P0-3  场景放置 MobSpawner
④ P0-4  WBP_LootSelection（符文三选一 UI）
⑤ P0-5  金币 HUD

--- P0 验收通过后继续 ---

⑥ P1-1  受击系统（GA_PlayerGetHit）
⑦ P1-2  冲刺朝向（GA_PlayerDash 读 LastInputDirection）
```
