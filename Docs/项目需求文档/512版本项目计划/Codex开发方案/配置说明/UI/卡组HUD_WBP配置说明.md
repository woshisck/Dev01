# 卡组 HUD WBP 配置说明

## 作用

卡组 HUD 由 3 个 WBP 组成：

| WBP                      | 作用                    | 配置文档                            |
| ------------------------ | --------------------- | ------------------------------- |
| `WBP_CombatDeckCardSlot` | 单张战斗卡牌槽位              | `WBP_CombatDeckCardSlot配置说明.md` |
| `WBP_CombatDeckBar`      | 卡组条，管理多个卡牌槽、洗牌进度和提示文本 | `WBP_CombatDeckBar配置说明.md`      |
| `WBP_HUDRoot`            | 主 HUD 容器，把卡组条挂到屏幕上    | `WBP_HUDRoot_卡组HUD挂载说明.md`      |

## 配置顺序

1. 先创建 `WBP_CombatDeckCardSlot`。
2. 再创建 `WBP_CombatDeckBar`，把 8 个 `WBP_CombatDeckCardSlot` 放进去。
3. 最后打开 `WBP_HUDRoot`，把 `WBP_CombatDeckBar` 放到主 HUD。

## 文字控件规则

推荐默认使用 `Yog Common Rich Text Block`，方便后续做关键词高亮、效果说明、流血/月光等说明文本。

普通 `TextBlock` 只作为旧 WBP 或临时测试兼容方案。

所有绑定控件必须：

- 名字和文档一致。
- 勾选 `Is Variable`。
- 不要改 C++ 绑定字段名。

