# 背包 UI 架构

> 背包系统 UI 的完整架构、各 Widget 职责、数据流、配置入口。
>
> 来源：原 memory `project_backpack_ui_arch.md`（2026-04-18 定稿）。已转写到本文件作为正式架构文档。
>
> 配套：[BackpackSystem_Guide](BackpackSystem_Guide.md)（玩家视角操作指南）· [BackpackSystem_Technical](BackpackSystem_Technical.md)（数据层 / 链路 / 升级）

---

## Widget 层级

```
WBP_BackpackScreen          (UCommonActivatableWidget)
├── WBP_BackpackGrid        (UBackpackGridWidget)   — 主背包格子，UniformGridPanel
│   └── WBP_RuneSlot × N    (URuneSlotWidget)       — 每格状态 + 图标
├── WBP_PendingGrid         (UPendingGridWidget)    — 待放置符文槽
└── WBP_RuneInfoCard        (URuneInfoCardWidget)   — 选中格子后显示符文详情
```

---

## 关键 C++ 文件

| 文件 | 职责 |
|---|---|
| `Public/UI/BackpackScreenWidget.h` + `.cpp` | 主入口：输入、拖拽、手柄、热度预览、生命周期 |
| `Public/UI/BackpackGridWidget.h` + `.cpp` | BuildGrid / RefreshCells，管理 RuneSlot 实例；NativePaint 画包围框 |
| `Public/UI/PendingGridWidget.h` + `.cpp` | 待定槽 BuildSlots / RefreshSlots / GetNearestSlotAtScreenPos |
| `Public/UI/RuneSlotWidget.h` + `.cpp` | 单格渲染：SetSlotState + SetRuneIcon，颜色动效；ShakeAndFlash 红闪 |
| `Public/UI/RuneInfoCardWidget.h` + `.cpp` | 显示符文名 / 描述 / 点阵 / 背景图 |
| `Public/UI/BackpackStyleDataAsset.h` | 所有颜色、格子尺寸、边距的统一配置 DA |
| `Public/Component/BackpackGridComponent.h` + `.cpp` | 数据层：格子占用、激活区、金币、符文增删、链路、升级 |

---

## RuneInfoCard 规格

- **ShapeGrid**：Blueprint 里放 CanvasPanel，固定 64×64 px
- **点阵**：C++ 动态生成，固定 8 px 点 / 2 px 间隔，自动居中
- **CardBG**：读 `FRuneConfig::CardBackground`；无图时显示不透明黑色
- **CardEffect**：独立 TextBlock，与 CardDesc 分开显示效果描述
- **WBP 根节点**：SizeBox 定宽高；Screen 里该实例勾选 **Size To Content**，只管位置

---

## 颜色方案（StyleDA 默认值）

| 状态（`EBackpackCellState`）| DA 字段 | 默认值 |
|---|---|---|
| Empty | `EmptyColor` | (0.40, 0.40, 0.42) 中灰 |
| EmptyActive | `HeatZone0Color` | 热度 1 区（最内层），用户自填 |
| EmptyZone1 | `HeatZone1Color` | 热度 2 区，用户自填 |
| EmptyZone2 | `HeatZone2Color` | 热度 3 区（最外层），用户自填 |
| OccupiedInactive | `OccupiedInactiveColor` | (0.18, 0.10, 0.25) 深紫灰 |
| OccupiedActive | `OccupiedActiveColor` | (0.10, 0.65, 1.00) 亮蓝 |
| Selected | `SelectedColor` | (1.00, 0.82, 0.10) 金黄 |
| GrabbedSource | `GrabbedSourceColor` | (1.00, 0.85, 0.10) 亮黄 |

---

## 热度阶段预览（PreviewPhase）

`BackpackScreenWidget::PreviewPhase`（int，-1 = 三阶叠加全显）

| 模式 | 显示 |
|---|---|
| **默认**（PreviewPhase = -1）| EmptyActive / Zone1 / Zone2 三色同时叠加，Zone0 优先级最高（最内层压盖最外层）|
| **单阶聚焦**（0 / 1 / 2）| 只显示该阶激活区，其余置灰 |

### 操作

- **键盘 1 / 2 / 3** 或 **鼠标点 HeatPhaseDot0/1/2** → 切到对应单阶；再按取消回 -1
- **手柄 R1** 正向循环 -1 → 0 → 1 → 2 → -1
- **手柄 L1** 反向
- **关闭背包** 自动重置为 -1
- **打开背包** 自动同步当前热度阶段（FIX-013）

### 重要：HeatPhaseDot 在 BackpackGrid 内

- HeatPhaseDot0 / 1 / 2 和 GamepadHintLabel 控件在 **WBP_BackpackGrid** 里（不在 WBP_BackpackScreen），因为 `BindWidgetOptional` 不递归到嵌套 UserWidget 内部树
- 通过 `FOnHeatPhaseButtonClicked` delegate 与 BackpackScreen 通信
- GamepadHintLabel：手柄输入时 C++ 自动显示 "L1/R1 切换热度显示"；鼠标输入时隐藏

---

## 背包配置注入

数据来源：`UWeaponDefinition::BackpackConfig`（`FBackpackConfig` — `GridWidth` / `GridHeight` / `ActivationZoneConfig`）

### 注入路径

```
WeaponSpawner::TryPickupWeapon
  └─→ BackpackGridComponent::ApplyBackpackConfig
      ├─ 同时重建 GridOccupancy 数组
      └─ 广播 OnConfigChanged（UI 重建格子）

NativeOnActivated
  └─→ BackpackGridWidget::BuildGrid
      └─ 确保格子尺寸与最新配置一致
```

> ⚠️ **不是** `WeaponDefinition::SetupWeaponToCharacter` — Spawner 是入口。

### 添加新武器 DA 时

1. 填 `BackpackConfig.GridWidth` / `GridHeight`
2. 填 `ActivationZoneConfig.ZoneShapes`（绝对坐标，X = 列 Y = 行，从 0 开始）
3. 填 `InitialRunes`（拾取时自动放置到热度一激活区，[FEAT-027]）

---

## 待放置区（PendingGrid）

详见 [REFACTOR-018]（[FeatureLog.md](../../FeatureLog.md)）：
- 旧版 UImage / Overlay 线性列表 → 新版 RuneSlotWidget 稀疏格子，与主背包视觉一致
- ZoneOpacity 固定 1.0（Pending 无热度分区，不压暗空格）
- 数据层：`BackpackScreenWidget::PendingGrid : TArray<FRuneInstance>`，大小 = `PendingGridCols × PendingGridRows`
- 同步：`SyncPendingFromPlayer`（打开时）/ `SyncPendingToPlayer`（关闭 / 操作后）

---

## 战斗阶段锁定

详见 [03_RunLoop.md](../../Survey/03_RunLoop.md) 的 [FIX-027]：
- `IsInCombatPhase()` 改用 `GM->CurrentPhase == ELevelPhase::Combat`（不再用 `HasAliveEnemies()`）
- 进关到第一波刷怪前的窗口锁定，主城自动跳过
- 触发点 5 处：拖拽开始 / 落点 / 点格 / 手柄确认；阻断后调 `RuneSlotWidget::ShakeAndFlash` 红闪 + 抖动

---

## 已知架构债

| 问题 | 影响 |
|---|---|
| BindWidget 不递归嵌套 UserWidget 内部树 | HeatPhaseDot 必须放在 WBP_BackpackGrid，不能放 WBP_BackpackScreen |
| WBP_BackpackScreen 中 PendingGrid 实例需勾 Size To Content | 否则 SizeBox 被父布局拉伸 |
| 多格 / 异形符文 UI 渲染未实现（Shape 数据已存）| 当前展示版本固定 1×1，待 P2 补 |
