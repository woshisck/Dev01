# WBP_CombatDeckEditCardSlot配置说明

## 1. 作用
背包卡组编辑列表中的单张卡牌行。显示卡牌类型、名称、连携方向，并提供选择、拖拽排序、反转入口。

## 2. 创建
| 项            | 配置                                       |
| ------------ | ---------------------------------------- |
| Widget 名称    | `WBP_CombatDeckEditCardSlot`             |
| Parent Class | `CombatDeckEditCardSlotWidget`           |
| 使用位置         | `WBP_CombatDeckEditWidget.CardSlotClass` |

## 3. 推荐层级
```text
Root: Border
  HorizontalBox
    CardIcon: Image
    TypeText: YogCommonUI Text 或 TextBlock
    CardNameText: YogCommonUI Text 或 TextBlock
    SelectedMark: Border/Image
    DirectionText: YogCommonUI Text 或 TextBlock
    ReverseButton: Button
```

## 4. 控件参数
| 控件 | 关键属性 |
| --- | --- |
| `Root` | Height Override: 44-52；Padding: 6；Color: 深色 `(0.06,0.06,0.08,0.9)` |
| `CardIcon` | Size: 36x36；无图标时 C++ 自动隐藏 |
| `TypeText` | Font Size: 12；显示 `普通` / `连携` / `终结技` |
| `CardNameText` | Font Size: 15；Bold；读取卡牌 `DisplayName`，为空时读取 DA 符文名 |
| `SelectedMark` | Width: 4-6；选中时显示，未选中自动隐藏 |
| `DirectionText` | Font Size: 20；连携卡显示 `↑` 或 `↓`，普通/终结技自动隐藏 |
| `ReverseButton` | 图标建议旋转/交换箭头；仅连携卡显示 |

## 5. 方向规则
| 显示 | 含义 |
| --- | --- |
| `↑` | 正向连携，读取上一张卡 |
| `↓` | 反向连携，等待下一张卡触发 |

拖动换位由 C++ 处理。只要该行 Widget 是 `CombatDeckEditCardSlotWidget` 子类，就可以把一行拖到另一行上调整顺序。

## 6. 交互规则
| 操作 | 效果 |
| --- | --- |
| 鼠标单击卡牌行 | 选中该卡牌 |
| 鼠标悬停卡牌行 | 自动选中该卡牌 |
| 手柄十字键/左摇杆移动焦点到卡牌行 | 自动选中该卡牌 |
| 长按并拖动卡牌行 | 开始拖拽排序 |
| 拖到另一张卡牌上半区松开 | 插入到目标卡牌前面 |
| 拖到另一张卡牌下半区松开 | 插入到目标卡牌后面 |
| 点击 `ReverseButton` | 仅连携卡切换 `↑` / `↓` |

不要在这个 WBP 上做上下移动按钮。卡牌位置只通过拖拽改变。

## 7. 功能边界
这个 WBP 只负责创建 Designer 组件和基础样式，不需要在蓝图里写时间、拖拽、排序、反转、锁定判断。

| 功能 | 处理位置 |
| --- | --- |
| 长按多久进入拖拽 | C++ 固定处理，目前为 `0.18s` |
| 拖拽浮空透明效果 | C++ 自动处理 |
| 战斗中禁止操作的红色闪烁/位移反馈 | C++ 自动处理 |
| 插入到上半区/下半区 | C++ 根据鼠标位置判断 |
| 点击、悬停、手柄焦点选中 | C++ 自动处理 |

WBP 蓝图里不需要实现 `Delay`、`Timer`、`Tick`、拖拽事件或排序逻辑。只要控件名称正确，功能会自动生效。
