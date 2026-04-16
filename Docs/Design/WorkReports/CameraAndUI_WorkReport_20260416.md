# 相机系统 + 背包UI手柄适配 工作报告

> 日期：2026-04-16（上午）  
> 涉及系统：相机管理、背包UI、三选一UI、战斗日志  
> 配套文档：[Camera_Design.md](../Systems/Camera_Design.md)、[BackpackGamepadAndUI.md](../BackpackGamepadAndUI.md)

---

## 一、今日完成内容

### 1.1 手柄适配 + 背包UI升级 + 战斗日志系统

**背包交互升级（BackpackScreenWidget）**

- D-Pad 光标移动格子
- A 键两步式操作：抓取 → 放置 / 互换（拖到有符文格自动 Swap，失败回滚）
- B 取消操作，Y 移除当前格符文
- 拖拽浮空视觉效果（1.08 倍放大 + 上移 8px）
- 鼠标/手柄悬停触发 RuneTooltip

**三选一手柄导航（LootSelectionWidget）**

- D-Pad 左右切换高亮
- A 键确认选择

**新增 C++ 基类（RuneTooltipWidget）**

- `ShowRuneInfo(RuneDA)` / `HideTooltip()` 接口
- 蓝图事件暴露，由 WBP 实现视觉

**战斗日志系统**

- `CombatLogStatics`：静态桥，任意位置写入日志条目
- `CombatLogWidget`：Dota2 风格浮动显示
- `YogAbilitySystemComponent`：`PushEntry` 自动写入伤害/治疗日志

---

### 1.2 相机管理系统完整实现（AYogCameraPawn）

**新增**

| 文件 | 说明 |
|------|------|
| `YogCameraPawn.h / .cpp` | 6 状态优先级相机系统 |
| `CameraConstraintActor.h / .cpp` | 多边形边界约束，视口可拖拽顶点 |
| `Camera_Design.md` | 系统设计文档 + 配置指南 |

**相机状态优先级**

```
Priority 1 ▶ Dash          冲刺：无延迟 1:1 跟随
Priority 2 ▶ CombatFocus   战斗中 + 附近有敌：向质心偏移
Priority 3 ▶ CombatSearch  战斗中 + 敌人不在范围：向最近敌人方向偏移
Priority 4 ▶ PickupFocus   整理阶段 + 拾取物存在：向拾取物偏移
Priority 5 ▶ LookAhead     玩家移动：前瞻偏移（随时间增强）
Priority 6 ▶ FocusCharacter 静止：慢速回归玩家中心
```

**修改的文件**

| 文件 | 修改内容 |
|------|----------|
| `YogGameMode` | 敌人注册表：`RegisterEnemy` / `UnregisterEnemy` / `GetEnemyCentroid` / `GetNearbyEnemies` |
| `EnemyCharacterBase` | BeginPlay / Die 自动注册 / 注销 GameMode 敌人表 |
| `GA_PlayerDash` | Activate / End 调用 `SetDashMode(true/false)` |
| `YogPlayerControllerBase` | 新增 `Input_CameraLook` 手柄右摇杆绑定 |

---

### 1.3 未提交变更（当前工作区）

- `YogPlayerControllerBase.cpp` +4 行
- `BackpackScreenWidget.cpp` +24 行

---

## 二、蓝图配置任务

### 任务 A — 创建并接入 BP_YogCameraPawn【P0】

> 阻塞：相机系统完全不生效

**Step 1：创建蓝图**

1. 内容浏览器 > 右键 > Blueprint Class
2. 父类搜索选 `YogCameraPawn`
3. 命名 `BP_YogCameraPawn`，保存到 `Content/Code/Camera/`

**Step 2：配置 BP_YogPlayerController**

1. 打开 `BP_YogPlayerController`
2. Details > `Camera Setting`：
   - `Camera Pawn Class` → `BP_YogCameraPawn`
3. Details > `Input`：
   - `Input Camera Look` → `IA_CameraLook`（见 Step 3）

**Step 3：创建 IA_CameraLook InputAction**

1. 内容浏览器 > 右键 > Input > Input Action
2. 命名 `IA_CameraLook`，保存到 `Content/Input/`
3. 打开，`Value Type` 设为 **Axis2D (Vector2D)**
4. 保存

**Step 4：在 IMC 绑定右摇杆**

1. 打开 `IMC_Default`（或当前使用的 IMC）
2. 点 `+`，选 `IA_CameraLook`
3. 映射选 **Gamepad Right Thumbstick 2D-Axis**
4. 无需额外修改器，保存

**Step 5：验证相机 Spawn**

- 进入 PIE，确认有相机跟随玩家
- 若没有，打开 `BP_YogPlayerController` > BeginPlay，确认存在节点：

```
Event BeginPlay → SpawnCameraPawn(GetControlledCharacter())
```

---

### 任务 B — 关卡中放置边界约束 Actor【P1】

> 阻塞：相机可能越出地图边界

1. 内容浏览器搜索 `CameraConstraintActor`，拖入关卡
2. 选中 > Details > **Camera Constraint** > **Boundary Points**
3. 点 `+` 添加 4~6 个顶点，或在视口中拖拽黄色控制柄
4. 围住地图可玩区域，Z 值无影响
5. 每个关卡只放**一个**

---

### 任务 C — 创建 WBP_RuneTooltip 蓝图【P0】

> 阻塞：符文悬停无反应

1. 内容浏览器 > 右键 > User Interface > Widget Blueprint
2. 父类选 `RuneTooltipWidget`（C++ 基类）
3. 命名 `WBP_RuneTooltip`，保存到 `Content/UI/Backpack/`
4. 实现蓝图事件：
   - `Event Show Rune Info(RuneDA)`：读取名称 / 描述 / 图标并更新显示
   - `Event Hide Tooltip`：隐藏 Widget
5. 在 `WBP_BackpackScreen` 和 `WBP_LootSelection` Details 中，将 `Tooltip Widget Class` 指向 `WBP_RuneTooltip`

---

### 任务 D — WBP_LootSelection 接入 D-Pad 导航【P0】

> 阻塞：手柄无法操作三选一

C++ 层 `NativeOnKeyDown` 已实现，蓝图只需：

1. 打开 `WBP_LootSelection`
2. 确认三个选项容器变量名与 C++ 中 `OptionWidgets[]` 对应
3. Graph 中绑定 `OnHighlightChanged` 事件 → 更新高亮视觉（描边 / 背景色切换）
4. 绑定 `OnOptionConfirmed` 事件 → 触发符文选择逻辑

---

### 任务 E — 相机震动蓝图（可选）【P2】

1. 右键 > Blueprint Class > 父类选 `MatineeCameraShake`
2. 制作两个蓝图：`BP_CameraShake_HeavyHit`、`BP_CameraShake_CritHit`
3. 打开 `BP_YogCameraPawn` > Details：
   - `Heavy Hit Shake Class` → `BP_CameraShake_HeavyHit`
   - `Crit Hit Shake Class` → `BP_CameraShake_CritHit`
4. 受击逻辑（GE / AnimNotify）中调用：
   ```
   PlayerCharacter → GetOwnCamera → NotifyHeavyHit()
   PlayerCharacter → GetOwnCamera → NotifyCritHit()
   ```

---

## 三、优先级总览

| 优先级 | 任务 | 阻塞内容 |
|--------|------|----------|
| P0 | A — BP_YogCameraPawn 创建 + PlayerController 接入 | 相机系统完全不生效 |
| P0 | A Step 3/4 — IA_CameraLook + IMC 绑定右摇杆 | 手柄右摇杆偏移无效 |
| P0 | C — WBP_RuneTooltip 蓝图 | 悬停提示无反应 |
| P0 | D — WBP_LootSelection D-Pad 事件绑定 | 手柄三选一无法操作 |
| P1 | B — CameraConstraintActor 放置 + 顶点配置 | 相机可能越出地图 |
| P2 | E — 相机震动蓝图 | 暴击 / 重伤无震动（纯表现） |

---

## 四、相关文档

| 文档 | 说明 |
|------|------|
| [Camera_Design.md](../Systems/Camera_Design.md) | 相机系统完整设计 + 参数说明 |
| [BackpackGamepadAndUI.md](../BackpackGamepadAndUI.md) | 背包手柄交互流程 + 键位映射 |
