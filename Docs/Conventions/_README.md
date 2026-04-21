# 生产规范总览

> **Claude 规则**：每次开始相关类型的编码任务前，必须先阅读对应规范文档。  
> **用户说明**：这些文档描述了 Claude 的编写习惯和分工边界，无需日常阅读。

---

## 📂 规范文档列表

| 文档 | 适用场景 | Claude 在以下情况下阅读 |
|---|---|---|
| [Material.md](Material.md) | 材质 / HLSL / .ush 文件 | 创建或修改任何材质相关代码 |
| [Widget.md](Widget.md) | UMG Widget / WBP / UI 系统 | 创建 C++ Widget 类或提供 WBP 布局规格 |
| [GAS.md](GAS.md) | GA / GE / AttributeSet | 创建 GA、GE、Attribute 相关 C++ |
| [AnimNotify.md](AnimNotify.md) | AnimNotify / AnimNotifyState | 创建 AN / ANS 类 |
| [DataAsset.md](DataAsset.md) | DA / 配置数据结构 | 创建新的 DataAsset 类型或字段 |

---

## 分工总原则

```
Claude 负责：
  ├── C++ 类（.h + .cpp）
  ├── .ush HLSL 函数文件
  ├── 蓝图父类逻辑
  └── 布局规格文档（WBP 参数表格）

用户负责：
  ├── 编辑器内创建 BP 子类 / WBP / DA 资产
  ├── 蒙太奇时间轴上放置 AN / ANS
  ├── 连线材质图（Custom Node 以外的节点）
  └── DA 字段数值填写
```

---

## 任务状态查询

当前开发任务 → [PM/TASKS.md](../PM/TASKS.md)  
功能完成记录 → [FeatureLog.md](../FeatureLog.md)
