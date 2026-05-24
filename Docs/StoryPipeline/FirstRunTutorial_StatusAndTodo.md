# 第一局教程流程完成结果与待做事项

## 已完成

- 新存档会写入 `Story.Flag.FirstRunTutorial.Active`，完成后可写入 `Story.Flag.FirstRunTutorial.Completed`。
- `YogGameInstanceBase` 支持 `FirstRunTutorialMap`，新存档首局教程可以加载独立教程地图；当前默认仍指向现有 `InitialRoom`。
- `RoomDataAsset` 增加教程字段：
  - `bForceSinglePortal`
  - `ForcedPortalIndex`
  - `bUseFixedRewardOptions`
  - `FixedRewardOptions`
- `RewardPickup` 支持三类奖励：
  - `Rune`：继续走现有卡牌选择 UI。
  - `Gold`：拾取后直接写入背包金币。
  - `Material`：拾取后写入指定 `MetaCurrencyTag`；未指定 tag 时按占位材料拾取处理。
- `CombatDeckComponent` 增加 `OnDeckCardsEntered`，`CombatDeckBarWidget` 已用 C++ 实现 1D 卡组进卡高亮；`BP_OnDeckCardsEntered` 只作为可选蓝图扩展点。
  - 默认时间：0.1s 渐入、0.2s 保持、0.15s 渐出。
  - 可调参数：`EntryHighlightFadeInDuration`、`EntryHighlightHoldDuration`、`EntryHighlightFadeOutDuration`、`EntryHighlightPeakScale`、`EntryHighlightPeakOpacity`。
- loading 图已改为 `imagegen` 生成的真实位图，不再使用程序绘制/SVG 风格占位。
  - 源图：`SourceArt/UI/Loading/T_FirstRunTutorial_Loading.png`
  - UE 资产：`/Game/UI/Loading/T_FirstRunTutorial_Loading`
  - 已绑定到：`/Game/UI/UI_LoadingScreen`
- 新增 `FirstRunLoadingScreenSetup` Commandlet，可重复导入 loading 图并绑定加载屏。

## 待做

- 配置教程专用地图或确认 `FirstRunTutorialMap` 目标地图。
- 在教程主城中摆放并绑定：
  - 移动提示 Trigger
  - 冲刺提示 Trigger
  - 教程武器拾取点
  - 木人桩
  - 木人桩死亡后的单卡奖励掉落点
  - 教程传送门
- 配置教程武器 `WeaponDefinition.InitialCombatDeck = [攻击, 攻击]`。
- 打开 `WBP_CombatDeckBar`，确认 `DeckEntryHighlightPanel` 存在，并按观感微调 C++ 暴露的 Entry Highlight 参数。
- 配置教程房间 `RoomDataAsset`：
  - 第一战斗房：金币固定奖励。
  - 第二战斗房：固定三选一卡牌 `[攻击, 重击, 分裂]`，指定 Portal。
  - 连携卡房：特殊敌人死亡掉落 `[月光]`。
  - 过渡房：金币/材料奖励。
  - 祈祷室：献祭获得武器终结技。
- 配置特殊敌人蓝色周身雾效，若走刷怪逻辑则绑定到最后一个敌人。
- 配置无限刷敌失败房，并在玩家死亡后显示回归主城选项。
- 配置教程完成后回主城的最终卡组 `[攻击, 攻击, 月光, 武器终结技]`。
- 修复或确认项目已有缺失资源 `/Game/SlashTrailElemental/Niagara/SlashTrail/NS_SlashTrail_Dark`；它会导致 Commandlet 进程返回失败码，但不影响 loading 图导入结果。
