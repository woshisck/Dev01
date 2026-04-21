# TASKS — 星骸降临 Dev01

> **唯一任务源**。更新时间：2026-04-21  
> P0 = 测试前必须 | P1 = 体验完善 | P2 = 后续迭代  
> 执行方：`Claude` = C++/蓝图逻辑代码 | `用户` = 编辑器内配置/DA填写/蒙太奇操作

---

## 🔲 P0 — 测试前必须完成

| ID | 任务 | 执行方 | 参考文档 | 依赖 |
|---|---|---|---|---|
| MUSKET-001 | 火绳枪 BP GAs：Light / Heavy / Sprint Attack | 用户 | [Musket_System_Guide](../Design/Weapon/Musket_System_Guide.md) | C++ GA 框架已完成 |
| MUSKET-002 | 火绳枪 BP GAs：Reload_Single / Reload_All / Sprint_Reload | 用户 | [Musket_System_Guide](../Design/Weapon/Musket_System_Guide.md) | C++ GA 框架已完成 |
| MUSKET-003 | GE 伤害 + 弹药 Attribute 初始值 | 用户 | [Musket_System_Guide](../Design/Weapon/Musket_System_Guide.md) | - |
| MUSKET-004 | WBP_AmmoCounter 蓝图配置（Parent = AmmoCounter C++ 类） | 用户 | [AmmoCounter.h](../../Source/DevKit/Public/UI/AmmoCounter.h) | - |
| VFX-004 | 所有**敌人**攻击蒙太奇：前摇帧拖入 `ANS Pre Attack Flash` | 用户 | [CharacterFlash_Technical](../Design/VFX/CharacterFlash_Technical.md) | ANS 类已完成 |
| VFX-005 | 玩家攻击蒙太奇命中帧：添加 `AN Hit Stop` + 配置参数 | 用户 | 轻攻 F=50ms；重攻 F=80ms/S=120ms@25% | AN 类已完成 |
| RUNE-P0-1 | 1017 符文 FA 制作（高感知测试符文） | 用户 | [TestRune_HighPerception_Guide](../Design/FeatureConfig/TestRune_HighPerception_Guide.md) | - |
| RUNE-P0-2 | 1018 符文 FA 制作 | 用户 | 同上 | - |
| RUNE-P0-3 | 1019 符文 FA 制作 | 用户 | 同上 | - |
| FIX-009 | BP_CelesPointLight → SphereVolume → Collision → DashTrace = Ignore | 用户 | 已用 `Dash.DebugTrace 1` 确认 | - |

---

## 🔲 P1 — 体验完善

| ID | 任务 | 执行方 | 参考文档 | 依赖 |
|---|---|---|---|---|
| VFX-006 | 所有**敌人** BP Details → Combat\|Visual → `CharacterFlashMaterial` 填入 Overlay 材质 | 用户 | [CharacterFlash_Technical](../Design/VFX/CharacterFlash_Technical.md) | - |
| UI-001 | WBP_WeaponGlassIcon + WeaponGlassAnimDA 编辑器配置 | 用户 | [WeaponPickupAnim_Technical](../Design/UI/WeaponPickupAnim_Technical.md) | - |
| UI-002 | WBP_WeaponTrail 创建 | 用户 | 同上 | - |
| CONTENT-001 | 献祭恩赐 FA（In → BFNode_SacrificeDecay）+ BP_SacrificePickup | 用户 + Claude | [EditorSetup_ChainAndSacrifice](../TODO/EditorSetup_ChainAndSacrifice.md) | - |
| FIX-010 | WBP_WeaponFloat 白屏 Bug：将背景移入 InfoContainer | Claude | WeaponPickupAnim_Technical.md | - |

---

## 🔲 P2 — 后续迭代

| ID | 任务 | 执行方 | 备注 |
|---|---|---|---|
| RUNE-VFX | 符文表现层：16 个 GameplayCue + Niagara + SFX + 浮字 | 用户 + Claude | [RuneMaster_ProductionGuide](../Design/FeatureConfig/RuneMaster_ProductionGuide.md) |
| RUNE-FA | 符文逻辑层 FA：1001–1016 | 用户 | [TestRune_CreationGuide](../Design/FeatureConfig/TestRune_CreationGuide.md) |
| UI-003 | 多格/异形符文 UI 渲染（Shape.Cells 数据已存） | Claude | BackpackSystem_Technical.md |
| BAL-001 | 数值平衡调参 | 用户 | 依赖测试反馈 |
| REMOTE-001 | 第二类远程武器框架 | Claude | 依赖火绳枪系统稳定后 |

---

## ✅ 已完成

> 详细记录见 [FeatureLog.md](../FeatureLog.md)

| ID | 功能 | 完成日期 |
|---|---|---|
| COMBAT-007b | GA_PlayerDash 冲刺调试 CVar（`Dash.DebugTrace 1`） | 2026-04-19 |
| COMBAT-007 | GA_PlayerDash 台阶/坡度越障支持 | 2026-04-19 |
| FIX-008 | ANS_AutoTarget 吸附死亡敌人修复 | 2026-04-19 |
| VFX-003 | ANS_PreAttackFlash 攻击前摇闪红 AnimNotifyState | 2026-04-19 |
| COMBAT-006 | HitStopManager + AN_HitStop 命中停顿 + 时间缩放 | 2026-04-18 |
| VFX-002 | ANS_PreAttackFlash 前摇闪红（C++ 类完成） | 2026-04-18 |
| VFX-001 | YogCharacterBase 命中闪白（自动触发） | 2026-04-17 |
| WEAPON-001 | 弹药计数器 AmmoCounter C++ 类 | 2026-04-19 |
| WEAPON-FW | 武器系统框架（WeaponSpawner / WeaponInstance / WeaponDefinition） | 2026-04-17 |
| UI-GLASS | 液态玻璃框 GlassFrameWidget + GlassFrameUI.ush | 2026-04-17 |
| UI-FLOAT | 武器拾取动画 WeaponFloatWidget + WeaponTrailWidget | 2026-04-17 |
| BACKPACK | 背包格半透 BackpackStyleDataAsset 配置 | 2026-04-18 |
| CAM-001 | YogCameraPawn 6 状态相机系统 | 2026-04-16 |
| UI-COMMONUI | BackpackScreenWidget / LootSelectionWidget 迁移 CommonUI | 2026-04-16 |
| BACKPACK-CTRL | 符文背包 手柄/鼠标双操作 | 2026-04-14 |
| RUNE-UPG | 符文升级系统（Lv.I/II/III 自动合并） | 2026-04-14 |
| HEAT-GLOW | 热度升阶发光 Phase1/2/3 颜色 DA 化 | 2026-04-17 |
| COMBAT-005 | 韧性系统 Poise vs Resilience | 2026-04-16 |
| COMBAT-004 | 充能/CD 系统 SkillChargeComponent | 2026-04-15 |
| COMBAT-003 | 三选一奖励 LootSelectionWidget | 2026-04-14 |
| COMBAT-002 | 冲刺系统 GA_PlayerDash（越障/根运动/无敌帧） | 2026-04-12 |
| COMBAT-001 | 近战攻击 + 连击链 GA_MeleeAttack | 2026-04-10 |
| AI-001 | 敌人行为树攻击任务 BTTask_ActivateAbilityByTag | 2026-04-13 |
| AI-002 | 前摇红光 BTTask_PreAttackFlash | 2026-04-13 |
| LOOP-001 | 主循环波次/计时 YogGameMode | 2026-04-10 |
| LEVEL-001 | 关卡切换 Portal + DA_Campaign | 2026-04-10 |
| SAVE-001 | 跨关持久化 YogSaveSubsystem | 2026-04-10 |
| TUT-001 | 新手引导 TutorialManager + UTutorialPopupWidget | 2026-04-17 |
| SACR-001 | 献祭恩赐 SacrificeGraceDA + BFNode_SacrificeDecay（C++ 完成） | 2026-04-17 |
| LFLOW-001 | LevelFlow 时间膨胀/延迟/引导弹窗节点 | 2026-04-17 |
| HEAT-001 | 热度系统 BackpackGridComponent 4 阶热度 | 2026-04-06 |
| TAG-AUTOTGT | ANS_AutoTarget 锥角过滤自动吸附 | 2026-04-10 |

---

## 📝 已知技术债务

| 问题 | 影响 | 优先级 |
|---|---|---|
| LevelFlow FlowComponent 缺 Identity Tags | 重载存档后触发器重复触发 | 低，后期补 |
| Slate 崩溃（三选一 C++ 化回滚） | `BuildShapeGrid` 运行时 NewObject 与 Slate 冲突 | 待排查，现用蓝图方案绕过 |
| GAS deprecated API 警告 | InheritableGameplayEffectTags 等旧 API | 不影响运行，后期迁移 |
| WBP_WeaponFloat 白屏 Bug | 背景不在 InfoContainer 内时折叠留白 | 已知修复方案（P1: FIX-010） |
