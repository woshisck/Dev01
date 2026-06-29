# UE5.8 EnvBatch Tagger 使用说明

更新时间：2026-06-29

`EnvBatch Tagger` 是一个编辑器窗口，用来给关卡中的静态环境 Actor 批量写入 `EnvBatch.*` Actor Tag。它不会自动合批，也不会自动保存地图；它只是给后续 `MaterialBatchBuild`、Geometry Merge、HLOD/Proxy 切换提供清晰输入。

## 1. 打开方式

在 UE 编辑器顶部菜单打开：

```text
Tools -> DevKit Tools -> Performance Tools -> EnvBatch Tagger
```

## 2. 什么时候需要打 tag

需要进入材质/模型批处理专项时打 tag：

- 要把一个走廊、房间、街区、建筑模块作为离线 Geometry Merge 候选。
- 要区分原始 Source Actor 和已经生成或人工制作的 Proxy Actor。
- 要明确排除不应该被合批的 Actor。
- 要准备跑 `MaterialBatchBuild -RequireTag=EnvBatch.Source`。

不需要在普通摆场景阶段立刻打 tag。建议在一个区域基本稳定、静态/动态边界确认后再做。

## 3. 当前提供的 tag

| 按钮 | 写入 tag | 用途 |
| --- | --- | --- |
| `Source.<Group>` | `EnvBatch.Source.<Group>` | 原始静态环境输入，允许进入批处理候选 |
| `Proxy.<Group>.Medium` | `EnvBatch.Proxy.<Group>.Medium` | Medium 及以下档位可用的代理对象 |
| `Proxy.<Group>.Low` | `EnvBatch.Proxy.<Group>.Low` | Low 档位可用的代理对象 |
| `Baked.Ground.Low` | `EnvBatch.Baked.Ground.Low` | Low 档地面/大面烘焙替代物 |
| `Exclude` | `EnvBatch.Exclude` | 明确排除批处理 |
| `Remove EnvBatch Tags` | 清理全部 `EnvBatch.*` | 回退或重新分组 |

当前实现是互斥写入：对选中 Actor 写入新的 `EnvBatch.*` 前，会先移除该 Actor 上已有的 `EnvBatch.*` tag。

## 4. 操作步骤

1. 在 Level Viewport 或 World Outliner 选择一个或多个 Actor。
2. 打开 `EnvBatch Tagger`。
3. 在 Group 输入框填写分组名，例如：

```text
Corridor_01b
Building_Stone_A
Dungeon_Wall_Set_02
```

4. 点击 `Source.<Group>` 给原始静态环境物体打 tag。
5. 如果有人工或工具生成的代理物体，选择代理物体后点击 `Proxy.<Group>.Medium` 或 `Proxy.<Group>.Low`。
6. 对不应合批的对象点击 `Exclude`。
7. 保存对应关卡或子关卡。

## 5. 打 tag 后跑什么

第一步先 dry-run，不要直接全量 apply：

```powershell
& "D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Self\GItGame\Dev01\DevKit.uproject" -run=MaterialBatchBuild -Map=/Game/Art/Map/Map_Data/L1_CommonLevel_corridor_S_Dungeon/L1_CommonLevel_corridor_01b -Cluster=Corridor_01b -Tier=Medium -RequireTag=EnvBatch.Source -MaxActors=2000 -unattended -nopause
```

dry-run 用来确认：

- 哪些 Actor 成为候选。
- 哪些 Actor 被拒绝。
- 拒绝原因是否合理，例如 `DynamicMobility`、材质不兼容、贴图尺寸不一致。
- 计划输出目录是否正确。

通过审查后才跑 partial apply：

```powershell
& "D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Self\GItGame\Dev01\DevKit.uproject" -run=MaterialBatchBuild -Map=/Game/Art/Map/Map_Data/L1_CommonLevel_corridor_S_Dungeon/L1_CommonLevel_corridor_01b -Cluster=Corridor_01b -Tier=Medium -RequireTag=EnvBatch.Source -ApplyMappingOnly -ApplyTextureArraysOnly -ApplyPropertyTextureOnly -ApplyProxyMeshOnly -ApplyBatchMaterialOnly -MaxActors=2000 -unattended -nopause
```

仍不建议使用全量 `-Apply`。当前全量替换保持禁用，避免未审查的代理资产直接替换关卡。

## 6. 与自动合批的关系

`EnvBatch Tagger` 本身不做合批。它解决的是“哪些物体允许被自动化工具处理”的输入问题。

后续真正节省 draw call 的链路是：

```text
Actor Tag -> MaterialBatchBuild dry-run -> 人工审查 -> Texture2DArray / PropertyTexture -> ProxyMesh / Geometry Merge -> Batch Material -> 性能对比
```

tag 是入口，Geometry Merge 和 batch material 才是 draw call 优化的核心。

## 7. 注意事项

- 打 tag 会修改 Actor tags，并标记关卡 package dirty。
- 工具不会自动保存地图。
- 不要给动态、可交互、可破坏、独立脚本事件对象打 `Source`。
- 一个 cluster 不要过大，优先按房间、走廊段、可见性 cell、HLOD cell 分组。
- `DynamicMobility` 是当前 corridor dry-run 的主要拒绝原因之一，需要关卡侧确认哪些视觉静态物可以转为 Static。
