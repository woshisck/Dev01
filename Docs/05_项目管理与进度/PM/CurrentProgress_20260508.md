# 星骸降临 — 完成度快照

> 更新：2026-05-08  
> 数据来源：[FeatureLog.md](../FeatureLog.md) + [TASKS.md](TASKS.md)

---

## 总体进度

| 维度 | 状态 |
|---|---|
| **核心系统（C++）** | ✅ 全部落地 |
| **P0 待办（编辑器配置）** | 🔲 约 15 项，均为用户侧 DA/WBP/BP 配置 |
| **P1 体验完善** | 🔲 5 项 |
| **P2 后续迭代** | 🔲 5 项 |
| **当前开发重心** | Rune Editor 数值表与连携配方系统 |

---

## ✅ 已完成功能（截至 2026-05-08）

### 今日完成（2026-05-08）

| ID | 功能 |
|---|---|
| RUNE-EDITOR-003 | Rune Editor — 连携配方表（ComboRecipe 编辑器页） |
| RUNE-EDITOR-004 | Rune Editor — 连招奖励数值表（ComboBonus 行级配置，含新枚举/结构/运行时重载） |

### 2026-05-07

| ID | 功能 |
|---|---|
| CombatLog-512 | 战斗日志系统 512 版本升级（11 个卡牌字段、4 个过滤枚举、卡牌消耗行、摘要统计） |

### 2026-04-28

| ID | 功能 |
|---|---|
| Weapon-TypeGuard | 武器类型守卫（Weapon.Type.Melee/Ranged Tag 自动隔离近/远程 GA 激活） |
| Rune-EnemyBuff | 5 个敌人专属符文 C++ 改动（死亡事件广播/暴击事件/BuffGiver语义修复/SuperArmor自动衔接） |

### 2026-04-27

| ID | 功能 |
|---|---|
| UI-010 | 传送门浮窗符文行扩展（描述 + GenericEffects 内联展示） |
| UI-009 | 背包 4 项改造（长按机制删除/Shape预览/EndPreview按钮/OperationHint浮窗） |
| MERGE-001 | 撤回合作者大规模误删（C++ 类/文档均已恢复） |
| SYS-001 | 关卡生命周期事件总线 + LootSelection 视觉高亮 C++ 化 + 卡片堆叠修复 |

### 2026-04-26

| ID | 功能 |
|---|---|
| UI-008 | LootSelection 改造（动态卡牌/跳过预览/GenericEffect聚焦/RichText按键图标） |
| LEVEL-006 | 传送门交互式选择 + 双层预览（HUD方位指引/单例浮窗）+ 渐黑过场 |
| COMBAT-009 | AN_MeleeDamage HitStop 模式选择器 + 命中事件广播 |

### 2026-04-23

| ID | 功能 |
|---|---|
| UI-007 | 武器玻璃图标热度颜色 — WeaponGlassIconWidget 自订阅 |
| FEAT-033 | 冲刺空气墙 BP_DashBarrier |
| FEAT-032 | HitStop 重构（AN → FA + Tag 驱动 + 蒙太奇暂停） |
| FIX-028 | EnemyArrow WBP 还原 + NativeTick 越界崩溃防御 |
| FEAT-030 | LENode_ShowTutorial 增加 bPauseGame 选项 |
| FEAT-031 | LevelFlow 新节点 LENode_WaitForLootSelected |
| FEAT-029 | 祭坛交互系统（AltarActor + 净化/升级/献祭三功能 UI） |
| COMBAT-008 | 霸体金光 + 敌人韧性 DA 化 |
| UI-023 | 液态血条 LiquidHealthBarWidget + LiquidHealthBar.ush |
| UI-024 | HUD 主容器架构 YogHUDRootWidget |

### 2026-04-22

| ID | 功能 |
|---|---|
| FIX-022/023 | 三选一 Loot UI 以拾取物为主导 + CommonUI 完全隔离 |
| FEAT-023 | 关卡结束视觉特效（时间膨胀/渐黑/圆形揭幕） |
| FEAT-024 | 开场镜头标记 ALevelIntroCameraMarker |
| FEAT-025 | 符文旋转系统 + Loot 8方向最佳落点算法 |
| FIX-024/025/026/027 | 跨关热度/满阶溢出/SacrificeGrace/战斗锁定修复 |
| FEAT-026 | 热度携带符文（余烬/炽核）C++ |
| FEAT-027 | 武器初始符文自动放置 |
| UI-021/022 | 背包交互重构 + 符文边框 NativePaint |
| FIX-021 | 战斗锁定补全三处红闪+抖动 |

### 2026-04-20

| ID | 功能 |
|---|---|
| INPUT-001 | 火绳枪输入系统（Reload绑定/重攻击松键修复/GA Tag激活） |
| FEAT-015/016 | 武器拾取 HUD 动画系统 + 液态玻璃框 UI |
| UI-017 | 弹药计数器 AmmoCounter C++ 类 |
| REFACTOR-018 | PendingGrid 全面重构 |
| FIX-019 | 背包手柄导航恢复 + CloseButton + 战斗锁定 |
| FEAT-020 | 背包热度预览默认同步当前阶段 |
| VFX-005/004 | 热度升阶发光颜色统一 + 菲涅尔炫彩 HLSL |
| AI-002 | BT 攻击前摇红光集成 |

### 2026-04-19

| ID | 功能 |
|---|---|
| COMBAT-007 | GA_PlayerDash 台阶越障支持 + 调试 CVar |
| SKILL-001 | 统一充能/CD 系统 SkillChargeComponent |
| FEAT-012 | LevelFlow 完善（LENode_Delay/OnClosed回调/节点隔离） |
| FEAT-013 | 链路系统（Producer/Consumer 传导激活） |
| FEAT-014 | 献祭恩赐系统（SacrificeGrace + BFNode_SacrificeDecay） |
| VFX-003 | ANS_PreAttackFlash 攻击前摇闪红 |
| FIX-008 | ANS_AutoTarget 吸附死亡敌人修复 |

### 2026-04-18

| ID | 功能 |
|---|---|
| COMBAT-006 | 命中停顿系统（HitStopManager + AN_HitStop） |
| MAP-002/003 | 关卡结束揭幕特效 + 开场镜头标记 |
| UI-005/006/007 | 背包热度阶段点按钮 + 格子1:1约束 + 热度区三色叠加 |
| VFX-001/002 | 热度升阶发光 + 命中闪白/攻击前闪红 |
| UI-013/014 | 武器浮窗→玻璃图标流程 + 三选一后自动打开背包 |
| UI-012 | 暂停弹窗屏幕变暗效果 |
| COMBAT-005 | 韧性系统 Poise + 霸体 |
| COMBAT-004 | 冲刺连招保存 DashSave |

### 2026-04-16

| ID | 功能 |
|---|---|
| CAM-001/002/003 | YogCameraPawn 6状态系统 + 平滑优化 + 鼠标偏移移除 |
| BACKPACK-001/002 | 背包网格 + 热度激活 + StyleDA + RuneInfoCard |
| COMBAT-001/002/003 | 近战攻击连击 + 冲刺 + 武器系统 |
| UI-001/002/003/004 | CommonUI 重构 + 金币系统 + 拖拽修复 |
| LOOP-001 | 主循环（波次刷怪/切关/三选一） |
| MAP-001 | 传送门系统 APortal |
| AI-001 | 敌人 BT 攻击任务 BTTask_ActivateAbilityByTag |
| SAVE-001 | 跨关持久化 YogSaveSubsystem |
| TUT-001 | 新手引导 TutorialManager |
| SACR-001 | 献祭恩赐 SacrificeGraceDA + BFNode_SacrificeDecay |
| LFLOW-001 | LevelFlow 时间膨胀/延迟/引导弹窗节点 |
| HEAT-001 | 热度系统 BackpackGridComponent 4 阶热度 |
| DATA-P0-DONE | 数值统一访问器 + RuneIdTag + DataEditor 三面板菜单 |

---

## 🔲 P0 待完成（编辑器配置，用户操作）

| ID | 任务 | 依赖 |
|---|---|---|
| HUD-001 | 创建 WBP_HUDRoot + WBP_PlayerHealthBar | C++ 已完成 |
| ENEMY-DA-001 | DA_Enemy_* 填 PreSpawnFX + Duration | C++ 已完成 |
| ENEMY-DA-002 | DA_Enemy_* 填 SuperArmorThreshold + Duration | C++ 已完成 |
| ALTAR-001~003 | DA_Altar + BP_AltarActor + WBP 三件套 | C++ 已完成 |
| MUSKET-001~004 | 火绳枪 BP GAs + GE + WBP_AmmoCounter | C++ GA 框架已完成 |
| LEVEL-END-CONFIG | WBP_LevelEndReveal + DA_LevelEndEffect + HUD 配置 | C++ 已完成 |
| VFX-004 | 敌人攻击蒙太奇前摇帧拖入 ANS_PreAttackFlash | ANS 类已完成 |
| VFX-005 | 玩家攻击蒙太奇命中帧加 AN_HitStop | AN 类已完成 |
| RUNE-P0-1~3 | 1017/1018/1019 符文 FA 制作 | - |
| HEAT-CARRY-001/002 | 余烬/炽核 FA_Rune 制作 | C++ 已完成 |
| FIX-009 | BP_CelesPointLight SphereVolume DashTrace = Ignore | - |
| DATA-P0-1/2 | 跑 DataEditor 迁移脚本 + SmokeTest | DATA-P0-DONE |

---

## 🔲 P1 待完成

| ID | 任务 |
|---|---|
| VFX-006 | 敌人 BP 填 CharacterFlashMaterial |
| LEVEL-INTRO-CONFIG | 关卡中放置 ALevelIntroCameraMarker |
| UI-001/002 | WBP_WeaponGlassIcon + WBP_WeaponTrail 编辑器配置 |
| CONTENT-001 | 献祭恩赐 FA + BP_SacrificePickup |
| FIX-010 | WBP_WeaponFloat 白屏（背景移入 InfoContainer） |

---

## 📝 技术债务

| 问题 | 优先级 |
|---|---|
| LevelFlow FlowComponent 缺 Identity Tags（重载存档后触发器重复触发） | 低 |
| Slate 崩溃（三选一 C++ 化回滚）`BuildShapeGrid` 运行时 NewObject 与 Slate 冲突 | 待排查 |
| GAS deprecated API 警告（InheritableGameplayEffectTags 等） | 低 |
| WBP_WeaponFloat 白屏（背景不在 InfoContainer 内） | P1 FIX-010 |
