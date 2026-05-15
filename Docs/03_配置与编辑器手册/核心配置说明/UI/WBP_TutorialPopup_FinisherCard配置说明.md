# WBP_TutorialPopup FinisherCard 配置说明

## 目的

为德式双手剑的终结技卡牌补充背包开启教程提示。玩家第一次打开背包/卡组编排界面时，`tutorial_backpack` 弹窗会在“调整卡牌顺序”和“切换连携方向”之后追加一页终结技说明。

## 配置位置

- 教程内容 DA：`/Game/Docs/UI/Tutorial/DA_Tutorial_Backpack`
- 教程注册表：`/Game/Docs/UI/Tutorial/DA_TutorialRegistry`
- 触发入口：`UBackpackScreenWidget::NativeOnActivated` -> `UTutorialManager::TryBackpackTutorial`
- 生成器：`Source/DevKitEditor/UI/Tutorial512SetupCommandlet.cpp`

## 页面内容

标题：`双手剑终结技卡牌`

正文：`卡组中的 Finisher 是德式双手剑的终结技准备牌。打出后会进入短暂强化窗口；在窗口内用重攻击连段完成最后一击，会触发终结技并引爆此前命中的终结印记。`

副文：`若卡牌显示锁定进度，先完成 3 场战斗解锁；普通攻击消耗它时只会获得准备效果，不会立刻打出终结技。`

## 验收

- 第一次打开背包时，`tutorial_backpack` 共显示 3 页。
- 第 3 页使用 `T_Tutorial512_WeaponCards` 插图。
- 关闭教程后进入背包编排界面，终结技卡牌仍由卡组 UI 显示锁定/解锁状态。
