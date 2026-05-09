# 星骸降临 — 文档库总入口

> 更新：2026-04-21  
> **查任务** → [PM/TASKS.md](PM/TASKS.md) | **查已有功能** → [Systems/CodeCatalog.md](Systems/CodeCatalog.md)

---

## 🎯 我想要… → 找到对应功能

### 效果 / Buff / 符文类

| 我想要… | 对应功能 | 文档 |
|---|---|---|
| 角色有可叠加的增益效果 | BuffFlow + FA 节点系统 | [BuffFlow_NodeUsageGuide](Systems/Rune/BuffFlow_NodeUsageGuide.md) |
| 符文逻辑全景（架构/节点/模式/敌人） | 符文完整指南 | [RuneLogic_Complete_Guide](Systems/Rune/RuneLogic_Complete_Guide.md) |
| 在 Rune Editor 中编辑符文数值表和流程图 | Yog Rune Flow 编辑器 | [YogRuneEditor_Technical](Systems/Rune/YogRuneEditor_Technical.md) |
| 设计一个新符文 | BuffFlow 符文工作流 | [BuffFlow_RuneWorkflow](Systems/Rune/BuffFlow_RuneWorkflow.md) |
| 制作已规划的符文 1001–1016 | FA 制作指南 | [TestRune_CreationGuide](Systems/Rune/TestRune_CreationGuide.md) |
| 制作高感知符文 1017–1021（P0） | FA 制作指南 | [TestRune_HighPerception_Guide](Systems/Rune/TestRune_HighPerception_Guide.md) |
| 背包式符文构筑界面 | BackpackGridComponent + WBP | [BackpackSystem_Guide](Systems/Rune/BackpackSystem_Guide.md) |
| 三选一符文奖励界面 | LootSelectionWidget + ARewardPickup | [LootSelection_Technical](Systems/UI/LootSelection_Technical.md) |
| 热度越高效果越强 | HeatSystem | [BackpackSystem_Technical](Systems/Rune/BackpackSystem_Technical.md) |
| 献祭 HP 换取强力恩赐 | SacrificeGrace + BFNode | [EditorSetup_ChainAndSacrifice](TODO/EditorSetup_ChainAndSacrifice.md) |
| 符文有链路传导（Producer/Consumer） | BackpackGridComponent | [BackpackSystem_Technical](Systems/Rune/BackpackSystem_Technical.md) |
| 敌人/动作 GA 也能用 FA | FA 通用架构（三层模型） | [FA_UniversalArchitecture](Systems/Rune/FA_UniversalArchitecture.md) |
| 子图 CustomInput 传参（父图→子图数据传递） | CustomInput DataPin | [BuffFlow_CustomInputDataPin](Systems/Rune/BuffFlow_CustomInputDataPin.md) |

---

### 战斗手感类

| 我想要… | 对应功能 | 文档 |
|---|---|---|
| 攻击命中有顿挫感（冻帧/慢动作） | HitStopManager + AN_HitStop | [AnimNotify规范](Conventions/AnimNotify.md) |
| 攻击自动锁定最近活着的敌人 | ANS_AutoTarget | [CodeCatalog](Systems/CodeCatalog.md) |
| 敌人攻击前摇有视觉预警（发红光） | ANS_PreAttackFlash | [CharacterFlash_Technical](Systems/VFX/CharacterFlash_Technical.md) |
| 角色受击有闪白反馈 | YogCharacterBase StartHitFlash | [CharacterFlash_Technical](Systems/VFX/CharacterFlash_Technical.md) |
| 近战有连击链 | GA_MeleeAttack | [AttackDamage_ConfigGuide](Systems/Combat/AttackDamage_ConfigGuide.md) |
| 敌人有多段连击蒙太奇 | EnemyCombo | [EnemyCombo_ConfigGuide](Systems/Combat/EnemyCombo_ConfigGuide.md) |
| 攻击特定连段触发特殊效果 | AdditionalRuneEffects | [MeleeCombo_NotifyRune_Guide](Systems/Combat/MeleeCombo_NotifyRune_Guide.md) |
| 角色能冲刺（越障/无敌帧） | GA_PlayerDash + SkillChargeComponent | [Dash_Design](Systems/Combat/Dash_Design.md) |
| 角色有韧性/霸体 | PoiseSystem | [PoiseSystem_ConfigGuide](Systems/Combat/PoiseSystem_ConfigGuide.md) |
| 能力有多段充能 + 冷却 | SkillChargeComponent | [SkillCharge_Guide](Systems/Combat/SkillCharge_Guide.md) |

---

### 武器类

| 我想要… | 对应功能 | 文档 |
|---|---|---|
| 远程武器（火绳枪） | GA_Musket_* 套件 | [Musket_System_Guide](Systems/Weapon/Musket_System_Guide.md) |
| 武器可拾取 + 装备 + 切关恢复 | WeaponSpawner + WeaponInstance | [WeaponSystem_Technical](Systems/Weapon/WeaponSystem_Technical.md) |
| 武器有弹药 HUD | AmmoCounter + WBP_AmmoCounter | [Musket_System_Guide](Systems/Weapon/Musket_System_Guide.md) |
| 武器随热度发光 | WeaponGlowOverlay.ush | [Material_Authoring_Guide](Systems/VFX/Material_Authoring_Guide.md) |

---

### 关卡 / 循环类

| 我想要… | 对应功能 | 文档 |
|---|---|---|
| 关卡内事件序列/指引（时间轴编排） | LevelFlow | [EditorSetup_LevelFlow](TODO/EditorSetup_LevelFlow_Tutorial_WeaponSpawner.md) |
| 新手引导弹窗 | TutorialManager | [EditorSetup_LevelFlow](TODO/EditorSetup_LevelFlow_Tutorial_WeaponSpawner.md) · [文案设计](Systems/UI/TutorialPopup_Copy.md) |
| 波次刷怪 + 补刷 | YogGameMode + MobSpawner | [LevelSystem_ConfigGuide](Systems/Level/LevelSystem_ConfigGuide.md) |
| 多分支传送门切关 | APortal + DA_Campaign | [Portal_ConfigGuide](Systems/Level/Portal_ConfigGuide.md) |
| HP/金币/背包跨关卡存档 | YogSaveSubsystem | [CrossLevelState_Technical](Systems/Level/CrossLevelState_Technical.md) |
| 敌人有行为树攻击 | BTTask_ActivateAbilityByTag | [Systems/AI/](Systems/AI/) |
| 关卡有 Buff 奖励池 | BuffDataAsset + DA_Room | [BuffPool_ConfigGuide](Systems/Level/BuffPool_ConfigGuide.md) |

---

### UI / 视觉类

| 我想要… | 对应功能 | 文档 |
|---|---|---|
| 磨砂玻璃感 UI 框 | GlassFrameWidget | [GlassFrame_Technical](Systems/UI/GlassFrame_Technical.md) |
| 武器拾取有华丽动画 | WeaponFloatWidget + WeaponTrailWidget | [WeaponPickupAnim_Technical](Systems/UI/WeaponPickupAnim_Technical.md) |
| 新手引导弹窗文案（5 条） | DA_TutorialStep 配置项 | [TutorialPopup_Copy](Systems/UI/TutorialPopup_Copy.md) |
| 相机跟随角色（俯视角） | YogCameraPawn | [Camera_Design](Systems/Camera/Camera_Design.md) |
| 角色随热度阶段发光 | PlayerGlowOverlay.ush | [CharacterFlash_Technical](Systems/VFX/CharacterFlash_Technical.md) |
| 新建材质用自定义 HLSL | .ush 体系 + Custom Node | [Conventions/Material.md](Conventions/Material.md) |

---

## ⚙️ 已实现功能目录

> 完整版（含接入方式、标签、配置参数）→ **[Systems/CodeCatalog.md](Systems/CodeCatalog.md)**

| 功能 | 标签 | 一句话 |
|---|---|---|
| HitStopManager | #Subsystem #蒙太奇驱动 | 命中停顿 + 全局时间缩放 |
| ANS_AutoTarget | #蒙太奇驱动 #可复用 | 攻击吸附最近活着的敌人 |
| ANS_PreAttackFlash | #蒙太奇驱动 #可复用 | 前摇帧发红光预警 |
| AN_HitStop | #蒙太奇驱动 #可复用 | 触发 HitStop |
| SkillChargeComponent | #可复用 #DA配置 #符文联动 | 多段充能 + 冷却 |
| GA_PlayerDash | #GAS #可复用 | 越障/穿透/无敌帧冲刺 |
| BackpackGridComponent | #可复用 #符文联动 | 背包格 + 热度 + 符文升级 |
| BuffFlow / FA | #可复用 #DA配置 | 可视化 Buff/符文逻辑节点图 |
| WeaponSpawner / Instance | #可复用 #DA配置 | 武器拾取装备系统 |
| 火绳枪 GA 套件（6个） | #GAS #DA配置 | 远程武器全套 GA |
| LevelFlow | #可复用 #DA配置 | 关卡事件时间轴 |
| TutorialManager | #可复用 #DA配置 | 新手引导两段流 |
| YogSaveSubsystem | #Subsystem | 跨关卡持久化 |
| GlassFrameWidget | #UI #可复用 | 液态玻璃框 |
| AmmoCounter | #UI #GAS | 弹药 HUD |
| LootSelectionWidget | #UI #可复用 | 三选一奖励 |
| YogCameraPawn | #可复用 | 6状态俯视角相机 |
| CharacterFlash | #可复用 | 命中闪白 + 前摇闪红 |

---

## 📚 生产规范（Claude 编码前必读）

| 文档 | 适用场景 |
|---|---|
| [Conventions/GAS.md](Conventions/GAS.md) | GA / GE / Attribute |
| [Conventions/Material.md](Conventions/Material.md) | 材质 / .ush / C++ 驱动 |
| [Conventions/Widget.md](Conventions/Widget.md) | UI Widget / WBP 布局规格 |
| [Conventions/AnimNotify.md](Conventions/AnimNotify.md) | AN / ANS 类 |
| [Conventions/DataAsset.md](Conventions/DataAsset.md) | DA 类设计 |
| [Conventions/DataAuthoring.md](Conventions/DataAuthoring.md) | 数值访问器/Rune.ID Tag/EUW 工具用法 |

---

## 🧪 测试与验收入口

| 文档 | 内容 |
|---|---|
| [DataEditor数值编辑器测试说明](项目需求文档/512版本项目计划/Codex开发方案/配置说明/编辑器工具/DataEditor数值编辑器测试说明.md) | `Tools > DataEditor` 菜单面板、smoke test、RuneID 迁移、批量操作和编辑器回归清单 |
| [DataEditor README](../Tools/DataEditor/README.md) | UE 菜单面板日常入口 + PowerShell / UE Python 自动化脚本入口 |

---

## 🏷️ 跨系统规范（Tag / 状态冲突）

| 文档 | 内容 |
|---|---|
| [GameplayTag总体设计指南](Tags/GameplayTag_MasterGuide.md) | 命名空间 + 创建决策树 |
| [Tag情景使用指南](Tags/Tag_SituationalGuide.md) | "我想做X用哪个Tag" |
| [GA Tag字段使用指南](Tags/GA_TagFields_Guide.md) | 5个Tag字段用法 |
| [Buff Tag规范](Tags/Buff_Tag_Spec.md) | `Buff.*` 5层模型 |
| [状态冲突规则表](Tags/StateConflict/StateConflict_TagBlock.md) | DA填表规范 |
| [状态冲突系统技术文档](Tags/StateConflict/StateConflict_Technical.md) | C++ 接入方式 |

---

## 📋 项目管理

| 文档 | 内容 |
|---|---|
| **[PM/TASKS.md](PM/TASKS.md)** | 任务看板 P0/P1/P2，唯一任务源 |
| [FeatureLog.md](FeatureLog.md) | 功能完成详细记录 |
| [PM/CurrentProgress_20260508.md](PM/CurrentProgress_20260508.md) | 完成度快照 |

---

## 🔧 调试

| 工具 | 使用方法 |
|---|---|
| 冲刺路径调试 | 控制台：`Dash.DebugTrace 1` |
| GM 命令 | [GMCommands_Guide](Systems/GMCommands_Guide.md) |

---

## 📂 按模块浏览

| 文件夹 | 内容 |
|---|---|
| [Systems/Combat/](Systems/Combat/) | 近战/冲刺/韧性/充能配置文档 |
| [Systems/Rune/](Systems/Rune/) | 符文/背包/BuffFlow/热度文档 |
| [Systems/Weapon/](Systems/Weapon/) | 火绳枪 + 武器系统文档 |
| [Systems/Level/](Systems/Level/) | 关卡/波次/传送门/存档文档 |
| [Systems/AI/](Systems/AI/) | 敌人行为树/朝向配置 |
| [Systems/UI/](Systems/UI/) | 玻璃框/武器拾取动画/背包UI文档 |
| [Systems/VFX/](Systems/VFX/) | 角色特效/材质规范 |
| [Systems/Camera/](Systems/Camera/) | 相机系统 |
| [Tags/](Tags/) | Tag 命名规范 + 状态冲突规则 |
| [Conventions/](Conventions/) | 编码规范（Claude编码前必读） |
| [TODO/](TODO/) | 待完成编辑器配置 + P2 内容 |
| [Research/](Research/) | 玩家调研报告 |
# DataEditor Entry Update

- Manual balance panels: `Tools > DevKit Data > Character Balance / Action Balance / Rune Balance`.
- Test guide: `Docs/项目需求文档/512版本项目计划/Codex开发方案/配置说明/编辑器工具/DataEditor数值编辑器测试说明.md`.
- Script guide: `Tools/DataEditor/README.md`.
