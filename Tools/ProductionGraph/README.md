# Production Graph Tool

本工具用于把教程、剧情、演出、关卡触发器和衍生任务整理成自由节点网络。

## 启动

在项目根目录运行：

```powershell
node Tools\ProductionGraph\server.js --port 4783
```

然后打开：

```text
http://localhost:4783
```

## 操作

- 鼠标左键拖动节点。
- 在画布空白处按住鼠标左键拖动，可以框选多个节点。
- 按住鼠标中键或右键拖动画布。
- 按 `F` 在“聚焦选中节点”和“聚焦全部节点”之间切换。
- 从节点左右两侧引脚拖动，可以连接另一个节点的引脚。
- 从引脚拖到空白处松开，会打开节点清单；选择节点后自动创建并连接。
- 右键画布空白处打开节点清单并创建节点。
- `Backspace` 或 `Delete` 删除选中节点/连线。
- `Ctrl+Z` 撤销，`Ctrl+Y` 或 `Ctrl+Shift+Z` 重做。
- `Ctrl+D` 复制一份当前选中的节点；框选多个节点时会一起复制，并保留它们之间的内部连线。
- `Ctrl+C` 复制选中节点，`Ctrl+V` 粘贴节点。

## 数据

图谱数据保存到：

```text
Docs/ProductionGraph/<graph-id>/graph.json
```

## 测试

```powershell
.\Tools\ProductionGraph\run_smoke_test.ps1
```
