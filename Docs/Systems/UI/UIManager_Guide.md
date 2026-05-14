# UI Manager 配置与使用指南

> 适用范围：`UYogUIManagerSubsystem` + `UYogUIRegistry` + `DA_YogUIRegistry`  
> 适用人群：策划（配置 DA）/ 程序（调用 API）  
> 配套文档：[LootSelection_Technical](LootSelection_Technical.md) · [Widget 规范](../../Conventions/Widget.md)  
> 最后更新：2026-05-13（Step 3/4/5 完成）

---

## 概述

把项目里所有"打开/关闭某个 UI 屏幕"的逻辑集中到 `UYogUIManagerSubsystem`，让 Widget 类引用、ZOrder、生命周期都在 DataAsset 里配，而不是散落在 HUD/Controller 的硬编码里。

> 类比：UIRegistry 就是 UI 屏幕的"配置表"，Subsystem 是按表抓 Widget 的"接待员"。

---

## 三个核心组件

| 组件 | 职责 | 路径 |
|---|---|---|
| `EYogUIScreenId` | 屏幕 ID 枚举（MainHUD / Backpack / PauseMenu …） | `Source/DevKit/Public/UI/YogUIRegistry.h` |
| `UYogUIRegistry` (DA) | ScreenId → WidgetClass + ZOrder + Description + 是否开局创建 | `/Game/UI/DA_YogUIRegistry` |
| `UYogUIManagerSubsystem` | LocalPlayerSubsystem，提供 PushScreen / PopScreen / EnsureWidget | `Source/DevKit/Public/UI/YogUIManagerSubsystem.h` |

---

## 第一步：配置 DA_YogUIRegistry

### 1.1 确认 DA 已挂到 Config

`Config/DefaultGame.ini` 必须有：

```ini
[/Script/DevKit.DevAssetManager]
UIRegistry=/Game/UI/DA_YogUIRegistry.DA_YogUIRegistry
```

### 1.2 在编辑器中编辑 `DA_YogUIRegistry`

双击打开 → 找到 `Entries` 数组 → 每一项填六个字段：

| 字段 | 含义 | 示例 |
|---|---|---|
| ScreenId | 从 `EYogUIScreenId` 枚举选 | `Backpack` |
| WidgetClass | 指向具体 WBP（TSoftClassPtr） | `WBP_BackpackScreen` |
| ZOrder | 视口层级整数，越大越靠上 | `5` |
| Layer | `Game` / `Menu` / `Modal`，决定激活时的 InputMode 切换 | `Menu` |
| bCreateOnHUDStart | true = HUD::BeginPlay 时立即创建+AddToViewport | ✅ / ❌ |
| Description | 这条 Entry 的用途说明，仅给编辑器人看，不参与运行时 | 见下表 1.3 |

Layer 字段含义：

| Layer | 用途 | InputMode 行为 |
|---|---|---|
| `Game` | 被动 overlay（HUD / 指引 / 世界锚定弹窗） | 保持 GameOnly，不抢焦点 |
| `Menu` | 需要焦点但不暂停的 in-game UI（背包 / 三选一 / 教程） | 切 GameAndUI，焦点设到顶层 widget 的 DesiredFocusTarget |
| `Modal` | 顶层独占（暂停菜单 / 系统弹窗） | 切 GameAndUI，显示鼠标 |

### 1.3 推荐初始填法（含 Description 文案）

> HUD 当前会预挂的屏幕，建议 `bCreateOnHUDStart=true`；按需弹出的留 false。  
> Description 列可以直接复制到 DA 对应 Entry 的 `Description` 字段里。

| ScreenId | Widget | ZOrder | Layer | bCreateOnHUDStart | Description（建议填写） |
|---|---|---|---|---|---|
| MainHUD | WBP_HUDRoot | 1 | Game | ✅ | 常驻 HUD 容器（血条 / 敌人箭头 / 武器图标 等）。AYogHUD::BeginPlay 必读，所有 HUD 子控件挂在其内。 |
| Backpack | WBP_BackpackScreen | 5 | Menu | ✅ | 玩家背包/符文格子界面。按键 ToggleBackpack 激活；CommonUI Activatable，PushScreen 打开 / PopScreen 关闭。 |
| LootSelection | WBP_LootSelection | 15 | Menu | ✅ | 三选一符文奖励界面。由 RewardPickup 触发 QueueLootSelection；常驻挂在 viewport，激活时显示。 |
| SacrificeGraceOption | WBP_SacrificeGraceOption | 16 | Menu | ✅ | 献祭恩赐 Yes/No 确认弹窗。由 SacrificeGracePickup::TryPickup 触发；常驻预挂，按需激活。 |
| CurrentRoomBuff | WBP_CurrentRoomBuff | 2 | Game | ❌ | 当前房间敌人符文/Buff 列表。直接 AddToViewport，进入房间时由 GameMode 创建并填充。 |
| CombatItemBar | WBP_CombatItemBar | —（忽略） | Game | ❌ | 战斗道具栏（消耗品/格挡卡片）。**不走 viewport，作为子控件挂在 MainHUD 的 BottomRightPlayerInfoRegion Overlay 里**，ZOrder 字段无效；填什么都不会改变叠放顺序。 |
| TutorialPopup | WBP_TutorialPopup | — | Menu | ❌ | 新手引导弹窗。TutorialManager 监听 EventID 触发；不 AddToViewport，由 Manager 控制显隐。 |
| PauseMenu | WBP_PauseMenu | 100 | Modal | ❌ | 暂停菜单（ESC / 开始键触发）。Activatable，最高层覆盖；OpenPauseMenu 时按需创建。 |
| FinisherQTE | WBP_FinisherQTE | 30 | Game | ❌ | 处决 QTE 提示窗口。AN_FinisherWindow 期间显示按键提示，确认/超时后隐藏。 |
| WeaponFloat | WBP_WeaponFloat | 13 | Game | ❌ | 武器拾取浮窗（卡牌列表）。靠近 WeaponSpawner 时世界坐标投影显示，淡入淡出。 |
| WeaponThumbnailFly | WBP_WeaponThumbnailFly | 10 | Game | ❌ | 拾武器后从拾取点飞向左下角玻璃图标的动画 Widget。一次性，TriggerWeaponPickup 创建。 |
| WeaponTrail | WBP_WeaponTrail | 9 | Game | ❌ | WeaponThumbnailFly 的流光拖尾伴随特效。 |
| LevelEndReveal | WBP_LevelEndReveal | 20 | Game | ❌ | 关卡结算圆形揭幕动画 Widget（材质驱动）。GameMode 进 ArrangementPhase 时触发。 |
| DamageEdgeFlash | WBP_DamageEdgeFlash | 4 | Game | ❌ | 角色受击屏幕边缘红色闪屏反馈。OnHealthChanged 触发。 |
| PortalPreview | WBP_PortalPreview | 15 | Game | ❌ | 关卡传送门预览浮窗（显示下一关类型/难度）。ShowPortalGuidance 时挂出。 |
| PortalDirection | WBP_PortalDirection | 14 | Game | ❌ | 屏幕边缘指向 Portal 的方位箭头。门在视口外时显示。 |
| InfoPopup | （按需配） | 25 | Menu | ❌ | 通用信息弹窗。 |

### 1.4 ZOrder 的实际作用范围

ZOrder 只对 **直接 AddToViewport 的 Widget** 生效（即 `EnsureWidget` / `PushScreen` 路径会调用 `AddToViewport(ZOrder)` 的那批）。

| 情况 | ZOrder 是否生效 |
|---|---|
| `EnsureWidget` / `PushScreen` 创建并 `AddToViewport` | ✅ 生效 |
| 作为子控件挂在另一个 Widget 内（如 `CombatItemBar` 挂在 MainHUD 里） | ❌ 忽略，叠放由父控件 Slot 决定 |
| 两个 Widget 配同样 ZOrder | 后 `AddToViewport` 的覆盖前者（非确定性，应避免） |

### 1.5 没在 DA 里配的屏幕

会回退到 `BP_YogHUD` Details 面板里手填的 `*Class` 字段。**留着这些字段做保底，迭代期间不要清空**。

---

## 第二步：从代码打开/关闭屏幕

### 2.1 拿到 Subsystem

```cpp
#include "UI/YogUIManagerSubsystem.h"

ULocalPlayer* LP = PC->GetLocalPlayer();
UYogUIManagerSubsystem* UI = LP->GetSubsystem<UYogUIManagerSubsystem>();
```

HUD 内部有个就近的 helper：

```cpp
if (UYogUIManagerSubsystem* UI = GetUIManagerFromHUD(this)) { ... }
```

### 2.2 三个生命周期方法

| 方法 | 用途 | 返回值 |
|---|---|---|
| `EnsureWidget(ScreenId)` | 创建并加到视口（不激活）。幂等。 | `UUserWidget*` |
| `PushScreen(ScreenId)` | Ensure + ActivateWidget。**只对 UCommonActivatableWidget 有效** | `UCommonActivatableWidget*` |
| `PopScreen(ScreenId)` | DeactivateWidget | void |

### 2.3 查询方法

| 方法 | 用途 |
|---|---|
| `GetWidget(ScreenId)` | 拿缓存实例（可能为 nullptr） |
| `GetTypedWidget<T>(ScreenId)` | 同上 + Cast 到具体类型 |
| `IsScreenActive(ScreenId)` | 是否处于激活态 |

### 2.4 典型调用

```cpp
// 打开背包
UI->PushScreen(EYogUIScreenId::Backpack);

// 关闭背包
UI->PopScreen(EYogUIScreenId::Backpack);

// 仅挂上视口、不激活（适合 Overlay）
UI->EnsureWidget(EYogUIScreenId::CurrentRoomBuff);

// 查询并访问
if (UBackpackScreenWidget* BP = UI->GetTypedWidget<UBackpackScreenWidget>(EYogUIScreenId::Backpack))
{
    BP->SetReadOnly(true);
}
```

---

## 第三步：从 Blueprint 调用

以下方法已标 `UFUNCTION(BlueprintCallable)`：

| Blueprint 节点 | 等价 C++ |
|---|---|
| Get Local Player Subsystem (YogUIManagerSubsystem) | 获取 Subsystem |
| Ensure Widget | `EnsureWidget` |
| Pop Screen | `PopScreen` |
| Get Widget | `GetWidget` |
| Is Screen Active | `IsScreenActive` |
| Create Auto Start Widgets | `CreateAutoStartWidgets`（一般不用手调）|

`PushScreen` 因返回 `UCommonActivatableWidget*`，目前仅 C++ 可调。需要 BP 支持时让程序加一个 `BlueprintCallable` 包装。

---

## 第四步：新增一个屏幕类型

### 步骤

1. **加枚举值**：`Source/DevKit/Public/UI/YogUIRegistry.h`
   ```cpp
   enum class EYogUIScreenId : uint8 {
       …
       MyNewScreen,   // ← 加在末尾
   };
   ```
2. **重编 C++**（反射要刷新枚举）。
3. **在 DA 加 Entry**：ScreenId = MyNewScreen，WidgetClass 指向 WBP。
4. **代码里调用**：
   ```cpp
   UI->PushScreen(EYogUIScreenId::MyNewScreen);  // Activatable
   // 或
   UI->EnsureWidget(EYogUIScreenId::MyNewScreen); // Overlay
   ```

### 选 Activatable 还是 Overlay？

| 你的 Widget 父类 | 用什么 API |
|---|---|
| `UCommonActivatableWidget` | `PushScreen` / `PopScreen` |
| `UUserWidget`（不是 Activatable） | `EnsureWidget`（一直在视口） |

> 简单判定：玩家"打开/关闭、按 ESC 退出、有焦点控制"→ Activatable。"血条/方向箭头/伤害闪屏"→ Overlay。

---

## 当前已迁移情况

| 屏幕 | 是否已走 Subsystem | 备注 |
|---|---|---|
| MainHUD | ✅ | HUD::BeginPlay 先问 Subsystem |
| Backpack | ✅ | OpenBackpack 用 PushScreen |
| LootSelection | ✅ | EnsureLootSelectionWidget 改走 EnsureWidget |
| SacrificeGraceOption | ✅ | 同上 |
| TutorialPopup | ✅ | HUD::BeginPlay 适配 |
| PauseMenu | ❌ | 仍用 OpenPauseMenu 内的 CreateWidget |
| FinisherQTE / CurrentRoomBuff / CombatItemBar | ❌ | 仍用 Ensure*Widget 老路 |
| WeaponFloat / Thumbnail / Trail / LevelEndReveal | ❌ | 仍用各自路径 |
| Portal* | ❌ | 仍用各自路径 |

> 未迁移的不影响功能，DA 里配上 WidgetClass 后 `ResolveManagedWidgetClass` 会自动用 DA 的类替换硬编码 `*Class`。

---

## 故障排查

| 现象 | 大概率原因 | 检查 |
|---|---|---|
| `UI->GetRegistry()` 返回 null | DefaultGame.ini 路径错 / 资产没烘焙 | `[/Script/DevKit.DevAssetManager] UIRegistry=...` |
| `GetWidgetClass(ScreenId)` 返回 null | DA 里没这个 ScreenId 的 Entry 或 WidgetClass 空 | 打开 DA 检查 Entries |
| `EnsureWidget` 成功但 `PushScreen` 返回 null | 注册的类不是 `UCommonActivatableWidget` | 改用 `EnsureWidget`，或把 WBP 父类换成 `UCommonActivatableWidget` |
| 屏幕出现了但手柄焦点丢失 | 顶层 Activatable 没实现 `GetDesiredFocusTarget()`，或 Layer 配成了 `Game` | 在 WBP override `GetDesiredFocusTarget` 返回希望初始聚焦的子控件；确认 Entry.Layer 是 `Menu` 或 `Modal` |
| 死亡复活后 D-pad 卡死在 UI | 复活时没把上层菜单 PopScreen | 走 Subsystem 的 PushScreen / PopScreen，OnTopLayerChanged 会自动切回 GameOnly |
| 关卡切换后旧 Widget 残留 | Subsystem 跟 LocalPlayer 同生命周期，关卡切换不销毁实例 | 关卡跳关前 `PopScreen` 或自己 RemoveFromParent |

---

## 第五步：Layer / InputMode / 异步加载

Step 3/4/5 已落地：Subsystem 自动跟踪 `UCommonActivatableWidget` 的激活/反激活，结合 Entry 上的 `Layer` 字段自动切 InputMode + 焦点，并提供 `PushScreenAsync`。

### 5.1 Layer 模型

激活的 ScreenId 集合实时算出 **Top Layer**：

- 集合里全是 `Game` → Top = `Game`
- 至少一个 `Menu`，没有 `Modal` → Top = `Menu`
- 至少一个 `Modal` → Top = `Modal`

每次集合变化都会重算 Top；如果跨层了，广播 `OnTopLayerChanged(NewLayer)`。

### 5.2 自动 InputMode 切换（Step 4）

`bAutoManageInputMode = true`（默认）时，Subsystem 在 RecomputeTopLayer 之后会：

| Top Layer | 调用 | 鼠标 | 焦点 |
|---|---|---|---|
| `Game` | `SetInputMode(FInputModeGameOnly)` | 隐藏 | 不主动指定 |
| `Menu` | `SetInputMode(FInputModeGameAndUI)` | 隐藏 | 顶层 widget 的 `GetDesiredFocusTarget()`（fallback 到 widget 本身）|
| `Modal` | `SetInputMode(FInputModeGameAndUI)` | 显示 | 同上 |

**这条链就是为了修死亡复活后手柄 D-pad 没焦点的旧 bug**：复活流程 PopScreen 顶层 Activatable → ActivatedScreens 收紧 → Top 回到 Game → 自动切回 GameOnly。

如果项目某条流程要自己接管 InputMode，把 `bAutoManageInputMode` 置为 false。

### 5.3 在 WBP 里指定焦点

继承 `UCommonActivatableWidget` 的 WBP override `Get Desired Focus Target`（蓝图函数）返回首选聚焦的子控件（比如三选一里的第一张卡）。Subsystem 切到 Menu/Modal 时会把 Slate 焦点送过去。

### 5.4 监听 Layer 变化

```cpp
UI->OnTopLayerChanged.AddDynamic(this, &UMyHUD::HandleTopLayerChanged);

UFUNCTION()
void HandleTopLayerChanged(EYogUILayer NewTopLayer);
```

蓝图同样可绑（`On Top Layer Changed`）。常见用途：菜单打开时降低环境音量、暂停粒子等。

### 5.5 异步加载（Step 5）

`bCreateOnHUDStart=false` 且未预加载的屏幕，首次 PushScreen 会同步加载 widget 类。对体积大的 WBP，改用：

```cpp
FOnAsyncScreenReady Cb;
Cb.BindDynamic(this, &UMyClass::HandlePauseMenuReady);
UI->PushScreenAsync(EYogUIScreenId::PauseMenu, Cb);

UFUNCTION()
void HandlePauseMenuReady(EYogUIScreenId ScreenId, UCommonActivatableWidget* Widget);
```

- 已加载过：立即同步走 PushScreen 路径，callback 当帧触发
- 没加载过：用 `FStreamableManager::RequestAsyncLoad` 加载，完成后 PushScreen + callback
- 同一 ScreenId 重复发起：覆盖 callback（最后一个调用方拿结果），不会重复发起异步请求

> 落地建议：所有 `LoadClass<UUserWidget>(nullptr, TEXT("/Game/..."))` 的硬路径回退都替换成 `PushScreenAsync`，配合 DA 里的 SoftClassPtr 即可。

---

## Roadmap 状态

| 步骤 | 状态 | 落地点 |
|---|---|---|
| Step 1 | ✅ | `UYogUIRegistry` + DA + DevAssetManager 接线 |
| Step 2 | ✅ | `PushScreen` / `PopScreen` / `EnsureWidget` 分流 Activatable vs Overlay |
| Step 3 | ✅ | `EYogUILayer` + `ActivatedScreens` 追踪 + `OnTopLayerChanged` |
| Step 4 | ✅ | `ApplyInputModeForLayer` 自动切 GameOnly / GameAndUI + 设 DesiredFocus |
| Step 5 | ✅ | `PushScreenAsync` 走 StreamableManager |
| 后续 | ⏳ | 把 PauseMenu / FinisherQTE / Portal* 也迁到 Subsystem；引入 `UCommonActivatableWidgetStack` 做真 stack 容器 |

---

## 相关文件

| 文件 | 角色 |
|---|---|
| `Source/DevKit/Public/UI/YogUIRegistry.h` | 枚举 + 数据结构定义 |
| `Source/DevKit/Public/UI/YogUIManagerSubsystem.h` | API |
| `Source/DevKit/Private/UI/YogHUD.cpp` | 集成点（BeginPlay / Ensure*Widget / OpenBackpack） |
| `Source/DevKit/Public/DevAssetManager.h` | Config 字段 + GetUIRegistry |
| `/Game/UI/DA_YogUIRegistry` | 数据资产 |
| `Config/DefaultGame.ini` | DA 路径配置 |
