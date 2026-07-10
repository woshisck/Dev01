# UE5.8 NoVT / Texture Collection / RVT 使用指南

版本：2026-07-10
适用：DevKit（UE5.8）

## 当前结论

普通场景模型、建筑、道具贴图默认不启用 `VirtualTextureStreaming`。

项目只在地面 Runtime Virtual Texture 路径使用 UE 的虚拟纹理系统。模型、建筑、道具如果需要共享贴图集合，使用普通 `UTextureCollection`，不是 `UVirtualTextureCollection`，也不是把源贴图改成 Streaming Virtual Texture。

## 工具入口

```text
Tools -> DevKit 工具 -> 美术资产工具 -> 贴图 NoVT 审计
Tools -> DevKit 工具 -> 美术资产工具 -> Texture Collection 管理器
Tools -> DevKit 工具 -> 性能工具 -> 关卡 RVT 工具
```

## 贴图规则

| 内容 | 源贴图设置 | 共享方式 |
| --- | --- | --- |
| 地面 RVT Writer 的源贴图 | 普通 Texture2D，`VirtualTextureStreaming=false` | 写入地面 RVT |
| 地面 RVT RuntimeVirtualTexture 资产 | 使用 RVT 系统 | 由关卡 RVT 工具创建/绑定 |
| 建筑/墙面源贴图 | 普通 Texture2D，`VirtualTextureStreaming=false` | 手动实例化、UDIM 或普通 Texture Collection |
| 静态道具源贴图 | 普通 Texture2D，`VirtualTextureStreaming=false` | 普通 Texture Collection + 后续合批 index |
| 旧 VTAtlas / VirtualTextureCollection | 历史路径 | 不作为新默认方案 |

## 为什么不再给普通物件开启 VT

UE 不允许 Virtual Texture sample 直接参与 Runtime Virtual Texture Output，因此地面 RVT Writer 的源贴图也必须保持普通 Texture2D。

当前场景大部分模型贴图是 512 到 1K。把这些普通物件贴图全部改成 Streaming Virtual Texture，不会自动减少 draw call，也会引入 VT pool、page cache、格式转换和排查复杂度。对于物件合批，我们只需要让多个材质能共享同一种采样入口和 index，普通 `UTextureCollection` 更符合当前目标。

## 贴图 NoVT 审计

该工具扫描 `/Game/Art` 下的 Texture2D：

- `VirtualTextureStreaming=true` 会被标为阻塞问题。
- 工具只提供批量关闭 VT，不再提供批量开启 VT。
- 通过检查的贴图可以作为普通材质贴图，也可以加入普通 Texture Collection。
- 4K 以上、非 POT、非常规格式会给出风险提示，但不会自动改资产。

## Texture Collection 管理器

该工具管理普通 `UTextureCollection`：

- 新建资产默认放在 `/Game/Art/Textures/TextureCollection`。
- 可以从 Content Browser 选中普通 Texture 后一键创建集合。
- 可以向当前集合追加或移除成员。
- 成员检查要求 Texture2D 且 `VirtualTextureStreaming=false`。

材质侧后续通过 Texture Collection 节点和外部 index 采样。index 的写入策略仍由环境合批工具链决定，当前优先考虑第二 UV 通道的可用分量，顶点色 A 作为兜底；不再默认补齐到 UV7。

## 地面 RVT

地面仍使用 Runtime Virtual Texture：

- 地面源材质可以用层材质、顶点色、贴花等方式混合。
- 混合结果写入 RVT Output。
- 最终地面渲染材质再采样 RVT，减少运行时多层材质重复计算。
- `Config/DefaultEngine.ini` 中的 `r.VirtualTextures=True` 是为了 RVT 保留，不代表普通模型贴图应开启 VT。

## EnvBatch 关系

`EnvBatch.Source.*` 只负责标记合批候选范围：

```text
EnvBatch.Source.<LevelName>.<Prop|Building|Ground>.<Instance|Batched>.<TextureCollectionGroup>.<Serial>
```

其中 `TextureCollectionGroup` 用 `TC-A`、`TC-B` 这类命名。它表示共享贴图集合父组，不再表示 VirtualTextureCollection。

## 历史路径

旧的 `VTAtlas` / `VirtualTextureCollection` / `Texture2DArray` 代码和文档片段仍可能作为历史兼容或研究记录存在。当前新制作口径以本文为准：

1. 普通模型贴图 NoVT。
2. 共享贴图走普通 Texture Collection。
3. 地面使用 RVT。
4. 旧 VTAtlas partial apply 不允许通过编辑器工具直接执行。
