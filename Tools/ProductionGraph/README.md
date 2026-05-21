# Production Graph Tool

本工具用于把教程、剧情、演出和玩法制作工作整理成自由节点网络。

第一版是本地浏览器工具：

- 数据保存在 `Docs/ProductionGraph/`
- 每张图是一个文件夹
- 每个节点和连线是独立 JSON 文件，方便 Git 异步协作
- 不依赖 npm 包，直接用本机 Node.js 运行

## 启动

在项目根目录运行：

```powershell
node Tools\ProductionGraph\server.js --port 4783
```

然后打开：

```text
http://localhost:4783
```

## 基础操作

- 鼠标左键拖动节点。
- 按住鼠标中键或右键拖动画布。
- 按 `F` 在“聚焦选中节点”和“聚焦全部节点”之间切换；没有选中节点时聚焦全部节点。
- 在画布空白处点击鼠标右键，打开节点清单并创建节点。
- 从节点左右两侧的引脚按住拖动，可以直接连到另一个节点的引脚。
- 从引脚拖到空白处松开，会打开节点清单；选择节点类型后会自动创建新节点并连线。
- 引脚松开后如果不想创建节点，在画布空白处点击一次左键即可退出。
- 选中节点或连线后，按 `Backspace` 或 `Delete` 删除。
- 点击节点右上角红色方块，直接删除该节点。
- 点击侧边栏的“撤销 / 重做”，或使用 `Ctrl+Z`、`Ctrl+Y` / `Ctrl+Shift+Z` 回退和恢复图谱编辑。
- 拖动画布只改变视图位置，不会计入未保存修改。

## 数据目录

示例图：

```text
Docs/ProductionGraph/tutorial-opening/
```

结构：

```text
manifest.json
nodes/*.json
edges/*.json
```

## 节点类型

- `need`：需求
- `solution`：解决方案
- `task`：制作任务
- `blocker`：阻塞或制作中发现的问题

## 连线类型

- `has_solution`：需求产生解决方案
- `creates_task`：方案拆出任务
- `depends_on`：依赖
- `blocked_by`：被阻塞
- `becomes_need`：阻塞转成新需求
- `changes_scope`：改变范围

## 测试

```powershell
.\Tools\ProductionGraph\run_smoke_test.ps1
```
