# 背包系统 UI 制作手册（零基础版）

> 适用范围：WBP_BackpackScreen 蓝图制作、Tab 键绑定、符文池配置、场景拾取物  
> 适用人群：完全不熟悉 UE5 蓝图和 UMG 的人  
> 配套文档：[背包系统配置指南](../FeatureConfig/BackpackSystem_Guide.md)  
> 最后更新：2026-04-14

---

## 读这个文档之前

**你需要完成的事情（按顺序）：**

| 步骤 | 任务 | 预计时间 |
|---|---|---|
| 1 | 确认三选一 UI 是否可用 | 10 分钟 |
| 2 | 配置游戏模式的符文池 | 10 分钟 |
| 3 | 制作背包 UI | 60 分钟 |
| 4 | 绑定 Tab 键打开背包 | 15 分钟 |
| 5 | 在场景放置拾取物 | 5 分钟 |

**本文约定：**
- `Content Browser`：编辑器左下角的资源浏览器
- `Details`：选中物体后右边出现的属性面板
- `Event Graph`：蓝图里的逻辑编辑区域
- `Designer`：Widget 蓝图里的布局编辑区域

---

## 第一步：确认三选一 UI

> 队友已经更新了 `WBP_LootSelection`，先检查它能不能用，省去重复制作。

### 1.1 打开文件

1. 在 `Content Browser` 导航到：`Content → UI → Playtest_UI`
2. 双击打开 **`WBP_LootSelection`**

### 1.2 检查 Designer 布局

打开后默认进入 **Designer** 视图（右上角有 `Designer` 和 `Graph` 两个按钮）。

**你应该能看到：** 一个显示了大约 3 张卡片区域的界面布局。

### 1.3 切换到 Event Graph，检查两个事件是否连线

点击右上角 **`Graph`** 按钮，进入蓝图逻辑视图。

在空白处右键 → 搜索 `OnLootOptionsReady`，看看能不能找到这个事件节点（橙色/红色的大块节点）。

**情况A — 找到了，且后面有连线** → 三选一 UI 已完成，**直接跳到第二步**。

**情况B — 找不到，或节点后面没有连线** → 需要手动连线，见下方 1.4。

### 1.4 如果需要手动连线（情况B）

在 Event Graph 空白处 **右键 → 搜索 `OnLootOptionsReady`** → 点击添加。

添加后这个节点是一个红色/橙色的大节点，从它的白色执行箭头（`►`）往后连：

> **如果你完全看不懂怎么连线，先跳过这步，去找队友确认，不要耽误其他步骤。**

---

## 第二步：配置符文池

> 这是最重要的配置，不做的话三选一界面里没有符文可选。

### 2.1 找到 GameMode 蓝图

在 `Content Browser` 导航到：`Content → Code → Core → GameMode`

找到 **`BP_YogGameModeBase`**（蓝色齿轮图标），**单击选中，不要双击**。

### 2.2 打开并找到 FallbackLootPool

双击打开 `BP_YogGameModeBase`，点击工具栏上的 **`Class Defaults`** 按钮（或者直接看右边的 Details 面板）。

在 Details 面板右上角的搜索框里输入 **`Fallback`**。

你会看到一个叫 **`Fallback Loot Pool`** 的属性，右边有一个 `+` 号按钮。

### 2.3 添加符文

点击 `+` 号 **8 次**，添加 8 个槽位。

然后逐个点击每个槽位右边的下拉框（或者"无"字样），搜索以下符文：

| 槽位 | 输入搜索词 | 选择 |
|---|---|---|
| 0 | `DA_Rune_AttackUp` | DA_Rune_AttackUp |
| 1 | `DA_Rune_Bleed` | DA_Rune_Bleed |
| 2 | `DA_Rune_DeadlyStrike` | DA_Rune_DeadlyStrike |
| 3 | `DA_Rune_Frenzy` | DA_Rune_Frenzy |
| 4 | `DA_Rune_ShadowDash` | DA_Rune_ShadowDash |
| 5 | `DA_Rune_VenomFang` | DA_Rune_VenomFang |
| 6 | `DA_Rune_Shockwave` | DA_Rune_Shockwave |
| 7 | `DA_Rune_WeaknessUnveiled` | DA_Rune_WeaknessUnveiled |

### 2.4 保存

按 **`Ctrl+S`** 保存。右上角弹出"Saved"提示即成功。

---

## 第三步：制作背包 UI（核心步骤）

> 这是最长的步骤，约 60 分钟，请找个安静的时间段完整做完。

### 3.1 创建 Widget 蓝图

1. 在 `Content Browser` 左上角点击 **`+ Add`（或绿色的 Add 按钮）**
2. 选择 **`User Interface → Widget Blueprint`**
3. 弹出"Pick Parent Class"对话框，在搜索框里输入 **`BackpackScreenWidget`**
4. 选中它，点击 **`Select`**
5. 文件命名为 **`WBP_BackpackScreen`**，保存到 `Content/UI/Backpack/`（如果文件夹不存在就新建）

> ⚠️ 父类**必须选 BackpackScreenWidget**，不能选默认的 UserWidget，否则所有 C++ 函数都找不到。

### 3.2 进入 Designer 视图

双击打开刚创建的 `WBP_BackpackScreen`，默认应该在 **Designer** 视图。

左边是 **Palette**（控件列表），中间是画布，右边是 **Details**（属性）。

### 3.3 修改画布尺寸

在 **Designer** 视图，点击中间画布空白区域（确保没有选中任何控件）。

在右边 **Details** 找到 **Screen Size**，改为：
- Width：**1920**
- Height：**1080**

### 3.4 搭建整体框架

#### 放第一个控件：Canvas Panel

在左边 **Palette** 搜索 **`Canvas Panel`**，拖到中间画布。

选中 Canvas Panel，在右边 Details：
- `Is Variable`：打勾（这样后面可以引用它）
- 把它拉满全屏

#### 放半透明背景：Border

在 Palette 搜索 **`Border`**，拖到 Canvas Panel 里面（注意要拖到 Canvas Panel 内部，左边 Hierarchy 面板里 Border 要缩进在 Canvas Panel 下方）。

选中 Border，在右边 Details：
- **Anchors**：点击右边的锚点图标，选 **"全屏拉伸"**（四个角的那个）
- **Offset Left/Top/Right/Bottom**：全部填 **0**
- 展开 **Appearance → Brush**：
  - **Draw As**：选 `Box`
  - **Tint（颜色）**：点击颜色块，R=0.1, G=0.1, B=0.1, A=**0.9**（深灰半透明）

---

### 3.5 搭建两栏布局

在左边 Palette 搜索 **`Horizontal Box`**，拖到 Border 里面。

选中 Horizontal Box，在 Details：
- **Anchors**：选中间居中的那个
- **Position X**：-450，**Position Y**：-300
- **Size X**：900，**Size Y**：600
- `Is Variable`：打勾

---

### 3.6 左侧：符文待放置列表区

在 Horizontal Box 里放一个 **`Vertical Box`**（Palette 搜索），命名时直接在左边 Hierarchy 双击改名为 `LeftPanel`。

选中这个 Vertical Box，Details 里：
- **Size → Fill** 改为 **自动/Fixed**，Size X：**260**

**在 LeftPanel（Vertical Box）里，依次放以下控件：**

**① 标题文字**

拖入 **`Text`**，选中后 Details：
- **Content → Text**：输入 `待放置符文`
- **Appearance → Font → Size**：16
- **Justification**：Center（居中）

**② 符文列表滚动区**

拖入 **`Scroll Box`**，Details：
- **Size → Fill**：拉伸填满剩余高度（选 Fill 模式）
- 勾选 **Is Variable**，变量名改为 **`RuneListBox`**

**③ 描述区**

拖入 **`Border`**，Details：
- Size Y：**100**
- Brush Tint：R=0.15, G=0.15, B=0.15, A=1

在这个 Border 里放一个 **Vertical Box**，再在里面放两个 **Text**：
- 第一个 Text：Is Variable = ✅，变量名 **`InfoName`**，Font Size 14，Bold
- 第二个 Text：Is Variable = ✅，变量名 **`InfoDesc`**，Font Size 11，Auto Wrap Text = ✅

---

### 3.7 右侧：5×5 背包网格区

在最外层 Horizontal Box 里，再放一个 **`Vertical Box`**，命名 `RightPanel`：
- Size X：**600**

**在 RightPanel 里，依次放：**

**① 标题文字**

拖入 **`Text`**：
- Content：`背包`
- Font Size：16，居中

**② 网格 Panel**

拖入 **`Uniform Grid Panel`**，Details：
- 勾选 **Is Variable**，变量名 **`BackpackGrid`**
- Slot Padding：5（每格间距）
- Size Y：480（高度给够放 5 行）

> ⚠️ 先**不要在这里放 Button**，等后面在蓝图里用代码生成。

**③ 底部操作按钮**

拖入 **`Horizontal Box`**，在里面放两个 **Button**：

第一个 Button：
- 里面放 Text，内容：`移除选中`
- 勾选 Is Variable，变量名：`BtnRemove`

第二个 Button：
- 里面放 Text，内容：`取消选中`
- 勾选 Is Variable，变量名：`BtnClear`

**④ 状态提示文字**

在 RightPanel 底部放一个 **`Text`**：
- 勾选 Is Variable，变量名：**`StatusText`**
- Font Size：12，居中，默认内容留空

---

### 3.8 在蓝图里生成 25 个格子

> 现在切到 **Event Graph** 视图（点击右上角 Graph 按钮）。

#### 新建变量：格子按钮数组

点击左边 **My Blueprint** 面板下方的 **`+ Variable`** 按钮：
- 变量名：**`GridButtons`**
- 类型：点击右边类型选择器，搜索 `Button`，选 **`Button`**，然后点旁边方块图标改为 **Array（数组）**

再新建一个变量：
- 变量名：**`GridImages`**
- 类型：`Image`，Array

再新建一个变量：
- 变量名：**`GridTexts`**
- 类型：`Text`，Array

---

#### 找到 Event Construct 节点

在 Event Graph 空白处右键 → 搜索 **`Event Construct`** → 点击添加。

（如果已经存在就直接用）

---

#### 连接逻辑：Event Construct 生成 25 个格子

从 **Event Construct** 的白色箭头（`►`）往后连，按以下步骤添加节点：

**第1步：清空数组**

右键 → 搜索 **`Clear`** → 选 `Clear(GridButtons)` → 连线

再右键 → Clear(GridImages) → 连线

再右键 → Clear(GridTexts) → 连线

**第2步：双层 For 循环（Col 0~4，Row 0~4）**

右键 → 搜索 **`For Loop`** → 添加

- `First Index`：0
- `Last Index`：4

把 `Loop Body`（循环体输出）→ 再接一个 **For Loop**（内层）：
- `First Index`：0
- `Last Index`：4

**第3步：在内层循环体里创建 Button**

从内层 For Loop 的 `Loop Body` 往后连：

右键 → 搜索 **`Create Widget`** → 选 `Create Widget`
- `Class`：选 **`Button`**（注意不是 Button 的子类，就是基础 Button）

> 实际上更快的方法：直接用 `Create Widget` 创建一个自定义的格子 Widget。但最简单的方式是用 `Add Child to Uniform Grid Panel`。

---

**⭐ 最快的实现方法（推荐）：**

在内层循环体里：

1. 右键 → 搜索 **`Add Child to Uniform Grid Panel`**
   - `Target`：连接 `BackpackGrid`（拖出你的变量）
   - `Column`：连 **外层 For Loop** 的 `Index`（即 Col）
   - `Row`：连 **内层 For Loop** 的 `Index`（即 Row）
   - `Content`：需要先创建一个 Widget

2. 在 `Add Child` 之前，先 **Create Widget（Button）**：
   - 连 `Add Child` 的 `Content` 输入引脚

3. 把创建出来的 Button 引用：
   - 从 `Create Widget` 的返回值 → 右键 → **`Add（GridButtons）`**（添加到数组）

4. 设置 Button 的 **OnClicked** 事件（最难的一步）：

   从 Button 引用拖出 → 搜索 **`Bind Event to On Clicked`**
   
   `Event` 引脚 → 右键 → **Create Event** → 选 **Create a matching function**
   
   然后在这个函数里调用 **`ClickCell`**（C++ 暴露的函数）：
   - `Col`：传入外层循环的 Index（需要用 Promote to Local Variable 或者直接连）
   - `Row`：传入内层循环的 Index

> ⚠️ **传 Col/Row 的注意事项**：For Loop 的 Index 是循环内部的值，在 Bind Event 的回调函数里**拿不到外部的 Index**。
> 
> **解决方法**：在内层循环体里，用 `Col * 5 + Row` 计算一个 `CellIndex`（整数），把这个整数存到 Button 的 Tag 或者用另一种方法。
> 
> **最简单的替代方案**：不用循环，直接手动放 25 个 Button（见 3.9 备选方案）。

---

### 3.9 备选方案：手动放 25 个格子（如果循环太复杂）

> 如果上面的循环方案太麻烦，用这个方法：速度慢但最直观。

回到 **Designer** 视图，在 `BackpackGrid`（Uniform Grid Panel）里：

手动拖入 25 个 **Button**，每个设置：
- Column：0~4（5列）
- Row：0~4（5行）
- Size：90×90
- Is Variable：✅
- 变量名：`Btn_0_0`，`Btn_0_1`，...，`Btn_4_4`（Col_Row格式）

每个 Button 里放一个 **Vertical Box**，里面放：
- **Image**（70×70）：Is Variable ✅，变量名 `Img_0_0` 等
- **Text**（字号 8）：Is Variable ✅，变量名 `Txt_0_0` 等

然后在 Event Graph 的 Event Construct 里，手动 Add 到 GridButtons 数组：

```
Event Construct
→ Clear (GridButtons)
→ Add Btn_0_0 → Add Btn_1_0 → Add Btn_2_0 → Add Btn_3_0 → Add Btn_4_0
→ Add Btn_0_1 → ...（以 Col*5+Row 的顺序，共 25 个）
```

> ⚠️ 顺序必须对：第 0 个对应 Col=0,Row=0；第 1 个对应 Col=1,Row=0；以此类推。
> 
> 顺序公式：Index = Col × 5 + Row

然后为每个 Button 的 OnClicked 绑定 ClickCell：

选中 `Btn_0_0` → 在 Details 找到 **Events → OnClicked → 点击 `+` 按钮**

→ 自动跳到 Event Graph，出现 `On Clicked (Btn_0_0)` 节点

→ 从白色箭头连出 → 右键搜索 **`ClickCell`** → 填入 Col=0, Row=0

重复 25 次（每个 Button 不同的 Col 和 Row 值）。

---

### 3.10 实现 OnGridNeedsRefresh

在 **My Blueprint** 面板找到 **Functions → 展开 Override → 找到 `OnGridNeedsRefresh`**

双击进入这个函数的编辑界面（或者在 Graph 里右键搜索 `OnGridNeedsRefresh` 添加）。

这个函数需要遍历 25 个格子，根据状态设置颜色。以下是最简化的版本：

**节点连接顺序：**

```
OnGridNeedsRefresh
→ For Each Loop（遍历 GridButtons 数组）
    Array Element → Set Brush Color（根据状态）
    Array Index   → Col = Index / 5（整数除法）
                    Row = Index % 5（取余）
    → GetCellVisualState(Col, Row) → Switch on EBackpackCellState
        Empty            → Set Color #3A3A3A
        EmptyActive      → Set Color #1A3A6A
        OccupiedActive   → Set Color #2266CC
        OccupiedInactive → Set Color #7A4A1A
    → IsCellSelected(Col, Row) → True → Set Color #FFAA00
    → IsCellOccupied(Col, Row) → True:
        GetRuneAtCell(Col, Row)
        → 拿到 RuneConfig.RuneName → 设置 GridTexts[Index].SetText
        → 拿到 RuneConfig.RuneIcon → 设置 GridImages[Index].SetBrushFromTexture
```

**颜色设置方法：**

选中 Button → 搜索 **`Set Style`** 或 **`Set Color And Opacity`**，或者用 **`Set Background Color`**（在 Button 节点上拖出引脚搜索）。

---

### 3.11 实现 OnSelectionChanged

在 My Blueprint → Override → 找 **`OnSelectionChanged`**，进入编辑：

```
OnSelectionChanged
→ 调用 OnGridNeedsRefresh（刷新格子颜色）
→ HasSelectedRune() → Branch（True/False）
    True  → GetSelectedRuneInfo()
             → InfoName.SetText = RuneConfig.RuneName
             → InfoDesc.SetText = RuneConfig.RuneDescription
    False → InfoName.SetText = ""
            InfoDesc.SetText = ""
```

---

### 3.12 实现 OnRuneListChanged

在 Override 里找 **`OnRuneListChanged`**，进入编辑：

```
OnRuneListChanged
→ 调用自定义函数 RefreshRuneList
```

然后新建一个 **Function** 叫 `RefreshRuneList`：

```
RefreshRuneList
→ RuneListBox → Remove All Children（清空列表）
→ GetRuneList() → For Each Loop
    Array Element = RuneInstance（符文信息）
    Array Index   = ListIndex
    → Create Widget（Button）
    → 设置 Button 文字 = RuneInstance.RuneConfig.RuneName
    → 如果 ListIndex < GetPendingRuneCount()：文字前加 "★ "
    → Bind OnClicked → SelectRuneFromList(ListIndex)
    → RuneListBox → Add Child（把 Button 加进去）
```

在 Event Construct 末尾加上：`调用 RefreshRuneList`

---

### 3.13 实现 OnStatusMessage

在 Override 里找 **`OnStatusMessage`**，进入编辑：

```
OnStatusMessage (Message)
→ StatusText → Set Text (Message)
→ Delay(2.0)
→ StatusText → Set Text ("")
```

---

### 3.14 绑定移除和取消按钮

在 Designer 视图，选中 `BtnRemove`（移除选中按钮）：

- Details → Events → **OnClicked** → 点 `+`
- 在弹出的 Graph 里：连接 **`RemoveRuneAtSelectedCell`**（直接搜索这个函数名）

选中 `BtnClear`（取消选中按钮）：

- Details → Events → **OnClicked** → 点 `+`
- 连接 **`ClearSelection`**

---

## 第四步：绑定 Tab 键打开背包

### 4.1 添加输入映射

打开 **Edit → Project Settings → Engine → Input**

在 **Action Mappings** 下点 **`+`** 添加：
- 名称：`OpenBackpack`
- 按键：`Tab`（在下拉里搜索 Tab）

点右上角 **Save** 保存。

### 4.2 在 PlayerController 蓝图里创建和控制背包 UI

找到并打开 `Content → Code → Core → Controller → B_YogPlayerControllerBase`（蓝图文件）。

#### 新建变量

在 My Blueprint → Variables → `+`：
- 变量名：**`BackpackWidget`**
- 类型：`WBP_BackpackScreen`（搜索你刚创建的那个），设为 **Object Reference**

#### Event BeginPlay

找到（或创建）**Event BeginPlay** 节点，在末尾加上：

```
Event BeginPlay（已有的，在末尾接上）
→ Create Widget
    Class：WBP_BackpackScreen
    Owning Player：Get Player Controller (Index 0)
→ 把返回值 Promote to Variable（右键 → Promote to Variable）变成 BackpackWidget
→ Add to Viewport
→ Set Visibility
    Target：BackpackWidget
    Visibility：Hidden
```

#### InputAction OpenBackpack

在 Event Graph 空白处右键 → 搜索 **`OpenBackpack`** → 选 **`InputAction OpenBackpack`**

从 **`Pressed`** 引脚连出：

```
Pressed
→ BackpackWidget → Is Visible?（Get Visibility 后比较，或搜索 Is Visible）
→ Branch
    True（当前可见，关闭）：
      BackpackWidget → Set Visibility(Hidden)
      Set Input Mode: Game Only（右键搜索 Set Input Mode Game Only）
    False（当前隐藏，打开）：
      BackpackWidget → Set Visibility(Visible)
      Set Input Mode: Game And UI
      Set Keyboard Focus: BackpackWidget
```

按 **Ctrl+S** 保存。

---

## 第五步：在场景放置拾取物

### 5.1 打开测试关卡

在 `Content Browser` 找到你的展示关卡文件（`*.umap`），双击打开。

### 5.2 放置 RewardPickup

1. 在 `Content Browser` 搜索 **`BP_RewardPickup`**
2. 找到它（蓝色 Actor 图标）
3. 直接**拖入场景**，放在玩家出生点附近
4. **重复拖入 3 次**，共放 3 个，彼此间隔一段距离

选中每个 BP_RewardPickup，在 Details 面板不需要配置任何东西，直接保存即可。

按 **Ctrl+S** 保存关卡。

---

## 最终验证（展示前必做）

按以下顺序测试：

### 验证1：符文池是否配置正确

1. 按编辑器工具栏 **Play** 按钮进入 PIE（在编辑器里游玩）
2. 打开 **Output Log**（菜单 Window → Output Log）
3. 走近场景中的任意一个 RewardPickup，按 **E**
4. 在 Output Log 里应该能看到：
   ```
   LogTemp: GenerateLootOptions: 生成 3 个符文选项
   ```
   如果看到 `无可用符文池` → 回到第二步重新配置 FallbackLootPool

### 验证2：三选一界面是否弹出

按 E 后，屏幕上应该弹出三选一 UI，显示 3 个符文名称。

如果没弹出 → 检查 WBP_LootSelection 是否已 Add to Viewport（在 PlayerController 的 BeginPlay 里）。

### 验证3：选符文后是否入背包

点击其中一个符文，在 Output Log 应该看到：
```
LogTemp: AddRuneToInventory: [符文名] 自动放置到 (0,0)
```

### 验证4：背包界面是否可以打开

按 **Tab** 键，屏幕上应该出现你制作的背包界面。

如果没出现 → 检查 PlayerController 的 BeginPlay 里是否创建了 WBP_BackpackScreen 并 Add to Viewport。

### 验证5：格子颜色是否正确

背包打开后：
- 中间某些格子应该是 **深蓝色**（激活区）
- 刚才放入的符文应该在 **左上角某个格子**，显示为 **橙色**（放置了但不在激活区）或 **亮蓝色**（在激活区且已激活）

---

## 遇到问题怎么办

| 问题 | 检查项 |
|---|---|
| 创建 Widget 时找不到 BackpackScreenWidget | 确认代码已编译（编辑器底部没有 Compile 错误） |
| ClickCell 函数找不到 | 父类是否选了 BackpackScreenWidget |
| 按 E 后 Output Log 无输出 | BP_RewardPickup 是否放到了场景里 |
| 背包 UI 打开但格子全灰 | OnGridNeedsRefresh 是否在 Event Construct 里调用了 |
| 选符文后没有反应 | SelectRuneFromList 的 Index 参数是否正确传入 |
| 符文放入格子后描述没刷新 | OnSelectionChanged 是否调用了 InfoName.SetText |

---

## 参考颜色速查

| 格子状态 | 颜色（十六进制） | R/G/B（0~1） |
|---|---|---|
| 空格子 | `#3A3A3A` | R=0.23, G=0.23, B=0.23 |
| 激活区空格 | `#1A3A6A` | R=0.10, G=0.23, B=0.42 |
| 激活中符文 | `#2266CC` | R=0.13, G=0.40, B=0.80 |
| 未激活符文 | `#7A4A1A` | R=0.48, G=0.29, B=0.10 |
| 选中高亮 | `#FFAA00` | R=1.0, G=0.67, B=0.0 |
