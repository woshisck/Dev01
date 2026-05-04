# ImGui 实时日志窗口 — 使用指南

> 项目：星骸降临 Dev01
> 系统版本：2026-05-05
> 适用人群：程序 + 策划（调试阶段）
> 配套文档：[GMCommands_Guide](../GMCommands_Guide.md)

---

## 一、系统概述

在 PIE / 打包版本中，实时捕获引擎 `UE_LOG` 输出（`Warning` / `Error` / `Display` 级别），
并以 ImGui 悬浮窗的形式在游戏画面上叠加显示。无需打开 Output Log，直接在游戏内过滤和查看关键警告。

```
┌─────────────────────────────────────────────────────────────────┐
│  UE_LOG(LogTemp, Warning, TEXT("[ComboRuntime] ..."))           │
│  UE_LOG(LogTemp, Error,   TEXT("[CDC][EndPlay] ..."))           │
│  UE_LOG(LogAbilitySystem, Warning, TEXT("GAS: ..."))            │
│           │                                                      │
│           ▼  FLogOutputDevice::Serialize()（任意线程）           │
│      CircularBuffer [512 条，mutex 保护]                        │
│           │                                                      │
│           ▼  FImGuiDelegates::OnMultiContextDebug()（游戏线程）  │
│      ImGui 悬浮窗（过滤 / 着色 / 自动滚动）                      │
└─────────────────────────────────────────────────────────────────┘
```

**捕获范围**

| 级别 | 说明 | 默认显示 |
|------|------|----------|
| `Error` / `Fatal` | 红色 `[ERR]` | ✓ |
| `Warning` | 黄色 `[WRN]` | ✓ |
| `Display` | 灰色 `[DSP]` | 关（可手动开启） |
| `Log` 及以下 | 不捕获 | — |

---

## 二、快速启动

### 2.1 开启 ImGui 输入模式

窗口始终渲染，但默认处于**只读**状态（鼠标/键盘输入仍传给游戏）。
需要点击过滤器或按钮时，先通过控制台开启 ImGui 输入：

```
~  →  ImGui.InputEnabled 1
```

关闭：

```
~  →  ImGui.InputEnabled 0
```

> **快捷键**：UnrealImGui 插件默认将 `F1` 映射为切换 ImGui 输入；
> 可在 **Project Settings → Plugins → ImGui → Input** 中修改。

### 2.2 窗口位置

首次启动自动定位于屏幕左上角（坐标 `20, 20`），尺寸 `980 × 440`。
可用鼠标拖拽标题栏任意移动，也可拖拽边框调整大小，下次打开 PIE 恢复到首次位置。

---

## 三、窗口界面说明

```
┌─ Yog Debug Log ─────────────────────────────────────────────────┐
│ [Clear] [Auto-scroll ✓]  |  [✓ Error] [✓ Warning] [  Display]  (23/512)│
│ [___filter text___]  Filter  [All][ComboRuntime][CDC][GAS][Damage][AI][Wave]│
│ ─────────────────────────────────────────────────────────────── │
│ [WRN]    1.23s  [ComboRuntime] No combo node for input=Light ... │
│ [ERR]    4.56s  [CDC][EndPlay] Ability task still active on ...  │
│ [WRN]    7.89s  GAS: CancelAbilities called with empty container │
└─────────────────────────────────────────────────────────────────┘
```

| 控件 | 功能 |
|------|------|
| **Clear** | 清空当前缓冲区所有日志条目 |
| **Auto-scroll** | 开启时每帧自动滚动到最新条目；关闭后可自由滚动回溯历史 |
| **Error / Warning / Display** | 按日志级别显示/隐藏条目（不影响捕获，仅过滤显示） |
| **(N/512)** | 当前缓冲条目数 / 最大容量 |
| **Filter 输入框** | 文本子串过滤，不区分大小写，空则显示全部 |
| **Preset 按钮** | 一键填入常用过滤关键词（当前激活 Preset 高亮显示） |

---

## 四、过滤 Preset 速查

| Preset | 填入字符串 | 过滤目标 |
|--------|-----------|----------|
| All | `（空）` | 显示全部 |
| ComboRuntime | `[ComboRuntime]` | 连击运行时：节点激活失败、无效节点、蒙太奇激活 |
| CDC | `[CDC]` | CombatDeckComponent：卡牌触发、EndPlay 残留 |
| GAS | `GAS` | Gameplay Ability System 内部警告 |
| Damage | `Damage` | 伤害管道：GE 应用、属性计算、击杀广播 |
| AI | `[AI]` | 敌人 AI：BT 决策、感知、移动 |
| Wave | `[Wave]` | 波次生成：Budget 计算、Spawn 失败 |

> **自定义过滤**：直接在输入框输入任意子串，例如 `node=` 可筛出所有含节点 ID 的日志；
> `montageConfig=None` 可快速定位蒙太奇配置缺失问题。

---

## 五、常用调试场景

### 场景 A：连击链断掉，不知道哪步出错

```
Preset → ComboRuntime
开启 Auto-scroll，正常操作连击
```

窗口会实时显示每次节点查找、激活结果和失败原因，例如：

```
[WRN]  [ComboRuntime] No combo node for input=Light current=Root graph=DA_THSword_Combo
[WRN]  [ComboRuntime] Failed to activate node=L1 input=Light montageConfig=None
```

---

### 场景 B：GAS 技能状态异常（无法激活 / 意外取消）

```
Preset → GAS
```

捕获 `LogAbilitySystem` 的所有 Warning，包括：
- 激活条件不满足（缺少 Tag / 冷却中）
- `CancelAbility` 被意外触发
- `EndAbility` 重复调用

---

### 场景 C：关卡切换后组件残留报错

```
Preset → CDC
```

切换关卡时出现 `[CDC][EndPlay]` 类错误，可在此处看到是哪个 Component 没有在 EndPlay 前清理。

---

### 场景 D：伤害数值异常（偏高 / 偏低 / 不触发）

```
Filter → Damage
开启 Display 显示（伤害详情通常以 Display 级别输出）
```

---

### 场景 E：多系统同时调试

过滤框支持任意子串，可以组合关键词：

```
Filter → [ComboRuntime]   →  只看连击
Filter → Damage           →  只看伤害
Filter → （空）            →  全部日志，开着 Auto-scroll 观察全局行为
```

---

## 六、扩展指南

### 6.1 新增 Preset 按钮

打开 `Source/DevKit/Private/Debug/YogDebugLogWindow.cpp`，
在 `DrawImGui()` 的 Preset 区块中追加一行 `Preset(...)` 调用：

```cpp
Preset("All",         "");
Preset("ComboRuntime","[ComboRuntime]");
Preset("CDC",         "[CDC]");
// ↓ 新增：过滤 BuffFlow 节点日志
Preset("BuffFlow",    "[BFNode]");
```

重新编译即可，不需要其他配置。

### 6.2 修改捕获的日志级别

默认只捕获 `Warning` / `Error` / `Display`。
若需捕获 `Log` 级别，修改 `FLogOutputDevice::Serialize`：

```cpp
// 当前
if (Verbosity > ELogVerbosity::Display)
    return;

// 改为：捕获 Log 及以上
if (Verbosity > ELogVerbosity::Log)
    return;
```

> **注意**：`Log` 级别日志量极大（每帧数十条），会快速填满 512 条缓冲区。
> 建议只在定向排查时临时开启，排查后还原。

### 6.3 增大缓冲区容量

```cpp
// YogDebugLogWindow.h，FLogOutputDevice 内
static constexpr int32 MaxEntries = 512;  // 改为所需值
```

---

## 七、C++ 实现细节

### 文件

| 文件 | 路径 |
|------|------|
| 头文件 | `Source/DevKit/Public/Debug/YogDebugLogWindow.h` |
| 实现 | `Source/DevKit/Private/Debug/YogDebugLogWindow.cpp` |
| Build 配置 | `Source/DevKit/DevKit.Build.cs`（已添加 `"ImGui"` 依赖） |

### 关键类与接口

| 类 / 接口 | 说明 |
|-----------|------|
| `UYogDebugLogWindow` | `UGameInstanceSubsystem` 子类，随 GameInstance 自动创建销毁 |
| `FLogOutputDevice` | 内部 `FOutputDevice` 子类，挂载到 `GLog`，线程安全捕获日志 |
| `GLog->AddOutputDevice` | `Initialize()` 时注册 |
| `GLog->RemoveOutputDevice` | `Deinitialize()` 时注销 |
| `FImGuiDelegates::OnMultiContextDebug()` | ImGui 每帧绘制回调，与 World 无关（跨关卡保持） |
| `FCriticalSection` | 保护 `Entries` 数组的读写互斥（Serialize 可在任意线程调用） |

### 生命周期

```
GameInstance::Init()
    └─ UYogDebugLogWindow::Initialize()
           ├─ GLog->AddOutputDevice(LogDevice)   ← 开始捕获
           └─ OnMultiContextDebug.AddUObject()   ← 开始绘制

PIE 结束 / 游戏退出
    └─ UYogDebugLogWindow::Deinitialize()
           ├─ OnMultiContextDebug.Remove()       ← 停止绘制
           └─ GLog->RemoveOutputDevice(LogDevice)← 停止捕获
```
