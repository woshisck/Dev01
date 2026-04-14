# 背包系统蓝图制作指南

> 适用范围：背包 UI 蓝图制作、三选一配置、场景拾取物放置  
> 适用人群：策划 + 程序  
> 配套文档：[背包系统技术文档](../Systems/BackpackSystem_Technical.md)  
> 最后更新：2026-04-14

---

## 概述

本指南指导完成明天展示所需的最低限度背包系统，包含：

1. **配置符文池**（GameMode BP，10 分钟）
2. **确认三选一 UI**（WBP_LootSelection，15 分钟）
3. **制作背包 UI**（WBP_BackpackScreen，45 分钟）
4. **绑定 Tab 键开关背包**（PlayerController，10 分钟）
5. **在场景放置拾取物**（5 分钟）

完成后即可跑通：**走近拾取物 → 三选一 → 符文自动入背包 → Tab 打开背包调整位置 → 热度上升自动激活**。

---

## 一、配置符文池（必做，最先做）

在 **BP_YogGameModeBase** Details 面板：

1. 找到 **Level Flow → Fallback Loot Pool**
2. 添加以下符文资产（在 `Content/Docs/BuffDocs/Playtest_GA/` 下）：

| 推荐填入的符文 | 路径 |
|---|---|
| `DA_Rune_AttackUp` | .../AttackUp/ |
| `DA_Rune_Bleed` | .../Bleed/ |
| `DA_Rune_DeadlyStrike` | .../DeadlyStrike/ |
| `DA_Rune_Frenzy` | .../Frenzy/ |
| `DA_Rune_ShadowDash` | .../ShadowDash/ |
| `DA_Rune_VenomFang` | .../VenomFang/ |
| `DA_Rune_Shockwave` | .../Shockwave/ |
| `DA_Rune_WeaknessUnveiled` | .../Weakness_Unveiled/ |

> ⚠️ 至少填 3 个，否则三选一无法显示。

---

## 二、确认三选一 UI（WBP_LootSelection）

打开 `Content/UI/Playtest_UI/WBP_LootSelection`，确认以下两个事件已实现：

### `OnLootOptionsReady(LootOptions)`

```
For Each (LootOptions, Index):
  找到第 Index 张卡片
  → 设置卡片名称文字 = LootOptions[Index].RuneAsset.RuneInfo.RuneConfig.RuneName
  → 设置卡片图标   = LootOptions[Index].RuneAsset.RuneInfo.RuneConfig.RuneIcon
  → 卡片按钮 OnClicked → SelectRuneLoot(Index)
Set Visibility → Visible（显示整个 Widget）
```

### `OnLevelPhaseChanged(NewPhase)`

```
Switch (NewPhase):
  Arrangement → Set Visibility Visible
  其他        → Set Visibility Hidden
```

### 添加到 Viewport

在 **PlayerController 或 HUD 的 Event BeginPlay**：

```
Create Widget [WBP_LootSelection] → Add to Viewport
```

---

## 三、制作背包 UI（WBP_BackpackScreen）

### 3.1 新建 Widget

- 路径：`Content/UI/Backpack/WBP_BackpackScreen`
- **父类**：选 `BackpackScreenWidget`（C++ 基类）

### 3.2 UMG 布局

```
Canvas Panel（填满屏幕）
└── Border [全屏半透明，Brush Color #1A1A1A, Alpha 0.9]
    └── Horizontal Box [宽900 × 高600，居中]
        │
        ├── [左侧] Vertical Box [宽260]
        │   ├── Text "待放置符文" [字号16，居中]
        │   ├── ScrollBox              ← 变量名: RuneListBox
        │   └── Border [高100，描述区]
        │       ├── Text               ← 变量名: InfoName  [字号14，加粗]
        │       └── Text               ← 变量名: InfoDesc  [字号11，自动换行]
        │
        └── [右侧] Vertical Box [宽600]
            ├── Text "背包" [字号16，居中]
            ├── Uniform Grid Panel     ← 变量名: BackpackGrid
            │   └── 25 个 Button（见 3.3）
            └── Horizontal Box [底部操作栏]
                ├── Button "移除选中" → OnClicked: RemoveRuneAtSelectedCell
                └── Button "取消选中" → OnClicked: ClearSelection
```

### 3.3 放置 25 个格子 Button

在 `BackpackGrid`（Uniform Grid Panel）里放 25 个 Button：

| 参数 | 设置 |
|---|---|
| Column | 0 ~ 4 |
| Row | 0 ~ 4 |
| 尺寸 | 90 × 90 |
| 每个 Button 内 | Vertical Box → Image（70×70）+ Text（字号8）|

建立数组变量 `GridButtons`（类型：`Button` 数组），在 Event Construct 里按 **Col × 5 + Row** 的顺序填入所有 25 个 Button 引用。

每个 Button 对应的 `OnClicked` 绑定：

```
Btn(Col, Row) OnClicked → ClickCell(Col, Row)
```

### 3.4 Event Construct

```
Event Construct
  → 填充 GridButtons 数组（Add 25个Button，顺序：Col=0,Row=0 到 Col=4,Row=4）
  → 调用自定义函数 [RefreshRuneList]
  → 调用 OnGridNeedsRefresh
```

### 3.5 自定义函数：`RefreshRuneList`

```
RuneListBox → Clear Children

GetRuneList() → For Each (ArrayIndex, RuneInstance)
  → Create Widget [Button]（或直接 Create Text Block）
      标签文字 = RuneInstance.RuneConfig.RuneName
      如果 ArrayIndex < GetPendingRuneCount()：
        标签文字前加 "★ "（表示三选一获得的符文）
      OnClicked → SelectRuneFromList(ArrayIndex)
  → RuneListBox → Add Child
```

### 3.6 实现 `OnGridNeedsRefresh`

```
For Col = 0 To 4:
  For Row = 0 To 4:
    Index = Col * 5 + Row
    Button = GridButtons[Index]

    [颜色] GetCellVisualState(Col, Row) → Switch:
      Empty            → Button BG Color = #3A3A3A
      EmptyActive      → Button BG Color = #1A3A6A
      OccupiedActive   → Button BG Color = #2266CC
      OccupiedInactive → Button BG Color = #7A4A1A

    [选中高亮] IsCellSelected(Col, Row) → True:
      Button BG Color = #FFAA00（覆盖上面的颜色）

    [内容] IsCellOccupied(Col, Row) → True:
      GetRuneAtCell(Col, Row) → 设置 Button 内的 Image = RuneConfig.RuneIcon
                               → 设置 Button 内的 Text  = RuneConfig.RuneName
    → False:
      Image = None，Text = ""
```

### 3.7 实现 `OnSelectionChanged`

```
调用 RefreshRuneList
调用 OnGridNeedsRefresh

HasSelectedRune() → True:
  GetSelectedRuneInfo() → InfoName.SetText = RuneConfig.RuneName
                        → InfoDesc.SetText = RuneConfig.RuneDescription
→ False:
  InfoName.SetText = ""
  InfoDesc.SetText = ""
```

### 3.8 实现 `OnRuneListChanged`

```
调用 RefreshRuneList
```

### 3.9 实现 `OnStatusMessage(Message)`

```
StatusText.SetText(Message)
Delay(2.0)
StatusText.SetText("")
```

> 需要在布局中添加一个底部 **Text 控件**，变量名 `StatusText`，字号 12，居中。

---

## 四、绑定 Tab 键开关背包

### 4.1 添加 InputAction

**Project Settings → Input → Action Mappings** → 新增：

| 名称 | 按键 |
|---|---|
| `OpenBackpack` | Tab |

### 4.2 在 PlayerController 蓝图绑定

```
Event Begin Play
  → Create Widget [WBP_BackpackScreen] → 存入变量 BackpackWidget
  → Add to Viewport
  → Set Visibility: Hidden

InputAction "OpenBackpack" (Pressed)
  → BackpackWidget Is Visible?
      True  → Set Visibility Hidden
              Set Input Mode: Game Only
      False → Set Visibility Visible
              Set Input Mode: Game And UI
              Set Keyboard Focus: BackpackWidget
```

---

## 五、在场景放置拾取物

1. 打开初始关卡（测试关卡）
2. 在 **Content Browser** 找到 `BP_RewardPickup`
3. 拖入场景，放置 **3 个**，分散摆放（玩家需要依次走近）
4. 无需配置，自动读取 GameMode 的 `FallbackLootPool`

---

## 六、展示前验证清单

| 检查项 | 验证方法 |
|---|---|
| 编译通过 | 编辑器无报错 |
| FallbackLootPool 已填 | GameMode BP Details 有符文资产 |
| WBP_LootSelection 已添加到 Viewport | PIE 走近拾取物按 E，弹出三选一 |
| 选符文后自动入格子 | Output Log 打印"自动放置到 (0,0)" |
| Tab 键打开背包 | 按 Tab 显示 WBP_BackpackScreen |
| 格子颜色正确 | 激活区深蓝，已放置符文橙色 |
| 点格子选中 → 点空格子移动 | 符文位置变化，颜色随之刷新 |
| 热度增加后激活区符文变蓝 | 符文颜色从橙 → 亮蓝 |

---

## 七、常见问题

**Q：走近拾取物按 E，但三选一没弹出？**  
A：检查以下几点：
1. GameMode BP 的 `FallbackLootPool` 是否有符文（至少 3 个）
2. `WBP_LootSelection` 是否已 Add to Viewport
3. Output Log 是否有"GenerateLootOptions: 无可用符文池"的 Warning

**Q：选了符文但背包 UI 没有显示？**  
A：
1. 检查 Output Log 是否有"自动放置到 (X,Y)"的 Log
2. 如果有 Log 说明格子已放置，是 UI 刷新问题 → 检查 `OnGridNeedsRefresh` 的 GridButtons 数组是否正确填充

**Q：格子颜色全是深灰，没有深蓝激活区？**  
A：`GetActivationZoneCells()` 依赖 `ActivationZoneConfig`。在 Player 角色蓝图的 BackpackGridComponent Details 面板检查 `Activation Zone Config` 是否有 ZoneShapes 数据（默认值由 C++ `MakeDefault()` 提供，无需手动填写）。

**Q：热度增加但符文没有激活（颜色不变蓝）？**  
A：`OnHeatValueChanged` 需要由 `PlayerAttributeSet::PostAttributeChange` 调用。检查 AttributeSet 里是否有相关调用逻辑。
