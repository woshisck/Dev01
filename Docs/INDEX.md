# Docs 文档库总入口

> 更新：2026-05-15  
> 原则：先按用途找入口，再进入具体系统或功能目录。

## 快速入口

| 我要找 | 入口 | 说明 |
| --- | --- | --- |
| 文档怎么写、该放哪里 | [分类规则](00_入口与规范/分类规则.md) | 新增文档前先看这里 |
| 旧路径搬到哪里 | [路径迁移表](00_入口与规范/路径迁移表.md) | 本次重构的完整映射 |
| 系统技术文档 | [01_长期系统文档/](01_长期系统文档) | 程序、系统实现、Tag、编码约定 |
| 核心方案和需求 | [02_版本计划与需求/核心方案/](02_版本计划与需求/核心方案) | 当前核心开发方案、需求、协作接口 |
| 策划配置和编辑器操作 | [03_配置与编辑器手册/README.md](03_配置与编辑器手册/README.md) | 配置说明、编辑器工具、WBP/DA/材质/Niagara |
| 调研和玩法设计 | [04_调研与玩法设计/](04_调研与玩法设计) | Survey、卡牌/符文玩法设计、系统设计草案 |
| 任务、进度、验收 | [05_项目管理与进度/](05_项目管理与进度) | 当前任务、进度快照、FeatureLog |
| 自动生成报告 | [90_自动生成报告/](90_自动生成报告) | Commandlet 等自动输出 |
| 旧方案和过程记录 | [99_归档/README.md](99_归档/README.md) | 旧 2D 背包方案、历史 WorkSession、旧 TODO |

## 常用文档

| 场景 | 文档 |
| --- | --- |
| 查核心方案入口 | [核心方案 README](02_版本计划与需求/核心方案/README.md) |
| 查已有功能目录 | [CodeCatalog](01_长期系统文档/系统/CodeCatalog.md) |
| 查任务看板 | [TASKS](05_项目管理与进度/PM/TASKS.md) |
| 查功能完成记录 | [FeatureLog](05_项目管理与进度/FeatureLog.md) |
| 查 GameplayTag 规范 | [GameplayTag_MasterGuide](01_长期系统文档/标签/GameplayTag_MasterGuide.md) |
| 查 UI Widget 规范 | [Widget](01_长期系统文档/编码规范/Widget.md) |
| 查符文系统完整指南 | [RuneLogic_Complete_Guide](01_长期系统文档/系统/Rune/RuneLogic_Complete_Guide.md) |
| 查 Rune Editor 使用 | [RuneEditor_UserGuide](01_长期系统文档/系统/Rune/RuneEditor_UserGuide.md) |
| 查 DataEditor 测试说明 | [DataEditor数值编辑器测试说明](03_配置与编辑器手册/核心配置说明/编辑器工具/DataEditor数值编辑器测试说明.md) |
| 查旧 2D 背包方案 | [2D背包方案归档](99_归档/旧方案/2D背包方案/README.md) |

## 新目录说明

| 目录 | 放置内容 | 不放什么 |
| --- | --- | --- |
| `00_入口与规范/` | 总规则、旧规范、迁移表 | 具体系统实现细节 |
| `01_长期系统文档/` | 可跨版本复用的系统、代码、Tag、约定 | 单次版本计划和临时讨论 |
| `02_版本计划与需求/` | 核心方案、需求拆解、开发方案、协作接口 | 具体编辑器配置手册 |
| `03_配置与编辑器手册/` | 策划照着操作的配置说明和编辑器说明 | 程序架构长文 |
| `04_调研与玩法设计/` | 调研、规则、卡牌设计、玩法方向 | 已确认的任务看板 |
| `05_项目管理与进度/` | 任务、验收、进度、FeatureLog | 配置细节和代码接口 |
| `90_自动生成报告/` | 工具自动产物 | 手写长期文档 |
| `99_归档/` | 历史过程、旧 TODO、旧方案 | 最新依据 |

## 维护规则

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

### 调试工具类

| 我想要… | 对应功能 | 文档 |
|---|---|---|
| 游戏内实时看 Warning / Error 日志 | ImGui 悬浮日志窗 | [ImGuiDebugLog_Guide](Systems/Debug/ImGuiDebugLog_Guide.md) |
| 快速调试热度 / 符文 / 血量 / 敌人 | GM CheatManager 命令 | [GMCommands_Guide](Systems/GMCommands_Guide.md) |

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
| [PM/CurrentProgress_20260421.md](PM/CurrentProgress_20260421.md) | 完成度快照 |

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
