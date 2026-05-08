# Yog Rune Flow 编辑器技术文档

> 适用范围：`Tools > DevKit Data > Rune Editor` 通用符文/技能流程编辑器  
> 适用人群：策划 / 程序  
> 配套文档：[RuneLogic_Complete_Guide](RuneLogic_Complete_Guide.md)、[FA_UniversalArchitecture](FA_UniversalArchitecture.md)、[BuffFlow_NodeUsageGuide](BuffFlow_NodeUsageGuide.md)  
> 最后更新：2026-05-08

---

## 概述

`Rune Editor` 当前已经从 `512 MVP` 专用预览页，重构为通用的 `Yog Rune Flow` 编辑器。

它负责把一个符文拆成两层：

| 层级 | 资产 | 作用 |
|---|---|---|
| 数据层 | `URuneDataAsset` | 名称、Tag、分类、摘要、数值表、关联流程资产 |
| 流程层 | `UYogRuneFlowAsset` | 蓝图风格 Flow 图，使用 Yog 专用节点描述运行逻辑 |

策划主要在一个窗口里完成三件事：

1. 在左侧管理符文资源。
2. 在中间 `数值表` 填写伤害、持续时间、概率、半径等参数。
3. 在中间 `流程图` 连接节点，右侧编辑选中节点属性。

---

## 一、代码入口

### 1.1 编辑器入口

```
UE 主菜单
  └─ Tools
      └─ DevKit Data
          └─ Rune Editor
              └─ SRuneEditorWidget
```

| 文件 | 职责 |
|---|---|
| [`Source/DevKitEditor/DevKitEditor.cpp`](../../../Source/DevKitEditor/DevKitEditor.cpp) | 注册 `DevKitRuneEditor` Nomad Tab，并在 `Tools > DevKit Data` 菜单加入入口 |
| [`Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h`](../../../Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h) | 编辑器 Slate Widget 状态、Tab、筛选枚举、按钮回调声明 |
| [`Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp`](../../../Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp) | 左侧资源栏、中间数值/流程页、底部面板、右侧详情栏的 UI 与交互 |

### 1.2 主窗口布局

```
SRuneEditorWidget
  ├─ 顶部工具条
  │   ├─ 运行符文
  │   ├─ 打开符文
  │   ├─ 打开流程
  │   └─ 刷新
  ├─ 左侧：资源管理
  │   ├─ 新建符文
  │   ├─ 复制 / 粘贴 / 重命名 / 删除 / 定位
  │   └─ 符文资源列表 + 分类筛选
  ├─ 中间：当前符文流程
  │   ├─ 数值表
  │   └─ 流程图
  │       ├─ SGraphEditor 蓝图风格图表
  │       └─ 底部面板：节点库 / 校验 / 运行日志 / 选中节点
  └─ 右侧：详情
      ├─ 基础信息
      ├─ 触发与运行
      ├─ 节点属性 DetailsView
      └─ 界面摘要
```

---

## 二、资产模型

### 2.1 `URuneDataAsset`

`URuneDataAsset` 是符文主资产，编辑器左侧只显示默认根目录下的符文：

```text
/Game/YogRuneEditor
  ├─ Runes
  │   └─ DA_Rune_xxx
  └─ Flows
      └─ FA_Rune_xxx
```

核心字段在 [`Source/DevKit/Public/Data/RuneDataAsset.h`](../../../Source/DevKit/Public/Data/RuneDataAsset.h)：

| 字段 | 说明 | 当前编辑器行为 |
|---|---|---|
| `RuneConfig.RuneName` | 符文显示名 | 右侧基础信息可编辑 |
| `RuneConfig.RuneIdTag` | 符文身份 Tag，推荐 `Rune.ID.*` | 右侧基础信息可编辑；保存时会尝试写入 GameplayTag ini |
| `RuneConfig.LibraryTags` | 资源筛选分类，命名空间 `Rune.Library.*` | 右侧分类按钮写入；左侧筛选读取 |
| `RuneConfig.RuneType` | 增益 / 减益 / 无 | 当前只显示，不在专用 UI 中编辑 |
| `RuneConfig.Rarity` | 普通 / 稀有 / 史诗 / 传说 | 当前只显示，不在专用 UI 中编辑 |
| `RuneConfig.TriggerType` | 被动、命中、冲刺、击杀等触发方式 | 当前只显示，不在专用 UI 中编辑 |
| `RuneConfig.TuningScalars` | 符文数值表 | 中间 `数值表` 可编辑主要字段 |
| `Flow.FlowAsset` | 关联 `UYogRuneFlowAsset` | 新建符文时自动创建并关联 |

### 2.2 `UYogRuneFlowAsset`

`UYogRuneFlowAsset` 继承自 `UFlowAsset`，显示名为 `Yog Rune Flow Graph`。

| 文件 | 职责 |
|---|---|
| [`Source/DevKit/Public/BuffFlow/YogRuneFlowAsset.h`](../../../Source/DevKit/Public/BuffFlow/YogRuneFlowAsset.h) | 定义专用 Flow Asset 类型 |
| [`Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp`](../../../Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp) | 在构造函数里限制 `AllowedNodeClasses` |

这个类的关键价值是让右键搜索菜单干净：新建的 FA 只允许 `Start` 节点、Yog 包装节点和必要 Flow 基础节点，不会混入旧 512 节点目录。

---

## 三、资源管理流程

### 3.1 新建符文

按钮入口：左侧 `新建符文`。

调用链：

```
SRuneEditorWidget::OnCreateRuneClicked()
  └─ FRuneEditorAuthoring::CreateRuneAuthoringAssets()
      ├─ 创建 UYogRuneFlowAsset：/Game/YogRuneEditor/Flows/FA_Rune_xxx
      ├─ 创建 URuneDataAsset：/Game/YogRuneEditor/Runes/DA_Rune_xxx
      ├─ EnsureGameplayTag(Rune.ID.xxx)
      ├─ RuneInfo.Flow.FlowAsset = FlowAsset
      ├─ MarkPackageDirty()
      └─ 刷新左侧资源列表并选中新符文
```

相关文件：

| 文件 | 关键函数 |
|---|---|
| [`RuneEditorAuthoring.h`](../../../Source/DevKitEditor/Private/RuneEditor/RuneEditorAuthoring.h) | `FRuneEditorCreateRuneRequest`、`FRuneEditorCreateRuneResult` |
| [`RuneEditorAuthoring.cpp`](../../../Source/DevKitEditor/Private/RuneEditor/RuneEditorAuthoring.cpp) | `CreateRuneAuthoringAssets()`、`EnsureGameplayTag()` |

### 3.2 复制 / 粘贴

复制按钮只记录当前选中资源引用，粘贴时执行真实复制。

当前行为：

| 源资源 | 粘贴结果 |
|---|---|
| `URuneDataAsset` | 复制 DA；如果源 DA 有 Flow，也复制一份 Flow 并重新关联到新 DA |
| `UFlowAsset` | 复制 Flow 到默认 Flow 文件夹 |

### 3.3 重命名 / 删除 / 定位

| 操作 | 调用 |
|---|---|
| 重命名 | `AssetTools.RenameAssets()` |
| 删除 | `ObjectTools::DeleteObjects()` |
| 定位 | `ContentBrowser.SyncBrowserToAssets()` |

注意：当前 UI 删除选中的单个资源。如果删除 DA，关联的 Flow 不会自动一起删除，需要后续补“删除符文时同时询问是否删除关联 Flow”。

---

## 四、资源分类和筛选

### 4.1 分类数据

资源分类使用 `RuneConfig.LibraryTags`，不是单独的 Card ID。

当前已有 GameplayTag：

| 分类 | Tag | 用途 |
|---|---|---|
| 基础节点 | `Rune.Library.Base` | 燃烧、中毒、流血等可复用基础符文 |
| 敌人 | `Rune.Library.Enemy` | 敌人固有能力或敌人 Buff |
| 关卡 | `Rune.Library.Level` | 关卡全局 Buff、房间规则 |
| 终结技 | `Rune.Library.Finisher` | 终结技或高阶主动效果 |
| 连携卡牌 | `Rune.Library.ComboCard` | 连携卡牌相关符文 |

定义位置：[`Config/DefaultGameplayTags.ini`](../../../Config/DefaultGameplayTags.ini)

### 4.2 UI 筛选逻辑

左侧资源列表筛选逻辑在 `DoesRuneMatchResourceFilter()`。

优先级：

1. 如果选中 `全部`，显示所有 `/Game/YogRuneEditor` 下的 `URuneDataAsset`。
2. 如果符文 `LibraryTags` 精确包含筛选 Tag，显示。
3. 如果没有分类 Tag，使用资产名、显示名、Tag、摘要、分类文本做关键词 fallback。

这让旧资源在未补分类时也可能被搜索到，但正式制作建议统一填写 `LibraryTags`。

---

## 五、数值表设计

### 5.1 目标

数值表希望尽量让一个符文只维护一张参数表，流程图节点通过 Key 引用数值。

典型 Key：

| Key | 说明 | 示例 |
|---|---|---|
| `Damage` | 主伤害 | `35` |
| `BurnDPS` | 燃烧每秒伤害 | `8` |
| `Duration` | 持续时间 | `5` |
| `Radius` | 范围半径 | `360` |
| `Chance` | 触发概率 | `0.25` |
| `StackMax` | 最大层数 | `5` |
| `Cooldown` | 冷却 | `8` |

### 5.2 数据结构

`FRuneTuningScalar` 定义在 [`RuneDataAsset.h`](../../../Source/DevKit/Public/Data/RuneDataAsset.h)。

| 字段 | 说明 | UI 状态 |
|---|---|---|
| `Key` | 节点引用用的稳定键 | 数值表可编辑 |
| `DisplayName` | 策划可读显示名 | 数值表可编辑 |
| `Category` | 数值分类，例如 `伤害`、`持续`、`范围` | 数值表可编辑 |
| `ValueSource` | 数值来源：具体值 / 公式 / MMC / 上下文 | 数值表按钮循环切换 |
| `Value` | 固定值，也是其他模式的 fallback | 数值表可编辑 |
| `FormulaExpression` | 公式文本 | 数值表可编辑 |
| `ContextKey` | 外部上下文 Key | 数值表可编辑 |
| `MagnitudeCalculationClass` | 自定义计算类 | 当前数值表未暴露 class picker，可在 DA 详情中编辑 |
| `MinValue` / `MaxValue` | 范围提示与校验 | 当前数值表未暴露；校验会检查 `Max < Min` |
| `UnitText` | 单位 | 数值表可编辑 |
| `Description` | 说明 | 数值表可编辑 |
| `ValueTag` | SetByCaller 或数据标签扩展 | 当前数值表未暴露 |

### 5.3 四种数值方式

| 方式 | 当前行为 | 适合场景 |
|---|---|---|
| 具体值 | 直接返回 `Value` | 固定伤害、固定持续时间 |
| 公式 | 用 `FBasicMathExpressionEvaluator` 计算 `FormulaExpression` | 随符文等级成长 |
| MMC | 调用 `URuneValueCalculation::CalculateValue()` | 复杂逻辑，需要蓝图或 C++ 自定义 |
| 上下文 | 当前返回 `Value` fallback | 后续接入配置表、战斗上下文、外部数据源 |

公式当前支持的内置变量：

| 变量 | 来源 |
|---|---|
| `Level` | `RuneInfo.Level` |
| `UpgradeLevel` | `RuneInfo.UpgradeLevel` |
| `Value` | 当前行 `Value` |

示例：

```text
Value + UpgradeLevel * 3
20 + Level * 2
Value * (1 + UpgradeLevel * 0.15)
```

### 5.4 运行时读取

通用读取链路：

```
Flow 节点
  └─ UBuffFlowComponent::GetRuneTuningValueForFlow(FlowAsset, Key, DefaultValue)
      └─ GetActiveSourceRuneData(FlowAsset)
          └─ URuneDataAsset::GetRuneTuningValue(Key, DefaultValue)
              ├─ Literal：Value
              ├─ Formula：EvaluateRuneTuningFormula()
              ├─ MMC：URuneValueCalculation::CalculateValue()
              └─ Context：Value fallback
```

已经接入 `GetRuneTuningValueForFlow()` 的节点路径：

| 节点/逻辑 | 用法 |
|---|---|
| `BFNode_ApplyRuneEffectProfile` | `SetByCallerValues[*].TuningKey` 覆盖 GE SetByCaller 数值 |
| `BFNode_SpawnRuneProjectileProfile` | `DamageTuningKey` 覆盖投射物伤害 |
| `UBuffFlowComponent` | 保存 Flow 与 Source Rune 的映射 |

当前发现的问题：

| 问题 | 影响 | 建议 |
|---|---|---|
| `BFNode_GetRuneTuningValue` 当前直接输出 `Scalar.Value`，没有走 `GetRuneTuningValue()` | 使用该节点时公式 / MMC 不生效 | 后续应改为 `SourceRune->GetRuneTuningValue(Key, DefaultValue)` |
| `Context` 模式尚未接入运行时上下文 | 目前等价于返回 fallback `Value` | 后续补 `FRuneRuntimeContext` 或表格查询服务 |
| 数值表 UI 未暴露 `MagnitudeCalculationClass`、`MinValue`、`MaxValue`、`ValueTag` | 高级配置需要打开原始 DA 详情面板 | 后续可加高级列或折叠面板 |

---

## 六、流程图与节点库

### 6.1 图表编辑

流程图使用 `SGraphEditor` 直接编辑 `FlowAsset->GetGraph()`。

| 操作 | 行为 |
|---|---|
| 右键空白区域 | 使用 Flow 原生 Action Menu 添加允许节点 |
| 从引脚拖线 | 使用 Flow 原生连线和搜索 |
| 底部节点库点击 `添加` | 调用 `FRuneEditorFlowAuthoring::AddNodeAfter()` |
| Delete 键 | 编辑器自定义删除逻辑，避免旧 FlowEditor 选区崩溃 |
| 选中节点 | 右侧 `节点属性` 显示 Flow 节点 DetailsView |

### 6.2 添加节点流程

```
节点库点击添加
  └─ SRuneEditorWidget::OnAddNodeFromLibrary()
      └─ FRuneEditorFlowAuthoring::AddNodeAfter()
          ├─ FlowAsset->IsNodeOrAddOnClassAllowed()
          ├─ FFlowGraphSchemaAction_NewNode::CreateNode()
          ├─ 尝试接到当前选中节点第一个输出引脚
          ├─ FlowAsset->HarvestNodeConnections()
          ├─ FlowAsset->PostEditChange()
          └─ Graph->NotifyGraphChanged()
```

### 6.3 节点分类

Yog 节点定义在 [`YogFlowNodes.h`](../../../Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h)。

| 分类 | 当前节点 |
|---|---|
| 技能 | 流程控制、造成伤害时、受到伤害时、暴击时、击杀时、冲刺时、等待事件 |
| 效果节点 | 伤害、治疗、消耗、属性修改、施加状态、效果配置、范围施加 GE、范围伤害、添加 Tag、移除 Tag、授予能力 |
| 任务节点 | 搜索目标、结束技能、动画 |
| 生成节点 | 生成投射物配置、生成区域配置、生成地面路径、生成远程弹幕、生成斩击波 |
| 条件节点 | 属性比较、拥有 Tag、概率判断、只执行一次、距离判断 |
| 表现节点 | Niagara 特效、Cue 到角色、Cue 到位置、VFX 配置、序列帧特效 |
| 生命周期 | 延迟、结束符文 |

节点本身多数是对已有 BuffFlow 节点的 Yog 包装类：

```text
UYogFlowNode_EffectDamage
  └─ UBFNode_DoDamage

UYogFlowNode_EffectApplyProfile
  └─ UBFNode_ApplyRuneEffectProfile

UYogFlowNode_SpawnProjectileProfile
  └─ UBFNode_SpawnRuneProjectileProfile
```

这样可以保留旧运行能力，同时让新编辑器菜单更干净。

---

## 七、运行和校验

### 7.1 校验

底部 `校验` 页调用 [`RuneEditorValidation.cpp`](../../../Source/DevKitEditor/Private/RuneEditor/RuneEditorValidation.cpp)。

当前检查项：

| 检查 | 严重级别 |
|---|---|
| 未选择符文 | 错误 |
| 符文名称为空 | 错误 |
| `RuneIdTag` 缺失或未加载 | 错误 |
| 缺少 `FlowAsset` | 错误 |
| Flow 没有 Graph | 错误 |
| Flow 没有节点 | 错误 |
| 缺少入口节点 | 错误 |
| 入口后没有可抵达运行节点 | 警告 |
| 存在无法从入口抵达的节点 | 警告 |
| 数值表 Key 为空 | 警告 |
| 数值表 Key 重复 | 警告 |
| `MaxValue < MinValue` | 警告 |

### 7.2 运行

顶部 `运行符文` 用于编辑器内快速试跑。

调用链：

```
SRuneEditorWidget::OnRunRuneClicked()
  └─ FRuneEditorAuthoring::RunRuneOnSelectedActor()
      ├─ 获取当前选中 Rune
      ├─ 获取 Rune.Flow.FlowAsset
      ├─ 获取编辑器当前选中的 Actor
      ├─ 检查 Actor 是否有 BuffFlowComponent
      └─ BuffFlowComponent->StartBuffFlowWithRune(FlowAsset, NewGuid, Rune, Actor, true)
```

运行前提：

| 前提 | 原因 |
|---|---|
| 当前有选中的符文 | 需要 Source Rune 提供数值表 |
| 符文有关联 FlowAsset | FlowSubsystem 运行的是 Flow |
| 编辑器中选中了 Actor | 运行目标来自当前选择 |
| Actor 有 `UBuffFlowComponent` | BuffFlow 运行入口 |
| 世界有 GameInstance | `UFlowSubsystem` 通过 GameInstance 获取 |

运行反馈写入底部 `运行日志` 页。

---

## 八、512 符文拆解建议

基于当前节点库，512 版本常见符文可以先按以下模式落地。

| 符文方向 | 推荐节点组合 | 数值表 Key |
|---|---|---|
| 攻击强化 | `流程控制` → `属性修改` | `AttackBonus`、`Duration` |
| 燃烧攻击 | `造成伤害时` → `概率判断` → `施加状态` → `Niagara特效` | `Chance`、`BurnDPS`、`Duration` |
| 中毒攻击 | `造成伤害时` → `施加状态` | `PoisonDPS`、`Duration`、`StackMax` |
| 月光攻击 | `造成伤害时` → `生成投射物配置` 或 `生成斩击波` → `Cue到位置` | `Damage`、`ProjectileCount`、`Speed` |
| 反向月光攻击 | `受到伤害时` 或 `等待事件` → `生成投射物配置` | `Damage`、`Angle`、`Count` |
| 穿透攻击 | `造成伤害时` → `搜索目标` → `伤害` 或 `生成远程弹幕` | `Damage`、`PierceCount`、`Range` |
| 减伤 | `受到伤害时` → `属性修改` 或 `施加状态` | `DamageReduction`、`Duration` |
| 护盾 | `流程控制` → `属性修改` / `施加状态` | `ShieldValue`、`Duration` |

建议优先使用 `数值表 Key` 控制可调参数，不要把伤害、持续时间直接写死在节点属性里。

---

## 九、后续待补

| 优先级 | 内容 | 说明 |
|---|---|---|
| 高 | 修正 `BFNode_GetRuneTuningValue` | 让它走 `GetRuneTuningValue()`，支持公式 / MMC |
| 高 | 右侧基础信息可编辑类型、品质、触发方式 | 当前只显示；后续应做下拉或分段按钮 |
| 高 | 数值表高级字段 UI | 暴露 `MMC Class`、`Min/Max`、`ValueTag` |
| 中 | Context 数值来源 | 接入配置表或运行时上下文 |
| 中 | 删除 DA 时联动关联 Flow | 防止产生孤儿 Flow |
| 中 | 数值表导入 / 导出 | 方便策划批量调整 |
| 中 | 节点模板 | 一键创建燃烧、中毒、月光等基础链路 |
| 低 | 资源列表异步加载 | 当前 `GetAllRuneDAs()` 同步加载，资产量很大时可能卡编辑器 |

---

## 十、快速验收清单

| 验收项 | 预期 |
|---|---|
| 打开 `Tools > DevKit Data > Rune Editor` | 出现中文 `符文编辑器` 页面 |
| 新建符文 | 自动创建 `DA_Rune_xxx` 和 `FA_Rune_xxx` |
| 左侧分类按钮 | 能按 `Rune.Library.*` 过滤资源 |
| 右侧分类按钮 | 能写入当前符文的 `LibraryTags` |
| 切到 `数值表` | 能新增、编辑、删除数值行 |
| 切到 `流程图` | 能右键或通过节点库添加 Yog 节点 |
| 选中 Flow 节点 | 右侧 `节点属性` 显示该节点属性 |
| 底部 `校验` | 能显示错误、警告、信息 |
| 顶部 `运行符文` | 选中带 `BuffFlowComponent` 的 Actor 后能调用 `StartBuffFlowWithRune()` |

