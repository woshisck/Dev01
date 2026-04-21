# 开发进展总结 — 2026-04-21

> 面向策划/程序双向阅读。完整功能细节见 [FeatureLog.md](../../FeatureLog.md)，文档索引见 [INDEX.md](../INDEX.md)。

---

## 一、项目概况

| 项 | 状态 |
|---|---|
| 引擎版本 | UE 5.4 |
| 当前阶段 | Playtest 版本构建中（战斗手感 + 武器系统 + UI 完善） |
| 目标里程碑 | EA 2027 |
| 核心玩法框架 | ✅ 完成（主循环 / 背包符文 / 热度 / 近战 / 冲刺 / AI） |
| 当前重点 | 战斗手感打磨、火绳枪武器系统、UI 流程打通 |

---

## 二、各系统完成度

### ✅ 已完成（C++ + 编辑器可用）

| 系统 | 关键文件 | 备注 |
|---|---|---|
| 近战攻击 + 连击 | `GA_MeleeAttack` | 环形扇区判定、连击链、冲刺连招桥接 |
| 冲刺系统 | `GA_PlayerDash` | 越障算法、根运动驱动、无敌帧、台阶支持 |
| 攻击自动吸附 | `ANS_AutoTarget` | 锥角过滤、死亡敌人过滤（本次修复） |
| 命中停顿 + 时间缩放 | `HitStopManager` + `AN_HitStop` | 两阶段冻结/慢动作，AnimNotify 驱动 |
| 攻击前摇闪红 | `ANS_PreAttackFlash` | 蒙太奇前摇帧放置即生效 |
| 命中闪白 | `YogCharacterBase` | 受伤自动触发，无需配置 |
| 热度升阶发光 | `PlayerCharacterBase` + `WeaponInstance` | Phase1/2/3 颜色走 DA，武器+玩家身体 |
| 热度系统 | `BackpackGridComponent` | 4 阶热度、激活区动态扩展 |
| 符文背包 | `BackpackGridComponent` + `BackpackScreenWidget` | 手柄/鼠标双操作、战斗锁定 |
| 符文升级 | `BackpackGridComponent` | Lv.I/II/III 自动合并，满级过滤奖励池 |
| 链路系统 | `BackpackGridComponent` | Producer/Consumer 邻格激活传导 |
| 三选一奖励 | `LootSelectionWidget` | 三选一结束自动打开背包 |
| 充能/CD 系统 | `SkillChargeComponent` | 多段冲刺 + 符文改 CD |
| 韧性系统 | `YogCharacterBase` | Poise vs Resilience 比较、霸体保护 |
| 主循环 | `YogGameMode` | 波次刷怪、计时/条件触发、整理阶段 |
| 关卡切换 | `Portal` + `YogGameMode` | 多分支传送门、DA_Campaign 配置 |
| 跨关持久化 | `YogSaveSubsystem` | HP / 金币 / 背包符文 |
| AI 行为树 | `BTTask_ActivateAbilityByTag` + `BTTask_PreAttackFlash` | 前摇红光同步 GA 生命周期 |
| 献祭恩赐 | `SacrificeGraceDA` + `BFNode_SacrificeDecay` | 满热度激活，衰退 HP 抽取 |
| LevelFlow | `LevelFlowAsset` + `LevelEventTrigger` | 时间膨胀 / 延迟 / 引导弹窗节点 |
| 新手引导 | `TutorialManager` + `UTutorialPopupWidget` | 武器/战斗后两段引导，存档记录 |
| 相机系统 | `YogCameraPawn` | 6 状态优先级、冲刺 1:1、边界约束 |
| 敌方向箭头 | `EnemyArrowWidget` | 离屏敌人方向指示，受伤重置延迟 |
| 武器系统框架 | `WeaponSpawner` + `WeaponInstance` + `WeaponDefinition` | 拾取交互、热度发光、切关恢复 |
| 液态玻璃框 | `GlassFrameWidget` | 双层模糊 + SDF 边框 + 底边炫彩 |
| 武器拾取动画 | `WeaponFloatWidget` + `WeaponTrailWidget` | 展示 → 折叠 → 缩小 → 飞行三阶段 |
| 背包格半透 | `BackpackStyleDataAsset` | `InactiveZoneOpacity` DA 配置 |
| 弹药计数器 | `WBP_AmmoCounter` | 图标式横排，属性变化自动刷新 |

---

### 🔧 C++ 完成 / 蓝图配置待完成

| 系统 | 待完成内容 |
|---|---|
| **火绳枪武器系统** | BP GA（轻/重/冲刺攻击/单/全装填/冲刺装填）、GE 伤害、弹药 Attribute、WBP_AmmoCounter 蓝图创建 |
| **ANS_PreAttackFlash** | 在所有敌人攻击蒙太奇前摇帧拖入 `ANS Pre Attack Flash` 区间 |
| **CharacterFlashMaterial** | 每个敌人 BP → Combat\|Visual → `CharacterFlashMaterial` 填入 Overlay 材质（含 FlashColor/FlashAlpha/Power 参数） |
| **AN_HitStop** | 在玩家攻击蒙太奇命中帧放置 `AN Hit Stop`，配置 FrozenDuration / SlowDuration / SlowTimeDilation |
| **武器拾取玻璃图标** | WBP_WeaponGlassIcon + WeaponGlassAnimDA 编辑器配置；WBP_WeaponTrail 创建 |
| **献祭恩赐** | 创建 FA（In→BFNode_SacrificeDecay）、创建拾取物蓝图 BP_SacrificePickup |
| **点光源碰撞修复** | `BP_CelesPointLight` → SphereVolume → Collision → DashTrace 设为 **Ignore** |

---

### ❌ 未开始

| 功能 | 优先级 | 说明 |
|---|---|---|
| 符文表现层（GameplayCue + Niagara） | P0 | 16 个新 GC 资产，详见符文制作主指南 |
| 符文逻辑层（FA 制作） | P0 | 1017/1018/1019 为测试必备符文 |
| 远程武器第二类型 | P1 | 火绳枪之外的远程武器 |
| 多格/异形符文 UI | P2 | Shape.Cells 数据已存，渲染未做 |
| 符文池平衡与数值调参 | P2 | 依赖测试反馈 |

---

## 三、本次会话完成（2026-04-19 第三次会话）

| ID | 功能 | 文件 |
|---|---|---|
| COMBAT-006 | 命中停顿 + 全局时间缩放（HitStop + AN_HitStop） | `HitStopManager.h/.cpp`、`AN_HitStop.h/.cpp` |
| VFX-003 | 攻击前摇闪红 AnimNotifyState（ANS_PreAttackFlash） | `ANS_PreAttackFlash.h/.cpp` |
| FIX-008 | ANS_AutoTarget 吸附死亡敌人修复 | `ANS_AutoTarget.cpp` |
| COMBAT-007 | GA_PlayerDash 台阶/坡度支持 | `GA_PlayerDash.cpp` |
| COMBAT-007 | 冲刺诊断 CVar（`Dash.DebugTrace 1`） | `GA_PlayerDash.cpp` |

---

## 四、待完成任务优先级

### P0 — 测试前必须完成

- [ ] 火绳枪 BP GAs / GEs / Attribute 完整实现
- [ ] 在所有敌人攻击蒙太奇配置 `ANS Pre Attack Flash`（前摇预警）
- [ ] 在玩家攻击蒙太奇命中帧配置 `AN Hit Stop`（命中停顿）
- [ ] 1017 / 1018 / 1019 三个高感知符文 FA 制作（测试三选一必备）
- [ ] 点光源 `BP_CelesPointLight` → SphereVolume → DashTrace = Ignore

### P1 — 体验完善

- [ ] 敌人 BP 填入 `CharacterFlashMaterial`（命中闪白 + 前摇闪红视觉）
- [ ] WBP_AmmoCounter 蓝图创建（火绳枪弹药 HUD）
- [ ] 武器拾取玻璃图标动画 WBP 配置
- [ ] 献祭恩赐 FA + 拾取物 BP

### P2 — 后续迭代

- [ ] 符文表现层 GC + Niagara（16 个资产，见 RuneMaster_ProductionGuide）
- [ ] 多格符文 UI 渲染
- [ ] 数值平衡调参

---

## 五、已知技术债务

| 问题 | 影响 | 状态 |
|---|---|---|
| LevelFlow FlowComponent 缺 Identity Tags | 重载存档后触发器重复触发 | 低优先，后期补 |
| Slate 崩溃（三选一 C++ 化回滚） | `BuildShapeGrid` 运行时 NewObject 与 Slate 生命周期冲突 | 待排查，现用蓝图方案绕过 |
| GAS deprecated API 警告 | `InheritableGameplayEffectTags` / `InheritableOwnedTagsContainer` | 不影响运行，后期迁移新 API |
| WBP_WeaponFloat 白屏 Bug | 背景不在 InfoContainer 内时折叠留白 | 已知修复方案：移入 InfoContainer |

---

## 六、快速定位

| 我想… | 看这里 |
|---|---|
| 了解符文制作优先级和表现层规格 | [RuneMaster_ProductionGuide](FeatureConfig/RuneMaster_ProductionGuide.md) |
| 配置火绳枪 GA/GE | [Musket_System_Guide](Weapon/Musket_System_Guide.md) |
| 了解 AnimNotify 全部类型 | [AnimNotify & VFX 速查](../../.claude/projects/memory/reference_anim_notify_vfx.md) |
| 配置攻击造成伤害 | [AttackDamage_ConfigGuide](FeatureConfig/AttackDamage_ConfigGuide.md) |
| 配置敌人行为树 | [BT_AttackTask_ConfigGuide](FeatureConfig/BT_AttackTask_ConfigGuide.md) |
| 了解背包/符文架构 | [BackpackSystem_Technical](Systems/BackpackSystem_Technical.md) |
| 冲刺系统调试 | 控制台输入 `Dash.DebugTrace 1` |
| 命中停顿配置 | 攻击蒙太奇命中帧 → 添加 Notify `AN Hit Stop` |
