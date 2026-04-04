# 背包 UI 与 DA 衔接方案

> 更新日期：2026-04-04  
> 对象：策划 + 程序（背包 UI 对接）  
> 前提：程序员已提供蓝图背包 UI（有拖拽效果），你有 `DA_Rune_xxx` 和 `BackpackGridComponent`

---

## 架构总览

```
DA_Rune_xxx (资产)
    ↓ CreateInstance()
FRuneInstance (运行时数据，含唯一 Guid)
    ↓ 作为参数
BackpackGridComponent.TryPlaceRune(Rune, Pivot)
    ↓ 触发委托
OnRunePlaced / OnRuneActivationChanged / OnHeatTierChanged
    ↓ 蓝图绑定
背包 UI Widget 刷新显示
    ↓ 同时
BackpackGridComponent → GAS（GE Apply / BuffFlow 启动）
```

---

## Step 1：背包 UI 初始化时获取组件

在背包 Widget 的 `Event Construct` 里：

```
Get Owning Player Pawn
  → Get Component by Class → BackpackGridComponent
  → 存为 Widget 变量 "BackpackComp"
```

同时绑定所有需要的委托（见 Step 5）。

---

## Step 2：符文拾取 / 奖励时，创建 FRuneInstance

在捡取或选择符文奖励的逻辑里（蓝图或 C++）：

```
DA_Rune_xxx（URuneDataAsset 引用）
  → Call "Create Instance"（C++ 函数，蓝图可调用）
  → 得到 FRuneInstance（自动生成唯一 RuneGuid）
  → 存入 UI 的"手持符文"变量，等待玩家放置
```

> `CreateInstance()` 在 `RuneDataAsset.h` 里已声明，直接调即可。

---

## Step 3：拖拽 Drop 时调用 TryPlaceRune

在背包格子 Widget 的 `On Drop` 事件里：

```
获取格子坐标 GridCell（FIntPoint，如 (2, 3)）
  → BackpackComp → Try Place Rune(FRuneInstance, GridCell)
  → 返回 bool
      true  → UI 在对应格子渲染符文图标和形状
      false → 播放"放不下"反馈动画，符文归位
```

**格子坐标从哪来：**  
每个格子子 Widget 需要有 `GridX` / `GridY` 变量。背包 Widget 生成格子时赋值，范围 (0,0)~(4,4)。  
如果程序员用的是 `UniformGridPanel`，行列号直接对应坐标。

---

## Step 4：格子内符文拖拽移动

```
拖起格子内已有符文
  → 从 FPlacedRune 读取 RuneGuid
  → Drop 到新格子位置 NewPivot
  → BackpackComp → Move Rune(RuneGuid, NewPivot)
      true  → UI 更新符文显示位置
      false → 归位到原始格子
```

---

## Step 5：绑定委托，UI 被动刷新

在 Widget Construct 里绑定以下委托（不要主动轮询，用事件驱动）：

| 委托 | 绑定响应 |
|---|---|
| `OnRunePlaced` | 在对应格子渲染符文图标 + 形状占格 |
| `OnRuneRemoved` | 清除对应格子的符文显示 |
| `OnRuneActivationChanged` | 激活 = 加金边/发光，未激活 = 灰显 |
| `OnHeatTierChanged` | 重新刷新激活区高亮（调 `GetActivationZoneCells`） |

---

## Step 6：激活区高亮

```
BackpackComp → Get Activation Zone Cells()
  → 返回 TArray<FIntPoint>
  → 对这些坐标的格子设置高亮颜色（如金色边框）
  → 其余格子恢复默认颜色
```

热度变化时（`OnHeatTierChanged` 委托触发），重新调用此流程刷新范围。

---

## Step 7：初始化时渲染已有符文

如果背包里已有符文（如存档读取后），Widget 打开时需要主动拉一次数据：

```
BackpackComp → Get All Placed Runes()
  → 返回 TArray<FPlacedRune>
  → 遍历每个 FPlacedRune
      → Pivot + Shape → 渲染到对应格子
      → bIsActivated → 是否加激活高亮
```

---

## 最小接入路径（给程序员的最简说明）

如果程序员的 UI 改动成本高，只需他做两件事：

1. **Drop 事件** 调 `BackpackComp.TryPlaceRune(FRuneInstance, FIntPoint)`
2. **Widget 打开时** 循环读 `BackpackComp.GetAllPlacedRunes()` 渲染已有符文

其余激活高亮/移除效果通过委托被动触发，不需要 UI 主动轮询，改动范围最小。

---

## 关键接口速查

```cpp
// 放置符文
bool TryPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot);

// 移除符文
bool RemoveRune(FGuid RuneGuid);

// 移动符文
bool MoveRune(FGuid RuneGuid, FIntPoint NewPivot);

// 查询能否放置（不实际放）
bool CanPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot) const;

// 获取所有已放置符文（UI 初始化用）
const TArray<FPlacedRune>& GetAllPlacedRunes() const;

// 获取激活区格子坐标（UI 高亮用）
TArray<FIntPoint> GetActivationZoneCells() const;

// 查询某格的符文索引（-1 = 空）
int32 GetRuneIndexAtCell(FIntPoint Cell) const;
```

所有接口均为 `BlueprintCallable` / `BlueprintPure`，蓝图直接可用。
