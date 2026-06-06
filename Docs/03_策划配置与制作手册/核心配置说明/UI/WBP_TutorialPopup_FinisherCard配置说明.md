# 终结技卡牌教程配置说明

## 目的

终结技卡牌不再在玩家拾取双手剑时自动进入卡组，也不再放进第一次打开背包的 `tutorial_backpack` 弹窗里。

当前教程版本只在最后一关/祈祷室获得终结技卡时显示独立弹窗：`tutorial_finisher`。

## 配置位置

- 教程内容 DA：`/Game/Docs/UI/Tutorial/DA_Tutorial_Finisher`
- 教程注册表：`/Game/Docs/UI/Tutorial/DA_TutorialRegistry`
- 触发入口：终结技卡进入卡组后，由卡牌入组 HintOnce 触发
- 生成器：`Source/DevKitEditor/UI/Tutorial512SetupCommandlet.cpp`

## 页面内容

标题：`终结技卡牌`

正文：`Finisher 卡进入卡组后，会在合适的攻击节奏里提供终结技准备窗口。窗口出现时，用 <input action="HeavyAttack"/> 完成确认连段。`

副文：`教程最后一关获得终结技卡牌后，可以直接把它编入卡组并使用。`

## 验收

- 第一次打开背包时，`tutorial_backpack` 不显示终结技说明。
- 获得终结技卡后，弹出 `tutorial_finisher`。
- 关闭教程后进入背包编排界面，终结技卡牌不再显示 3 场战斗锁定进度。
