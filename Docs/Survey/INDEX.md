# 已完成功能盘点 — INDEX

> 更新：2026-04-26
>
> **口径**：✅ 完成 = C++ 编译通过；⚙ 待配置 = C++ 已完成、编辑器内 WBP/DA/BP/蒙太奇等资产配置由用户挨个看。
>
> 路径前缀：`Source/DevKit/` 之下，分册中以 `Public/...` / `Private/...` 简写。
>
> 若要查看每条功能的完整时间线（含修复 / 重构）→ [FeatureLog.md](../FeatureLog.md)；项目任务看板 → [PM/TASKS.md](../PM/TASKS.md)；按"我想要…"反查 → [INDEX.md](../INDEX.md)。

## 分册导航（按玩家体验维度拆分）

| 分册 | 内容范围 | 文件 |
|---|---|---|
| **01 战斗核心** | 攻击 / 连击 / 闪避 / HitStop / 韧性 / 霸体 / 自动吸附 / 敌人 BT 攻击 / 敌人连招 | [01_CombatCore.md](01_CombatCore.md) |
| **02 构筑系统** | 符文 / 背包格 / 4 阶热度 / 链路 / 升级 / 献祭 / 热度携带 / Tag 命名空间 | [02_BuildUp.md](02_BuildUp.md) |
| **03 关卡循环** | 主循环 / 波次 / 传送门 v3 / 存档 / 三选一 / Buff 池 / LevelFlow / 关卡过场 | [03_RunLoop.md](03_RunLoop.md) |
| **04 武器系统** | 拾取装备 / 切关恢复 / 火绳枪 6 GA / 弹药 HUD / 输入接入 / 热度发光 | [04_Weapons.md](04_Weapons.md) |
| **05 反馈层** | 命中闪白 / 前摇红 / 升阶发光 / 手柄震动 / 相机系统 / 材质 .ush / 敌人预生成 FX | [05_FeedbackLayer.md](05_FeedbackLayer.md) |
| **06 UI / HUD** | 背包 UI / 三选一 UI / 玻璃框 / 液态血条 / HUD 容器 / 武器拾取动画 / 教程 / 敌人方向箭头 / 暂停效果 | [06_UI_HUD.md](06_UI_HUD.md) |

## 总进度概览

| 分册 | ✅ C++完成 | ⚙ 待配置 | 备注 |
|---|---|---|---|
| 01 战斗核心 | 10 | 1 | 热度携带 FA 待做 |
| 02 构筑系统 | 9 | 2 | 祭坛 WBP / 献祭 FA |
| 03 关卡循环 | 11 | 3 | LEVEL-006 WBP / 关卡结束揭幕 / 开场镜头 |
| 04 武器系统 | 5 | 3 | 火绳枪 GA 蓝图 / 弹药 HUD WBP |
| 05 反馈层 | 7 | 2 | 敌人预生成 DA / 敌人 CharacterFlashMaterial |
| 06 UI / HUD | 18 | 4 | HUD-001 WBP / 武器玻璃图标 / 敌人箭头赋值 / 武器拾取浮窗白屏 |
| **合计** | **60** | **15** | 详细列表见各分册末尾"待配置清单" |

## 验收口径

每个功能在分册中给出 1-2 条 **可执行验证** — 通常是"开 PIE 后做某操作、看到某结果"的简短脚本。这部分回答 Codex 审查里"全完成≠功能完成"的口径模糊问题。

## 关于 ID

- ID 沿用 [FeatureLog.md](../FeatureLog.md) 的编号体系
- **撞 ID 已拆分**：原 FeatureLog 中 `COMBAT-003` / `UI-007` 等被多功能复用，盘点中拆为 `COMBAT-003a/b`、`UI-007a/b` — 详见各分册脚注
- 一个功能横跨多分册（如 SkillChargeComponent 既是战斗也是构筑接口）只在主归属分册详写，其他分册只放交叉引用

## 历史方案

- 传送门 v3 开发方案（已实施）→ [../WorkSession/archive/portal_v3_plan.md](../WorkSession/archive/portal_v3_plan.md)
