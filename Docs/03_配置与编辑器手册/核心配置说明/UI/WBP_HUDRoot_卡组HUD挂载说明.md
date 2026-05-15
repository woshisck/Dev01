# WBP_HUDRoot 卡组 HUD 挂载说明

## 作用

`WBP_HUDRoot` 是主 HUD 容器。

卡组条需要作为子控件放进 `WBP_HUDRoot`，这样 `YogHUD` 才能找到它，并把玩家的 `CombatDeckComponent` 绑定到卡组 HUD。

## 父类

`WBP_HUDRoot` 的父类应保持为项目现有的：

```text
YogHUDRootWidget
```

## 推荐层级

在 `WBP_HUDRoot` 的主 `CanvasPanel` 中加入：

```text
CombatDeckBar: WBP_CombatDeckBar
```

## 组件用途和数据来源

| 控件名 | 类型 | 用途 | 数据来源 |
| --- | --- | --- | --- |
| `CombatDeckBar` | `WBP_CombatDeckBar` | 显示战斗卡组 HUD | `YogHUD` 在玩家 Possess 后绑定 `PlayerCharacterBase -> CombatDeckComponent` |

## 推荐配置

| 控件 | 关键属性 |
| --- | --- |
| `CombatDeckBar` | `Class=WBP_CombatDeckBar; Name=CombatDeckBar; Is Variable=true` |
| `CombatDeckBar` | `Slot -> Anchors=Bottom Center; Alignment=(0.5,1.0); Position X=0; Position Y=0` |
| `CombatDeckBar` | `Size X=900; Size Y=120; ZOrder=20` |
| `CombatDeckBar` | `Visibility=Hit Test Invisible` |
| `CombatDeckBar` | `Render Opacity=1.0` |

## 屏幕适配参数

| 分辨率情况 | 参数 |
| --- | --- |
| 16:9 桌面默认 | `Position Y=0; Size=(900,120)` |
| UI 挡住玩家血条 | `Position Y=-40` |
| 卡牌太大 | `WBP_CombatDeckBar Size X=760; Host Size Y=112` |
| 卡牌太小 | `WBP_CombatDeckBar Size X=960; Host Size Y=140` |

## 必须检查

- `WBP_HUDRoot` 内存在一个 `WBP_CombatDeckBar` 子控件。
- 子控件变量名必须是 `CombatDeckBar`。
- `CombatDeckBar` 必须勾选 `Is Variable`。
- 玩家角色需要有 `CombatDeckComponent`，否则卡组 HUD 会显示为空。
