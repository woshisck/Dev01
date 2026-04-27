# 01 战斗核心 — 已完成功能盘点

> 范围：玩家近战 / 远程基础攻击循环、连击、闪避、命中反馈、敌人攻击行为、韧性 / 霸体。
> 武器细节（火绳枪 / 弹药 / 拾取）→ [04_Weapons.md](04_Weapons.md)；视听反馈层（闪白 / 升阶发光）→ [05_FeedbackLayer.md](05_FeedbackLayer.md)。

---

## 功能总览

| 功能名称                                               | 需求简介                               | 完成状态（代码） | 完成状态（编辑器）                                |
| -------------------------------------------------- | ---------------------------------- | -------- | ---------------------------------------- |
| 近战攻击 + 多段连击 (COMBAT-001)                           | 玩家轻 / 重攻击 + 连招打断 / 桥接窗口            | ✅ 编译通过   | ✅ BP GA 派生 + 蒙太奇已配                       |
| AN_MeleeDamage HitStop 模式选择器 + 命中事件广播 (COMBAT-009) | AN 上直选 HitStop 类型，命中广播事件           | ✅ 编译通过   | AN Details 直接配                           |
| 冲刺连招桥接 DashSave (COMBAT-004)                       | 冲刺接住连招进度                           | ✅ 编译通过   | ✅ ANS 已挂蒙太奇                              |
| 冲刺 GA_PlayerDash (COMBAT-002)                      | Roguelite 闪避 + 越障 + 无敌帧            | ✅ 编译通过   |                                          |
| 冲刺空气墙 DashBarrier (FEAT-033)                       | 关卡边界精确挡冲刺                          | ✅ 编译通过   | ✅ BP_DashBarrier 待创建 + 关卡放置              |
| 台阶 / 坡度越障 + 调试 CVar (COMBAT-007)                   | 球扫描上移 MaxStepHeight                | ✅ 编译通过   | — 无需                                     |
| 充能 / CD 系统 SkillChargeComponent (SKILL-001)        | 多格充能 + 顺序回复                        | ✅ 编译通过   | ⚙ 各 GA 蓝图接 HasCharge / ConsumeCharge     |
| HitStop 命中停顿（FA + Tag 驱动）(COMBAT-006 / FEAT-032)   | 攻击命中冻帧 + 慢动作                       | ✅ 编译通过   | ⚙ 玩家攻击蒙太奇加 AN_HitStop（VFX-005）           |
| 韧性系统 Poise vs Resilience (COMBAT-005)              | 攻方 / 防方韧性比较 + 霸体计数                 | ✅ 编译通过   | ⚙ 敌人 Resilience 初始值已填，霸体阈值待批量配           |
| 霸体金光 + 敌人韧性 DA 化 (COMBAT-008)                      | 金黄脉冲视觉 + DA 配 Threshold / Duration | ✅ 编译通过   | ⚙ ENEMY-DA-002 待填                        |
| ANS_AutoTarget 攻击自动吸附 (TAG-AUTOTGT)                | 攻击瞬间吸附最近活敌                         | ✅ 编译通过   | ✅ ANS 已挂蒙太奇                              |
| 敌人 BT 攻击任务 (AI-001)                                | BT 节点用 Tag 激活 GA                   | ✅ 编译通过   | ✅ BT 已用                                  |
| BT 攻击前摇红光 + 装饰器 (AI-002)                           | 攻击前预警 + 过滤无可激活 GA                  | ✅ 编译通过   | ✅ BT 已用                                  |
| 敌人多段连击蒙太奇 (EnemyCombo)                             | 精英敌人连招而非单段                         | ✅ 编译通过   | ✅                                        |
| 热度携带符文（余烬 / 炽核）(FEAT-026)                          | 切关带走 1 / 2 阶热度                     | ✅ 编译通过   | ⚙ FA_Rune_余烬 / 炽核 待做（HEAT-CARRY-001/002） |

> 状态口径：✅ 完成 / ⚙ 待配置（注明待办）/ — 不涉及编辑器配置

---

## 玩家攻击与连招

### [COMBAT-001] 近战攻击 + 多段连击
- **设计需求**：玩家需要"按节奏轻 / 重攻击"的核心动作循环；连招要有打断 / 桥接窗口；命中点用环形扇区判定，不用胶囊体。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/AbilitySystem/Abilities/GA_MeleeAttack.h` + `Private/AbilitySystem/Abilities/GA_MeleeAttack.cpp`
  - `Public/AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h`（中间基类）+ 各 LightAtk1~4 / HeavyAtk1~4 / DashAtk 派生
  - `Public/Animation/AN_MeleeDamage.h` + `Private/Animation/AN_MeleeDamage.cpp`（命中判定 Notify）
  - `Public/AbilitySystem/Abilities/YogTargetType_Melee.h`（IsInAnnulus 环形扇区）
- **设计文档**：[AttackDamage_ConfigGuide](../Systems/Combat/AttackDamage_ConfigGuide.md)
- **验收方式**：
  1. PIE 中按 4 次轻攻击，应播完 LightAtk1~4 蒙太奇且每段都触发命中检测
  2. 连击窗口外按攻击，应从 LightAtk1 重新开始

### [COMBAT-009] AN_MeleeDamage HitStop 模式选择器 + 命中事件广播
- **设计需求**：设计师在 AN 上直接选 HitStop 类型（None/Freeze/Slow），不用每个 FA 写 Tag；命中时能广播事件给镜头 / 音效订阅。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Animation/AN_MeleeDamage.h`（`EHitStopMode` 枚举 + `OnHitEventTags` 数组）
  - `Public/Character/YogCharacterBase.h`（`PendingHitStopOverride` / `PendingOnHitEventTags`）
  - `Private/AbilitySystem/Abilities/GA_MeleeAttack.cpp`（EndAbility 清空 PendingOnHitEventTags）
- **设计文档**：[HitStop_Design](../Design/Combat/HitStop_Design.md)
- **验收方式**：AN_MeleeDamage Details 选 Freeze，命中时屏幕短暂冻结；选 Slow，命中后短暂慢动作

### [COMBAT-004] 冲刺连招桥接（DashSave）
- **设计需求**：在攻击连招"桥接窗口"内冲刺，可保留连招进度继续接后续攻击 — 避免冲一下就要从头打。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/AbilitySystem/Abilities/GA_PlayerDash.cpp`（CanActivateAbility 检测 `Action.Combo.DashSavePoint`）
  - `Public/AbilitySystem/YogAbilitySystemComponent.h` + `.cpp`（`ApplyDashSave` / `ConsumeDashSave` / `DashSaveExpired` 2s 过期）
  - 各 LightAtk4 / HeavyAtk4 派生（ActivateAbility 调 `ConsumeDashSave`）
- **设计文档**：[Dash_Design](../Systems/Combat/Dash_Design.md)
- **验收方式**：连招到第 3 段后冲刺，下一击应继续从第 4 段开始而非第 1 段

---

## 玩家闪避

### [COMBAT-002] 冲刺（GA_PlayerDash）— 越障 / 根运动 / 无敌帧
- **设计需求**：Roguelite 必备闪避；要能跨小台阶（上 step），但被"边界墙"硬挡；冲刺期间无敌。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/AbilitySystem/Abilities/GA_PlayerDash.h` + `Private/AbilitySystem/Abilities/GA_PlayerDash.cpp`
- **设计文档**：[Dash_Design](../Systems/Combat/Dash_Design.md)
- **验收方式**：
  1. 朝小台阶冲刺，应越过；朝高墙冲刺，应停在墙前
  2. 冲刺期间被敌人攻击，HP 不掉（State.Invincible Tag 生效）

### [FEAT-033] 冲刺空气墙（DashBarrier）
- **设计需求**：关卡边界要能精确挡住冲刺，但不影响越障逻辑。
- **状态**：✅ C++完成；⚙ 关卡中放置 `BP_DashBarrier`（添加 BoxCollision + ActorTag `DashBarrier` + DashTrace 通道 Block）
- **核心文件**：
  - `Private/AbilitySystem/Abilities/GA_PlayerDash.cpp`（`GetFurthestValidDashDistance` 中 ActorHasTag 检测）
- **设计文档**：[FeatureLog FEAT-033](../FeatureLog.md)
- **验收方式**：朝放置了 BP_DashBarrier 的方向冲刺，应精准停在墙前；朝普通台阶冲刺，越障行为不受影响

### [COMBAT-007] GA_PlayerDash 台阶 / 坡度越障 + 调试 CVar
- **设计需求**：球形扫描在角色当前 Z 做水平射线时，台阶竖面会 Block 导致冲刺提前停止 — 要能跨小台阶。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/AbilitySystem/Abilities/GA_PlayerDash.cpp`（Sweep 整体上移 `CMC->MaxStepHeight`）
- **验收方式**：控制台 `Dash.DebugTrace 1` 开调试线条；冲过 45cm 以下台阶应正常通过

### [SKILL-001] 充能 / CD 系统（SkillChargeComponent）
- **设计需求**：冲刺 / 技能要支持"多格充能 + 顺序回复"；符文可加格数 / 改 CD 速 — 单 CD 与多格充能用同一组件处理。
- **状态**：✅ C++完成；⚙ 各 GA 蓝图 CanActivate / Activate 接 `HasCharge` / `ConsumeCharge`
- **核心文件**：
  - `Public/Component/SkillChargeComponent.h` + `Private/Component/SkillChargeComponent.cpp`
  - `Public/AbilitySystem/Attribute/PlayerAttributeSet.h`（`MaxDashCharge` / `DashCooldownDuration`）
- **设计文档**：[SkillCharge_Guide](../Systems/Combat/SkillCharge_Guide.md)
- **验收方式**：用符文加 +1 冲刺格，BeginPlay 后玩家应能连冲 2 次；CD 缩短 25% 符文应让单格 CD 变短

---

## 命中反馈与韧性

### [COMBAT-006 / FEAT-032] HitStop 命中停顿（冻帧 + 慢动作）
- **设计需求**：攻击命中要有顿挫感；轻击短冻、重击带慢放；旧 AN_HitStop 已迁到 FA + Tag 驱动 + Montage Pause/PlayRate 控制。
- **状态**：✅ C++完成；⚙ 玩家攻击蒙太奇命中帧加 AN_HitStop（VFX-005 任务）
- **核心文件**：
  - `Public/Animation/HitStopManager.h` + `Private/Animation/HitStopManager.cpp`（`UTickableWorldSubsystem`，真实时间计时）
  - `Public/BuffFlow/Nodes/BFNode_HitStop.h` + `Private/BuffFlow/Nodes/BFNode_HitStop.cpp`
  - Tags：`Buff.Status.HitStop.Freeze` / `Buff.Status.HitStop.Slow`（`Config/Tags/BuffTag.ini`）
- **设计文档**：[HitStop_Design](../Design/Combat/HitStop_Design.md)
- **验收方式**：
  1. 玩家攻击命中敌人，应短暂冻结约 50ms
  2. FA 中加 `[OnCritHit] → [AddTag: Buff.Status.HitStop.Freeze]`，暴击命中应触发更强冻结

### [COMBAT-005] 韧性系统（Poise vs Resilience）+ 霸体
- **设计需求**：普通敌人能被打断，精英要重击 / 暴击才能打断；连续受击触发霸体反制 — 玩家不能无限连击。
- **状态**：✅ C++完成；⚙ DA 待填（ENEMY-DA-002）
- **核心文件**：
  - `Public/AbilitySystem/Attribute/BaseAttributeSet.h`（`Resilience` 属性）
  - `Public/AbilitySystem/YogAbilitySystemComponent.h` + `.cpp`（`ReceiveDamage` 比较攻方 / 防方韧性，`PoiseHitCount` 计数 5s 内重置）
  - `Private/AbilitySystem/Abilities/GA_MeleeAttack.cpp`（OnEventReceived 写 `CurrentActionPoiseBonus`）
- **设计文档**：[PoiseSystem_ConfigGuide](../Systems/Combat/PoiseSystem_ConfigGuide.md)
- **验收方式**：精英敌人 Resilience=150，玩家轻击（20+100=120 < 150）应不打出受击；重击（50+100=150 ≥ 150）应打出

### [COMBAT-008] 霸体金光 + 敌人韧性 DA 化
- **设计需求**：霸体要有视觉提示（金黄脉冲）；不同敌人的韧性参数（Threshold / Duration）能在 DA 配。
- **状态**：✅ C++完成；⚙ DA 待填（ENEMY-DA-002 — 给所有 DA_Enemy_* 填 SuperArmorThreshold/Duration）
- **核心文件**：
  - `Public/Data/EnemyData.h`（`SuperArmorThreshold` / `SuperArmorDuration` 字段）
  - `Private/Character/EnemyCharacterBase.cpp`（BeginPlay 自动推到 ASC）
  - `Public/AbilitySystem/YogAbilitySystemComponent.h` + `.cpp`（`StartSuperArmorFlash` / `StopSuperArmorFlash`，6Hz 脉冲）
- **设计文档**：[CharacterFlash_Technical](../Systems/VFX/CharacterFlash_Technical.md)
- **验收方式**：连续轻击同一敌人 3 次，第 4 次起敌人应发金黄脉冲 2s 内免疫受击

---

## 攻击辅助（自动吸附 / 攻击转向）

### [TAG-AUTOTGT] ANS_AutoTarget — 攻击自动吸附最近活敌
- **设计需求**：俯视角动作游戏的攻击不需要精确瞄准；攻击瞬间自动转向最近的活着敌人，且吸附目标若死亡要重新搜索。
- **状态**：✅ C++完成（含 [FIX-008] 死亡敌人过滤）
- **核心文件**：
  - `Public/Animation/ANS_AutoTarget.h` + `Private/Animation/ANS_AutoTarget.cpp`（`FindBestTarget` 中 Cast 到 `AYogCharacterBase` 用 `IsAlive()` 过滤）
- **设计文档**：[Systems/CodeCatalog](../Systems/CodeCatalog.md)
- **验收方式**：攻击时角色应朝最近活敌转向；目标半路死亡，下一次攻击应吸到下一个活敌

---

## 敌人 AI 攻击行为

### [AI-001] 敌人行为树攻击任务（BTTask_ActivateAbilityByTag）
- **设计需求**：行为树用 Tag 激活 GA，不绑死具体技能；不同敌人复用同一 BT 节点，按 DA 中能力配置随机激活。
- **状态**：✅ C++完成（含 [FIX-010] 自动过滤无蒙太奇 Tag）
- **核心文件**：
  - `Public/AI/BTTask_ActivateAbilityByTag.h` + `Private/AI/BTTask_ActivateAbilityByTag.cpp`
- **设计文档**：[Systems/AI/](../Systems/AI/)
- **验收方式**：敌人 BT 跑到攻击节点，应根据 DA_AbilityMontage_* 中可用 Tag 随机激活一个攻击 GA；蒙太奇为 None 的 Tag 应被自动跳过

### [AI-002] BT 攻击前摇红光集成 + BTDecorator_HasAbilityWithTag
- **设计需求**：敌人攻击前摇要预警（发红光），玩家有反应窗口；装饰器先过滤无可激活 GA 防止节点空转。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/AI/BTTask_ActivateAbilityByTag.h`（`bPreAttackFlash` 字段，与 GA 生命周期同步）
  - `Public/AI/BTTask_PreAttackFlash.h` + `Private/AI/BTTask_PreAttackFlash.cpp`（独立前摇任务）
  - `Public/AI/BTDecorator_HasAbilityWithTag.h` + `Private/AI/BTDecorator_HasAbilityWithTag.cpp`
- **验收方式**：敌人即将攻击时身体应发红光脉冲；GA 结束后红光自动停

### [EnemyCombo] 敌人多段连击蒙太奇
- **设计需求**：精英敌人能连招而不是单段攻击 — 多段攻击要能用蒙太奇 Section 串接。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/AbilitySystem/Abilities/GA_EnemyMeleeAttacks.h` + `Private/AbilitySystem/Abilities/GA_EnemyMeleeAttacks.cpp`
  - `Public/Animation/AN_EnemyComboSection.h`
- **设计文档**：[EnemyCombo_ConfigGuide](../Systems/Combat/EnemyCombo_ConfigGuide.md)
- **验收方式**：精英敌人攻击时应连续打出多段而非单段，蒙太奇 Section 切换无停顿

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| ENEMY-DA-002 | 所有 `DA_Enemy_*` → Enemy\|Poise → 填 `SuperArmorThreshold`（建议 3）+ `SuperArmorDuration`（建议 2s） | [TASKS](../PM/TASKS.md) |
| VFX-005（玩家蒙太奇）| 玩家攻击蒙太奇命中帧添加 `AN Hit Stop`（轻攻 F=50ms；重攻 F=80ms/S=120ms@25%） | [TASKS](../PM/TASKS.md) |
| VFX-004（敌人蒙太奇）| 敌人攻击蒙太奇前摇帧拖入 `ANS Pre Attack Flash` 区间 | [CharacterFlash_Technical](../Systems/VFX/CharacterFlash_Technical.md) |
