# UE5.8 EnvBatch Actor Tagger 使用说明

更新时间：2026-07-10

`EnvBatch Actor Tagger` 是编辑器里的关卡 Actor 标记工具。它只负责给场景里的 Actor 写入普通 `Actor Tags`，不创建 GameplayTags，不合并模型，不改材质，不保存地图。

## 打开方式

```text
Tools -> DevKit 工具 -> 性能工具 -> 环境合批标记
```

也可以从 `性能工具快捷入口` 打开。

## 当前 Tag 命名

每个 Actor 同一时间只保留一个 `EnvBatch.Source.*`。工具写入新 Source Tag 前，会先移除该 Actor 上旧的 `EnvBatch.Source.*`，但不会清理其他普通 Actor Tag。

```text
EnvBatch.Source.<LevelName>.<Prop|Building>.<Instance|Batched>.<TextureCollectionGroup>.<Serial>
EnvBatch.Source.<LevelName>.Ground.Batched.<Serial>
```

示例：

```text
EnvBatch.Source.L1_CommonLevel_corridor_01b.Prop.Instance.TC-A.01
EnvBatch.Source.L1_CommonLevel_corridor_01b.Prop.Batched.TC-A.01
EnvBatch.Source.L1_CommonLevel_corridor_01b.Prop.Batched.TC-A.02
EnvBatch.Source.L1_CommonLevel_corridor_01b.Building.Instance.TC-B.01
EnvBatch.Source.L1_CommonLevel_corridor_01b.Ground.Batched.01
```

字段含义：

| 字段 | 含义 |
| --- | --- |
| `LevelName` | 当前关卡名，或美术手动输入的区域名。工具提供“获取当前关卡名”。 |
| `Prop` | 道具、摆件、静态小物件。后续自动合批主要看这一类。 |
| `Building` | 建筑、墙面类对象。当前主要用于组织和审计，默认不做自动合批。 |
| `Ground` | 地面对象。地面只走 RVT 和地面专用合批，不需要贴图集合组段。 |
| `Instance` | 重复模型或手动实例化对象，后续优先走 ISM/HISM 或普通 Texture Collection。 |
| `Batched` | 需要几何合并和材质合批的静态物件。 |
| `TextureCollectionGroup` | 共享贴图集合父组，例如 `TC-A`、`TC-B`。同一组下的多个流水号可以共享普通 Texture Collection。 |
| `Serial` | 两位流水号，例如 `01`、`02`。工具提供“下一个流水号”。 |

## 推荐分类

| 内容类型 | 推荐处理 |
| --- | --- |
| 地形/地面 | 使用关卡 RVT 工具打 `Ground.Batched`，走 RVT。 |
| 墙面/建筑 | 默认不走自动合批，主要依赖手动实例化、Mesh Paint、UDIM 或建筑母材质规则。必要时打 `Building.Instance` 做组织和审计。 |
| 静态实例化物件 | 打 `Prop.Instance`。后续可用 ISM/HISM 或 Texture Collection 管理同模型多贴图。 |
| 静态合批物件 | 打 `Prop.Batched`。这是后续几何合并、材质集合化、draw call 优化的主要对象。 |

## 写入 Tag 的流程

1. 在 Level Viewport 或 World Outliner 里选择一个或多个 Actor。
2. 打开 `环境合批标记`。
3. `关卡/区域名` 可以手动输入，也可以点击 `获取当前关卡名`。
4. 在 `对象类型` 中二选一：`物件 Prop` 或 `建筑 Building`。
5. 在 `处理方式` 中二选一：`实例化 Instance` 或 `合批 Batched`。
6. 设置 `共享贴图集合组`，例如 `TC-A`。同一组可跨 `01`、`02` 等多个 merge 组共享普通 Texture Collection。
7. 设置 `流水号`，或点击 `下一个流水号` 自动查找当前同类 Tag + 贴图集合组的下一个编号。
8. 确认 `预览` 中的最终 Tag。
9. 点击 `写入 Source Tag`。
10. 保存对应关卡、子关卡或 Level Instance。

## 查看当前场景中已经打的 Tag

窗口下方有 `当前场景 Source Tag 列表`：

- `刷新场景列表` 会重新扫描当前编辑器世界里已经加载的 Actor。
- 列表只显示 `EnvBatch.Source.*`，并显示每个 Tag 下面有多少个 Actor。
- 单击或双击列表行，会在编辑器里自动选中所有带该 Tag 的 Actor。
- 单击列表行时，也会把这个 Tag 的 `LevelName / Prop|Building / Instance|Batched / TextureCollectionGroup / Serial` 回填到上方输入区。
- `选择列表对象` 用于重新选择当前列表行对应 Actor。
- `套用列表 Tag` 只回填参数，不改变当前选择对象的 Tag。

如果列表显示 0：

- 当前已加载的编辑器世界里还没有 Actor 带 `EnvBatch.Source.*`。
- 或目标对象在未加载的子关卡、Level Instance、World Partition 区域或 Data Layer 中。
- 或刚写入后还没有点击 `刷新场景列表`。
- 写入后必须保存对应关卡，否则重开编辑器后 Tag 会丢失。

## 快速自检

1. 选择一个测试用 StaticMeshActor。
2. 选择 `物件 Prop` + `合批 Batched`，共享贴图集合组填 `TC-A`，流水号填 `01`。
3. 点击 `写入 Source Tag`。
4. 点击 `刷新场景列表`。
5. 列表应出现类似 `EnvBatch.Source.<LevelName>.Prop.Batched.TC-A.01` 的行。
6. 点选该行后，Viewport / World Outliner 会自动选中所有带这个 Tag 的 Actor。

## 和真正合批的关系

这个窗口只解决“哪些 Actor 属于哪个候选组”的问题。真正节省 draw call 的链路在后续工具中：

```text
Actor Tags
-> 候选扫描
-> 人工审查
-> 普通 Texture Collection 贴图集合化
-> 几何合并 / ISM / HISM
-> 合批材质
-> 性能对比
```

当前方向：

- 地面使用 `Ground.Batched` + RVT。
- 建筑/墙面不默认自动合批，主要走手动实例化、Mesh Paint、UDIM 和建筑母材质。
- `Prop.Batched` 是自动合批主对象。
- 同一个完整 Source Tag 是一个模型 merge 组。
- 同一个 `TextureCollectionGroup` 可以让多个 merge 组共享按通道拆分的 SharedProp Texture Collection，例如 `TC_<Level>_TC-A_SharedProp_BaseColor`、`TC_<Level>_TC-A_SharedProp_Normal`、`TC_<Level>_TC-A_SharedProp_ORM`。
- 批处理 index 默认不再使用 UV7；优先考虑第二 UV 通道的可用分量，顶点色 A 作为兜底方案。
