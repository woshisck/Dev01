## Weave Language
一种为 AI 优化的图布局语言，目前用于蓝图AI。

蓝图功能很多，但转义为 `K2Node` 文本时，对 `LLM` 来说是相当大的负担。  
`Weave语言` 是人和 `LLM` 都能读写蓝图的方式。

本插件旨在打通 LLM 与 蓝图之间的交互壁垒，制作 AI 与 蓝图 交互的中间件，未来将会逐步支持材质图表，动画图表，Control Rig 图表等，希望广大开发者可以一起共同开发。

### 特性
- **跨蓝图**: 你可以在不打开蓝图的情况下对蓝图进行编辑
- **可读性**：人类和 AI 都能看懂
- **双向转换**：蓝图 与 Weave 之间无损转换（更新中）
- **Patch 机制**：AI 只需说"改哪里"，不用重写整个文件
- **Schema 约束**：AI 不可能生成不存在的节点或非法连接
- **Token 高效**：比直接读取蓝图数据省 `70%以上` token


### Weave Language 语法参考

一个 `.weave` 文件包含多个声明块：
```
graphset <名称> <蓝图路径>
graph <函数名>

node ...
link ...
set ...
```

---
| 关键字 | 用途 |
|--------|------|
| `graphset` | 声明蓝图资源，这张蓝图在哪里 |
| `graph` | 声明事件图/函数，具体图表是哪个 |
| `node` | 声明节点 |
| `link` | 连接节点端点 |
| `set` | 设置节点属性值 |
---



#### 示例
##### 开始游戏后使用 Sequence 按顺序执行四个 Print String
- 此时操作的是 /Game/MyActor2 蓝图的事件图表。
```weave
graphset MyActor2 /Game/MyActor2.MyActor2
graph EventGraph

node beginplay : event.Actor.ReceiveBeginPlay @ (-144, 16)
node sequence : special.Sequence @ (96, 16)
node print01 : call.KismetSystemLibrary.PrintString @ (384, 16)
node d : call.KismetSystemLibrary.PrintString @ (384, 160)
node e : call.KismetSystemLibrary.PrintString @ (384, 304)
node f : call.KismetSystemLibrary.PrintString @ (384, 448)

link beginplay.then -> sequence.execute
link sequence.then_0 -> print01.execute
link sequence.then_1 -> d.execute
link sequence.then_2 -> e.execute
link sequence.then_3 -> f.execute

set print01.WorldContextObject = self
set print01.InString = Print 1
set print01.bPrintToScreen = true
set print01.bPrintToLog = true
set print01.TextColor = (R=0.000000,G=0.660000,B=1.000000,A=1.000000)
set print01.Duration = 2.000000
set d.WorldContextObject = self
set d.InString = Print 2
set d.bPrintToScreen = true
set e.WorldContextObject = self
set e.InString = Print 3
set f.WorldContextObject = self
set f.InString = Print 4
```

### 如何使用？
拉取插件源代码，放到虚幻`cpp`项目的插件文件夹中，重新生成项目文件，编译后启动。
也可以下载 Release 中的发布版本直接放到项目插件文件夹中（若有）。

#### 快速测试
如果成功安装了插件，在虚幻引擎编辑器主界面上方应当有 `Weaver` 字样的下拉菜单，打开菜单后点击 `Weave 生成解释调试器` 即可打开。选中蓝图节点，点击 `Generate from Selection` 即可将选中的整个蓝图链翻译为 Weave 语言，当然点击 `Apply to Blueprint` 即可应用会对应的图表。


# Weave Language API 参考

所有操作通过 UWeaveOperator 调用。
你需要先调用 `GenerateWeaveLanguage` 因为这会让插件重新计算所有的事件，函数，宏，类型等等，Weave 需要使用此函数所生成的 Schema 进行可靠的解析，此函数在编辑器启动后默认调用一次。

### 获取蓝图 Weave 代码

FString GetBlueprintWeave(BlueprintPath, GraphName, EntryNode = TEXT(""))

把蓝图转成 Weave 语言。

参数：
- BlueprintPath: 蓝图路径，如 /Game/MyActor.MyActor
- GraphName: 函数名或事件名，如 TestFunction
- EntryNode: 可选，指定入口节点

返回：Weave 代码字符串。

---

### 应用 Diff

FString ApplyWeaveDiff(OriginalWeave, DiffCode, OutError)

将 Diff 应用到 Weave 代码，返回修改后的完整 Weave。

参数：
- OriginalWeave: 原始 Weave 代码
- DiffCode: AI 生成的 Diff
- OutError: 错误信息，成功时为空

返回：修改后的完整 Weave 代码。

---

### 验证 Diff

FString DiffCheck(BlueprintPath, GraphName, DiffCode, OutError)

在实际应用前预览 Diff 效果，不修改蓝图。

参数：
- BlueprintPath: 蓝图路径
- GraphName: 函数名
- DiffCode: AI 生成的 Diff
- OutError: 错误信息

返回：预览后的 Weave 代码。

---

### 应用到蓝图

bool ApplyWeaveToBlueprintWithUndo(WeaveCode, BlueprintPath, GraphName, OutError)

将 Weave 代码写入蓝图，支持撤销。

参数：
- WeaveCode: 完整的 Weave 代码
- BlueprintPath: 蓝图路径
- GraphName: 函数名
- OutError: 错误信息

返回：true 表示成功。

---

### 生成 Weave

void GenerateWeaveLanguage()

从当前蓝图生成 Weave 语言。

---

### 搜索节点

TArray SearchNode(Query)

在当前蓝图中搜索节点。

参数：
- Query: 搜索关键词

返回：匹配的节点列表。

---

### 搜索类型

TArray SearchType(Query)

搜索可用的节点类型。

参数：
- Query: 类型关键词

返回：匹配的类型列表。

---

### 搜索蓝图变量

TArray SearchBlueprintVariables(BlueprintPath, Query)

搜索蓝图中的变量。

参数：
- BlueprintPath: 蓝图路径
- Query: 变量名关键词

返回：匹配的变量列表。

---

### 搜索上下文变量

TArray SearchContextVar(BlueprintPath, Query)

搜索当前上下文可用的变量。

参数：
- BlueprintPath: 蓝图路径
- Query: 变量名关键词

返回：匹配的变量列表。

---

### 搜索上下文函数

TArray SearchContextFunctions(BlueprintPath, Query)

搜索当前上下文可用的函数。

参数：
- BlueprintPath: 蓝图路径
- Query: 函数名关键词

返回：匹配的函数列表。

---

### 搜索资源

TArray SearchAsset(Query, MaxResults = 20)

在项目中搜索资源。

参数：
- Query: 资源名关键词
- MaxResults: 最大返回数量，默认 20

返回：匹配的资源路径列表。

---

### 获取资源引用

TArray GetAssetReferences(AssetPath, MaxResults = 20)

获取资源的引用关系。

参数：
- AssetPath: 资源路径
- MaxResults: 最大返回数量，默认 20

返回：引用该资源的其他资源列表。

---

## 变量操作

### 修改变量

bool ModifyVar(BlueprintPath, VarName, NewValue, OutError)

修改蓝图变量的值。

参数：
- BlueprintPath: 蓝图路径
- VarName: 变量名
- NewValue: 新值
- OutError: 错误信息

返回：true 表示成功。

---

### 删除变量

bool DeleteVar(BlueprintPath, VarName, OutError)

删除蓝图变量。

参数：
- BlueprintPath: 蓝图路径
- VarName: 变量名
- OutError: 错误信息

返回：true 表示成功。

---

## 节点操作

### 获取节点

FString GetNodeById(Id)

通过 ID 获取节点信息。

参数：
- Id: 节点 ID

返回：节点 JSON 字符串。

---

### 按类别获取节点

TArray GetNodesByCategory(Category)

按类别获取节点列表。

参数：
- Category: 类别名

返回：节点列表。

---

### 添加节点

void AddNodeFromJson(JsonString)

通过 JSON 添加节点。

参数：
- JsonString: 节点的 JSON 描述

---

### 删除节点

bool RemoveNode(Id)

删除指定节点。

参数：
- Id: 节点 ID

返回：true 表示成功。

---

### 获取所有节点

TArray GetAllNodesAsJson()

获取当前所有节点的 JSON 列表。

返回：所有节点的 JSON 字符串列表。

---

### 清空节点

void ClearNodes()

清空当前所有节点。

---

### 获取节点数量

int32 GetNodeCount()

返回：当前节点总数。

---

### 应用 Diff 到蓝图

bool ApplyDiff(OutError)

将暂存的 Diff 应用到蓝图。

参数：
- OutError: 错误信息

返回：true 表示成功。

---

## 异步接口

void SearchNodeAsync(Query, OnComplete)

异步搜索节点，避免阻塞。

参数：
- Query: 搜索关键词
- OnComplete: 回调函数，参数为搜索结果