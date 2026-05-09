# 开发方案：符文多层数据表架构

> 创建：2026-05-09  
> 关联：[RuneEditor_UserGuide.md](../Systems/Rune/RuneEditor_UserGuide.md)

---

## 需求描述

通过多张关联配置表实现符文/技能数据的模块化管理，遵循"面向对象、模块化、参数化"设计原则：

- **主表（触发层）**：技能基础信息、触发时机、消耗、持续时间、状态效果
- **数值层**：伤害计算、通用参数键名传递给 FA 节点
- **物理表现表**：飞行物速度、数量、碰撞、投射物类
- **空间数据表**：光环半径、场地形状、持续触发间隔
- **组件化开关**：数据模块不必全部启用，由设计师按需勾选

---

## 现状分析

### 已有的机制（无需重建）

| 功能 | 现有实现 | 位置 |
|---|---|---|
| 主表（触发层）| `FRuneConfig`（RuneName/Tag/TriggerType/GoldCost/Rarity 等） | `RuneDataAsset.h` |
| 数值层 | `TArray<FRuneTuningScalar>` + `BFNode_GetRuneTuningValue` | `RuneDataAsset.h` |
| FA 通用参数传递 | `FFlowDataPinInputProperty_Float` 连线 + TuningKey 查询 | FlowGraph 插件 |
| 编辑器数值表视图 | `ValueTable` 标签页（已有 Category 字段） | `SRuneEditorWidget` |
| 战斗卡连携配方 | `ComboRecipe` 标签页 | `SRuneEditorWidget` |

### 缺少的机制（本次新增）

1. **模块开关**：没有"飞行物模块是否启用"这类总开关
2. **类型化模块字段**：飞行物速度/数量等只能靠 TuningScalar Key 传递，没有强类型结构体
3. **编辑器模块视图**：ValueTable 是平铺列表，无模块分组/折叠
4. **一键初始化**：启用某模块时无法自动预填标准键值行

---

## 架构分层设计

```
URuneDataAsset
├── 主表层（Base Layer）           ← 现有 FRuneConfig，基本不动
│   ├── 身份：RuneIdTag / RuneName / Icon / Rarity
│   ├── 触发：TriggerType / GoldCost / ChainRole
│   ├── 逻辑：FlowAsset / CombatCard / LinkRecipes
│   └── 描述：RuneDescription / HUDSummaryText
│
├── 数值层（Tuning Layer）         ← 现有 TArray<FRuneTuningScalar>，扩展 Category 组织
│   ├── [Base]       基础通用数值（Duration / CooldownTime 等）
│   ├── [Damage]     伤害参数（Damage / CritChance 等）
│   ├── [Projectile] 飞行物数值键（模块启用时自动添加）
│   ├── [Aura]       光环/场地数值键（模块启用时自动添加）
│   └── [Status]     状态效果数值键（Duration / StackLimit 等）
│
└── 模块层（Module Layer）         ← 新增
    ├── FRuneModuleFlags            — 哪些模块激活（多个 bool）
    ├── FRuneProjectileModule       — 飞行物类型化字段（bEnabled 保护）
    ├── FRuneAuraModule             — 光环/场地类型化字段
    └── FRuneStatusModule           — 状态效果类型化字段
```

---

## 方案设计

### A. RuneDA 新增结构体（C++）

#### FRuneModuleFlags（模块总开关）

```cpp
USTRUCT(BlueprintType)
struct FRuneModuleFlags
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Modules")
    bool bProjectile = false;   // 飞行物模块

    UPROPERTY(EditAnywhere, Category = "Modules")
    bool bAura = false;         // 光环/场地模块

    UPROPERTY(EditAnywhere, Category = "Modules")
    bool bStatus = false;       // 状态效果模块

    UPROPERTY(EditAnywhere, Category = "Modules")
    bool bChain = false;        // 连携增益模块（正向/反向）
};
```

#### FRuneProjectileModule（物理表现表）

```cpp
USTRUCT(BlueprintType)
struct FRuneProjectileModule
{
    GENERATED_BODY()

    // 投射物类 — 决定外观/碰撞形状
    UPROPERTY(EditAnywhere, Category = "Projectile")
    TSubclassOf<AMusketBullet> ProjectileClass;

    // 初始速度（cm/s）— 供参考；若FA节点已配置则以节点值为准
    UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "100.0"))
    float Speed = 1400.f;

    // 基础发射数量
    UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "1"))
    int32 Count = 1;

    // 散布锥角（度）
    UPROPERTY(EditAnywhere, Category = "Projectile",
        meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float ConeAngleDegrees = 0.f;

    // 穿透飞行（Sweep）— 开启后投射物进行扫描碰撞而非点碰撞
    UPROPERTY(EditAnywhere, Category = "Projectile")
    bool bSweepCollision = false;

    // 命中回调 Tag — 投射物命中后向源 ASC 发送的事件 Tag
    UPROPERTY(EditAnywhere, Category = "Projectile")
    FGameplayTag HitEventTag;
};
```

#### FRuneAuraModule（空间数据表）

```cpp
USTRUCT(BlueprintType)
struct FRuneAuraModule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Aura")
    ERuneGroundPathShape Shape = ERuneGroundPathShape::Rectangle;

    UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "1.0"))
    float Length = 520.f;

    UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "1.0"))
    float Width = 220.f;

    UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "1.0"))
    float Height = 120.f;

    UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "0.01"))
    float Duration = 3.f;

    UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "0.01"))
    float TickInterval = 1.f;

    UPROPERTY(EditAnywhere, Category = "Aura")
    TObjectPtr<UMaterialInterface> DecalMaterial;

    UPROPERTY(EditAnywhere, Category = "Aura")
    TObjectPtr<UNiagaraSystem> NiagaraSystem;
};
```

#### FRuneStatusModule（状态效果表）

```cpp
USTRUCT(BlueprintType)
struct FRuneStatusModule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Status")
    TSubclassOf<UGameplayEffect> StatusGE;

    UPROPERTY(EditAnywhere, Category = "Status", meta = (ClampMin = "0.0"))
    float Duration = 4.f;

    UPROPERTY(EditAnywhere, Category = "Status", meta = (ClampMin = "1"))
    int32 StackLimit = 3;

    UPROPERTY(EditAnywhere, Category = "Status")
    FGameplayTag StatusTag;
};
```

#### RuneDataAsset 追加字段

在 `FRuneConfig` 末尾追加：

```cpp
// 模块开关 — 控制哪些扩展模块对此符文有效
UPROPERTY(EditAnywhere, Category = "Modules")
FRuneModuleFlags ActiveModules;

// 飞行物模块（仅 bProjectile = true 时有意义）
UPROPERTY(EditAnywhere, Category = "Modules",
    meta = (EditCondition = "ActiveModules.bProjectile", EditConditionHides))
FRuneProjectileModule ProjectileModule;

// 光环/场地模块
UPROPERTY(EditAnywhere, Category = "Modules",
    meta = (EditCondition = "ActiveModules.bAura", EditConditionHides))
FRuneAuraModule AuraModule;

// 状态效果模块
UPROPERTY(EditAnywhere, Category = "Modules",
    meta = (EditCondition = "ActiveModules.bStatus", EditConditionHides))
FRuneStatusModule StatusModule;
```

---

### B. 编辑器新增 Modules 标签页

在 `SRuneEditorWidget` 的中间面板新增第 4 个标签页 **Modules**：

```
┌─────────────────────────────────────────────────┐
│  [ValueTable] [FlowGraph] [ComboRecipe] [Modules] │
├─────────────────────────────────────────────────┤
│  ☑ 飞行物模块 (Projectile)                        │
│  │  投射物类：[BP_Proj_Moonlight ▼]               │
│  │  速度：[1400]     数量：[1]                    │
│  │  锥角：[0°]       Sweep：[☐]                  │
│  │  命中Tag：[Event.Moonlight.Hit ▼]              │
│  │  [一键填充标准 Tuning Key]                     │
│  ──────────────────────────────────────────────  │
│  ☐ 光环/场地模块 (Aura)                          │
│  ──────────────────────────────────────────────  │
│  ☐ 状态效果模块 (Status)                         │
│  ──────────────────────────────────────────────  │
│  ☐ 连携增益模块 (Chain)                          │
└─────────────────────────────────────────────────┘
```

**"一键填充标准 Tuning Key" 按钮行为：**

| 模块 | 自动添加的 TuningScalar Key |
|---|---|
| Projectile | `Projectile.Speed` / `Projectile.Count` / `Projectile.ConeAngle` |
| Aura | `Aura.Length` / `Aura.Width` / `Aura.Duration` / `Aura.TickInterval` |
| Status | `Status.Duration` / `Status.StackLimit` |
| Chain | `Chain.ForwardMultiplier` / `Chain.BackwardMultiplier` |

---

### C. ValueTable 标签页增强

- 按 `Category` 字段分组折叠（已有字段，只需编辑器逻辑）
- 分组标题行：可展开/折叠，显示该分组的行数
- 已启用模块对应的 Category 高亮显示（浅蓝色背景）
- 未启用模块的 Category 行灰色显示（视觉提示）

---

### D. 数据流：表 → FA 节点（不变）

> **重要**：模块层的类型化字段供**编辑器参考和 BP 直接读取**；FA 节点仍通过**数值层的 Key 查询**获取运行时值。两者职责分离，不冲突。

```
模块字段（类型化）         数值层（Key-Value）          FA 节点
FRuneProjectileModule  →  TuningScalars              →  BFNode_GetRuneTuningValue
Speed: 1400            →  Key="Projectile.Speed"      →  OutputPin → SpawnProjectile.Speed
Count: 1               →  Key="Projectile.Count"      →  OutputPin → ProjectileCount
```

（可选）将来新增 `BFNode_GetProjectileModule` 节点直接读取类型化字段，跳过 Key 查询。

---

## 实现步骤

### 阶段一：C++ 结构体（约 2 小时）

1. **新建** `Source/DevKit/Public/Data/RuneModules.h`  
   定义 `FRuneModuleFlags`、`FRuneProjectileModule`、`FRuneAuraModule`、`FRuneStatusModule`

2. **修改** `Source/DevKit/Public/Data/RuneDataAsset.h`  
   `#include "Data/RuneModules.h"` + 在 `FRuneConfig` 末尾追加 4 个字段

3. **编译确认** — 只新增 UPROPERTY，现有 DA 数据不受影响

### 阶段二：编辑器 Modules 标签页（约 4 小时）

4. **`SRuneEditorWidget.h`** 新增 `ECenterPanelTab::Modules` + `BuildModulesPanel()` 声明

5. **`SRuneEditorWidget.cpp`**：
   - 中间面板 TabBar 添加第 4 个 Tab
   - 实现 `BuildModulesPanel()`：模块列表 + 展开详情
   - "一键填充标准 Key" 按钮回调：检查已有 Key，只插入缺少的行

6. **ValueTable 面板增强**：按 Category 分组，使用 `SExpandableArea` 折叠头部

### 阶段三：FA 读取节点（可选，后期）

7. **`BFNode_GetProjectileModule`** — 数据输出：Speed/Count/ConeAngle/ProjectileClass
8. **`BFNode_GetAuraModule`** — 数据输出：Length/Width/Height/Duration/TickInterval

---

## 涉及文件

| 文件 | 变更类型 | 说明 |
|---|---|---|
| `Source/DevKit/Public/Data/RuneModules.h` | **新建** | 4 个模块结构体定义 |
| `Source/DevKit/Public/Data/RuneDataAsset.h` | 修改（追加字段） | FRuneConfig 末尾加 ActiveModules + 3 个模块 |
| `Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h` | 修改 | 新增 Modules Tab 枚举值 + BuildModulesPanel 声明 |
| `Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp` | 修改 | Modules 面板 UI + ValueTable 分组增强 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetProjectileModule.h` | 新建（可选） | 飞行物模块读取节点 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_GetProjectileModule.cpp` | 新建（可选） | 同上实现 |

---

## 潜在风险

| 风险 | 影响 | 规避方式 |
|---|---|---|
| RuneDA 序列化兼容性 | 旧 DA 打开时新字段以默认值填充 | 只追加字段（不删、不改字段名），UE 自动填默认值 |
| 模块字段与 TuningScalar 重复 | 设计师可能在两处填写不同值 | 文档约定：类型化字段是配置参考，FA 运行时以 TuningScalar 为准 |
| 编辑器 Tab 增多 | Modules Tab 和 ValueTable Tab 有一定重叠 | Modules Tab 定位为"快速总览+一键初始化"，ValueTable 定位为"精细调参" |

---

## 待确认问题

1. **模块字段要不要直接参与 FA 执行？**  
   - 选项 A：只作参考，FA 仍读 TuningScalar Key（现有机制，零风险）  
   - 选项 B：新增 `BFNode_GetProjectileModule` 直接读类型化字段（阶段三）  
   - **建议**：先做 A，后期视需要加 B

2. **是否需要 Excel/CSV 导入？**  
   - 当前方案全部基于 DA，无 DataTable/CSV 导入  
   - 若需 Excel 协同编辑，需额外引入 `FTableRowBase` + DataTable 资产，工作量 ×2  
   - **建议**：独立开发阶段优先 DA 方案，量产阶段再评估

3. **Aura 模块字段是否与 `BFNode_SpawnRuneGroundPathEffect` 直接联动？**  
   - 目前 Aura 模块字段与 Ground Path 节点字段镜像，可加"同步到 FA"功能

4. **模块字段是否需要参与 `generate_runes.py` 脚本生成？**

---

## 不在本次范围内

- Excel 双向同步（CSV 导入导出）
- 运行时热更新数值
- 跨符文共享数值模板（现有 `UGenericRuneEffectDA` 可承担部分）
