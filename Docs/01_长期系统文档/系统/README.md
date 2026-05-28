# 系统文档目录

> 每个子目录对应一个功能模块，内含该模块的架构说明、接口参考和配置指南。  
> 全局手册见 [Docs/MANUAL.md](../../MANUAL.md)

## 功能模块索引

| 模块 | 说明 | 入口 |
| --- | --- | --- |
| **3C** | 摄像机、角色运动、输入控制 | [Camera/](Camera/) |
| **Combat** | 攻击判定、连击、闪避、架势、终结技 | [Combat/](Combat/) |
| **Rune** | 符文/BuffFlow 体系，背包构筑核心 | [Rune/README.md](Rune/README.md) *(如存在)* |
| **Story** | StoryEngine + Director + 教程流程 | [Story/README.md](Story/README.md) |
| **Level** | 关卡流程、传送门、Buff 池、房间数据 | [Level/](Level/) |
| **Progression** | 元进度（局外升级）+ 存档系统 | [Progression/README.md](Progression/README.md) |
| **UI** | Widget 层级、HUD、背包界面 | [UI/](UI/) |
| **VFX** | 材质、Niagara、CharacterFlash | [VFX/](VFX/) |
| **Weapon** | 武器框架、火绳枪、投射物 | [Weapon/](Weapon/) |
| **AI** | 敌人 AI、旋转配置 | [AI/](AI/) |
| **Mob** | 刷怪器 | [Mob/](Mob/) |

## 跨系统架构文档

| 文档 | 内容 |
| --- | --- |
| [SystemDependencyMap.md](SystemDependencyMap.md) | 系统依赖关系图（全局） |
| [CodeCatalog.md](CodeCatalog.md) | 已有代码功能目录 |
