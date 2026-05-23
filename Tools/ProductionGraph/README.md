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

剧情编辑器入口：

```text
http://localhost:4783/story.html
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

剧情源数据保存到：

```text
Docs/StorySource/<ArcId>/<StoryId>.story.json
Docs/StorySource/<ArcId>/<StoryId>.story.md
```

剧情编辑器可以导出：

```text
Docs/StoryPipeline/StoryImportManifest.json
Docs/StoryPipeline/StoryWorkload.md
Docs/StoryPipeline/ValidationReport.md
Docs/StoryPipeline/Metadata/project_assets.json
Docs/StoryPipeline/Requests/*.json
Docs/StoryPipeline/Requests/*.md
Docs/StoryPipeline/Runs/*.final.md
Docs/StoryPipeline/Runs/*.log.txt
```

`生成 Codex 任务` 只创建请求文件；`运行 Codex 整理` 会调用本机 Codex CLI 读取该请求并整理剧情源数据。默认优先使用：

```text
%LOCALAPPDATA%\OpenAI\Codex\bin\codex.exe
```

如果需要指定其他 Codex CLI 路径，可以先设置环境变量：

```powershell
$env:CODEX_CLI_PATH="C:\Users\g\AppData\Local\OpenAI\Codex\bin\codex.exe"
```

`同步项目资产` 会快速扫描 `Content` 和 `Config/Tags`，生成 `Docs/StoryPipeline/Metadata/project_assets.json`。剧情编辑器会用它给地图、RoomData、卡牌、教程配置和 tag 字段提供下拉候选；字段仍允许手动输入。

UE 导入使用 `StoryImport` Commandlet。默认是 dry-run，只报告将创建/更新哪些资产：

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=StoryImport -unattended -nop4
```

确认后加 `Apply` 写入剧情工作台资产：

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=StoryImport Apply -unattended -nop4
```

也可以指定 manifest：

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=StoryImport -Manifest="Docs/StoryPipeline/StoryImportManifest.json" -unattended -nop4
```

## 测试

```powershell
.\Tools\ProductionGraph\run_smoke_test.ps1
```
