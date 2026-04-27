# WBP_LootSelection 布局规格 v3

> 战利品多选一 UI（动态卡牌 N=1~6 + 跳过 + 背包预览 + 自动换行）。
> 父类：`ULootSelectionWidget`（[Source/DevKit/Public/UI/LootSelectionWidget.h](../../../Source/DevKit/Public/UI/LootSelectionWidget.h)）
>
> **v3 关键变化（2026-04-27）：** `CardContainer` 类型从 `UHorizontalBox` → `UWrapBox`；`MaxCards` 5→6；卡牌尺寸按有效卡数动态切换（≤4 用 320×420，≥5 用 240×360 强制 3 列换行）。

---

## 类信息

| 项目 | 内容 |
|------|------|
| WBP 名 | `WBP_LootSelection` |
| 路径 | `Content/UI/Playtest_UI/WBP_LootSelection.uasset` |
| 父类（C++） | `ULootSelectionWidget`（继承 `UCommonActivatableWidget`） |
| 加入方式 | `AYogHUD::QueueLootSelection` 内 `CreateWidget + AddToViewport(Z=15)`（按需创建/复用） |
| 配置入口 | `BP_YogHUD` → Details → `Loot Selection Widget Class = WBP_LootSelection` |
| 触发 | `ARewardPickup::TryPickup` → `HUD::QueueLootSelection(Options, this)`；HUD 内部决定立即弹/排队 |
| 卡片来源 | C++ `RebuildCards(Options)` 用 `RuneCardClass` 动态创建 N≤6 张实例 AddChild 到 `CardContainer` |

---

## 控件层级（树形）

```
CanvasPanel（根，Full Screen）
└── Overlay（RootOverlay，Full Screen）
    ├── Image（DimBG，Fill，#000000 α=0.5，HitTestInvisible）
    └── VerticalBox（CenterColumn，HCenter VCenter）
        ├── TextBlock（TitleText [可选 Variable]，"选择战利品"，字号24 Bold #ECECEC，居中，PadB=24)
        ├── WrapBox（CardContainer [BindWidget]，HCenter VCenter，PadB=24)  ⭐ v3 改为 WrapBox
        │   └── (运行时由 C++ RebuildCards 动态填充 N≤6 张 WBP_RuneInfoCard，外包 SizeBox+UButton)
        └── HorizontalBox（BottomBar，HCenter VCenter，PadT=8)
            ├── Button（BtnSkip [BindWidget]，Auto，PadR=16)
            │   └── YogCommonRichTextBlock（按钮文字，"按 <input action='Esc'/> 跳过")
            └── Button（BtnBackpackPreview [BindWidget]，Auto)
                └── YogCommonRichTextBlock（按钮文字，"按 <input action='OpenBackback'/> 预览背包")
```

> **重要**：`CardContainer` **必须是 `UWrapBox` 类型**（不可用 HorizontalBox / VerticalBox）—— C++ `BindWidget` 严格类型匹配，且 Slot cast 是 `UWrapBoxSlot`。
>
> **从 v2 升级注意**：原 WBP 里的 HorizontalBox 必须**先在 BP Event Graph 删除所有引用**，再在 Designer 删除控件，重新添加 WrapBox 控件并命名 `CardContainer`，否则 BindWidget 类型校验会报错。

---

## 各控件详细属性

### CanvasPanel（根）

| 属性 | 值 |
|---|---|
| Anchors | Min(0,0) Max(1,1)（全屏） |
| Position / Size | 0/0 / 撑开 |

### Overlay（RootOverlay）

#### Slot（Canvas Slot）
| 属性 | 值 |
|---|---|
| Anchors | Min(0,0) Max(1,1) |
| Position X/Y | 0 / 0 |
| Size | 撑开 |

### Image（DimBG）

#### Slot（Overlay Slot）
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Fill / Fill |

#### 自身
| 属性 | 值 |
|---|---|
| Brush → Tint | #000000 α=0.5（线性 (0, 0, 0, 0.5)） |
| Brush → Draw As | Image |
| Visibility | HitTestInvisible（不挡按钮点击） |

### VerticalBox（CenterColumn）

#### Slot（Overlay Slot）
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Center / Center |

子控件 Slot 全部 HAlign=Center。

### TextBlock（TitleText）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign | Center |
| Padding B | 24 |

#### 自身
| 属性 | 值 |
|---|---|
| Text | `选择战利品` |
| Font Size | 24 |
| Typeface | Bold |
| Color and Opacity | #ECECEC（线性 (0.93, 0.93, 0.93, 1.0)） |
| Justification | Center |

### WrapBox（CardContainer）⭐ BindWidget ⭐ v3 类型变更

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign | Center |
| VAlign | Center |
| Padding B | 24 |

#### 自身
| 属性 | 值 |
|---|---|
| Orientation | Horizontal（横向排列，超过 WrapSize 时换行） |
| Inner Slot Padding | (0, 0)（C++ 用 slot.SetPadding(16) 自管间距，避免双重累计） |
| Horizontal Alignment | Center（C++ RebuildCards 也会强制设此值，让换行后整行居中） |
| Wrap Size | 9999（默认大值；C++ 在 ValidCount≥5 时改为 820 强制换行） |
| Use Explicit Wrap Size | false（C++ 在 ValidCount≥5 时切 true）|

#### 自身行为
- 子节点由 C++ `RebuildCards` 动态填充 — Designer 不要手动加任何子节点
- 单卡间距由 C++ 设置（`UWrapBoxSlot::SetPadding(FMargin(16))` 四边各 16px）
- C++ 在每次 ShowLootUI 时根据**有效卡数**动态决定：
  - ValidCount ≤ 4：CardSize=320×420（LargeCardSize），不强制换行 → 单行展示
  - ValidCount ≥ 5：CardSize=240×360（SmallCardSize），WrapSize=820 → 强制 3 列换行（5 卡 = 3+2，6 卡 = 3+3）

> **关键**：上面 5 项 WBP 设置只是兜底默认；C++ RebuildCards 会按需重新覆盖 SetHorizontalAlignment / SetExplicitWrapSize / SetWrapSize 三项。WBP 端**不要**强制设固定 WrapSize 或 ExplicitWrapSize=true，会与 C++ 的动态切换冲突。

### HorizontalBox（BottomBar）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign | Center |
| Padding T | 8 |

### Button（BtnSkip）⭐ BindWidget

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| Padding R | 16 |
| VAlign | Center |

#### 自身
| 属性 | 值 |
|---|---|
| Style → Normal → Tint | #3A3A4A α=0.85（线性 (0.23, 0.23, 0.29, 0.85)） |
| Style → Hovered → Tint | #5A5A6A α=0.95（线性 (0.35, 0.35, 0.41, 0.95)） |
| Style → Pressed → Tint | #2A2A3A α=1（线性 (0.13, 0.13, 0.18, 1.0)） |
| Style → Normal Padding | L=24 T=10 R=24 B=10 |
| Is Focusable | true |
| OnClicked | C++ 已绑（无需 BP 拖线）— `ULootSelectionWidget::OnBtnSkipClicked` |

### YogCommonRichTextBlock（BtnSkip 内的子文字）

| 属性 | 值 |
|---|---|
| Text | `按 <input action="Esc"/> 跳过` |
| Decorator Classes [0] | `BP_InputActionDecorator` |
| Yog Style Override → Font Style Class | `BP_InfoPopupTextStyle` |
| Yog Style Override → Override Font Size | `15` |
| Yog Style Override → Override Color | `(R=0.93, G=0.93, B=0.93, A=1.0)` |
| Justification | Center |
| Override Default Style（引擎自带）| ❌ 不勾 |

### Button（BtnBackpackPreview）⭐ BindWidget

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| VAlign | Center |

#### 自身
- Style 同 BtnSkip
- OnClicked 由 C++ 自动绑定 → `ULootSelectionWidget::OnBtnBackpackPreviewClicked`

### YogCommonRichTextBlock（BtnBackpackPreview 内的子文字）

| 属性 | 值 |
|---|---|
| Text | `按 <input action="OpenBackback"/> 预览背包` |
| Decorator Classes [0] | `BP_InputActionDecorator` |
| Yog Style Override → Font Style Class | `BP_InfoPopupTextStyle` |
| Yog Style Override → Override Font Size | `15` |
| Yog Style Override → Override Color | `(R=0.93, G=0.93, B=0.93, A=1.0)` |
| Justification | Center |

---

## BindWidget 汇总

### 必须（BindWidget）— 缺失会导致编辑器编译报错
| C++ 变量名 | 控件类型 | WBP 控件名 | 说明 |
|---|---|---|---|
| `CardContainer` | **UWrapBox** ⭐ v3 改 | `CardContainer` | C++ `RebuildCards` 调 `ClearChildren` + `AddChild` 动态填充；slot 类型为 `UWrapBoxSlot` |
| `BtnSkip` | UButton | `BtnSkip` | C++ 自动绑 OnClicked → `SkipSelection()` |
| `BtnBackpackPreview` | UButton | `BtnBackpackPreview` | C++ 自动绑 OnClicked → `OpenBackpackPreview()` |

> 命名一字不差，区分大小写

---

## Class Defaults 配置

| 字段 | 值 | 必须 |
|---|---|---|
| **`RuneCardClass`** | `WBP_RuneInfoCard` | ✅ 必填，否则 `RebuildCards` 跳过创建卡片 |

---

## 颜色速查

| 元素 | Hex | Linear RGBA |
|---|---|---|
| 暗背景 DimBG | #000000 α=0.5 | (0, 0, 0, 0.5) |
| 标题文字 | #ECECEC | (0.93, 0.93, 0.93, 1.0) |
| 按钮 Normal | #3A3A4A α=0.85 | (0.23, 0.23, 0.29, 0.85) |
| 按钮 Hovered | #5A5A6A α=0.95 | (0.35, 0.35, 0.41, 0.95) |
| 按钮 Pressed | #2A2A3A α=1.0 | (0.13, 0.13, 0.18, 1.0) |
| 高亮边框（BP 中实现）| #FFD966 | (1.0, 0.85, 0.4, 1.0) |

---

## Event Graph 实现

**理论上可完全清空**（C++ 已自管 RebuildCards / SelectRuneLoot / SkipSelection / OpenBackpackPreview / 段焦点）。

如果想要**视觉高亮反馈**，建议实现以下 BP 事件（非必须）：

### `OnCardFocused(int32 FocusedIndex)`

```
OnCardFocused (FocusedIndex)
    │
    ├─ 取 SpawnedCards 数组（C++ private — 实际无法直接访问）
    │
    └─ 推荐替代：在 BP 里维护一个本地数组 LastSpawnedCards，
       通过 OnLootOptionsReady 时同步获取（用 GetChildAt 遍历 CardContainer）
       然后给 FocusedIndex 那张缩放 1.05 + 加亮边框
```

> 实操更简单的做法：在 `WBP_RuneInfoCard` 内部加一个高亮 Border 子控件，由聚焦时通过 BP 子 Widget 接口控制

### `OnSectionFocused(NewSection, IndexInSection)`

```
OnSectionFocused (NewSection, IndexInSection)
    │
    ├─ Switch on NewSection:
    │   ├─ Cards: BtnSkip/BtnBackpackPreview 取消高亮
    │   └─ Buttons: 卡片群取消高亮，对应按钮（IndexInSection 0=Skip 1=Preview）加边框
```

### `OnLevelPhaseChanged(NewPhase)`

```
OnLevelPhaseChanged (NewPhase)
    │
    └─ Switch on NewPhase:
        ├─ Combat / Transitioning: SetVisibility (Collapsed)
        └─ Arrangement: 不动（C++ 决定显示）
```

---

## 注意事项

### 类型严格匹配

`CardContainer` **必须是 `UWrapBox`**（v3 改），`BtnSkip` / `BtnBackpackPreview` **必须是 `UButton`**（不是 `UCommonButtonBase` 等子类）。Common UI 的派生 Button 会因 C++ BindWidget 类型校验失败。

### 删除旧控件前先清 Event Graph

如果原 WBP 有 `OptionCard0/1/2`、`BtnConfirm` 这些旧控件，**先在 BP Event Graph 里删除所有引用**再在 Designer 里删控件，否则 BP 编译报错。

### 卡片间距 / 排版微调

- 单卡 Padding 由 C++ 控制（默认四边各 16px，UWrapBoxSlot），改要去 [LootSelectionWidget.cpp](../../../Source/DevKit/Private/UI/LootSelectionWidget.cpp) `RebuildCards` 函数
- 卡片尺寸由 C++ 强制 SizeBox.SetWidthOverride/SetHeightOverride 设定，按 ValidCount ≤4/≥5 切换 LargeCardSize / SmallCardSize（详见 LootSelectionWidget.h Loot|Layout 字段）
- 强制换行的 WrapSize（820）也是 EditAnywhere 字段 `WrapBoxWrapWidthFor3Plus`，可在蓝图 Class Defaults 调

### 按键标签的 InputAction 命名

`<input action="Esc"/>` 中的 "Esc" 等 key 对应 `BP_InputActionDecorator` 的 `ActionMap` 配置。如果项目里 InputAction 资产名是 `IA_Esc`，标签写 `Esc`（Decoder 自动加 IA_ 前缀）。可用的标签查 `Content/Code/Core/Input/Actions/IA_*.uasset`：

| 可用标签 | 对应 InputAction |
|---|---|
| `Esc` | IA_Esc |
| `OpenBackback` | IA_OpenBackback（注意是 Backback 不是 Backpack — 项目历史拼写）|
| `Interact` | IA_Interact |
| `Dash` | IA_Dash |
| `LightAttack` | IA_LightAttack |
| `HeavyAttack` | IA_HeavyAttack |
| `Move` | IA_Move |
| `L1` / `R1` | IA_L1 / IA_R1 |

### Visibility 初始值

WBP 根 Visibility 默认 `Visible` 即可，C++ `ShowLootUI` 调用时会切到 `SelfHitTestInvisible`，关闭时切到 `Collapsed`。

### 不需要做的 BP 事件

以下 BP 事件 **不要实现**（C++ 已全部接管）：
- `OnLootOptionsReady` — C++ RebuildCards 已自动处理
- `SelectRuneLoot` / `SkipSelection` / `OpenBackpackPreview` 的实现 — C++ 已绑定到 Button OnClicked
- `BtnConfirm` 的处理 — 此控件已删除，不存在

---

## 验证流程

### 1. 基础显示

1. PIE 启动，杀完所有敌人 → 关卡结束触发 RewardPickup spawn
2. 走近 RewardPickup，按 E
3. ✅ **WBP_LootSelection 弹出**：
   - DimBG 半透明覆盖
   - 标题"选择战利品"居中
   - CardContainer 显示动态生成的 N 张 WBP_RuneInfoCard
   - 底部 BtnSkip + BtnBackpackPreview 按钮

### 2. 卡片数量边界（v3 更新）

- 配 1 个 LootOption → ✅ 显示 1 张大卡（320×420）
- 配 3 个 LootOption → ✅ 显示 3 张大卡单行
- 配 4 个 LootOption → ✅ 显示 4 张大卡单行
- 配 5 个 LootOption → ✅ 显示 5 张小卡（240×360）= 3+2 双行（第二行居中）
- 配 6 个 LootOption → ✅ 显示 6 张小卡 = 3+3 双行
- 配 7 个 LootOption → ✅ 显示 6 张小卡 + Output Log 警告"截断到 6"
- 配 6 个但其中 3 个 LootType≠Rune → ✅ 显示 3 张大卡（按 ValidCount=3 算）

### 3. 选符文路径

- 鼠标点中间那张卡 → ✅ UI 关 → ✅ pickup actor 销毁 → ✅ 背包自动打开（整理模式，可拖拽）

### 4. 跳过路径

- 点 BtnSkip / 按 B / 按 Esc → ✅ UI 关 → ✅ pickup actor **保留在场景**
- 走开 RewardPickup 范围（如有 Box overlap）→ 走回 → 按 E → ✅ **重新弹出 WBP_LootSelection**

### 5. 背包预览路径

- 点 BtnBackpackPreview / 按 Y / 按 Tab → ✅ LootSelection 隐藏 → ✅ 背包打开
- 在背包里尝试拖拽符文 → ✅ **拖不动**（只读模式）
- 关闭背包 → ✅ **自动回到 LootSelection**（焦点 / 暂停状态恢复）

### 6. 通用效果子窗

- 在某符文 DA 的 `RuneConfig.GenericEffects` 数组添加 `DA_RGE_Knockback`
- 触发 LootSelection
- 用 DPad / 鼠标移动到该卡 → ✅ **卡片右侧出现"击退"小窗**
- 切到别的卡 → ✅ 上一张的小窗收起

### 7. 段焦点切换（手柄）

- DPad Up → ✅ 焦点切到卡片段
- DPad Down → ✅ 焦点切到按钮段
- 按钮段下 DPad Left/Right → ✅ Skip ↔ BackpackPreview 切换
- 按 A / Enter → ✅ 触发当前段的当前选项

### 7b. 鼠标 hover 高亮（v3 新增）

- 当前段是 Buttons → 鼠标移到任意一张卡 → ✅ 自动切到 Cards 段，该卡显示选中态（描边 + 缩放 + GenericEffectList 展开）
- Cards 段下鼠标在卡间移动 → ✅ 高亮跟随，旧卡同步取消选中
- 鼠标移开所有卡（移到屏外或移到按钮）→ ✅ 最后 hover 的卡保持高亮（不复位 — 避免视觉闪烁）
- 鼠标点击 hover 着的卡 → ✅ 直接选中该卡（与现有 OnCardClicked 路径一致）

### 8. 多 pickup 排队

- 同时刷 2 个 RewardPickup 在玩家附近
- 按 E 触发第一个 → ✅ 第一个 LootSelection 弹出
- 跳过 → ✅ 第一个关闭，**第二个 LootSelection 立即弹出**（队列 FIFO）

---

## 排错速查

| 现象 | 原因 | 解决 |
|---|---|---|
| 编辑器编译报错 "BindWidget CardContainer not found" / "type mismatch" | WBP 没命名 / 名错 / 类型错 | 命名 `CardContainer`，类型必须 **UWrapBox**（v3 改）；从 v2 升级时先删旧 HBox 再建 WrapBox |
| PIE 弹 LootSelection 但没卡片 | RuneCardClass 没配 | Class Defaults 选 WBP_RuneInfoCard |
| 卡片重叠 / 跑屏外 | RuneInfoCard 自定义尺寸覆盖了 SizeBox | C++ 已用 SizeBox.SetWidthOverride 强制；如还是重叠检查 ShowRune 是否被 BP 子类覆盖 |
| 5 张卡第二行没居中 | WrapBox 自身 HorizontalAlignment 没设 Center | C++ RebuildCards 已强制设；如果手动改了 BP 端覆盖，删除 BP 端设置 |
| 4 张卡却被缩成小卡 | 误把 LootOptions 配了 5+ 项但部分无效 | 看 Output Log "ValidCount=N"，对照 GameMode 生成逻辑 |
| 跳过后按 E 不再触发 | 老 RewardPickup 失效 / `bPickedUp` 没复位 | 看 `[RewardPickup] ResetForSkip` 日志，确认 `Player->PendingPickup = this` 重新挂上 |
| 选完没自动开背包 | HUD::OpenBackpack 没配 BackpackScreenClass | BP_YogHUD → BackpackScreenClass = WBP_BackpackScreen |
| 背包预览能拖拽 | SetPreviewMode 没生效 | 看 `[Backpack] SetPreviewMode(true)` 日志；确认 BackpackScreen 拖拽入口已加 bIsPreviewMode 守卫 |
| 切到下一关后按键图标变错 | InputAction 资产引用丢失 | 重新打开 BP_InputActionDecorator → ActionMap 重新指 IA_* 资产 |
| 焦点丢失 / 输入卡死 | 背包关闭后 LootSelection.ReactivateAfterPreview 没触发 | 看 `[LootSelection] ReactivateAfterPreview` 日志，确认 BackpackWidget OnDeactivated 多播触发 |

---

## 关联文档

- [LootSelection C++ 接口](../../../Source/DevKit/Public/UI/LootSelectionWidget.h)
- [当前任务方案 v2](../../WorkSession/current_plan.md) — 含设计决策、Codex 4 轮审查迭代
- [WBP 布局规格标准](WBP_LayoutSpec_Standard.md)
- [WBP_RuneInfoCard](../../../Content/UI/Playtest_UI/Runes/WBP_RuneInfoCard.uasset) — 单张卡 WBP
- [WBP_GenericEffectList](../../../Content/UI/Playtest_UI/Runes/WBP_GenericEffectList.uasset) — 通用效果子窗
- [PendingDecisions DEC-001](../../PM/PendingDecisions.md) — 通用效果默认展开（待评审）
