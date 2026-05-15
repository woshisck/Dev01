# EUW_CombatLog配置说明

## 1. 作用
`EUW_CombatLog` 是编辑器内战斗日志工具，用于在 PIE 调试时查看玩家造成的普通伤害、暴击、符文伤害、流血、卡牌消耗、卡牌命中、连携、终结技和洗牌事件。

当前版本参考 Dota2 战斗记录表现：默认显示可读短句，并用颜色区分时间、来源、目标、伤害数字和目标生命变化；勾选调试模式后，会追加 `DamageType`、伤害公式和卡牌触发标记。

## 2. 配置位置
| 项 | 配置 |
| --- | --- |
| 资产路径 | `Content/UI/DebugUI/EUW_CombatLog` |
| Widget 类型 | Editor Utility Widget Blueprint |
| Parent Class | `CombatLogEditorUtilityWidget`（长期正式方案） |
| 打开方式 | 推荐 `Tools > DevKit > Combat Log` 打开独立编辑器窗口；也可 Content Browser 右键资产 `Run` |
| 数据来源 | `UCombatLogStatics` 静态日志桥，PIE 中由战斗和卡组逻辑写入 |

## 3. 必要控件命名
以下控件建议在 Designer 中手动放置并命名。字段是 `BindWidgetOptional`，未放置不会报错，但对应区域可能显示不完整。

| 控件名 | 类型 | 作用 |
| --- | --- | --- |
| `FilterButtonBox` | `WrapBox` | 顶部控制栏容器，C++ 自动生成类型按钮、攻击者/目标下拉、调试勾选和时间窗口滑条 |
| `LogScrollBox` | `ScrollBox` | 日志滚动区域，C++ 动态生成分段上色日志行 |
| `SummaryText` | `TextBlock` | 底部统计摘要 |
| `BtnReset` | `Button`，可选 | 绑定 `ResetLog()`，清空日志和统计 |

## 4. 推荐布局
```text
Root: CanvasPanel
  FilterButtonBox: WrapBox
  LogScrollBox: ScrollBox
  SummaryText: TextBlock
  BtnReset: Button（可选）
```

长期迁移步骤：
1. 打开 `Content/UI/DebugUI/EUW_CombatLog`。
2. 在 Class Settings 中把 Parent Class 改为 `CombatLogEditorUtilityWidget`。
3. Designer 中保留或新建 `FilterButtonBox`、`LogScrollBox`、`SummaryText`；旧的 `LogText` 可以删除或隐藏。
4. 编译并保存蓝图，重新 Run 该 EUW。
5. 若保留 `BtnReset`，OnClicked 绑定 `ResetLog()`。

推荐启动方式：
1. 编译并重启 UE 编辑器。
2. 在顶部菜单打开 `Tools > DevKit > Combat Log`。
3. 该入口会创建 `DevKitCombatLog` 编辑器窗口，内部直接使用 `CombatLogEditorUtilityWidget`，无需手动运行 EUW 资产。
4. 若仍从 Content Browser 运行 `EUW_CombatLog`，请确保父类已改为 `CombatLogEditorUtilityWidget`；旧父类只走兼容层。

推荐位置：
| 控件 | 推荐值 |
| --- | --- |
| `FilterButtonBox` | 顶部横展，高度 104；C++ 会自动生成定宽筛选按钮，避免出现空白小方块 |
| `LogScrollBox` | 位于控制栏下方并避开底部统计区；C++ 会自动修正旧 Canvas Slot，日志行使用深色底和分段阴影文字 |
| `SummaryText` | 底部固定高度 126，字体 10；用于显示三行紧凑统计 |
| `BtnReset` | 右上角或控制栏末尾，OnClicked 调用 `ResetLog()` |

## 5. Class Defaults
| 字段 | 推荐值 | 说明 |
| --- | --- | --- |
| `LogFontSize` | `11` | 日志行字体大小；运行时会被限制在 `MinLogFontSize` 与 `MaxLogFontSize` 之间，避免旧蓝图默认值把日志撑爆 |
| `FilterFontSize` | `10` | 控制栏文字大小；运行时限制在 9-14，保证按钮文字可读 |
| `SummaryFontSize` | `10` | 底部统计文字大小；运行时限制在 9-13 |
| `MinLogFontSize` / `MaxLogFontSize` | `10` / `16` | 日志字体保护范围，用于修正截图中超大字号、横向溢出的情况 |
| `ControlBarHeight` | `104` | 顶部筛选栏高度；C++ 会自动修正旧 WBP 的 Canvas Slot |
| `SummaryHeight` | `126` | 底部统计区高度；C++ 会自动把日志区留出空间 |
| `DefaultTimeWindowSeconds` | `30` | 默认只看最近 30 秒；滑条拖到最左为“全部” |
| `MaxTimeWindowSeconds` | `60` | 时间窗口滑条最大秒数 |
| `bDefaultDebugMode` | `false` | 默认关闭公式和内部标记 |
| `LogRowPadding` | `(2,2,2,2)` | 每条日志行内边距 |
| `LogRowBackgroundColor` | 深色透明 | 日志行背景色；可根据 UI 主题微调 |
| `PanelBackgroundColor` | 深色不透明 | 工具根背景色，覆盖编辑器默认灰底 |
| `ControlTextColor` / `SummaryTextColor` | 冷灰色 | 控制栏和摘要区文字颜色 |

## 6. 使用说明
| 操作 | 效果 |
| --- | --- |
| 点击 `全部/普通/暴击/符文/流血/卡牌/终结技/连携/洗牌` | 按事件类型过滤 |
| 攻击者下拉 | 只看指定来源；名称会自动清理 `BP_`、`_C`、数字后缀 |
| 目标下拉 | 只看指定目标 |
| 调试勾选 | 日志行追加 `DamageType`、`BaseAttack x ActionMultiplier x DmgTaken` 和卡牌标记 |
| 时间窗口滑条 | 默认最近 30 秒；最左为全部历史 |
| 重置按钮 | 调用 `ResetLog()` 清空静态日志 |

## 7. 日志颜色
| 内容 | 颜色语义 |
| --- | --- |
| 时间戳 | 灰色 |
| 来源 | 紫色 |
| 目标 | 蓝色 |
| 伤害数字 | 红色 |
| 生命变化 `(前->后)` | 绿色 |
| 符文 | 紫色 |
| 卡牌 | 淡蓝 |
| 匹配 | 青色 |
| 连携 | 橙色 |
| 终结技 | 金色 |

## 8. 验收方式
1. 打开 `EUW_CombatLog`，进入 PIE。
   - 推荐路径：`Tools > DevKit > Combat Log`。
   - 兼容路径：Content Browser 右键 `EUW_CombatLog` 资产并 Run。
2. 使用玩家攻击敌人，确认出现类似 `[09:51.32] 玩家击中了敌人，造成81点伤害 (392->311)。` 的短句。
3. 触发暴击、符文、流血、卡牌消耗、连携、终结技和洗牌，确认类型过滤能筛出对应行。
4. 切换攻击者和目标下拉，确认列表只保留匹配对象。
5. 拖动时间窗口滑条，确认最近时间窗口会过滤旧记录，最左显示全部。
6. 勾选调试模式，确认行尾出现 `DamageType`、公式和卡牌触发标记。
7. 点击重置按钮，确认日志区和统计区清空。

## 9. 注意事项
- 当前工具只面向编辑器调试，不作为运行时 HUD。
- 不需要在蓝图中手动创建过滤按钮、下拉框、勾选框或滑条；这些由 C++ 自动生成到 `FilterButtonBox`。
- 如果旧蓝图里 `LogFontSize` 被设置得很大，运行时会自动夹到 10-16；仍建议在 Class Defaults 中保留推荐值，避免 Designer 预览误判。
- 当前 UI 优化使用 UMG 原生控件、颜色和文字阴影完成；没有缺失的位图素材，因此本轮未调用 GPT Image 2 生成图片。
- 旧版 `EUW_CombatLog` 资产若仍然继承默认 `EditorUtilityWidget` 并使用单个 `LogText`，C++ 会在 `GetFormattedLog()` 调用时动态创建 `CombatLogLegacyScrollBox` 覆盖日志区，隐藏旧 `LogText`，从而提供滚动条和分段上色；重启编辑器后生效。
- 长期推荐把 `EUW_CombatLog` 父类改为 `CombatLogEditorUtilityWidget`，并在 Designer 中保留 `FilterButtonBox`、`LogScrollBox`、`SummaryText` 三个控件，这样无需兼容层即可获得完整 Dota2 式日志行。
- 目标生命变化由伤害执行阶段的快照估算，已考虑基础护甲吸收，但不是最终回放系统；如果后续需要完全精确的落地血量，应把日志写入点移动到 `DamageAttributeSet` 的最终扣血路径。
- `FilterButtonBox` 如果不在 Designer 手动绑定，C++ 会自动创建，但位置只适合作兜底；正式使用建议手动放置。
- `DamageBreakdownWidget` 源码已移除，当前战斗日志编辑器入口以 `EUW_CombatLog` + `CombatLogEditorUtilityWidget` 为准。
