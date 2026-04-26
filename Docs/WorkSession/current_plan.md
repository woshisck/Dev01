# 开发方案：WBP_LootSelection 改造（动态卡牌 + 跳过 + 背包预览 + 通用效果聚焦）

> v2 — 整合 Codex 审查反馈 + 用户决策（2026-04-26）

## 需求描述

将 `WBP_LootSelection` 进行改造，使用 `RuneInfoCard` 作为选择的卡牌：

1. **支持手柄**（含 LB/RB / 摇杆 / DPad / A / B / Y）+ 鼠标点击
2. **跳过选项**：跳过仅关 UI，关卡中 `RewardPickup` 保留且**可再次按 E 重开**；选某张符文才销毁 pickup
3. **聚焦时显示通用效果浮窗**：当前选中的卡若引用了通用效果（击退/流血等），其右侧出现解释小窗；切到别的卡时上一张收起
4. **动态卡牌数量**（上限 5）：N=1~5 都能正确显示，超过 5 由生成逻辑限制
5. **背包预览选项**：按某键打开背包 UI（**只读模式**），关闭后自动恢复 LootSelection
6. **选完符文 → 自动打开背包**（**整理模式**，沿用现有行为）
7. **重掷接口（本版本不实现）**：留出 `RerollCard(int32)` 公开接口，后续做 UI 动画

---

## 用户决策（2026-04-26）

| # | 问题 | 决策 |
|---|---|---|
| 1 | 选完符文后是否自动打开背包 | ✅ 是（保留现有 OpenBackpack 行为，整理模式） |
| 2 | 跳过后能否再按 E 重开同一 pickup | ✅ 是（新增 `RewardPickup::ResetForSkip`） |
| 2.5 | 重掷功能 | 🔧 留接口 `RerollCard(idx)`，本版本不实现 |
| 3 | N 是否任意？ | ❌ 上限 5，由 GameMode 生成逻辑保证 |
| 4 | 背包预览是只读还是可整理 | ✅ 只读；选符文后开的整理模式 |
| 5 | LootSelection 期间新 pickup | ✅ 排队（队列机制） |
| 6 | 背包详情卡通用效果是否默认展开 | ✅ 默认展开；待定事件 → [Docs/PM/PendingDecisions.md](../PM/PendingDecisions.md) |
| 7 | 跳过按钮形态 | ✅ 底部按钮行 |
| 8 | 卡片是否可鼠标点击 | ✅ 是（卡片外包 UButton） |

---

## 方案设计

### 总体架构

```text
RewardPickup (E键)
   ↓ 不再 Destroy 自己
   ↓ HUD::QueueLootSelection(Options, SourcePickup)
HUD 队列管理
   ├─ 当前没活跃选择 → 立即 ShowLootUI
   └─ 已有活跃选择 → 入队，活跃结束后弹出
LootSelectionWidget (动态构建 N 张 RuneInfoCard, N<=5)
   ├─ 鼠标点击卡 / A / Enter → SelectRuneLoot(idx)
   │       → SourcePickup->ConsumeAndDestroy()
   │       → HUD->OpenBackpack()  ← 整理模式
   │       → 弹出队列下一项（如有）
   ├─ 点 BtnSkip / B / Esc → SkipSelection()
   │       → SourcePickup->ResetForSkip(NearbyPlayer)
   │       → 弹出队列下一项
   └─ 点 BtnBackpackPreview / Y / Tab → OpenBackpackPreview()
         → HUD->OpenBackpackForPreview(OnClosed=ReactivateAfterPreview)
         → BackpackScreen 进入只读模式
```

### 关键设计决策

| 决策 | 方案 | 理由 |
|---|---|---|
| RewardPickup 销毁时机 | 选符文后才销毁；跳过仅复位 | 满足"跳过保留"需求 |
| 卡牌容器类型 | `UHorizontalBox CardContainer`（固定类型，不用 UPanelWidget）| Slot 类型确定，避免 cast |
| 卡牌动态实例 | C++ `CreateWidget<URuneInfoCardWidget>` 后包 UButton 再 AddChild | 数据驱动 + 鼠标可点 |
| 卡牌外包 UButton | C++ 创建 ULootCardButton（薄壳 UUserWidget），内含 RuneInfoCard | 鼠标点击触发 SelectRuneLoot(idx) |
| 通用效果聚焦控制 | `URuneInfoCardWidget::SetGenericEffectsExpanded(bool)` | 切焦点时启停子窗 |
| 跳过/预览按钮 | 底部按钮行，独立于卡牌 | 视觉清晰 |
| 背包预览返回 | `AYogHUD::OpenBackpackForPreview(FSimpleDelegate OnClosed)` | 封装 BackpackWidget 私有访问 |
| 背包只读模式 | `UBackpackScreenWidget::SetPreviewMode(bool)` 禁拖拽/旋转 | 只读 vs 整理切换 |
| 多 pickup 排队 | `AYogHUD` 维护 `TArray<FQueuedLootRequest>` | 严格 FIFO，避免覆盖 |
| 焦点状态机 | `ELootFocusSection { Cards, Buttons }` | DPad Up/Down 切段 |
| 重掷接口 | `RerollCard(int32)` 留空实现 + 日志 | 后续扩展 |

### 焦点状态机

```cpp
enum class ELootFocusSection : uint8 { Cards, Buttons };
```

- DPad Up/Down → 在 Cards / Buttons 段切换
- DPad Left/Right、LB/RB、左摇杆 X → 段内移动（卡片段范围 `[0, SpawnedCards.Num()-1]`）
- A/Enter/鼠标点击卡 → Cards 段：选当前符文；Buttons 段：触发当前按钮
- B/Circle/Esc → 全局：跳过
- Y/Triangle/Tab → 全局：背包预览

---

## 实现步骤

### 步骤 1 — RewardPickup 延迟销毁 + 跳过复位

**[RewardPickup.h](Source/DevKit/Public/Map/RewardPickup.h)**：
```cpp
public:
    void ConsumeAndDestroy();                                  // 选符文后：销毁自己
    void ResetForSkip(APlayerCharacterBase* Player);           // 跳过后：复位状态
```

**[RewardPickup.cpp](Source/DevKit/Private/Map/RewardPickup.cpp)**：
- `TryPickup`：不再 `Destroy()`；改为 `HUD->QueueLootSelection(Options, this)`
- `ConsumeAndDestroy`：执行原 Destroy 逻辑
- `ResetForSkip(Player)`：复位 `bPickedUp=false`、若 Player 还在范围内则恢复 `Player->PendingPickup=this` 和 RuneInfoWidgetComp 可见

### 步骤 2 — HUD 队列与背包预览接口

**[YogHUD.h](Source/DevKit/Public/UI/YogHUD.h)**：
```cpp
public:
    void QueueLootSelection(const TArray<FLootOption>& Options, ARewardPickup* SourcePickup);
    void OnLootSelectionFinished();   // LootSelection 调用，弹队列下一项

    void OpenBackpackForPreview(FSimpleDelegate OnClosed);  // 只读模式打开

private:
    struct FQueuedLootRequest
    {
        TArray<FLootOption> Options;
        TWeakObjectPtr<ARewardPickup> SourcePickup;
    };
    TArray<FQueuedLootRequest> LootQueue;
    bool bLootSelectionActive = false;

    FDelegateHandle BackpackPreviewClosedHandle;
```

**[YogHUD.cpp](Source/DevKit/Private/UI/YogHUD.cpp)**：
- `QueueLootSelection`：若 `!bLootSelectionActive` → 立即 ShowLootUI；否则 push 到队列
- `OnLootSelectionFinished`：`bLootSelectionActive=false`；若队列非空 → pop 第一个并 ShowLootUI
- `OpenBackpackForPreview`：调 BackpackWidget->SetPreviewMode(true) + ActivateWidget；绑 OnDeactivated → 调用回调 + SetPreviewMode(false)
- 移除原 `ShowLootSelectionUI` 直接公开调用，统一走 `QueueLootSelection`

### 步骤 3 — LootSelectionWidget C++ 改造

**[LootSelectionWidget.h](Source/DevKit/Public/UI/LootSelectionWidget.h)**：
```cpp
public:
    void ShowLootUI(const TArray<FLootOption>& Options, ARewardPickup* SourcePickup);

    UFUNCTION(BlueprintCallable, Category = "Loot")
    void SelectRuneLoot(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Loot")
    void SkipSelection();

    UFUNCTION(BlueprintCallable, Category = "Loot")
    void OpenBackpackPreview();

    /** 重掷接口（本版本未实现，留作后续 UI 动画扩展） */
    UFUNCTION(BlueprintCallable, Category = "Loot")
    void RerollCard(int32 Index);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    TSubclassOf<URuneInfoCardWidget> RuneCardClass;

    /** 卡片数量上限（生成逻辑保证不超过此值，UI 也强制 clamp） */
    static constexpr int32 MaxCards = 5;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHorizontalBox> CardContainer;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BtnSkip;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BtnBackpackPreview;

private:
    TWeakObjectPtr<ARewardPickup> SourcePickup;
    TArray<TObjectPtr<URuneInfoCardWidget>> SpawnedCards;
    TArray<TObjectPtr<UButton>> SpawnedCardButtons;       // 外包按钮
    ELootFocusSection CurrentSection = ELootFocusSection::Cards;
    int32 CurrentButtonIndex = 0;                          // 0=Skip 1=Preview

    void RebuildCards(const TArray<FLootOption>& Options);
    void FocusCard(int32 Idx);
    void FinishAndNotifyHUD();                             // 通知 HUD 弹队列

    UFUNCTION()
    void OnCardButtonClicked0();                           // 按钮 OnClicked 必须 UFUNCTION
    UFUNCTION()
    void OnCardButtonClicked1();
    // ... 静态生成 0-4 共 5 个，对应 MaxCards
    UFUNCTION() void OnCardButtonClicked2();
    UFUNCTION() void OnCardButtonClicked3();
    UFUNCTION() void OnCardButtonClicked4();
```

> 关于 5 个独立 OnClicked UFUNCTION：UButton OnClicked 不支持带参，UE 5.4 也不允许 lambda 绑定 dynamic delegate。因此为每个固定槽位预声明 5 个回调，绕开此限制。

**[LootSelectionWidget.cpp](Source/DevKit/Private/UI/LootSelectionWidget.cpp)**：

- `NativeConstruct`：解绑 `GM->OnLootGenerated`（旧路径丢失 SourcePickup，废弃）；只保留 OnPhaseChanged 用于显示控制
- `RebuildCards`：clamp `Min(Options.Num(), MaxCards)`；遍历创建 RuneInfoCard，调 ShowRune + SetGenericEffectsExpanded(false)；外包 UButton 加到 CardContainer；UButton OnClicked 绑对应静态 OnCardButtonClickedN
- `FocusCard(idx)`：所有 SpawnedCards 调 SetGenericEffectsExpanded(false)；focused 那张调 (true)；调 BP `OnCardFocused`
- `SelectRuneLoot`：调 GameMode->SelectLoot；`SourcePickup->ConsumeAndDestroy()`；隐藏 UI；HUD->OpenBackpack()（整理）；FinishAndNotifyHUD
- `SkipSelection`：`SourcePickup->ResetForSkip(...)`；隐藏 UI；FinishAndNotifyHUD
- `OpenBackpackPreview`：隐藏自身；HUD->OpenBackpackForPreview(FSimpleDelegate::CreateUObject(this, &::ReactivateAfterPreview))
- `ReactivateAfterPreview`：恢复 Visible + SetUserFocus
- `RerollCard`：暂留空，UE_LOG 占位
- `FinishAndNotifyHUD`：调 HUD->OnLootSelectionFinished()
- `NativeOnKeyDown` / `NativeOnAnalogValueChanged`：所有硬编码 0/2 改为 `Min(SpawnedCards.Num()-1, ...)`；新增 Up/Down 切 Section、Y/Tab → OpenBackpackPreview、B → SkipSelection；按钮段 Left/Right 在 [0,1] 之间切；A/Enter 按段触发不同动作

### 步骤 4 — RuneInfoCardWidget 加聚焦控制

**[RuneInfoCardWidget.h](Source/DevKit/Public/UI/RuneInfoCardWidget.h)**：
```cpp
UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
void SetGenericEffectsExpanded(bool bExpanded);

private:
    bool bGenericEffectsExpanded = true;   // 默认展开（背包卡用）
    TArray<TObjectPtr<UGenericRuneEffectDA>> CachedEffects;  // ShowRune 时缓存
```

**[RuneInfoCardWidget.cpp](Source/DevKit/Private/UI/RuneInfoCardWidget.cpp)**：
- `ShowRune`：缓存 `Rune.RuneConfig.GenericEffects` 到 `CachedEffects`；按 `bGenericEffectsExpanded` 决定是否调 `GenericEffectList->SetEffects`，否则强制 Collapsed
- `SetGenericEffectsExpanded(bool)`：更新 flag；按当前 CachedEffects + flag 重新调 SetEffects 或 Collapse

> **行为兼容**：默认 `bGenericEffectsExpanded=true` → BackpackScreen 不需要任何改动（与现有行为一致）；LootSelection 在 RebuildCards 中显式 SetGenericEffectsExpanded(false)，按需 FocusCard 切换

### 步骤 5 — BackpackScreenWidget 只读模式

**[BackpackScreenWidget.h](Source/DevKit/Public/UI/BackpackScreenWidget.h)**：
```cpp
public:
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SetPreviewMode(bool bReadOnly);

private:
    bool bIsPreviewMode = false;
```

**[BackpackScreenWidget.cpp](Source/DevKit/Private/UI/BackpackScreenWidget.cpp)**：
- `SetPreviewMode(bool)`：设 flag；通知 BackpackGrid 子控件锁定拖拽；隐藏可能的"操作类"按钮（出售、整理）
- 拖拽相关入口（OnDragDetected / OnDrop 等）：开头判断 `bIsPreviewMode` → 直接 return
- `NativeOnDeactivated`：复位 `bIsPreviewMode = false`，避免下次以脏状态打开

### 步骤 6 — WBP 改造

`WBP_LootSelection`：
- 删除现有 3 张固定卡（OptionCard0/1/2）
- 新增 `CardContainer`（**HorizontalBox**），运行时动态填充
- 底部行 `BtnSkip` + `BtnBackpackPreview`（带按键提示文字 RichText）
- 移除 `BtnConfirm`（卡片选中即触发，不再需要单独确认）
- BP `OnLootOptionsReady` 事件简化或删除（C++ 自管 RebuildCards）

### 步骤 7 — 编译 + PIE 验证

| 用例 | 预期 |
|---|---|
| N=1 | 1 张卡 + 跳过/预览按钮 |
| N=3 | 3 张卡水平 + 按钮 |
| N=5 | 5 张卡水平 + 按钮（不溢出）|
| 鼠标点卡 | 选中并 OpenBackpack |
| 跳过 | UI 关，pickup 保留，走开再回按 E 重开 |
| 选符文 | UI 关，pickup 销毁，背包打开（整理）|
| 预览背包 | LootSelection 折叠，背包只读，关闭后回到 LootSelection |
| 多 pickup 同时按 E | 第二个进队列，第一个结束后弹 |
| 焦点切换 | 卡片间 Left/Right；按钮间 Left/Right；段间 Up/Down |
| RerollCard 接口 | 调用后 Log 出 "未实现" 即可 |

---

## 涉及文件

### C++ 新建/修改

| 文件 | 改动 |
|---|---|
| [LootSelectionWidget.h](Source/DevKit/Public/UI/LootSelectionWidget.h) | 重写：SourcePickup / SpawnedCards / Section / 5 个 OnCardButtonClicked / RerollCard 等 |
| [LootSelectionWidget.cpp](Source/DevKit/Private/UI/LootSelectionWidget.cpp) | 重写：RebuildCards / SkipSelection / OpenBackpackPreview / ReactivateAfterPreview / 输入逻辑 |
| [RewardPickup.h](Source/DevKit/Public/Map/RewardPickup.h) | 新增 ConsumeAndDestroy / ResetForSkip |
| [RewardPickup.cpp](Source/DevKit/Private/Map/RewardPickup.cpp) | TryPickup 改为 QueueLootSelection；新增两方法实现 |
| [YogHUD.h](Source/DevKit/Public/UI/YogHUD.h) | 新增 QueueLootSelection / OnLootSelectionFinished / OpenBackpackForPreview / FQueuedLootRequest 队列 |
| [YogHUD.cpp](Source/DevKit/Private/UI/YogHUD.cpp) | 队列管理 + 背包预览封装 |
| [RuneInfoCardWidget.h](Source/DevKit/Public/UI/RuneInfoCardWidget.h) | 新增 SetGenericEffectsExpanded / CachedEffects |
| [RuneInfoCardWidget.cpp](Source/DevKit/Private/UI/RuneInfoCardWidget.cpp) | ShowRune 缓存 + 按 flag 切换子窗 |
| [BackpackScreenWidget.h](Source/DevKit/Public/UI/BackpackScreenWidget.h) | 新增 SetPreviewMode / bIsPreviewMode |
| [BackpackScreenWidget.cpp](Source/DevKit/Private/UI/BackpackScreenWidget.cpp) | 拖拽入口判 PreviewMode；NativeOnDeactivated 复位 |

### WBP 修改

| 文件 | 改动 |
|---|---|
| `WBP_LootSelection` | 删 3 卡，加 `CardContainer (HBox)` + `BtnSkip` + `BtnBackpackPreview` |
| `WBP_RuneInfoCard` | 已规划（CardDesc/CardEffect → YogCommonRichTextBlock + GenericEffectList 子控件） |
| `WBP_GenericEffectEntry` | 新建（已规划） |
| `WBP_GenericEffectList` | 新建（已规划） |

### 待定事件单（新文档）

| 文件 | 内容 |
|---|---|
| [Docs/PM/PendingDecisions.md](../PM/PendingDecisions.md) | 记录"背包详情卡通用效果默认展开"等需要后续重新评审的决策 |

---

## 潜在风险

1. **5 个静态 OnCardButtonClicked 回调**：UE 5.4 dynamic delegate 不支持带参/lambda，所以预声明 5 个固定槽。若后续 MaxCards 升到 6 需要手动添加第 6 个，加注释提醒
2. **HUD::BackpackWidget 私有访问**：通过 `OpenBackpackForPreview` 接口封装，避免 LootSelection 直接 friend
3. **ResetForSkip 的玩家范围状态恢复**：若玩家在 LootSelection 期间已经走出 OverlapVolume，ResetForSkip 不应强行恢复 PendingPickup（否则玩家位置已远，按 E 不会触发但状态错乱）。需要判断 NearbyPlayer 是否仍在范围内
4. **队列在玩家死亡/切关时的清理**：HUD 切关重建时 LootQueue 应清空，否则跨关弹出错误的 pickup 引用
5. **背包只读模式覆盖范围**：拖拽是主要交互，但还有"长按出售"、"右键菜单"等。需 grep 排查所有交互入口都加 `if (bIsPreviewMode) return`
6. **WBP_RuneInfoCard 包到 UButton 内的可见性影响**：UButton 默认 ButtonStyle 可能盖住卡片视觉。可设 ButtonStyle.Normal/Hovered/Pressed 全透明 brush

## 待确认问题（无）

所有原 7 项问题已由用户决策定夺。
