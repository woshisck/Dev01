# WBP_CombatDeckEditWidget配置说明

## 1. 作用
用于背包中编辑当前武器的战斗卡组：纵向显示卡牌、选择卡牌、拖动换位、反转连携方向，并把选中卡牌信息显示到 `WBP_RuneInfoCard`。

## 2. 创建
| 项            | 配置                                                                  |
| ------------ | ------------------------------------------------------------------- |
| Widget 名称    | `WBP_CombatDeckEditWidget`                                          |
| Parent Class | `CombatDeckEditWidget`                                              |
| 挂载位置         | 放到 `WBP_BackpackScreen` 中                                           |
| 子控件命名        | 根背包中该控件命名为 `CombatDeckEditWidget`，C++ 会自动绑定玩家 `CombatDeckComponent` |

## 3. 推荐层级
```text
Root: Border 或 SizeBox
  HorizontalBox
    CardListPanel: ScrollBox
      CardListBox: VerticalBox
    RuneInfoCard: WBP_RuneInfoCard（已有信息卡，可选）
```

说明：这里不是新建一个叫 `RuneInfoCard` 的 WBP。这里是把已有的 `WBP_RuneInfoCard` 放进 `WBP_CombatDeckEditWidget`，并把这个子控件实例命名为 `RuneInfoCard`，C++ 才能自动把选中卡牌信息传给它。

## 4. 控件参数
| 控件              | 关键属性                                                    |
| --------------- | ------------------------------------------------------- |
| `Root`          | Width Override: 520-640；Height Override: 720；Padding: 8 |
| `CardListPanel` | Width Override: 300-360；Scroll Bar Visibility: `Visible`。如果不想常驻显示滚动条，可填 `Hidden`，仍然可以滚动 |
| `CardListBox`   | 必须命名为 `CardListBox`，类型 `VerticalBox`                    |
| `RuneInfoCard`  | 可选；类型用已有 `WBP_RuneInfoCard`，子控件实例命名为 `RuneInfoCard`，用于显示选中卡牌 DA 信息 |
| `CardSlotClass` | 填 `WBP_CombatDeckEditCardSlot`                          |

## 5. 使用
把 `WBP_CombatDeckEditWidget` 放入现有背包界面后，不需要蓝图手动绑定。打开背包时会读取玩家当前武器的战斗卡组。

策划操作：
| 操作 | 效果 |
| --- | --- |
| 点击/悬停/手柄焦点移动到卡牌 | 选中并在右侧显示卡牌信息 |
| 长按拖动卡牌到另一张卡牌上半区 | 插入到目标卡牌前面 |
| 长按拖动卡牌到另一张卡牌下半区 | 插入到目标卡牌后面 |
| 点击连携箭头/反转按钮 | 正向/反向连携切换 |

卡牌排序只通过拖拽完成，不需要在卡牌行上配置上下移动按钮。战斗中或只读预览状态下，C++ 会锁定编辑交互，并自动播放基础红色反馈。

## 6. 功能边界
`WBP_CombatDeckEditWidget` 只负责 Designer 层级和组件命名。以下功能都由 C++ 完成，不需要在蓝图里写逻辑：

| 功能 | 处理位置 |
| --- | --- |
| 打开背包自动绑定玩家战斗卡组 | C++ |
| 生成卡牌列表 | C++ |
| 点击/悬停/手柄焦点自动选中 | C++ |
| 长按拖拽排序 | C++ |
| 反转连携方向 | C++ |
| 战斗中或预览状态锁定交互 | C++ |
| 锁定时红色闪烁/位移反馈 | C++ |

WBP 不需要配置时间参数，也不要写 `Delay`、`Timer`、`Tick` 或拖拽排序蓝图。
