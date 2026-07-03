# UE5.8 Epic/High/Mid/Low 模型材质性能分级落地计划

更新时间：2026-06-30

## 0.0A 当前执行说明

当前阶段只做工具和接口落地，不做全项目资产审计、不做场景灯光预算、不做最终验收自动化扩展。这三项统一放到项目完成前出包阶段处理。

面向美术和 TA 的当前可执行入口已经单独整理到：

```text
Docs/04_开发实现与系统文档/性能/UE58_性能分级美术工具使用与测试说明.md
```

后续执行优先读取本节、`0.0 当前纠偏后的有效口径`、`0.1 当前已提供的美术侧功能入口` 和上述工具说明。本文后半段保留了若干历史续跑记录，其中出现的 `AuthorProxy`、`DataLayer`、`M_Env_Building_Batch`、`Texture2DArray` 主线路径等旧描述只作为历史记录，不作为当前任务依据。当前口径是：

- 场景组织：传统 Streaming Level，不使用 World Partition / DataLayer。
- 模型合规：默认 `Epic/High=LOD0`、`Mid/Low=LOD1`；模型自身的 LOD、碰撞、材质槽和特殊处理直接在 StaticMesh 资产里维护，不再生成 `UYogModelPerformanceConfig`。
- 材质模板：Source 模板为 `M_Env_MasterA_Source`，baked/batch 合同为 `M_Env_Baked_VTAtlas`。
- 贴图后端：优先 VT Atlas / UDIM-style SVT；Texture2DArray 只作为 legacy fallback。
- 当前 Codex 会话已连接 UE MCP；`M_Env_MasterA_Source` 和 `MI_Env_MasterA_Source_Epic/High/Mid/Low` 已通过 MCP `MaterialTools`、`MaterialInstanceTools`、`ProgrammaticToolset` 创建并反查。`MaterialPerformanceTemplateSetup` commandlet 保留为自动化重建 fallback。

本文是后续 UE5.8 模型、材质、地面/墙面 bake、VT Atlas、RVT、Source/Proxy、玩家画质设置和性能验收的当前主计划。旧的 `PCUltra / Handheld15W / Switch2Candidate / Handheld5W` 只作为测试设备或参考场景，不再作为项目内部正式画质档位。正式档位只保留 `Epic / High / Mid / Low`。

## 0.0 当前纠偏后的有效口径

本节优先级高于本文后续所有早期过程记录。2026-06-30 起，当前项目按以下方案继续落地：

- 场景组织：使用传统 Streaming Level，不开启 World Partition，不使用 DataLayer，不引入 Data Layer 作为 Source/Proxy/Baked 互斥、验收或美术工作流前提。
- 模型合批：仍使用 `EnvBatch.*` 标签表达 Source / Proxy / Baked / Exclude 意图；真实归属证据读取 actor/component 所在的 Streaming Level，并写入 report/manifest/mapping data。
- 模型分级：静态环境模型默认按 `Epic/High=LOD0`、`Mid/Low=LOD1` 检查；LOD 不足时工具记录阻断。特殊模型不再写单独配置资产，直接在 StaticMesh 资产内处理 LOD、碰撞和材质槽。
- Source/Proxy/Baked：Source 仍是编辑和最高档来源；GeneratedBatchProxy/Baked 由工具生成并按 Streaming Level 或后续加载规则互斥。第一阶段不再把 AuthorProxy 作为必须工作流。
- 材质：当前阶段落地性能分级接口、Source 母材质节点模板、VT Atlas/UDIM-style SVT/PropertyTexture/MappingData 契约；不要求现在完成最终地面/墙面复杂材质网络。
- 地面/墙面：地面静态混合、mesh paint/顶点色、贴花混合在 bake 前完成，bake 后如需动态效果再走 RVT/overlay；墙面默认走 wall static bake / VT Atlas / baked replacement，不把 RVT 作为墙面默认方案。
- VT/非 VT 混用：正式生产路径优先 SVT/UDIM-style 虚拟大贴图或等价 VT Atlas；非 VT fallback 只保留为兼容/调试语义，必须通过 `ResidencyRiskPlan` 观察重复驻留风险。
- 四挡：正式 API、UI、DeviceProfile 和自动化目标只保留 `Epic / High / Mid / Low`。掌机 15W、Switch 候选等只作为测试场景描述，不作为正式目标档位名。
- 验收：`DryRunCaptured` 必须同时满足 `Layer evidence mode=StreamingLevel`、`Actual layer evidence=True`、Source/Proxy asset readiness、Source/Proxy config-set 和 residency evidence。
- 暂缓项：全项目资产审计、场景灯光预算、验收自动化扩展暂不执行；这些等项目完成前出包或正式性能验收阶段再做。

## 0.1 当前已提供的美术侧功能入口

本阶段先提供工具和接口，不做全项目批量审计、不做最终性能验收。

| 功能 | 入口 | 当前产物 | 用途 |
| --- | --- | --- | --- |
| 场景分层/合批标签 | `Tools -> DevKit Tools -> Performance Tools -> EnvBatch Tagger` | Actor Tags：`EnvBatch.Source.<Group>`、`EnvBatch.Proxy.<Group>.Mid/Low`、`EnvBatch.Baked.*`、`EnvBatch.Exclude` | 美术在当前 Streaming Level 里选择静态物件并标记 Source/Baked/Exclude 意图 |
| 模型资产合规检查 | Level Editor 顶部 `模型合规` / `Tools -> DevKit 工具 -> 美术资产工具 -> 模型资产合规检查` | 不生成模型配置资产；右侧 `打开模型设置` 直接打开 StaticMesh 资产编辑器 | 扫描 `/Game/Art` 下 StaticMesh，检查 LOD0/LOD1、材质槽、碰撞、三角面和分类排除；左侧目录树用于定位资产 |
| 材质合规检查 | Level Editor 顶部 `材质合规` / `Tools -> DevKit 工具 -> 美术资产工具 -> 材质合规检查` | `/Game/Docs/Performance/MaterialTextureRules/DA_MaterialTextureNamingRules_Default`，类型 `UYogMaterialTextureNamingConvention` | 检查贴图后缀、sRGB、尺寸、VT 建议，以及材质是否暴露 Source/Baked/VT Atlas 和性能分级参数；后续 bake/VT 工具读取该规则资产 |
| Source 材质性能模板 | UE MCP：`MaterialTools` / `MaterialInstanceTools` / `ProgrammaticToolset`；Commandlet fallback：`MaterialPerformanceTemplateSetup` | `M_Env_MasterA_Source` 和 `MI_Env_MasterA_Source_Epic/High/Mid/Low` | 用引擎默认材质节点表达 Source 材质分级参数，不依赖 `.ush` |
| Baked/VT 合批母材质合同 | Commandlet：`MaterialBatchParentMaterialSetup` | `M_Env_Baked_VTAtlas` | 合批后通过 `TexCoord7.x -> _PropTexture -> VT_Atlas` 读取 baked/VT 结果 |

推荐创建材质模板的命令：

```powershell
UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=MaterialPerformanceTemplateSetup -Apply -Force
UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=MaterialBatchParentMaterialSetup -Apply -Force
```

本轮不要求立刻运行验证。后续测试方式：

```powershell
UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=MaterialPerformanceTemplateSetup
UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=MaterialBatchBuild -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 -Cluster=Prison_S_01 -Tier=Mid -TextureBackend=VTAtlas -RequireTag=EnvBatch.Source.
```

## 0. 落地执行前置规则

本计划不是独立方案文档，后续每个开发、审计、续跑和验收任务都必须先满足仓库执行规则：

1. 启动任务前读取仓库根目录 `guide.md`；若 `guide.md` 缺失，只在缺失会影响任务判断时说明。
2. 读取当前主计划，即本文档；不得只参考旧 HTML 方案或单次聊天结论。
3. 检查 `git status --short --branch`，只处理当前模型/材质/性能分级范围内的文件，不回滚无关改动。
4. 运行时功能归 `Source/DevKit`，编辑器工具和 commandlet 归 `Source/DevKitEditor`，性能档位和 CVar 归 `Config/DefaultDeviceProfiles.ini` / `Config/DefaultScalability.ini`，标签归 `Config/Tags` 或 `Config/DefaultGameplayTags.ini`，证据归 `Docs` / `Docs/GeneratedReports`。
5. 不手工编辑二进制 `.uasset`。材质、VT Atlas、PropertyTexture、MappingData、batch parent material、ProxyMesh、测试关卡资产都必须通过 Commandlet、编辑器工具或明确的资产生成流程创建。
6. 第一阶段禁止把角色、交互物、VFX、Gameplay 动态物、运行时动态材质对象纳入静态合批；这些对象默认 `EnvBatch.Exclude` 或等价排除。
7. Source / Proxy / Baked 必须互斥加载。当前互斥证据来自传统 Streaming Level + `EnvBatch.*` 标签 + Source/Proxy asset readiness；任何 VT Atlas 与非 VT fallback 混用方案，都必须同时通过 `ResidencyRiskPlan`。
8. 本目标允许编译，但如果当前失败原因是打开的 UnrealEditor 占用 DLL，不得在未确认编辑器可关闭时强行关闭；先推进不依赖干净链接的计划、工具和证据 gate。

## 1. 总目标

建立一条可审计、可回退、可分级的环境美术性能管线：

```text
美术在关卡内制作 Source 场景
-> 工具审计模型、材质、贴花、顶点色、mobility、交互风险
-> 地面和墙面静态混合效果离线 bake
-> 自动生成 VT Atlas、PropertyTexture、ProxyMesh、BatchMaterial、MappingData
-> Epic / High / Mid / Low 按档位选择 Source / Proxy / Baked
-> 通过视觉、性能、内存和加载互斥验收后再推广
```

第一轮只处理代表性 corridor 小 cluster，不做整关卡自动替换，不直接修改正式关卡表现层。

## 2. 统一画质档位

### 2.1 正式档位

| 档位 | 定位 | 模型策略 | 材质策略 | 光照/反射策略 | 运行目标 |
| --- | --- | --- | --- | --- | --- |
| Epic | 最高画质 | Source 为主，少量远景 HLOD，可保留高 LOD | 最高 VT Atlas 分辨率，完整静态 bake 结果，允许高质量动态 overlay | Lumen High/Epic，高反射，高阴影预算 | 高配 PC |
| High | 高画质 | Source + 选择性 Proxy/HLOD | 高质量 VT Atlas，少量降级细节层 | Lumen Lite Plus，SSR Medium，低 VSM 依赖 | 普通 PC |
| Mid | 默认推荐 | Proxy/HLOD 优先，必要 Source | 标准 VT Atlas，bake 结果为主，限制动态 overlay | Lumen Lite / Medium，SSR Low/Medium，默认随实测调参 | 主流 PC / 掌机候选 |
| Low | 低配/省电 | Proxy/Baked 优先，Source 最少 | 降低 VT Atlas 分辨率，关闭高成本材质层和非必要动态 overlay | Lumen Lite Minimal，SSR Low/Off，Lumen Off 仅作 fallback | 低配 PC / 低功耗 |

### 2.2 商业游戏常见设置项

玩家设置 UI、存档和内部配置应围绕以下结构组织：

```text
Display
- Display Mode
- Resolution
- Render Scale
- Upscaler / AA
- VSync
- Frame Limit

Graphics
- Preset: Epic / High / Mid / Low
- View Distance
- Shadow Quality
- Texture Quality
- Material Quality
- Model Quality
- Effects Quality
- Post Process Quality
- Global Illumination
- Reflection Quality
- Dynamic Light Quality
- Decal / Dynamic Overlay Quality

Advanced
- Use Batch Proxies
- Use Baked Ground
- Lumen Mode
- VT Atlas Quality
- Material Light Quality
```

内部默认映射：

```text
Epic -> sg.* = 3
High  -> sg.* = 2
Mid   -> sg.* = 1
Low   -> sg.* = 0
```

项目自定义预算 CVar 预留：

```text
r.Yog.MaterialQuality
r.Yog.DynamicOverlayQuality
r.Yog.MaterialLightQuality
r.Yog.MaterialLight.MaxLightInfoCount
r.Yog.BatchProxyPreference
r.Yog.VTAtlasQuality
```

商业游戏参考参数不作为死值，但作为第一轮默认落地基线：

| 设置项 | Epic | High | Mid | Low | 说明 |
| --- | --- | --- | --- | --- | --- |
| Render Scale | 100% | 90-100% | 75-90% | 60-75% | 若开启 TSR/FSR/DLSS，实际输出分辨率可高于内部比例 |
| View Distance | 3 | 2 | 1 | 0 | 对应 UE `sg.ViewDistanceQuality` |
| Shadow Quality | 3 | 2 | 1 | 0 | Low 只保留必要角色/近景阴影 |
| Texture Quality | 3 | 2 | 1 | 0 | 绑定 VT Atlas 分辨率和 streaming budget |
| Material Quality | 3 | 2 | 1 | 0 | 控制材质层数、材质灯、复杂 overlay |
| Model Quality | 3 | 2 | 1 | 0 | 控制 Source / Proxy / Baked 倾向 |
| Effects Quality | 3 | 2 | 1 | 0 | VFX 不进入合批，但需随档位降级 |
| Post Process Quality | 3 | 2 | 1 | 0 | Bloom、DOF、SSR 等常规后处理 |
| Global Illumination | Lumen High/Epic | Lumen Lite Plus | Lumen Lite / Medium | Lumen Lite Minimal | Lumen Off 仅作为超预算 fallback 或诊断路线 |
| Reflection Quality | Lumen/SSR High | SSR Medium | SSR Low | Off/Low | 反射成本不得掩盖合批收益 |
| Dynamic Overlay | 3 | 2 | 1 | 0 | 运行时 RVT/Decal/overlay 质量 |
| Batch Proxy Preference | SourceFirst | Hybrid | ProxyFirst | BakedFirst | 资源层选择策略 |
| VT Atlas Quality | 3 | 2 | 1 | 0 | 控制 atlas cell、mip、通道质量 |
| Material Light Quality | 3 | 2 | 1 | 0 | `Tex_LightInfo` 或等价材质灯数据独立降级 |

四档对资源层的默认绑定：

| 档位 | 模型加载 | 材质后端 | 地面 | 墙面 | 动态层 |
| --- | --- | --- | --- | --- | --- |
| Epic | Source 为主，远景可 HLOD | VT Atlas 高质量，可保留更多 Source MI 供对比 | Ground bake 可启用但不牺牲视觉 | Wall source/bake 对比均需保留测试 | 动态 RVT/overlay 高质量 |
| High | Source + 已审核 Proxy | VT Atlas 高质量 | Ground bake 默认启用 | Wall static bake 对高压 cluster 启用 | 动态 overlay 限量 |
| Mid | Proxy/HLOD 优先 | VT Atlas 标准质量 | Ground baked 默认启用 | Wall baked 默认启用 | 动态 overlay 限制 |
| Low | Baked/Proxy 优先 | VT Atlas 低质量，禁用高成本分支 | Ground baked 强制优先 | Wall baked 强制优先 | 非必要动态 overlay 关闭 |

### 2.3 平台只作为测试参考

平台不再进入核心档位枚举。建议测试映射：

| 测试设备/场景 | 推荐起点 |
| --- | --- |
| 高配 PC | Epic |
| 普通 PC | High |
| 主流 PC 默认 | Mid |
| Steam Deck / 掌机候选 | Mid，必要时降 Low |
| 低功耗/低配 | Low |

## 3. 模型工作流

### 3.1 模型资产三层

| 层级 | 来源 | 用途 |
| --- | --- | --- |
| Source | 原始美术资产、正式关卡摆放物、LOD0 | Epic/High 主显示、编辑和回退 |
| GeneratedBatchProxy | 工具按 cluster 自动合并生成，默认语义可视为 LOD1 之后的批处理代理 | Mid/Low、远景或低功耗显示 |
| Baked | 工具把静态材质效果 bake 到 VT Atlas / baked texture 后的替代结果 | Mid/Low，尤其地面和墙面静态结果 |

静态环境模型自身的基础检查规则为 `Epic/High=LOD0`、`Mid/Low=LOD1`。如果某个模型没有 LOD1，模型资产合规检查会把它记录为阻断；美术后续直接在 StaticMesh 资产里补 LOD 或调整资产，不再通过单独模型分级配置资产覆写。

Source、GeneratedBatchProxy 和 Baked 必须互斥加载。不能只隐藏 Source，否则原始贴图和生成 VT Atlas 可能同时常驻，导致普通 streaming pool 与 VT physical pool 同时上涨。

### 3.2 合批策略优先级

1. 重复同 mesh、同材质、重复摆放的对象优先使用 ISM/HISM。
2. 静态墙体、柱子、固定建筑件、远景装饰和非交互小件进入 Geometry Merge 候选。
3. 中远景、房间外和 Low 档优先使用 HLOD / GeneratedBatchProxy。
4. 地面不与墙面/建筑件合并为同一批 mesh，地面单独走 Baked Ground 管线。
5. 近景英雄资产、需要独立材质变化的重点物件保留 Source 或在模型性能配置资产中单独标记，不进入第一轮合批。

### 3.3 默认排除对象

以下对象默认 `EnvBatch.Exclude`：

- 角色、敌人、武器、VFX。
- 门、机关、宝箱、拾取物、可破坏物。
- 剧情触发器、独立碰撞事件、脚本移动物。
- 运行时动态材质对象、Gameplay 需要独立引用的 Actor。

这些对象第一阶段不启用批处理 VT Atlas，也不进入静态 Geometry Merge。

### 3.4 Source / Proxy 切换机制

当前项目只使用传统 Streaming Level 或等价 soft reference 组织，不使用 World Partition，不使用 Data Layer：

| 档位 | Source | GeneratedBatchProxy | Baked |
| --- | --- | --- | --- |
| Epic | 默认加载 | 只加载远景/极高 draw call 区 | 仅地面必要 bake |
| High | 大部分加载 | 高压区域加载 | 地面 bake 可启用 |
| Mid | 只保留必要 Source | 默认启用 | 地面/墙面 bake 默认启用 |
| Low | 尽量不加载 | 默认启用 | 默认启用 |

切换时机：

- 玩家 UI 可即时保存设置。
- Source / Proxy / Baked 替代建议在重新加载房间、切换关卡或进入下一房间时生效。
- 不要求运行时热生成 ProxyMesh 或 bake 资产。

### 3.5 模型资产合规管理

模型分级不只依赖 Actor tag，还需要先保证 StaticMesh 自身合规。当前阶段不再维护独立的模型分级配置资产，资产侧标准由 StaticMesh 的 LOD、碰撞、材质槽和命名/目录分类共同表达。

默认策略：

- `Source` 默认取正式摆放或导入的 StaticMesh，最高档使用 `LOD0`。
- 静态环境模型默认按 `Epic/High=LOD0`、`Mid/Low=LOD1` 检查；场景模型至少需要 LOD0 和 LOD1 才算合规。
- `GeneratedBatchProxy` 默认由 cluster 构建生成，作为 Mid/Low 的合批代理；当前不要求美术维护 AuthorProxy。特殊模型如确实需要手工代理，只作为可选 explicit proxy 配置。
- 如果某个模型特别重要或 LOD 链不符合默认规则，直接在 StaticMesh 资产编辑器内处理 LOD、材质槽或碰撞设置，然后回到合规检查刷新验证。
- Source、GeneratedBatchProxy、Baked 之间必须能回溯到同一 source actor 或 source mesh，方便视觉 parity、加载互斥和内存 residency 检查。

工具层当前提供：

- EnvBatch Tagger：在关卡内选择静态物件，打 `EnvBatch.Source.<Group>` / `EnvBatch.Baked.*` / `EnvBatch.Exclude`。
- 模型资产合规检查：扫描 StaticMesh，提供目录树、分类筛选、状态切换、预览、日志和 `打开模型设置`，但不执行合批、不生成模型配置资产。
- 材质合规检查：写出贴图后缀表资产，并检查贴图命名、sRGB、尺寸、VT 建议和材质 Source/Baked/VT Atlas 性能接口，供后续材质 bake/VT 合并工具扩展。

后续验收字段：

- `SourceProxyAssetReadiness`：按 entry 输出 SourceAsset、ProxyAsset、SourceLODIndex、ProxyLODIndex、是否 generated proxy、是否 optional explicit proxy、是否 ready。
- Source tag 条目：默认 `SourceLODIndex=0`，Proxy 指向 planned `SM_BatchProxy_<Cluster>`，`ProxyLODIndex=1`。
- Proxy/Baked 条目：必须能回溯到同一 Source actor、Source mesh 或 MappingData 记录；不能只看到生成资产就认为配置完成。
- Exclude/Baked 条目不强制 Source/Proxy asset pair，但仍需通过 tag、StreamingLevel layer evidence、bake manifest 的独立验收。

## 4. 材质与 bake 工作流

### 4.1 主线决策

- 所有 batch 贴图后端优先使用 Generated VT Atlas。
- 暂不把 Texture2DArray 作为新主线。
- 美术继续提供普通 unique 贴图，不手动维护 VT Atlas、PropertyTexture 或 batchIndex。
- 工具在构建阶段自动收集贴图和参数，生成 VT Atlas 与映射数据。

### 4.2 地面材质

地面使用独立 Authoring 与 Runtime 材质：

```text
M_Env_Ground_Authoring
  -> 多层材质
  -> Height Blend
  -> Mesh Paint / 顶点色
  -> 静态贴花混合
  -> Bake
  -> Baked SVT / VT
  -> M_Env_Ground_BakedRuntime
```

地面默认是静态材质需求。所有需要混合但不需要运行时变化的效果都应 bake 进结果。

动态逻辑只发生在 bake 后：

- 法术残留、脚印、水渍、临时污渍走 RVT/overlay。
- 动态贴花不进入静态 bake。
- Low 档可关闭或降低动态 overlay。

### 4.3 墙面材质

墙面不默认走 RVT 主写入。推荐：

```text
M_Env_Wall_Authoring
  -> Height Blend
  -> Mesh Paint / 顶点色
  -> 静态贴花/污渍/裂缝/苔藓
  -> Bake
  -> Wall VT Atlas
  -> M_Env_Wall_Batch_VT
```

原因：

- 墙面通常是大量垂直模块和建筑件，直接全部 RVT 化会增加 volume、方向、边界和 VT pool 管理复杂度。
- 墙面静态效果更适合在构建阶段 bake 到 cluster VT Atlas。
- 局部动态墙面效果仍保留 runtime decal 或 overlay，不进入第一轮静态合批。

### 4.4 运行时 batch 材质采样契约

```text
ProxyMesh UV7.x
  -> BatchMaterialIndex
  -> PropertyTexture row
  -> VT Atlas UVRect / 参数
  -> BaseColor / Normal / ORM / Mask / 可选 Emissive
```

`PropertyTexture` 至少记录：

- `UVRectMin`
- `UVRectMax`
- `BaseColor 参数`
- `Normal 强度`
- `Roughness/Metallic/AO 参数`
- `SurfaceKind`
- `BakePolicy`
- `MaterialQualityFlags`
- `LightInfoTexturePath` 或材质灯降级参数

`Tex_LightInfo` 保持独立质量分级路径，不并入普通 BaseColor/Normal/ORM bake。

### 4.5 静态贴花分类

| 类型 | 处理方式 |
| --- | --- |
| BakeStatic | 参与地面/墙面 bake，运行时移除或不加载 |
| RuntimeDynamic | 保留为运行时 Decal/RVT/overlay，不参与静态 bake |
| GameplayIndicator | 玩法提示/范围显示，不参与 bake，不进入合批 |

## 5. 工具与数据结构任务

### 5.1 EnvBatch Tagger / 美术资产管理器

需要支持：

- 给选中 Actor 打 `EnvBatch.Source.<Group>`。
- 给代理或生成结果标记 `EnvBatch.Proxy.<Group>.<Tier>`。
- 给地面 bake 标记 `EnvBatch.Baked.Ground.<Tier>`。
- 给静态贴花标记 `EnvBatch.BakeStaticDecal.<Group>`。
- 给动态贴花标记 `EnvBatch.RuntimeDecal`。
- 给不可处理对象标记 `EnvBatch.Exclude`。

资产管理视图应显示：

- Source Mesh。
- Optional explicit proxy mesh。
- GeneratedBatchProxy Mesh。
- Source LOD index，默认 0。
- Proxy LOD index，默认 1。
- Source/Proxy asset pairing readiness。
- 是否使用 GeneratedBatchProxy fallback。
- Optional explicit proxy 是否缺 Source 引用。
- SurfaceKind。
- InteractionPolicy。
- BakePolicy。
- TextureBackend。
- 当前档位推荐显示层。
- ValidationWarnings。

### 5.2 MaterialBatchBuild 扩展

新增或等价支持：

```text
-TextureBackend=VTAtlas
-SurfaceKind=Ground|Wall|Building|MixedStatic
-BakePolicy=StaticBake|RuntimeOverlay|Exclude
-Tier=Epic|High|Mid|Low
-Cluster=<Name>
-RequireTag=EnvBatch.Source
-ReportStaticDecals
-ValidateSourceProxyExclusivity
```

现有 `-ApplyTextureArraysOnly` 只作为旧路径保留。新主线应新增 `-ApplyVTAtlasOnly` 或等价阶段。

### 5.3 MappingData 扩展

`UMaterialBatchMappingDataAsset` 后续应增加或迁移到通用字段：

- `TextureBackend = VTAtlas`
- `VTAtlasPackage`
- `VTAtlasChannel`
- `UVRectMin`
- `UVRectMax`
- `SourceTexturePath`
- `SurfaceKind`
- `BakePolicy`
- `SourceProxyExclusivityGroup`
- `SourceProxyAssetReadiness`
- `SourceAssetPath`
- `ProxyAssetPath`
- `SourceLODIndex`
- `ProxyLODIndex`
- `bUsesGeneratedProxy`
- `bUsesAuthoredProxy`
- `bReadyForAssetPairing`
- `EstimatedStreamingPoolMB`
- `EstimatedVTPoolMB`
- `DuplicateResidencyRisk`

保留现有：

- `BatchMaterialIndex`
- `PropertyTexture`
- `GeometrySources`
- `MaterialSlotRemap`
- `ProxyMeshPackage`

## 6. 任务拆分与执行顺序

### Phase 0：只读审计和计划同步

任务：

1. 用 MCP 读取 `M_Env_Building`、候选地面材质、候选墙面材质。
2. 生成材质参数、Static Switch、MaterialAttributes、贴图输入和 `Tex_LightInfo` 审计报告。
3. 跑 corridor 关卡静态审计，统计 Actor、StaticMeshComponent、material slot、LOD、mobility、贴花、灯光。
4. 确认 `r.VirtualTextures` 当前状态和相关 DeviceProfile/Scalability 配置。
5. 输出当前风险清单。

产物：

- 材质 MCP 审计报告。
- 场景静态审计报告。
- 当前配置审计报告。

验收：

- 每个候选和拒绝对象都有原因。
- 能明确哪些材质分支属于地面、墙面、贴花、材质灯。

### Phase 1：标签和资产管理工作流

任务：

1. 扩展 EnvBatch Tagger 标签集合。
2. 新增或扩展美术资产管理编辑器。
3. 支持查看 Source/GeneratedBatchProxy/optional explicit proxy 状态。
4. 标出误入合批的动态物、交互物、角色/VFX。
5. 支持静态贴花和动态贴花分类。
6. 增加 Source/Proxy 资产配对检查：Source 默认 LOD0，Proxy 默认 LOD1。
7. optional explicit proxy 如果被配置，必须能填写或追溯 Source；缺失时在资产管理器和报告中标红。普通资产不要求配置 explicit proxy。
8. 支持导入设置或集中资产管理器两种配置来源，最终都写入同一套 MappingData/report 字段。

产物：

- 编辑器工具入口。
- 资产状态表。
- 标签使用说明。
- Source/Proxy 配置状态表。
- SourceProxyAssetReadiness report/manifest/mapping data。

验收：

- 美术能在编辑器中看出哪些对象缺 Source/Proxy。
- 工具能阻止交互物和动态对象进入第一轮 batch。
- Source tag 对象默认生成 `SourceLODIndex=0`、`ProxyLODIndex=1` 的 generated proxy fallback 记录。
- optional explicit proxy 没有 Source 引用时状态必须为 `MissingSourceAssetReference`，不能被当作已配置完成；普通资产使用 generated proxy fallback。

### Phase 2：模型合批与 Source/Proxy

任务：

1. 对 corridor 小 cluster 运行 dry-run。
2. 对重复件输出 ISM/HISM 建议。
3. 对静态墙体/建筑件生成 Geometry Merge 计划。
4. 生成 `SM_BatchProxy_<Cluster>`。
5. 写入 `UV7.x = BatchMaterialIndex`。
6. 建立 Source/Proxy/Baked 互斥加载测试层。
7. 输出 Source/Proxy asset readiness，确认每个 Source actor 可追溯到 Source Mesh 和 proxy 方案。
8. 对 optional explicit proxy 跑缺失 Source 引用检查，并生成修复清单。

产物：

- ProxyMesh。
- Geometry merge manifest。
- Source/Proxy 切换测试 Streaming Level 或测试加载规则。
- Source/Proxy asset pairing manifest。
- optional explicit proxy 缺失 Source 引用清单。

验收：

- ProxyMesh 能回溯每个 source actor。
- Source 和 Proxy 不会同时加载。
- `UV7.x` 数据可被材质验证。
- `SourceProxyAssetReadiness`、`SourceProxyLayerReadiness`、`ResidencyRiskPlan` 三个 gate 同时通过后，才允许进入真实 cluster 推广。

### Phase 3：地面和墙面 bake

任务：

1. 定义 Ground Authoring 输入：多层材质、Height Blend、Mesh Paint、顶点色、静态贴花。
2. 定义 Wall Authoring 输入：Height Blend、顶点色、静态贴花、unique 贴图。
3. 实现或接入静态 bake 输出协议。
4. 标记 dynamic overlay 不参与 bake。
5. 生成地面 Baked SVT/VT 和墙面 Wall VT Atlas 测试资源。

产物：

- Ground bake 样本。
- Wall bake 样本。
- Bake manifest。

验收：

- Bake 后视觉接近 authoring 预览。
- 静态贴花 bake 后不再产生运行时 decal 成本。
- 动态贴花仍可运行时叠加。

### Phase 4：VT Atlas 和 batch runtime 材质

任务：

1. 自动收集 batch 候选的 unique 贴图。
2. 生成 cluster VT Atlas。
3. 生成 PropertyTexture。
4. 扩展 MappingData 记录 UVRect 和 VT 包路径。
5. 创建或调整 `M_Env_Wall_Batch_VT` 和相关 runtime 材质函数。
6. 确认 `Tex_LightInfo` 按性能档独立降级。

产物：

- VT Atlas。
- PropertyTexture。
- Batch runtime material。
- MappingData。

验收：

- `UV7.x -> PropertyTexture -> VT Atlas UVRect` 采样正确。
- 同一 cluster 不同时使用 VT Atlas 和 Texture2DArray 主路径。

### Phase 5：性能分级接入

任务：

1. 将正式档位统一为 Epic / High / Mid / Low。
2. 更新 DeviceProfile/Scalability 映射。
3. 更新玩家设置 UI 和存档字段显示。
4. 接入项目自定义 CVar。
5. 将 Source/Proxy/Baked 选择绑定到档位。
6. VT/RVT 先在测试配置启用，实测通过后再决定默认。

产物：

- 四档画质配置。
- 玩家设置 UI 映射。
- CVar 应用逻辑。
- 档位到资源层的映射表。

验收：

- Epic/High/Mid/Low 四档可独立应用。
- Mid/Low 会偏向 Proxy/Baked。
- Epic/High 默认保留更多 Source。

### Phase 6：测试、验收和推广

任务：

1. 对 corridor cluster 跑完整测试矩阵。
2. 输出视觉对比。
3. 输出性能数据。
4. 输出内存数据。
5. 记录失败项和下一轮修正。
6. 通过后制定更大 cluster 推广规则。

产物：

- Profiling 报告。
- 视觉 parity 报告。
- VT pool / streaming 报告。
- 推广准入清单。

验收：

- Draw call 明确下降。
- GPU ms 不恶化。
- VT pool 无持续溢出或严重 page miss。
- Source/Proxy 不双加载。
- Mid 档可作为默认推荐档。
- Low 档稳定可玩。

### 6.7 端到端工作包拆分

后续开发按工作包推进，不能只完成框架或字段后停止。每个工作包都必须有输入、实现、报告和阻断条件。

| 工作包 | 范围 | 具体任务 | 产物 | 阻断条件 |
| --- | --- | --- | --- | --- |
| WP0 执行上下文 | 仓库规则、计划、状态 | 读 `guide.md`、本文档、heartbeat、audit、dry-run latest；确认 dirty 文件范围 | 本次执行记录、heartbeat | 未读取 `guide.md` 或主计划 |
| WP1 目标 cluster 选择 | corridor 小 cluster | 确认 Map、Actor 集合、静态/动态/交互分类、Ground/Wall/Building 分组 | cluster 候选清单 | 无法区分静态环境与交互对象 |
| WP2 材质接口/模板 | `M_Env_MasterA_Source`、`M_Env_Baked_VTAtlas`、地面、墙面、贴花 | 固定性能分级接口、Source 节点模板、baked/VT 采样合同；MCP 已创建并反查 Source 模板参数和基础输出连线 | 材质接口规范、模板 MCP 证据、模板 commandlet fallback 报告 | 没有模板/接口合同，或只看文档不检查实际材质资产 |
| WP3 EnvBatch 标记入口 | 原生 Slate 与 Python fallback | Source/Proxy/Baked/Exclude、Ground/Wall Mid/Low、静态/动态贴花、readiness summary | `EnvBatchTagTools/LATEST.md` | native 与 Python tag 合同不一致 |
| WP4 Source/Proxy 配置 | 导入设置/资产管理器/MappingData | Source 默认 LOD0、Proxy 默认 LOD1、特殊模型可选 explicit proxy、generated fallback 标记 | `SourceProxyAssetReadiness` | optional explicit proxy 缺 Source 仍被当作 ready，或普通资产被误要求手工代理 |
| WP5 模型合批计划 | ISM/HISM、Geometry Merge、HLOD/Proxy | 输出重复件建议、merge 分桶、ProxyMesh 命名、`UV7.x` batch index | Geometry merge manifest | 交互物、角色、动态物误入 |
| WP6 StreamingLevel 互斥 | Source/Proxy/Baked | 生成 expected StreamingLevel 名、读取 actual actor StreamingLevel、比较 mismatch | `SourceProxyLayerReadiness` | Source 与 Proxy/Baked 同时加载 |
| WP7 地面 bake | Ground authoring -> baked runtime | 多层材质、height blend、mesh paint/顶点色、静态贴花进入 bake；动态 RVT/overlay 留在 bake 后 | Ground bake manifest、视觉对比 | 静态贴花仍以 runtime decal 常驻 |
| WP8 墙面 bake | Wall authoring -> Wall VT Atlas | 墙面 height blend、顶点色、污渍/裂缝/苔藓、静态贴花 bake 到墙面 atlas | Wall bake manifest、视觉对比 | 默认把墙面全量 RVT 化且无 pool 评估 |
| WP9 VT Atlas 生成 | batch 材质贴图后端 | 收集 unique 贴图、生成 shared VT Atlas、写 UVRect、写 PropertyTexture | VT Atlas、PropertyTexture、MappingData | 只给每张 unique 贴图单独开 SVT 就声称材质合批 |
| WP10 Batch parent material | `VT_Atlas + _PropTexture` | 材质采样链路、Base/Normal/ORM UVRect、`Tex_LightInfo` 独立降级、Texture2DArray fallback 降级 | batch material setup report | 主线仍依赖 Texture2DArray |
| WP11 玩家性能设置 | UI、SaveGame、DeviceProfile、Scalability | Epic/High/Mid/Low、商业设置项、自定义 CVar、资源层选择 | 设置 UI、config、自动化测试 | 旧平台档位重新进入正式 API |
| WP12 真实 dry-run | 目标 Map + cluster | `MaterialBatchAudit` + `MaterialBatchBuild` dry-run，输出 report/manifest，不写资产 | `MaterialBatchDryRun/LATEST.md` | build 未干净链接或缺 StreamingLevel/SourceProxy/residency 证据 |
| WP13 实测 profiling | 视觉、性能、内存 | Baseline 与各变体对比，记录 draw call、GPU ms、streaming pool、VT pool、page miss、hitch | profiling 报告、截图/CSV | 只有静态契约，没有 UE 实测 |
| WP14 提交/上传 gate | 提交前验收 | submission gate、final upload preflight、artifact contract、audit、heartbeat 全部刷新 | `SubmissionGate/LATEST.md` | `MaterialBatch dry-run ready=False` |

### 6.8 阶段门禁

每个阶段必须满足以下 gate 才能进入下一阶段：

| Gate | 必须为 True 的证据 | 对应报告 |
| --- | --- | --- |
| Governance | `guide.md` 已读、主计划定位、模块边界、生成资产规则、Source/Proxy/Residency gate 存在 | `UE58PerformanceAutomation/LATEST.md` |
| Material Interface | `FYogMaterialPerformanceTierInterface`、Source 模板、baked/VT 合同、贴图命名规则已存在；Source 模板已通过 MCP 创建和反查 | 材质规范、`MaterialTemplateMcp/LATEST.md`、模板 commandlet fallback 报告 |
| EnvBatch Tooling | native/Python tag 合同一致，Ground/Wall Mid/Low baked tags 存在 | `EnvBatchTagTools/LATEST.md` |
| SourceProxy Asset | 每个 Source/Proxy entry 有 asset path、LOD、readiness status | `SourceProxyAssetReadiness` |
| Layer Readiness | expected 与 actual StreamingLevel 可比对，Source/Proxy/Baked 不冲突 | `SourceProxyLayerReadiness` |
| Residency | VT Atlas 主路径、非 VT fallback、Source unload 要求、pool 估算可见 | `ResidencyRiskPlan` |
| DryRun | `Status: DryRunCaptured`，且包含 StreamingLevel layer、Source/Proxy asset、residency 三项证据 | `MaterialBatchDryRun/LATEST.md` |
| Profiling | Baseline 与 batch 变体的视觉、draw call、GPU、streaming、VT pool 对比完成 | runtime profiling 报告 |
| Submission | artifact contract、static audit、heartbeat、submission gate 均刷新且无阻断 | `SubmissionGate/LATEST.md` |

## 7. 测试矩阵

每个代表 cluster 至少测试：

```text
Baseline
Ground Baked Only
Wall VT Batch Only
Batch Proxy + VT Atlas
Ground Baked + Wall Batch
Lumen Lite Only
Batch + Lumen Lite
Low
Epic
```

记录指标：

```text
FPS
Game ms
GPU ms
RHI ms
Mesh Draw Calls
Total Draw Calls
Material pass cost
Shadow cost
Lumen cost
Texture streaming pool
VT physical pool
VT page miss
Streaming hitch
Source/Proxy 是否双加载
```

命令：

```text
stat unit
stat rhi
stat scenerendering
stat gpu
profilegpu
r.MeshDrawCommands.LogDynamicInstancingStats 1
```

VT 相关测试需额外记录 VT residency、pool 使用和 page miss。

## 8. 自动化续跑安排

目标：每 5 小时恢复一次当前线程，读取最新计划和报告，继续从未完成任务推进。

续跑任务必须执行：

1. 读取 `guide.md`。
2. 读取本文档。
3. 检查 `git status --short --branch`，不回滚无关改动。
4. 读取最新 heartbeat 报告。
5. 读取最新 MCP 材质审计、MaterialBatch dry-run、性能审计报告。
6. 找出 Phase 0-6 中第一个未完成任务。
7. 继续执行该任务。
8. 每次结束前更新计划进度和 heartbeat 报告。

已有脚本入口：

```powershell
BuildScripts\Automation\Run-UE58PerformanceGoalHeartbeat.ps1
```

自动化 prompt 应始终引用本文档作为当前主计划。

### 8.1 仓库执行规则纳入计划

本项目落地时必须把 `AGENTS.md` 与仓库根目录 `guide.md` 当作执行前置条件，而不是独立备注。所有模型、材质、性能分级、自动化续跑任务都按以下规则推进：

1. 启动前读取仓库根目录 `guide.md`，确认当前项目方向、编译限制、GameplayTag 迁移规则和运行时架构约束。
2. 读取本文档，确认当前阶段、未完成项、最近验证结果和阻塞原因。
3. 检查 `git status --short --branch`，只处理本任务相关文件；发现无关改动时保留，不回滚。
4. 按模块边界拆分任务：
   - `Source/DevKit`：运行时画质档位、保存数据、UI 设置、MappingData 运行时读取。
   - `Source/DevKitEditor`：MaterialBatch、EnvBatch Tagger、Commandlet、性能 profiling 计划、编辑器验证工具。
   - `Config/Tags` 与 `Config/DefaultGameplayTags.ini`：只在确实需要 gameplay tag 支撑批处理/排除/层级标记时修改，并遵守 `guide.md` 的标签归属规则。
   - `Config/DefaultDeviceProfiles.ini` 与 `Config/DefaultScalability.ini`：只维护 `Epic / High / Mid / Low` 四档与项目 CVar 映射。
   - `Docs` 与 `Docs/GeneratedReports`：记录计划、heartbeat、审计报告、dry-run 报告和验收证据。
5. 手工文本/代码改动使用 `apply_patch`；批量格式化或自动生成报告可由脚本产生。
6. 不手工编辑二进制 `.uasset`。材质、贴图、MappingData、batch parent material 等资产必须通过 Commandlet 或编辑器工具生成。
7. 不把角色、交互物、VFX、Gameplay 动态物纳入第一轮静态合批；这些对象默认通过 `EnvBatch.Exclude` 或等价排除策略处理。
8. 不让 Source、Proxy、Baked 在同一目标档位同时常驻。涉及 VT Atlas 与非 VT fallback 的任务必须同时检查 `ResidencyRiskPlan` 与 Source/Proxy/Baked layer readiness。
9. 编译验证遵守当前线程授权：未授权时只做静态契约、搜索、脚本检查；已授权时优先使用 UE5.8 `Build.bat` 或项目认可的 `CompileAndOpen.bat`。
10. 每次实现或续跑结束，必须把真实验证结果写回本文档或 heartbeat，包括未验证项和阻塞原因。

这些规则也进入验收：如果某个任务只完成了功能代码，但没有说明模块归属、生成方式、验证结果和是否触碰 `.uasset`，该任务不算完整完成。

## 9. 当前默认约束

- 不编译，除非当前线程明确要求。
- 不手动编辑二进制 `.uasset`。
- 不替换正式关卡。
- 不全关卡自动合并。
- 不把 VT/SVT 误认为 draw call 合批本身。
- 不让 Source 和 Proxy 同时加载。
- 不让交互物、角色、VFX、动态物进入第一轮合批。
- Texture2DArray 旧路径保留，但不作为新主线验收目标。
- 仓库执行约束纳入计划验收：启动前读 `guide.md`，按 `Source/DevKit`、`Source/DevKitEditor`、`Config`、`Docs` 的职责边界推进；所有生成资产必须走 Commandlet/编辑器工具；所有续跑必须记录真实验证和阻塞。
- 材质处理要求优先通过材质 MCP 审计或 MCP 创建证据完成；不能把静态脚本审计冒充为实际材质图证据。本轮已通过 UE MCP 创建并反查 Source 材质模板。
- 每个阶段结束不能只写“框架完成”。必须写明：已处理对象、排除对象、生成资产方式、实际报告路径、测试命令、未验证项和下一步阻塞。

## 10. 当前实现进展

更新时间：2026-06-30 00:51

计划整理补充：2026-06-30 00:51

已整理进计划：

- 已把 `AGENTS.md` 的仓库执行要求前移为 `## 0. 落地执行前置规则`，后续每个模型、材质、性能分级任务都必须先读 `guide.md`、读本文档、检查 `git status`，并按模块职责推进。
- 已补齐商业游戏常见四挡默认参数表：Render Scale、View Distance、Shadow、Texture、Material、Model、Effects、Post Process、GI、Reflection、Dynamic Overlay、Batch Proxy、VT Atlas、Material Light。
- 已明确四挡到资源层的默认绑定：
  - `Epic`：Source 为主，高质量 VT Atlas，保留对比能力。
  - `High`：Source + 已审核 Proxy，Ground bake 默认启用，Wall bake 进入高压 cluster。
  - `Mid`：Proxy/HLOD 与 Ground/Wall baked 默认启用。
  - `Low`：Baked/Proxy 优先，非必要动态 overlay 关闭。
- 已新增 `6.7 端到端工作包拆分`，把后续落地拆成 WP0-WP14：执行上下文、目标 cluster、材质接口/模板、EnvBatch 工具、Source/Proxy 配置、模型合批、StreamingLevel 互斥、地面 bake、墙面 bake、VT Atlas、batch parent material、玩家性能设置、真实 dry-run、profiling、提交 gate。
- 已新增 `6.8 阶段门禁`，把 Governance、Material Interface、EnvBatch Tooling、SourceProxy Asset、Layer Readiness、Residency、DryRun、Profiling、Submission 全部作为可检查 gate。
- 已把“材质 MCP 审计或 MCP 创建证据缺失不能算材质兼容性通过”写入当前默认约束；本轮已补充 Source 模板的 MCP 创建和参数/输出反查证据。
- 已明确每个阶段结束必须记录对象、排除项、生成方式、报告路径、测试命令、未验证项和阻塞，不能只以“框架完成”收尾。

已落地：

- 运行时正式目标档位收敛为 `Epic / High / Mid / Low`，旧 `PCUltra / SteamDeck15W / Switch2Candidate / SteamDeck5W / FallbackLow` 不再作为玩家目标档位 API。
- `FYogGraphicsSettings` 增加 `MaterialQuality`、`DynamicOverlayQuality`、`VTAtlasQuality`，保留动态灯和材质灯预算字段。
- `YogPerformanceSettingsLibrary` 已写入 UE 原生 `r.MaterialQualityLevel`，用于驱动材质 `Quality Switch`；同时写入 `r.Yog.MaterialQuality`、`r.Yog.DynamicOverlayQuality`、`r.Yog.BatchProxyPreference`、`r.Yog.VTAtlasQuality`、`r.Yog.MaterialLightQuality` 和 `r.Yog.MaterialLight.MaxLightInfoCount` 作为项目增强预算。
- UE 材质质量枚举顺序为 `Low=0, High=1, Medium=2, Epic=3`；项目面向玩家和美术仍使用 `Low=0, Mid=1, High=2, Epic=3`，代码中显式完成映射。
- `DefaultDeviceProfiles.ini` 已改为 `DevKit_Epic / DevKit_High / DevKit_Mid / DevKit_Low`，并补齐项目自定义 CVar 默认值。
- Graphics Settings 原生 Widget 和生成命令入口已切到 `BtnTierEpic / BtnTierHigh / BtnTierMid / BtnTierLow`。
- `MaterialBatchBuild` 默认 Tier 改为 `Mid`，新增 `-TextureBackend=VTAtlas`、`-SurfaceKind=Ground|Wall|Building|MixedStatic`、`-BakePolicy=StaticBake|RuntimeOverlay|Exclude`、`-SourceProxyExclusivityGroup=...`、`-ReportStaticDecals`、`-ValidateSourceProxyExclusivity`、`-ApplyVTAtlasOnly` 的记录入口。
- `UMaterialBatchMappingDataAsset` 已增加 `TextureBackend`、`VTAtlasPackage`、`VTAtlasChannel`、`SurfaceKind`、`BakePolicy`、`SourceProxyExclusivityGroup`、`EstimatedStreamingPoolMB`、`EstimatedVTPoolMB`、`bDuplicateResidencyRisk`，并在 property row 记录 surface/bake/material quality 信息。
- EnvBatch Tagger 的代理档位标签已从 `Medium` 改为 `Mid`。
- runtime profiling 场景命名已从平台名切到 `Mid / Low / Epic` 语义。
- `Test-UE58PerformanceArtifactContract.ps1` 已更新到四档和 VT Atlas 契约，并已通过一次静态检查。

未完成：

- `-ApplyVTAtlasOnly` 目前只记录 VT Atlas 资产路径契约，尚未真正生成 VT Atlas 贴图资产。
- `Texture2DArray` 旧路径仍存在于已有生成链路中，后续需要降级为 fallback，不再作为主线验收。
- Source/Proxy 互斥目前已有字段和风险标记，尚未实现真实 Data Layer/子关卡切换验证。
- 地面/墙面 bake 样本、视觉 parity、VT pool/streaming pool 实测尚未生成。
- 本轮未编译，符合 `guide.md` 的当前约束。

更新补充：2026-06-29 22:45

已继续落地：

- `MaterialBatchBuild` 扫描 Map Actor 时会收集 `EnvBatch.*` 标签，并写入每个 planned entry 的 `EnvBatchTags`。
- 新增 `FMaterialBatchBuildTagDiagnostics`，统计 Source、Proxy、Baked、Exclude、BakeStaticDecal、RuntimeDecal、GameplayIndicator 等 Actor 数量。
- `-ValidateSourceProxyExclusivity` 会在同一 Actor 同时带有 Source 与 Proxy/Baked 标签时输出 Source/Proxy 互斥警告。
- `-ReportStaticDecals` 会在同一 Actor 同时标记 BakeStaticDecal 与 RuntimeDecal 时输出警告，方便区分静态贴花 bake 与运行时动态贴花。
- MaterialBatch Markdown 报告新增 `EnvBatch Tag Diagnostics`，JSON manifest 新增 `tagDiagnostics` 与每个 entry 的 `envBatchTags`，用于后续编辑器资产管理器、自动化验收和 CI 契约消费。
- `Test-UE58PerformanceArtifactContract.ps1` 已加入 Source/Proxy 与贴花诊断契约检查。

仍未完成：

- `-ApplyVTAtlasOnly` 仍未真正生成 VT Atlas 贴图资产。
- Source/Proxy/Baked 仍未接入真实 Data Layer 或子关卡加载互斥验证。
- 静态地面/墙面 bake 的视觉 parity、VT pool、streaming pool 实测还未生成。

更新补充：2026-06-29 22:55

已继续落地：

- 新增 `FMaterialBatchBuildVTAtlasEntry` 和 `FMaterialBatchMappingVTAtlasEntry`，把每个唯一贴图通道的 atlas entry、UVRect、源贴图路径、尺寸和估算 MB 写成稳定数据。
- `BuildVTAtlasEntries` 已提供确定性 `DeterministicGridByUniqueTexture` 布局，后续 `-ApplyVTAtlasOnly` 可以基于该计划真正写入 VT Atlas 贴图资产。
- MaterialBatch Markdown 报告新增 `Planned VT Atlas Entries`，JSON manifest 新增 `vtAtlas.entries`、`estimatedVTPoolMB`、`estimatedStreamingPoolMB` 和 `duplicateResidencyRisk`。
- `UMaterialBatchMappingDataAsset` 现在保存 `VTAtlasEntries`，并按同一批源贴图估算 VT pool 与非 VT streaming pool；当 `VTAtlas` backend 和 Texture2DArray fallback 同时有源贴图时，会显式标记混用驻留风险。
- `BuildBatchMaterialPayload` 在 `TextureBackend=VTAtlas` 时默认绑定 `VT_Atlas + _PropTexture`，Texture2DArray 只保留为 fallback 路线。
- artifact contract 已加入 VT Atlas build plan、UVRect、pool 估算和材质绑定检查。

仍未完成：

- `-ApplyVTAtlasOnly` 仍是记录计划，不会真正创建或合并 VT Atlas 贴图像素。
- batch parent material 资产本身尚未实测 `VT_Atlas + _PropTexture + UVRect` 采样链路。
- VT 与非 VT 混用目前有计划级风险标记，尚未跑真实 VT physical pool / streaming pool profiling。

更新补充：2026-06-29 23:05

已继续落地：

- `MaterialBatchBuild -ApplyVTAtlasOnly` 已接入 `SaveVTAtlasAsset`，会按 `BuildVTAtlasPayload` 创建或更新 `/Game/Generated/MaterialBatch/<Tier>/<Cluster>/VT_Atlas_<Cluster>`。
- 第一版真实写资产只接受 `TSF_BGRA8` 的源 `Texture2D`，会读取 mip0 并拷贝到确定性 atlas cell；不兼容贴图会失败并写入报告原因，避免静默生成错误像素。
- 生成的 atlas 是 `UTexture2D`，设置 `VirtualTextureStreaming=true`、`NeverStream=false`，作为后续 SVT/VT 主线材质输入。
- `BuildVTAtlasPayload` 现在输出 atlas 宽高、行列数和 entries，Markdown/manifest 会记录 planned atlas size/grid。
- `Test-UE58PerformanceArtifactContract.ps1` 已覆盖 `SaveVTAtlasAsset`、`CopyTextureSourceIntoAtlasCell`、`TSF_BGRA8`、`VirtualTextureStreaming` 和 `bSavedVTAtlas`。

仍未完成：

- 还没有在真实美术 cluster 上运行 `-ApplyVTAtlasOnly`，所以资产生成路径未经过 UE Editor 实测。
- 当前 atlas 写入只支持 BGRA8 源贴图；HDR、BC 压缩源、法线/ORM 的格式差异和色彩空间策略还需要扩展。
- batch parent material 仍需实测 `VT_Atlas + _PropTexture + UVRect` 采样链路，并确认墙面/地面 bake 后的视觉 parity。

更新补充：2026-06-29 23:20

已继续落地：

- `_PropTexture` layout 从 5 列扩展到 9 列：前 5 列保留 BaseColor/Normal/ORM/Emissive/Mask slice，后 4 列新增 `VTUVMinX / VTUVMinY / VTUVMaxX / VTUVMaxY`。
- `BuildPropertyTexturePayload` 已写入 VT atlas UVRect float 数据，后续 batch parent material 可通过 `BatchMaterialIndex -> _PropTexture row -> VT_Atlas UVRect` 采样。
- MappingData、Markdown report、JSON manifest 的 property rows 同步记录 `SourceTexturePath` 和 `UVRectMin/UVRectMax`，与实际 property texture payload 保持一致。
- `MaterialBatchParentMaterialSetupCommandlet` 的 `PropertyColumnCount` 默认值从 5 改为 9，避免生成材质参数与 property texture layout 不一致。
- 已运行 UE5.8 `Build.bat DevKitEditor Win64 Development`。相关变更文件 `MaterialBatchBuildPlan.cpp`、`MaterialBatchBuildCommandlet.cpp`、`MaterialBatchCandidateRulesTests.cpp`、`MaterialBatchParentMaterialSetupCommandlet.cpp` 已通过 C++ 编译阶段；最终链接失败原因是当前打开的 UnrealEditor 占用 `UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll` 和 `UnrealEditor-YogComboGraphEditor.dll`。

仍未完成：

- 需要关闭当前 UnrealEditor 后重新执行一次完整 build，确认链接阶段通过。
- batch parent material 的 custom HLSL 仍是 Texture2DArray fallback 采样路径，尚未真正切到 `VT_Atlas + UVRect` 采样。
- 真实 cluster 上的 `-ApplyVTAtlasOnly` 资产生成和 VT pool profiling 尚未执行。

更新补充：2026-06-29 23:30

已继续落地：

- `_PropTexture` layout 从 9 列继续扩展到 17 列：前 5 列保留 BaseColor/Normal/ORM/Emissive/Mask slice，后 12 列分别记录 BaseColor、Normal、ORM 三类贴图的 `VTUVMin/VTUVMax`。
- `BuildPropertyTexturePayload`、MappingData、Markdown report、JSON manifest 已同步记录 Base/Normal/ORM 的 source texture 与 UVRect，避免只用 BaseColor rect 去采样法线或 ORM。
- `MaterialBatchParentMaterialSetupCommandlet` 已把主采样路径切到 `VT_Atlas + _PropTexture UVRect`；`T_Array_A/N/M` 只作为旧生成资产兼容参数保留，不再作为新主线验收目标。
- `PropertyColumnCount` 默认值已从 9 改为 17，`BuildBatchMaterialPayload` 和自动化测试同步使用 17 列契约。
- `Analyze-UE58PerformanceInputs.ps1` 与 `Test-UE58PerformanceArtifactContract.ps1` 已改为检查 `VT_Atlas` 采样证据，不再把 `Texture2DArraySample` 当作主线成功条件。
- 已按仓库执行约束把 `guide.md`、不回滚无关改动、需要编译时记录真实 UBT 结果、二进制 `.uasset` 不手工改写等要求纳入续跑流程与 heartbeat。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `git diff --check`：通过，仅有 CRLF 提示。
- UE5.8 `Build.bat DevKitEditor Win64 Development`：相关 C++ 文件已完成编译；最终失败停在链接阶段，原因是当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll`、`UnrealEditor-YogComboGraphEditor.dll`，错误为 `LNK1104 cannot open file`。

仍未完成：

- 尚未关闭当前 UnrealEditor 重新跑完整链接验证；关闭编辑器前需要确认没有未保存内容。
- 尚未运行 `MaterialBatchParentMaterialSetup -Apply -Force` 更新实际 `M_Env_Baked_VTAtlas.uasset`，当前只完成生成命令和契约层修改。
- 真实 cluster 上的 `-ApplyVTAtlasOnly`、`-ApplyPropertyTextureOnly`、batch parent material 视觉校验、VT physical pool / streaming pool profiling 仍未执行。
- Source/Proxy/Baked Data Layer 互斥加载仍未做真实关卡验证。

更新补充：2026-06-29 23:45

已继续落地：

- 新增 `SourceProxyLayerPlan`，把每个 cluster 的 Source / Proxy / Baked 推荐加载层写成稳定契约：`DL_<Group>_Source`、`DL_<Group>_Proxy`、`DL_<Group>_Baked`。
- `MaterialBatchBuild` 的 Markdown report、JSON manifest 和 `UMaterialBatchMappingDataAsset` 现在都会记录 Source/Proxy/Baked layer plan。
- layer plan 已绑定正式四档：
  - `Epic`：加载 Source，保持最高视觉一致性。
  - `High`：默认加载 Source，允许后续按已审核 cluster 切 Proxy。
  - `Mid`：加载 Proxy，走生成 proxy mesh + VT atlas batch material。
  - `Low`：加载 Baked，若 baked 输出缺失再降级回 Proxy。
- layer plan 会把 `SourceProxyConflictActorCount > 0` 反映成 `hasTagConflicts=true` / `Tag conflict gate: Blocked`，用于阻止 Source 与 Proxy/Baked 双加载直接进入验收。
- `Test-UE58PerformanceArtifactContract.ps1` 已加入 Source/Proxy/Baked layer plan 契约检查，要求 report/manifest/mapping data 都具备四档加载选择。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `git diff --check`：通过，仅有 CRLF 提示。
- UE5.8 `Build.bat DevKitEditor Win64 Development`：新增/相关 C++ 文件已完成编译；最终仍失败在链接阶段，原因仍是当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll`、`UnrealEditor-YogComboGraphEditor.dll`，错误为 `LNK1104 cannot open file`。

仍未完成：

- 真实 DataLayer 或子关卡资产尚未创建/修改；当前完成的是可消费的 layer plan 契约和自动化检查。
- 需要在关闭 UnrealEditor 并确认无未保存内容后，重新跑完整链接验证。
- 需要后续用真实 cluster 运行 `MaterialBatchBuild`，检查 Source/Proxy/Baked layer plan 与实际关卡 DataLayer 设置是否一致。

更新补充：2026-06-30 00:05

已继续落地：

- 新增 `SourceProxyLayerReadiness`，把 `MaterialBatchBuild` 的 planned entries 逐项映射到 `Source / Proxy / Baked / Excluded / Conflict / Unassigned`。
- readiness 会为每个 entry 输出预期 DataLayer 名、是否可进入后续 DataLayer 验证、失败原因和原始 `EnvBatch.*` 标签。
- Markdown report 新增 `Source/Proxy/Baked Layer Readiness` 表；JSON manifest 新增 `sourceProxyLayerReadiness`；`UMaterialBatchMappingDataAsset` 新增同名结构，供后续编辑器工具、CI 或蓝图读取。
- readiness gate 当前覆盖：
  - 缺少 `EnvBatch.Source.* / EnvBatch.Proxy.* / EnvBatch.Baked.*` 的条目记为 `Unassigned`。
  - 同一条目同时带多个 Source/Proxy/Baked 标签记为 `Conflict`。
  - `EnvBatch.Exclude` 条目记为 `Excluded`，不进入 batch 层验证。
  - 正常 Source/Proxy/Baked 条目会指向对应 `DL_<Group>_Source / Proxy / Baked`。
- `Test-UE58PerformanceArtifactContract.ps1` 已加入 readiness 契约检查，自动确认 report、manifest、MappingData 都有 per-entry layer readiness 数据。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `git diff --check`：通过，仅有 CRLF 提示。
- UE5.8 `Build.bat DevKitEditor Win64 Development`：新增/相关 C++ 文件已完成编译；最终仍失败在链接阶段，原因仍是当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll`、`UnrealEditor-YogComboGraphEditor.dll`，错误为 `LNK1104 cannot open file`。

仍未完成：

- readiness 目前是基于 planned entry 和 `EnvBatch.*` 标签的可审计映射，尚未读取真实 DataLayer 资产成员关系。
- 仍需在真实 cluster 上运行 `MaterialBatchBuild`，生成实际 manifest/report 后按 readiness 修正关卡 DataLayer 或子关卡组织。
- 仍需关闭 UnrealEditor 后重新跑完整链接验证。

更新补充：2026-06-30 00:20

已继续落地：

- 新增 `ResidencyRiskPlan`，把 VT 与非 VT 混用从单个 `bDuplicateResidencyRisk` 扩展为可执行的内存驻留 gate。
- `MaterialBatchBuild` 的 Markdown report 新增 `VT/Non-VT Residency Risk Plan`，JSON manifest 新增 `residencyRiskPlan`，`UMaterialBatchMappingDataAsset` 新增 `ResidencyRiskPlan`。
- 当前策略明确为：`TextureBackend=VTAtlas` 时，`VT_Atlas` 是生产主路径，`Texture2DArray` 只允许作为显式 legacy fallback，不允许和 VT Atlas 在生产加载路径同时常驻。
- 当 VT Atlas 源数据和非 VT fallback 源数据同时存在时，gate 写为 `BlockedUntilSourceProxyUnloaded`，并要求 Source actor 在加载 Proxy/Baked 层前被卸载，避免普通 texture streaming pool 与 VT physical pool 同时上涨。
- risk plan 会记录：
  - `vtAtlasMainPath`
  - `textureArrayFallbackPresent`
  - `allowTextureArrayFallbackInProduction`
  - `requiresSourceProxyUnload`
  - `estimatedVTPoolMB`
  - `estimatedStreamingPoolMB`
  - `estimatedCombinedPoolMB`
  - `residencyGate`
  - `recommendation`
- `Test-UE58PerformanceArtifactContract.ps1` 已加入 residency gate 契约检查；自动化测试也覆盖 report、manifest、MappingData 三处输出。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `git diff --check`：通过，仅有 CRLF 提示。
- UE5.8 `Build.bat DevKitEditor Win64 Development`：新增/相关 C++ 文件已完成编译；最终仍失败在链接阶段，原因仍是当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll`、`UnrealEditor-YogComboGraphEditor.dll`，错误为 `LNK1104 cannot open file`。

仍未完成：

- residency gate 当前仍是计划/估算层验证，尚未替代真实 `stat streaming`、VT physical pool 和 page miss profiling。
- 需要真实 cluster 跑 `MaterialBatchBuild` 后确认 `residencyRiskPlan` 是否能准确反映该 cluster 的 Source/Proxy/Baked 实际加载状态。
- 需要关闭 UnrealEditor 后重新跑完整链接验证。

更新补充：2026-06-30 00:35

已整理进计划：

- 已把本仓库 `AGENTS.md` 中的执行要求纳入主计划，不再只作为外部说明。
- `## 8. 自动化续跑安排` 下新增 `8.1 仓库执行规则纳入计划`，要求每次续跑先读 `guide.md`、再读本文档、检查 `git status --short --branch`，并按模块职责推进。
- 已明确模型、材质、性能分级相关任务的模块边界：
  - `Source/DevKit` 负责运行时画质档位、保存数据、UI 设置、MappingData 运行时读取。
  - `Source/DevKitEditor` 负责 MaterialBatch、EnvBatch Tagger、Commandlet、性能 profiling 计划和编辑器验证工具。
  - `Config/Tags` 与 `Config/DefaultGameplayTags.ini` 只在确实需要批处理/排除/层级标记时修改，并遵守标签归属规则。
  - `Config/DefaultDeviceProfiles.ini` 与 `Config/DefaultScalability.ini` 只维护 `Epic / High / Mid / Low` 四档和项目 CVar 映射。
  - `Docs` 与 `Docs/GeneratedReports` 负责计划、heartbeat、审计报告、dry-run 报告和验收证据。
- 已把生成资产规则写入验收：不手工编辑二进制 `.uasset`，材质、贴图、MappingData、batch parent material 等资产必须通过 Commandlet 或编辑器工具生成。
- 已把 Source/Proxy/Baked 互斥、VT/非 VT residency 风险、`EnvBatch.Exclude` 默认排除对象写入仓库执行约束，防止后续只实现材质或模型功能但漏掉加载/内存验收。

验证状态：

- 本次只更新计划文档，未修改代码和资产。
- 仍需继续把这些执行规则接入 heartbeat 检查项和后续真实 cluster 验收报告。

更新补充：2026-06-30 00:45

已继续落地：

- `Run-UE58PerformanceGoalHeartbeat.ps1` 已新增 `Governance Gate`，每次续跑报告都会检查：
  - 仓库根目录 `guide.md` 是否存在。
  - 当前主计划 `UE58_EpicHighMidLow_*.md` 是否可定位。
  - 主计划是否记录 `Source/DevKit`、`Source/DevKitEditor`、`Config/DefaultDeviceProfiles.ini`、`Docs/GeneratedReports` 等模块边界。
  - 生成资产是否要求走 Commandlet/编辑器工具，而不是手工编辑 `.uasset`。
  - 是否保留 `ResidencyRiskPlan` 与 `Source/Proxy/Baked layer readiness` 作为 VT/非 VT 和加载互斥验收 gate。
- `Test-UE58PerformanceArtifactContract.ps1` 已加入 heartbeat governance 和当前主计划执行规则的静态契约检查；后续如果续跑上下文或主计划丢失这些约束，contract 会失败。
- `Analyze-UE58PerformanceInputs.ps1` 已优先定位当前 `UE58_EpicHighMidLow_*.md` 主计划，不再随机取第一个 `UE58_*.md` 作为综合计划。
- 静态审计的下一步证据提示已从旧的 `Texture2DArray slice indices` 改为当前生产主线：`TexCoord7.x -> _PropTexture -> VT Atlas UVRect -> VT_Atlas`。`Texture2DArray` 仅保留为 legacy fallback 语义。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Run-UE58PerformanceGoalHeartbeat.ps1 -RepoRoot X:\Project\Dev01 -SkipMcpProbe`：通过，并生成最新 `Governance gate: Passed (0 failed)`。
- `git diff --check`：通过，仅有仓库已有 LF/CRLF 提示。
- 本轮未重新编译；上一次 UE5.8 编译仍停在已打开 UnrealEditor 占用 DLL 的链接阶段，需要确认可关闭编辑器后再跑完整链接验证。

仍未完成：

- 真实 cluster 的 `MaterialBatchBuild`、Source/Proxy/Baked 传统 Streaming Level 层级/可见性成员关系验证、VT physical pool / streaming pool profiling 仍未执行。
- `M_Env_Building_Batch` 实际 `.uasset` 仍需通过 Commandlet/编辑器工具生成或验证，不能手工改二进制资产。

更新补充：2026-06-29 23:55

已继续落地：

- `MaterialBatchBuildCommandlet` 在扫描真实 Map Actor 时会读取 `Actor.GetDataLayerInstanceNames()`，并把实际 DataLayer 名写入每个 `FMaterialBatchBuildPlannedEntry.ActualDataLayerNames`。
- `SourceProxyLayerReadiness` 不再只判断 `EnvBatch.Source/Proxy/Baked` 标签是否准备好，现在会同时输出预期层和实际层：
  - `ActualLayerMatchCount`
  - `MissingActualLayerCount`
  - `UnexpectedActualLayerCount`
  - `NotRequiredActualLayerCount`
  - 每个 assignment 的 `ActualDataLayerNames`
  - `bMatchesExpectedDataLayer`
  - `DataLayerValidationStatus`
- Markdown report 的 `Source/Proxy/Baked Layer Readiness` 表已增加 `Actual DataLayers`、`Match`、`DataLayer Status` 列。
- JSON manifest 会输出 `sourceProxyLayerReadiness.assignments[].actualDataLayers` 与 `dataLayerValidationStatus`，并在 `entries[]` 里记录每个 actor 的实际 DataLayer。
- `UMaterialBatchMappingDataAsset` 已同步保存实际 DataLayer 名、匹配结果和状态，后续编辑器工具/蓝图/CI 不需要解析 markdown 也能读取验证结果。
- `Analyze-UE58PerformanceInputs.ps1` 已把 actual DataLayer readiness 纳入静态审计摘要，并把 batch material 绑定检测从旧 `T_Array_A` 主线改为 `VT_Atlas` 主线。
- `Test-UE58PerformanceArtifactContract.ps1` 已把 `GetDataLayerInstanceNames`、`actualDataLayers`、`MatchedExpectedDataLayer`、`ActualDataLayerNames` 纳入静态契约。

验证状态：

- UE5.8 `Build.bat DevKitEditor Win64 Development`：`MaterialBatchMappingDataAsset.cpp`、`MaterialBatchBuildCommandlet.cpp`、`MaterialBatchBuildPlan.cpp`、`MaterialBatchCandidateRulesTests.cpp` 等相关 C++ 文件已完成编译；最终仍失败在链接阶段，原因是当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-YogComboGraphEditor.dll`、`UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll`，错误为 `LNK1104 cannot open file`。
- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `git diff --check`：通过，仅有仓库已有 LF/CRLF 提示。

仍未完成：

- 尚未在真实美术 cluster 上运行 `MaterialBatchBuild` 生成实际 report/manifest，所以新的 actual DataLayer readiness 还没有真实关卡数据样本。
- 尚未关闭 UnrealEditor 后重新跑完整链接验证；关闭编辑器前需要确认没有未保存内容。
- VT physical pool / streaming pool profiling 和 `M_Env_Building_Batch` 实际资产生成/验证仍待执行。
更新补充：2026-06-30 00:00

已继续落地：

- 新增 `BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1`，作为第一条真实 cluster 材质/模型合批干跑入口。默认目标为：
  - Map：`/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
  - Cluster：`Prison_S_01_SourceProxy`
  - Tier：`Mid`
  - TextureBackend：`VTAtlas`
  - SurfaceKind：`MixedStatic`
  - BakePolicy：`StaticBake`
  - RequireTag：`EnvBatch.`
- 该脚本会串起 `MaterialBatchAudit` 和 `MaterialBatchBuild`，但默认只做 dry-run/report-only，不携带任何 `-Apply*` 参数，不生成或修改 `.uasset`。
- 脚本会先读取最新 UBT 结果；如果最新构建仍是失败状态，并且没有显式传入 `-AllowStaleBinaries`，则直接输出 `Status: BlockedByBuild`，避免在陈旧二进制上误跑命令行验收。
- `MaterialBatchDryRun/LATEST.md` 已纳入自动化证据链，当前状态为 `BlockedByBuild`，原因是上一轮 UE5.8 构建最终停在 DLL 被当前打开的 `UnrealEditor.exe` 占用导致的链接失败。
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已纳入 MaterialBatch dry-run 状态，后续 heartbeat 会直接显示：
  - `MaterialBatch dry-run: Prepared / BlockedByBuild / Failed / DryRunCaptured`
  - 最新 dry-run report 路径
- `Analyze-UE58PerformanceInputs.ps1` 已纳入 dry-run 脚本、latest report、BlockedByBuild、DryRunCaptured 检测。静态审计会把它作为真实 cluster 验收前置项，而不是只检查框架是否存在。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增 dry-run 自动化契约检查，要求脚本必须覆盖：
  - `MaterialBatchAudit`
  - `MaterialBatchBuild`
  - `BlockedByBuild`
  - `AllowStaleBinaries`
  - `Actual layer evidence`
  - `Residency risk evidence`

验收口径调整：

- 真实 cluster 干跑的合格状态不是脚本存在，而是 `Status: DryRunCaptured`。
- `DryRunCaptured` 必须同时满足：
  - `MaterialBatchAudit` 成功完成。
  - `MaterialBatchBuild` 成功完成。
  - report/manifest 中能读到 actual DataLayer readiness 证据。
  - report/manifest 中能读到 VT/Non-VT residency risk 证据。
- 当前 `BlockedByBuild` 不是功能失败，而是正确阻断：在 UnrealEditor 占用 DLL、链接未完成前，不应该让后续命令误用旧 DLL 产物。

仍未完成：

- 需要在确认当前 UnrealEditor 没有未保存内容后关闭编辑器，重新跑 UE5.8 `Build.bat DevKitEditor Win64 Development`，完成干净链接。
- 完成链接后，运行：
  - `BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1 -RepoRoot X:\Project\Dev01 -Run`
- dry-run report 达到 `DryRunCaptured` 后，再进入真实 cluster 的 Source/Proxy/Baked 传统 Streaming Level 层级/可见性成员关系修正、VT atlas 合并产物检查、RVT/贴花 bake 结果检查、VT physical pool / streaming pool profiling。
更新补充：2026-06-30 00:10

已继续落地：

- Codex App 自动化 `ue58-epic-high-mid-low` 已通过 automation 工具更新为当前线程续跑目标，目标节奏为每 5 小时恢复一次。
- 续跑提示已更新为当前目标口径：
  - 先读取 `guide.md`。
  - 再读取 `git status --short --branch`。
  - 再读取 `Docs/GeneratedReports/UE58PerformanceAutomation/LATEST.md`。
  - 再读取 `Docs/GeneratedReports/UE58PerformanceAutomation/MaterialBatchDryRun/LATEST.md`。
  - 再读取当前主计划 `UE58_EpicHighMidLow_模型材质性能分级落地计划.md`。
  - 从 heartbeat/audit 中显示的第一个未完成阶段继续执行。
  - 本目标允许编译。
  - 如果 dry-run 为 `BlockedByBuild` 且原因是 UnrealEditor 占用 DLL，不允许在未得到确认时关闭编辑器；应继续推进不依赖干净链接的计划、工具和证据 gate。
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已把续跑自动化写入每次 heartbeat 报告：
  - automation id：`ue58-epic-high-mid-low`
  - cadence：`5 hours`
  - resume prompt：包含 dry-run latest、Source/Proxy/Baked readiness、ResidencyRiskPlan、允许编译、不可擅自关闭编辑器等约束
- `Test-UE58PerformanceArtifactContract.ps1` 已新增静态契约，防止后续改动误删 5 小时续跑目标、允许编译规则、dry-run 恢复入口或 UnrealEditor 关闭保护。

验收口径调整：

- 每次计划阶段结束或续跑结束前，必须至少刷新：
  - artifact contract
  - static audit
  - heartbeat latest
- heartbeat latest 必须能直接回答：
  - 当前主计划是哪一个文件。
  - 当前 dry-run 状态是什么。
  - 当前 UE build 最近结果是什么。
  - 当前 governance gate 是否通过。
  - 当前续跑 automation id 和 5 小时节奏是什么。
- 外部 Codex App 自动化由 automation 工具管理；仓库内 heartbeat 和 artifact contract 负责保留可恢复上下文与静态防回退约束。

仍未完成：

- Codex App 的 automation update 已返回成功，但本机 `.codex/automations` 下的 toml 缓存显示可能滞后；后续如果需要以磁盘文件作为唯一证据，应再次用 App 侧自动化视图确认或由用户在 UI 中确认。
- 真实 cluster dry-run 仍然停在 `BlockedByBuild`，必须等 UnrealEditor 可关闭并完成干净链接后再推进到 `DryRunCaptured`。
更新补充：2026-06-30 00:25

已继续落地：

- 原生 `EnvBatch Tagger` 已补齐静态 baked surface 标记入口，不再只支持地面 Low：
  - `EnvBatch.Baked.Ground.Mid`
  - `EnvBatch.Baked.Ground.Low`
  - `EnvBatch.Baked.Wall.Mid`
  - `EnvBatch.Baked.Wall.Low`
- 这对应前面墙面/地面材质处理需求：
  - 地面：静态多层混合、mesh paint/顶点色、静态贴花结果可以 bake 后进入 Ground baked layer。
  - 墙面：静态混合和贴花结果不强依赖 RVT；优先作为 wall static bake 结果进入 Wall baked tag，再由 Source/Proxy/Baked readiness 与真实 DataLayer 检查验收。
- 原生 Tagger 中 Source / Proxy / Baked 仍保持 `EnvBatch.*` 命名空间互斥；美术给某个 Actor 标记 Wall baked 后，会清掉旧的 Source/Proxy/Baked tag，避免同一物件同时作为 Source 和 Baked 参与验收。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增契约：必须存在地面和墙面 Mid/Low baked tags，否则静态契约失败。
- `Analyze-UE58PerformanceInputs.ps1` 已新增审计行：`EnvBatch native baked surface tags`，后续 heartbeat 刷新时会把该能力写入 `UE58PerformanceAudit/LATEST.md`。

验收口径调整：

- 墙面静态 bake 方案正式进入工作流：不是 RVT 默认覆盖墙面，而是先通过 `EnvBatch.Baked.Wall.Mid/Low` 标记 baked 替代对象，再由 MaterialBatch dry-run 输出 actual DataLayer readiness。
- `RVT` 仍优先用于地面动态/运行时混合层；墙面若未来需要类似运行时混合，应作为单独实验项，不阻塞当前静态 bake 管线。
- 后续真实 cluster 验收时，Ground baked 和 Wall baked 都必须能在 report/manifest 中体现为 Baked role，且 Source/Proxy/Baked 传统 Streaming Level 层级/可见性互斥关系要通过。

仍未完成：

- 由于当前 dry-run 仍为 `BlockedByBuild`，这些 tag 还没有经过真实关卡 actor 的 commandlet report 验证。
- 需要在干净链接后通过 `Invoke-UE58MaterialBatchDryRun.ps1 -Run` 确认 `EnvBatch.Baked.Wall.*` 和 `EnvBatch.Baked.Ground.*` 能被真实 map actor 扫描并写入 readiness。
更新补充：2026-06-30 00:40

已继续落地：

- 修复并重写 `Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py`，作为原生 Slate `EnvBatch Tagger` 的备用入口。
- Python fallback 现在与原生工具使用同一套标签语义：
  - Source：`EnvBatch.Source.<Group>`
  - Proxy：`EnvBatch.Proxy.<Group>.Mid` / `EnvBatch.Proxy.<Group>.Low`
  - Baked Ground：`EnvBatch.Baked.Ground.Mid` / `EnvBatch.Baked.Ground.Low`
  - Baked Wall：`EnvBatch.Baked.Wall.Mid` / `EnvBatch.Baked.Wall.Low`
  - Exclude：`EnvBatch.Exclude`
- Python fallback 也保留 Source / Proxy / Baked 互斥规则，避免美术通过备用工具打出与原生工具不一致的状态。
- `python -m py_compile Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py` 已通过，用于证明该备用工具至少语法可加载。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增 Python fallback 契约：如果备用工具缺少 ground/wall Mid/Low baked tags 或互斥规则，静态契约会失败。
- `Analyze-UE58PerformanceInputs.ps1` 已新增审计行：`EnvBatch Python fallback tags`，后续 heartbeat 会把该入口是否对齐写入 latest audit。

验收口径调整：

- 美术工作流入口现在按优先级分为：
  - 首选：原生 Slate `EnvBatch Tagger`。
  - 备用：Python `EnvBatchTagTool.py`。
- 两个入口必须保持同一套 tag 合同；后续不允许只改其中一个入口导致 Source/Proxy/Baked 或 Wall/Ground baked tag 不一致。
- 真实验收仍以 `MaterialBatchBuild` report/manifest 中的 readiness 为准；Tagger 只是写入美术意图，不能替代 commandlet 对实际 Actor/DataLayer 的验证。

仍未完成：

- Python fallback 只完成语法和静态合同验证，尚未在 UE Python 控制台实际打开 UI 测试。
- 真实关卡中的 Source/Proxy/Baked/Wall/Ground 标签仍需在干净链接后通过 `Invoke-UE58MaterialBatchDryRun.ps1 -Run` 验证。
更新补充：2026-06-30 00:55

已继续落地：

- 新增 `BuildScripts/Automation/Test-UE58EnvBatchTagTools.ps1`，作为不依赖 Unreal 启动的 EnvBatch 工具合同检查。
- 该脚本会校验：
  - 原生 Slate `SEnvBatchTaggerWidget` 是否包含 Source / Proxy / Baked / Exclude 基础合同。
  - 原生工具是否包含 `EnvBatch.Baked.Ground.Mid/Low` 与 `EnvBatch.Baked.Wall.Mid/Low`。
  - Python fallback `EnvBatchTagTool.py` 是否包含同一套 ground/wall Mid/Low baked tags。
  - Python fallback 是否保留 Source / Proxy / Baked 互斥规则。
  - Python fallback 是否可以通过 `py_compile` 语法检查，并且不会在源码目录写入 `__pycache__`。
- 该脚本会输出：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/EnvBatchTagTools/LATEST.md`
- `Analyze-UE58PerformanceInputs.ps1` 已接入该报告：
  - `UE58 EnvBatch tag tool script`
  - `UE58 EnvBatch tag tool report`
  - `UE58 EnvBatch tag tool passed`
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已在每次 heartbeat 中运行该检查，并把 latest path 写入 heartbeat 报告。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增契约：如果没有独立 EnvBatch tag tool 合同脚本，或脚本不校验 native/Python ground/wall baked tag parity，则 contract 失败。

验收口径调整：

- 美术入口不再只靠“工具存在”验收，必须有 `EnvBatchTagTools/LATEST.md` 证明 native 与 Python fallback 的 tag 合同一致。
- 后续任何 Source/Proxy/Baked 或 Ground/Wall baked tag 命名调整，都必须同步更新：
  - 原生 Slate Tagger
  - Python fallback
  - EnvBatch tag tool 合同脚本
  - 静态审计和 artifact contract

仍未完成：

- 该检查只证明工具入口合同一致，不能替代真实关卡中的 Actor tag、DataLayer、Source/Proxy/Baked readiness 验证。
- 真实验证仍依赖干净链接后的 `Invoke-UE58MaterialBatchDryRun.ps1 -Run`。

更新补充：2026-06-30 00:30

已继续落地：

- 将 Source/Proxy 配置管理正式纳入模型与工具计划：
  - Source 默认 `LOD0`。
  - Proxy 默认 `LOD1`。
  - Source tag 条目可先使用 generated cluster proxy fallback。
  - AuthorProxy 必须显式记录 Source 引用，否则不能视为配置完成。
- `MaterialBatchBuildPlan` 新增 `SourceProxyAssetReadiness`，并同步输出到：
  - Markdown report：`## Source/Proxy Asset Readiness`
  - JSON manifest：`sourceProxyAssetReadiness`
  - `UMaterialBatchMappingDataAsset.SourceProxyAssetReadiness`
- `SourceProxyAssetReadiness` 记录：
  - `SourceAssetPath`
  - `ProxyAssetPath`
  - `SourceLODIndex`
  - `ProxyLODIndex`
  - `bUsesGeneratedProxy`
  - `bUsesAuthoredProxy`
  - `bReadyForAssetPairing`
  - `ReadinessStatus`
- `MissingSourceAssetReference` 作为 AuthorProxy 缺少 Source 配置的明确阻断状态，后续美术资产管理器和导入设置都应消费同一字段。
- 真实 cluster dry-run 通过条件已增加 Source/Proxy asset evidence；`DryRunCaptured` 现在必须同时满足：
  - actual DataLayer readiness
  - Source/Proxy asset readiness
  - residency risk evidence

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Analyze-UE58PerformanceInputs.ps1 -RepoRoot X:\Project\Dev01`：已刷新 latest audit。
- `Test-UE58EnvBatchTagTools.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Run-UE58PerformanceGoalHeartbeat.ps1 -RepoRoot X:\Project\Dev01 -SkipMcpProbe`：已刷新 latest heartbeat。
- `git diff --check`：通过，仅保留仓库既有 LF/CRLF 提示。

仍未完成：

- C++ 完整链接仍需要在确认 UnrealEditor 可关闭后重跑；当前真实 cluster dry-run 仍依赖干净链接后执行。
- Source/Proxy 配置管理的 UI 入口还未实现，本轮先完成了数据契约、报告和自动化 gate。

更新补充：2026-06-30 00:35

已继续落地：

- `Write-UE58SubmissionGateReport.ps1` 已把真实 cluster MaterialBatch dry-run 证据提升为提交 gate 顶层字段：
  - `MaterialBatch dry-run captured`
  - `MaterialBatch dry-run layer evidence`
  - `MaterialBatch dry-run Source/Proxy asset evidence`
  - `MaterialBatch dry-run residency evidence`
  - `MaterialBatch dry-run ready`
- Phase 1 提交 gate 现在必须同时满足 MaterialBatch dry-run ready，避免只通过静态契约就误判可以提交。
- `Invoke-UE58FinalUpload.ps1` 的 final upload preflight 已同步读取 `MaterialBatch dry-run ready`，最终上传前必须有真实 cluster 的 DataLayer、Source/Proxy asset、residency 三项证据。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增 submission/final-upload gate 契约检查，防止后续误删真实 cluster evidence gate。
- `Analyze-UE58PerformanceInputs.ps1` 的风险描述已同步新口径：真实 cluster dry-run 不只要求 DataLayer 和 residency，还要求 Source/Proxy asset readiness。
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已在顶层摘要 submission gate 的 `Submission MaterialBatch dry-run ready` 和 `Submission Source/Proxy asset evidence`，5 小时续跑时无需打开 submission gate 就能看到真实 cluster gate 是否满足。
- heartbeat resume checklist 已同步要求同时检查 `ResidencyRiskPlan`、`Source/Proxy/Baked layer readiness`、`Source/Proxy asset readiness` 和 `MaterialBatch dry-run ready`。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Write-UE58SubmissionGateReport.ps1 -RepoRoot X:\Project\Dev01`：已刷新 latest submission gate。
- `Analyze-UE58PerformanceInputs.ps1 -RepoRoot X:\Project\Dev01`：已刷新 latest audit。
- `Run-UE58PerformanceGoalHeartbeat.ps1 -RepoRoot X:\Project\Dev01 -SkipMcpProbe`：已刷新 latest heartbeat。
- `git diff --check`：通过，仅保留仓库既有 LF/CRLF 提示。

当前 gate 状态：

- `MaterialBatch dry-run ready=False` 是正确结果；因为真实 cluster dry-run 仍未在干净链接后的当前二进制上运行到 `DryRunCaptured`。
- 下一步仍是确认 UnrealEditor 可关闭后完成干净链接，再运行 `Invoke-UE58MaterialBatchDryRun.ps1 -Run` 生成真实 report/manifest 证据。

更新补充：2026-06-30 00:44

已继续落地：

- 原生 `EnvBatch Tagger` 增加选中对象的 Source/Proxy asset readiness 摘要：
  - Source ready 计数。
  - generated proxy fallback 计数。
  - AuthorProxy missing Source reference 计数。
  - proxy mesh missing 计数。
  - baked/excluded not required 计数。
  - conflicts / unassigned 计数。
  - 默认规则直接显示 `SourceLOD0` / `ProxyLOD1`。
- Python fallback `EnvBatchTagTool.py` 同步增加 `_asset_readiness_summary` 和 `Print Asset Readiness`：
  - 选中对象状态行会显示 Source/Proxy asset readiness。
  - 可把每个 actor 的 role、ready、status、hasStaticMesh、SourceLOD0/ProxyLOD1 和 EnvBatchTags 打到日志。
  - AuthorProxy 缺 Source 引用时输出 `MissingSourceAssetReference`，与 MaterialBatchBuild 的 readiness 状态保持一致。
- `Test-UE58EnvBatchTagTools.ps1` 已把 native/Python readiness UI 纳入合同检查。
- `Analyze-UE58PerformanceInputs.ps1` 已新增 `EnvBatch asset readiness UI` 审计项。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增 EnvBatch asset readiness UI 静态契约。

验证状态：

- `Test-UE58EnvBatchTagTools.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `python -m py_compile Source\DevKitEditor\MaterialBatch\EnvBatchTagTool.py`：通过，且已确认无 `__pycache__` 残留。
- UE5.8 `Build.bat DevKitEditor Win64 Development`：`SEnvBatchTaggerWidget.cpp` 编译通过；最终仍因当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-YogComboGraphEditor.dll`、`UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll` 在链接阶段失败。

当前意义：

- 美术工作流入口不再只负责打 tag；选中物件后能直接看到 Source/Proxy 配置是否满足 SourceLOD0 / ProxyLOD1 的基础规则。
- 这仍不能替代真实 cluster `MaterialBatchBuild` report/manifest；最终验收仍以干净链接后的 `DryRunCaptured` 为准。

更新补充：2026-06-30 01:01

已继续落地：

- 新增 Source/Proxy asset config set 数据契约，作为导入设置、美术资产管理器、commandlet、report、manifest 和 MappingData 的共同持久化结构。
- `UMaterialBatchMappingDataAsset` 新增：
  - `FMaterialBatchMappingSourceProxyAssetConfig`
  - `FMaterialBatchMappingSourceProxyAssetConfigSet`
  - `SourceProxyAssetConfigSet`
- `MaterialBatchBuildPlan` 新增：
  - `FMaterialBatchBuildSourceProxyAssetConfig`
  - `FMaterialBatchBuildSourceProxyAssetConfigSet`
  - `BuildSourceProxyAssetConfigSet`
- 配置集会从 `SourceProxyAssetReadiness` 自动生成默认记录：
  - Source 条目：`ConfigSource=GeneratedFallback`，Source 默认 LOD0，GeneratedProxy 默认 LOD1。
  - AuthorProxy 条目：`ConfigSource=ImportSettingsOrArtAssetManagerRequired`，缺 Source 时继续保持 `MissingSourceAssetReference`。
  - Baked 条目：`ConfigSource=BakeManifest`。
  - Excluded 条目：`ConfigSource=EnvBatchExclude`。
  - Conflict/Unassigned 条目分别标记为 `InvalidEnvBatchTags` / `UnassignedEnvBatchTags`。
- Markdown report 新增 `## Source/Proxy Asset Config Set`，JSON manifest 新增 `sourceProxyAssetConfigSet`，MappingData 会保存同一套 config set。
- 真实 cluster dry-run gate 已加严：`Source/Proxy asset evidence` 现在必须同时具备 `sourceProxyAssetReadiness` 与 `sourceProxyAssetConfigSet`，否则不能进入 `DryRunCaptured`。
- 静态审计已新增 `Source/Proxy asset config set` 检测项。

验证状态：

- `Test-UE58PerformanceArtifactContract.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Test-UE58EnvBatchTagTools.ps1 -RepoRoot X:\Project\Dev01`：通过。
- `Analyze-UE58PerformanceInputs.ps1 -RepoRoot X:\Project\Dev01`：已刷新 latest audit，显示 `Source/Proxy asset config set=True`。
- `Invoke-UE58MaterialBatchDryRun.ps1 -RepoRoot X:\Project\Dev01`：已刷新 latest dry-run report，当前仍为 `Prepared`，未运行 commandlet。
- `Write-UE58SubmissionGateReport.ps1 -RepoRoot X:\Project\Dev01`：已刷新 latest submission gate。
- UE5.8 `Build.bat DevKitEditor Win64 Development`：本轮新增/相关 C++ 文件已完成编译；最终仍因当前打开的 `UnrealEditor.exe` 占用 `UnrealEditor-YogComboGraphEditor.dll`、`UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll` 在链接阶段失败，错误为 `LNK1104 cannot open file`。

仍未完成：

- `SourceProxyAssetConfigSet` 目前由 commandlet 计划自动生成默认配置，还没有独立的美术资产管理编辑器写入 UI。
- 真实 cluster dry-run 仍需在干净链接后执行 `Invoke-UE58MaterialBatchDryRun.ps1 -Run`，并产出 actual DataLayer、Source/Proxy readiness/config-set、residency 三项真实证据。
- 仍未生成或验证实际 `M_Env_Baked_VTAtlas.uasset`、VT physical pool / streaming pool profiling、地面/墙面 bake 视觉 parity。

更新补充：2026-06-30 01:10

已继续落地：

- 已通过 UE MCP 调用 `MaterialTools` 与 `MaterialInstanceTools` 审计 `/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building`，并生成：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/UE58MaterialMcpAudit_LATEST.md`
- 本轮 MCP 审计采用快速默认模式：
  - `Expression graph mode: SkippedQuickMaterialOutputs`
  - `Output probe mode: MaterialAttributesOnly`
  - `Parameter count: 69`
  - `MaterialAttributes connected: True`
  - `Required batch parameters present: True`
- MCP 已确认当前源材质暴露 batch 计划需要的关键参数：
  - `T_Array_A`
  - `T_Array_M`
  - `T_Array_N`
  - `Tex_LightInfo`
  - `LightInfoCount`
- MCP 已确认当前源材质存在地面、墙面、贴花相关 Static Switch：
  - `Is Floor?`
  - `Is Wall?`
  - `Is Decal?`
- `Analyze-UE58PerformanceInputs.ps1` 已把材质 MCP 证据拆成两层：
  - `UE58 Material MCP output connection evidence`：快速模式下可作为已完成证据，用于证明 `MP_MaterialAttributes` 输出连接和 batch 所需参数存在。
  - `UE58 Material MCP full expression graph`：必须由 `Invoke-UE58MaterialMcpAudit.ps1 -FullExpressionGraph -ProbeAllOutputs` 产生，当前仍未完成。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增静态契约，要求 `Invoke-UE58MaterialMcpAudit.ps1` 保留：
  - 快速默认审计模式。
  - `-FullExpressionGraph` 完整表达式图 opt-in。
  - `-ProbeAllOutputs` 全输出 pin opt-in。
  - `get_expressions` 只在明确请求完整图时执行，避免 MCP 在大材质图上超时。

验收口径调整：

- 材质 MCP 已完成“参数、Static Switch、MaterialAttributes 输出连接”的阶段性证据，不再视为完全缺失。
- 材质 MCP 还未完成“完整表达式图遍历”证据；该项只作为更深层图审计，不阻塞当前 Source/Proxy、VT Atlas、dry-run gate 继续推进，但不能用快速审计替代最终 batch parent material 的采样链路验收。
- 最终 batch 材质仍必须单独验证：
  - `M_Env_Building_Batch` 或等价 batch parent material 是否采样 VT Atlas。
  - 是否读取 `UV7.x` / batch index。
  - 是否读取 PropertyTexture 中的 Base/Normal/ORM UVRect。
  - `Tex_LightInfo` 或材质灯降级参数是否接入 Epic/High/Mid/Low。

仍未完成：

- 完整 MCP 表达式图审计尚未跑通；此前直接调用 `get_expressions` 在该材质上耗时过长，因此默认脚本改为快速模式。
- 真实 cluster dry-run 仍需在干净链接后执行 `Invoke-UE58MaterialBatchDryRun.ps1 -Run`。
- 实际 batch parent material、VT Atlas 产物、地面/墙面 bake 视觉 parity 和 VT/Non-VT residency profiling 仍未验收。

更新补充：2026-06-30 03:59

已继续落地：

- 新增 `BuildScripts/Automation/Select-UE58PilotCluster.ps1`，作为 WP1 目标 cluster 选择与证据合同脚本。
- 该脚本不会启动 Unreal，也不会写入或修改 `.uasset`；它只检查当前 pilot cluster 的静态前置条件，并输出：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/PilotCluster/LATEST.md`
- 当前 pilot cluster 固定为：
  - Map：`/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
  - Cluster：`Prison_S_01_SourceProxy`
  - Tier：`Mid`
  - Texture backend：`VTAtlas`
  - Surface kind：`MixedStatic`
  - Bake policy：`StaticBake`
  - Require tag：`EnvBatch.`
  - Max actors：`2000`
- Pilot cluster 报告当前状态为 `ReadyForCommandlet`，表示：
  - 目标 map 文件存在。
  - `M_Env_Building.uasset` 存在。
  - 材质 MCP 快速审计已证明 batch 所需参数和 `MP_MaterialAttributes` 输出连接。
  - EnvBatch native/Python tag 工具合同已通过。
  - `Invoke-UE58MaterialBatchDryRun.ps1` 与同一 Map/Cluster/Tier/VTAtlas 参数保持一致。
- Pilot cluster 报告明确列出美术/关卡需要使用的 EnvBatch tag 合同：
  - `EnvBatch.Source.Prison_S_01`
  - `EnvBatch.Proxy.Prison_S_01.Mid`
  - `EnvBatch.Proxy.Prison_S_01.Low`
  - `EnvBatch.Baked.Ground.Mid`
  - `EnvBatch.Baked.Ground.Low`
  - `EnvBatch.Baked.Wall.Mid`
  - `EnvBatch.Baked.Wall.Low`
  - `EnvBatch.BakeStaticDecal.Prison_S_01`
  - `EnvBatch.RuntimeDecal`
  - `EnvBatch.Exclude`
- `Analyze-UE58PerformanceInputs.ps1` 已新增 pilot cluster 检测项：
  - `UE58 pilot cluster script`
  - `UE58 pilot cluster report`
  - `UE58 pilot cluster ready`
  - summary 中会显示 `Pilot cluster report`、`Pilot cluster ready for commandlet`、`Pilot cluster actor tag evidence`
- `Test-UE58PerformanceArtifactContract.ps1` 已新增静态契约，要求 pilot cluster 选择脚本必须记录：
  - 第一轮真实 cluster map。
  - `Prison_S_01_SourceProxy` cluster。
  - `Required EnvBatch Tags`。
  - dry-run command。
  - `ReadyForCommandlet` 与 `Actor tag evidence captured` 的状态分离。
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已纳入 pilot cluster 自动刷新；每次 heartbeat 会先运行 `Select-UE58PilotCluster.ps1`，再把当前 pilot cluster 状态和 latest report 路径写入 heartbeat。

验收口径调整：

- WP1 目标 cluster 选择现在可以用 `PilotCluster/LATEST.md` 验收；当前为 `ReadyForCommandlet`，可作为后续干跑的目标依据。
- `ReadyForCommandlet` 不等于 WP12 完成；它只证明 map/material/MCP/EnvBatch/dry-run 参数合同已对齐。
- `Actor tag evidence captured=False` 是当前正确状态；必须等干净链接后运行 `Invoke-UE58MaterialBatchDryRun.ps1 -Run`，由 commandlet 读取真实 map actor、DataLayer、Source/Proxy asset readiness/config-set 和 residency 后，才能把真实 cluster 验收推进到 `DryRunCaptured`。

仍未完成：

- 尚未用当前二进制运行真实 cluster commandlet，因此还没有真实 actor tag、DataLayer、Source/Proxy asset 和 residency 证据。
- 仍未生成或验证实际 `M_Env_Baked_VTAtlas.uasset`、VT Atlas 产物、地面/墙面 bake 视觉 parity、VT/Non-VT residency profiling。

更新补充：2026-06-30 09:18

已继续落地：

- 在当前 `UnrealEditor.exe` 仍持有 DLL 的情况下，没有强行关闭编辑器，也没有重复执行必然失败的链接。
- 已运行 `BuildScripts/Automation/Invoke-UE58RuntimeProfilingMcpSmoke.ps1 -SkipPie`，生成：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingSmoke/LATEST.md`
- Runtime profiling MCP smoke 当前为 `Status: Ready`，已通过 MCP 确认可查询以下 profiling/画质 CVar：
  - `sg.GlobalIlluminationQuality`
  - `r.ScreenPercentage`
  - `r.Lumen.DiffuseIndirect.Allow`
  - `r.MeshDrawCommands.LogDynamicInstancingStats`
  - `t.MaxFPS`
- 尝试运行 `UE58RuntimeProfilingPlan` commandlet，但当前已加载二进制尚未包含 `UE58RuntimeProfilingPlanCommandlet` 类，日志显示：
  - `UE58RuntimeProfilingPlanCommandlet looked like a commandlet, but we could not find the class.`
  - 该问题需要干净链接后再用 commandlet 覆盖报告。
- 新增 `BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1`，在 commandlet 类尚未链接进当前二进制时，生成同名 profiling 计划报告：
  - `Docs/GeneratedReports/CommandletReports/UE58RuntimeProfilingPlanReport.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingPlanFallback/LATEST.md`
- fallback report 明确写入：
  - `Commandlet fallback: True`
  - `Commandlet class pending clean link: True`
  - `Evidence status: NotMeasured`
- fallback report 当前包含 `Epic / Mid / Low` 的 profiling 场景矩阵：
  - `Baseline_LumenOff_NoBatch`
  - `LumenLite_NoBatch`
  - `BatchProxy_LumenOff`
  - `BatchProxy_LumenLite`
  - `Low_LumenOff_Aggressive`
  - `Epic_LumenHigh`
- 每个场景都写入了 CVar setup 和 capture commands：
  - `stat unit`
  - `stat rhi`
  - `stat scenerendering`
  - `stat gpu`
  - `r.MeshDrawCommands.LogDynamicInstancingStats 1`
  - `profilegpu`
- `Analyze-UE58PerformanceInputs.ps1` 已新增 fallback 检测项：
  - `UE58RuntimeProfilingPlan fallback script`
  - `UE58RuntimeProfilingPlan fallback report`
  - 当 fallback report 生效时，静态风险会提示干净链接后必须重新运行 commandlet。
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已在每次 heartbeat 中自动刷新 runtime profiling plan fallback，再刷新 static audit。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增 fallback 脚本静态契约，确保 profiling 场景矩阵、`Commandlet fallback` 标记和 `profilegpu` 采集命令不会丢失。

验收口径调整：

- `UE58RuntimeProfilingPlanReport.md` 现在可作为 profiling 计划清单使用，但当前不是 commandlet 运行证据。
- `RuntimeProfilingSmoke/LATEST.md` 只证明 MCP/CVar 查询可用，不证明 GPU 实测结果。
- 最终性能验收仍必须运行：
  - 干净链接后的 `UE58RuntimeProfilingPlan` commandlet，覆盖 fallback report。
  - `Invoke-UE58RuntimeProfilingCapture.ps1 -Run`，生成 baseline、Lumen Lite、batch proxy 场景的真实 log/profilegpu 证据。

仍未完成：

- `UE58RuntimeProfilingPlanCommandlet` 尚未通过当前二进制实际运行；当前报告为 fallback。
- 尚未采集真实 `stat rhi`、`stat scenerendering`、`stat gpu`、`profilegpu` 的场景结果。
- 真实 cluster `MaterialBatchDryRun`、batch parent material、VT Atlas 产物、地面/墙面 bake parity 和 VT/Non-VT residency profiling 仍未完成。

更新补充：2026-06-30 09:23

已继续落地：

- 已尝试运行以下 report-only commandlet，均不写资产：
  - `GraphicsSettingsWidgetSetup`
  - `MaterialBatchMaterialAudit`
  - `UE58RuntimeProfilingPlan`
- 三者当前都能启动 `UnrealEditor-Cmd.exe`，但旧二进制中找不到对应 commandlet class：
  - `GraphicsSettingsWidgetSetupCommandlet looked like a commandlet, but we could not find the class.`
  - `MaterialBatchMaterialAuditCommandlet looked like a commandlet, but we could not find the class.`
  - `UE58RuntimeProfilingPlanCommandlet looked like a commandlet, but we could not find the class.`
- 新增 `BuildScripts/Automation/Test-UE58CommandletAvailability.ps1`，集中记录当前 report-only commandlet 可用性：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/CommandletAvailability/LATEST.md`
- 当前 availability 状态为 `BlockedByCleanLink`：
  - latest UBT result 仍是 `Failed (OtherCompilationError)`。
  - latest UBT link blocked 为 `True`。
  - 现有 `UnrealEditor.exe` 持有 DLL，导致已编译的新 commandlet 类无法进入当前可执行二进制。
- `Analyze-UE58PerformanceInputs.ps1` 已新增检测项：
  - `UE58 commandlet availability script`
  - `UE58 commandlet availability report`
  - `UE58 commandlet availability blocked by clean link`
- `Run-UE58PerformanceGoalHeartbeat.ps1` 已在每次 heartbeat 中自动刷新 commandlet availability，再刷新 static audit。
- `Test-UE58PerformanceArtifactContract.ps1` 已新增 commandlet availability 静态契约。

验收口径调整：

- `GraphicsSettingsWidgetSetupReport.md`、`MaterialBatchMaterialAuditReport.md`、`UE58RuntimeProfilingPlanReport.md` 的 commandlet 运行证据必须等干净链接后重新生成。
- 当前 fallback 或 class-missing 报告只能证明流程和阻塞原因，不能替代 commandlet 运行证据。
- 在未确认用户保存并关闭编辑器前，不自动关闭 `UnrealEditor.exe`。

仍未完成：

- 干净链接后需要重新运行 report-only commandlets。
- 干净链接后需要运行真实 cluster `Invoke-UE58MaterialBatchDryRun.ps1 -Run`。
- 仍未生成/验收实际 batch material、VT Atlas、ProxyMesh、MappingData、地面/墙面 bake parity 和真实性能 profiling。
更新补充：2026-06-30 09:28

已继续落地：

- 将 `CommandletAvailability/LATEST.md` 的状态接入 `Write-UE58SubmissionGateReport.ps1`。
- Submission gate 现在会显式输出：
  - `Commandlet availability ready`
  - `Commandlet availability blocked by clean link`
- 当 report-only commandlet 仍为 `BlockedByCleanLink` 时，Submission gate 会把它列为 hard block 和 evidence gap，避免在旧编辑器二进制尚未包含 commandlet class 时误判为可提交。
- 将 `Commandlet availability ready` 纳入 `Invoke-UE58FinalUpload.ps1` 的 evidence gates；final upload 不能绕过 commandlet availability。
- 将该约束补进 `Test-UE58PerformanceArtifactContract.ps1`，后续静态 contract 会防止提交/上传 gate 丢失 commandlet availability 条件。
- 已刷新：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/CommandletAvailability/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAudit/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/LATEST.md`

当前验收状态：

- `Test-UE58PerformanceArtifactContract.ps1`：Passed。
- `git diff --check`：无 whitespace error，仅有 LF/CRLF 提示。
- 最新 heartbeat：`2026-06-30T09:28:27+08:00`。
- 最新 commandlet availability：`BlockedByCleanLink`。
- 最新 submission gate：`Can commit Phase 1 scope=False`，`Can upload final main=False`。

仍未完成：

- 不自动关闭 `UnrealEditor.exe`。需要用户保存并关闭编辑器后，完成干净 DevKitEditor 链接。
- 干净链接后重新运行 `GraphicsSettingsWidgetSetup`、`MaterialBatchMaterialAudit`、`UE58RuntimeProfilingPlan` 三个 report-only commandlet。
- commandlet 可用后再运行真实 cluster `Invoke-UE58MaterialBatchDryRun.ps1 -Run`，采集 DataLayer、Source/Proxy asset readiness/config-set、residency 证据。
- 真实 batch material、VT Atlas、ProxyMesh、MappingData、地面/墙面 bake parity 和 runtime profiling 仍未完成验收。

更新补充：2026-06-30 09:36

已继续落地：

- 新增 `BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1`，作为干净链接后的统一续跑入口。
- 默认模式只生成报告，不启动 UE commandlet，不修改资产：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/PostCleanLinkValidation/LATEST.md`
- 显式传入 `-RunReportCommandlets` 时，才会重跑三个 report-only commandlet：
  - `GraphicsSettingsWidgetSetup`
  - `MaterialBatchMaterialAudit`
  - `UE58RuntimeProfilingPlan`
- 显式传入 `-RunMaterialBatchDryRun` 时，且 `CommandletAvailability/LATEST.md` 已为 `Available` 后，才会继续跑真实 cluster dry-run。
- 该脚本不会关闭 `UnrealEditor.exe`。如果编辑器仍在运行并传入 run switch，会输出 `BlockedByOpenEditor`，不启动 commandlet。
- 已接入：
  - `Analyze-UE58PerformanceInputs.ps1`
  - `Run-UE58PerformanceGoalHeartbeat.ps1`
  - `Test-UE58PerformanceArtifactContract.ps1`

本次编译结果：

- 已按当前目标允许执行 UE5.8 编译：
  - `Z:\GZA_Software\RealityCapture\UE_5.8\Engine\Build\BatchFiles\Build.bat DevKitEditor Win64 Development -Project=X:\Project\Dev01\DevKit.uproject -WaitMutex -FromMsBuild`
- 编译结果仍为 `Result: Failed (OtherCompilationError)`。
- 失败原因是当前打开的 `UnrealEditor.exe` 持有 DLL，链接阶段无法写入：
  - `Plugins\YogComboGraph\Binaries\Win64\UnrealEditor-YogComboGraphEditor.dll`
  - `Binaries\Win64\UnrealEditor-DevKit.dll`
  - `Binaries\Win64\UnrealEditor-DevKitEditor.dll`
- 当前不自动关闭编辑器，等待用户保存并关闭后再进行干净链接。

当前验收状态：

- `PostCleanLinkValidation/LATEST.md`：`Status: WaitingForEditorClose`。
- `CommandletAvailability/LATEST.md`：`Status: BlockedByCleanLink`。
- `SubmissionGate/LATEST.md`：仍不可提交/上传。

下一步：

- 用户保存并关闭 `UnrealEditor.exe` 后，重新编译 `DevKitEditor`。
- 编译成功后运行：
  - `BuildScripts\Automation\Invoke-UE58PostCleanLinkValidation.ps1 -RepoRoot X:\Project\Dev01 -RunReportCommandlets -RunMaterialBatchDryRun`
- 该命令会先刷新三个 report-only commandlet，再在 commandlet 可用时推进真实 MaterialBatch dry-run。

更新补充：2026-06-30 09:40

已继续落地：

- 新增 `BuildScripts/Automation/Invoke-UE58BuildValidation.ps1`，作为 UE5.8 编译状态的独立证据入口。
- 默认模式只读取当前 `UnrealBuildTool\Log.txt`、打开的 `UnrealEditor.exe` 进程和 DLL 锁信息，不执行编译。
- 显式传入 `-RunBuild` 时才执行：
  - `DevKitEditor Win64 Development`
  - `-Project=X:\Project\Dev01\DevKit.uproject`
  - `-WaitMutex -FromMsBuild`
- 如果 `UnrealEditor.exe` 仍在运行，`-RunBuild` 默认输出 `BlockedByOpenEditor`，除非显式传入 `-AllowOpenEditor`。
- 该脚本不会关闭 `UnrealEditor.exe`。
- 已接入：
  - `Run-UE58PerformanceGoalHeartbeat.ps1`：心跳只刷新非编译 build-status report，不自动编译。
  - `Analyze-UE58PerformanceInputs.ps1`：静态审计可识别 build validation script/report/blocked/succeeded。
  - `Test-UE58PerformanceArtifactContract.ps1`：静态 contract 防止该自动化入口和“心跳不自动编译”约束丢失。

当前验收状态：

- `BuildValidation/LATEST.md`：`Status: LatestBuildBlockedByOpenEditor`。
- `ArtifactContract/LATEST.md`：`Status: Passed`。
- `PostCleanLinkValidation/LATEST.md`：仍等待编辑器关闭。
- `CommandletAvailability/LATEST.md`：仍为 `BlockedByCleanLink`。

下一步：

- 用户保存并关闭 `UnrealEditor.exe` 后，先运行：
  - `BuildScripts\Automation\Invoke-UE58BuildValidation.ps1 -RepoRoot X:\Project\Dev01 -RunBuild`
- 编译成功后再运行：
  - `BuildScripts\Automation\Invoke-UE58PostCleanLinkValidation.ps1 -RepoRoot X:\Project\Dev01 -RunReportCommandlets -RunMaterialBatchDryRun`

更新补充：2026-06-30 09:42

已继续落地：

- 刷新 `BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1` 的默认 UE5.8 引擎搜索路径，加入：
  - `Z:\GZA_Software\RealityCapture\UE_5.8`
- 生成 runtime profiling capture 的默认准备报告：
  - `Docs/GeneratedReports/UE58PerformanceAutomation/RuntimeProfilingCapture/LATEST.md`
- `RuntimeProfilingCapture/LATEST.md` 当前为 `Status: Prepared`，只证明采集命令链路准备好，不代表已完成性能实测。
- 将 runtime profiling capture 默认刷新接入 `Run-UE58PerformanceGoalHeartbeat.ps1`，后续 5H 恢复时会继续保留 profiling capture 的状态。
- 将该约束补入 `Test-UE58PerformanceArtifactContract.ps1`，确保：
  - 默认 Z 盘 UE5.8 路径不会丢失。
  - heartbeat 会刷新非运行模式的 capture report。
  - 真正 GPU 证据仍必须显式运行 `-Run`。

当前验收状态：

- `ArtifactContract/LATEST.md`：`Status: Passed`。
- `RuntimeProfilingCapture/LATEST.md`：`Status: Prepared`。
- 仍未采集真实 `stat rhi`、`stat scenerendering`、`stat gpu`、`profilegpu` 结果。

下一步：

- 干净链接和真实 MaterialBatch dry-run 完成后，再运行：
  - `BuildScripts\Automation\Invoke-UE58RuntimeProfilingCapture.ps1 -RepoRoot X:\Project\Dev01 -Run`
- 至少对比：
  - `Baseline_LumenOff_NoBatch`
  - `LumenLite_NoBatch`
  - 后续有 batch proxy 产物后再加入 `BatchProxy_LumenOff` / `BatchProxy_LumenLite`
更新补充：2026-06-30 09:48

已继续落地：

- 已按用户确认的“本次目标可以进行编译”尝试走统一编译验证入口：
  - `BuildScripts\Automation\Invoke-UE58BuildValidation.ps1 -RepoRoot X:\Project\Dev01 -RunBuild`
- 本次编译入口没有强行关闭编辑器；由于当前仍存在 `UnrealEditor.exe` 进程，脚本在编译前停止并写入阻塞证据：
  - `Docs\GeneratedReports\UE58PerformanceAutomation\BuildValidation\LATEST.md`
  - 当前状态：`BlockedByOpenEditor`
  - 打开的编辑器进程：`UnrealEditor.exe` PID `3640`
  - DLL 锁定证据仍包括 `UnrealEditor-DevKit.dll`、`UnrealEditor-DevKitEditor.dll`、`UnrealEditor-YogComboGraphEditor.dll`
- 已完成材质 MCP 深度审计，补齐此前“只跑 quick mode、未读取完整表达式图”的缺口：
  - `BuildScripts\Automation\Invoke-UE58MaterialMcpAudit.ps1 -RepoRoot X:\Project\Dev01 -FullExpressionGraph -ProbeAllOutputs -TimeoutSec 120`
  - 最新报告：`Docs\GeneratedReports\UE58PerformanceAutomation\UE58MaterialMcpAudit_LATEST.md`
  - `Expression graph mode: Full`
  - `Output probe mode: AllCommonOutputs`
  - `Expression count: 145`
  - `MaterialAttributes connected: True`
  - `Required batch parameters present: True`
  - `Texture-array related expression count: 3`
  - `PerInstanceCustomData expression count: 4`
  - `TextureCoordinate expression count: 5`
- 已刷新静态审计、artifact contract、submission gate 和 heartbeat：
  - `Docs\GeneratedReports\UE58PerformanceAudit\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\ArtifactContract\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\LATEST.md`
- `UE58 Material MCP full expression graph` 当前已为 `True`。
- `Test-UE58PerformanceArtifactContract.ps1` 当前仍为 `Passed`。

验收口径调整：

- `M_Env_Building` 源材质的参数、`MP_MaterialAttributes`、表达式图和常见输出 probe 现在可以作为 Phase 0 材质审计证据。
- 这仍不等于最终 batch parent material 已验收；`M_Env_Building_Batch` 或等价 batch parent material 仍需单独验证 `TexCoord7.x` / batch index / PropertyTexture / VT Atlas 采样链路。
- 编译验证已经允许执行，但当前 clean link 仍等待用户保存并关闭 UnrealEditor；自动化不会擅自关闭编辑器。

仍未完成：

- 干净链接后的 `DevKitEditor Win64 Development` 编译成功记录。
- 干净链接后的 report-only commandlet：
  - `GraphicsSettingsWidgetSetup`
  - `MaterialBatchMaterialAudit`
  - `UE58RuntimeProfilingPlan`
- 真实 cluster `Invoke-UE58MaterialBatchDryRun.ps1 -Run`。
- 实际 batch parent material、VT Atlas、ProxyMesh、MappingData、地面/墙面 bake parity 和 VT/Non-VT residency profiling。

更新补充：2026-06-30 09:51

已继续落地：

- 已运行低风险的 batch visual MCP 资产缩略图审计：
  - `BuildScripts\Automation\Invoke-UE58BatchVisualMcpAudit.ps1 -RepoRoot X:\Project\Dev01 -TimeoutSec 60`
  - 最新报告：`Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\LATEST.md`
- 当前报告状态为 `Incomplete`，但已经从“无报告”推进为“可定位缺失产物”的状态：
  - Source floor mesh 捕获成功：`source_floor_mesh.png`
  - Source floor material instance 捕获成功：`source_floor_material_instance.png`
  - Generated batch proxy mesh 捕获失败
  - Generated batch material instance 捕获失败
- 已刷新：
  - `Docs\GeneratedReports\UE58PerformanceAudit\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\ArtifactContract\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\LATEST.md`
- 当前静态审计状态：
  - `UE58 Batch Visual MCP audit report: True`
  - `UE58 Batch Visual MCP captures: False`
  - `UE58 Batch Visual MCP PNG files: False`

验收口径调整：

- Source 侧资产已经可以通过 MCP 生成缩略图证据。
- Batch visual gate 仍不能通过，因为真正需要审核的 generated batch proxy mesh 和 generated batch material instance 尚未由干净链接后的 commandlet/dry-run 生成或注册到当前可加载资产路径。
- 不在用户可能仍有未保存编辑器状态时运行 `Invoke-UE58SceneParityMcpAudit.ps1`，因为该脚本会加载目标关卡并临时放置 scratch actor；这一步应放到用户保存/关闭或明确允许切换编辑器关卡后再执行。

仍未完成：

- 生成并捕获 `SM_BatchProxy_*`、`MI_Env_Batch_*` 等真实 batch 产物缩略图。
- Scene-level source/proxy side-by-side viewport parity PNG。
- clean link、report-only commandlet、真实 cluster dry-run、runtime profiling。

更新补充：2026-06-30 10:04

已继续落地：

- 根据目标更新，自动化策略已调整：
  - 如果编译被打开的 `UnrealEditor` 阻塞，`Invoke-UE58BuildValidation.ps1 -RunBuild` 默认允许自动关闭阻塞的编辑器。
  - 如果 MCP 证据需要编辑器但当前未开启，则允许自动启动 UE5.8 编辑器用于 MCP。
  - heartbeat 本身仍只刷新状态，不自动编译。
- 已实际执行一次该策略：
  - 正常关闭 `UnrealEditor` PID `3640`。
  - 执行 `DevKitEditor Win64 Development` 编译。
  - 最新编译结果：`Result: Succeeded`。
  - 最新报告：`Docs\GeneratedReports\UE58PerformanceAutomation\BuildValidation\LATEST.md`
- 编译后已运行 post-clean-link 验证：
  - `GraphicsSettingsWidgetSetup`
  - `MaterialBatchMaterialAudit`
  - `UE58RuntimeProfilingPlan`
  - 三个 report-only commandlet 当前均可用。
  - `CommandletAvailability/LATEST.md` 当前为 `Status: Available`。
  - `PostCleanLinkValidation/LATEST.md` 当前为 `Status: ReportCommandletsAvailable`。
- 已运行真实 cluster MaterialBatch dry-run：
  - Map：`/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
  - Cluster：`Prison_S_01_SourceProxy`
  - Tier：`Mid`
  - Texture backend：`VTAtlas`
  - 当前状态：`Partial`
  - `MaterialBatchAudit` 与 `MaterialBatchBuild` 均写出共享报告和 manifest。
  - 当前 `First blocking evidence: MissingEnvBatchTags`
  - 当前 `EnvBatch actor count: 0`
  - 当前 `Manifest source found: 0`
  - 当前 `Source/Proxy asset evidence: True`
  - 当前 `Residency risk evidence: True`
- 已修正 dry-run wrapper：
  - 当编辑器为 MCP 打开并占用 `127.0.0.1:8765` 时，`UnrealEditor-Cmd` 可能因 secondary MCP listener bind 失败返回进程码 `1`。
  - wrapper 现在会识别日志中的 commandlet `result 0` 与已写出的共享报告，把该情况标为 `EffectiveSucceeded: True`，同时保留 `McpPortBindError: True`。
- 已修正 batch visual MCP 审计：
  - 生成资产路径不再使用旧 `FloorBrick03_Probe`，而是优先读取 `Docs\GeneratedReports\CommandletReports\MaterialBatchBuildManifest.json` 中的真实 planned package。
  - 当前 generated proxy/material 仍为 `MissingAssetFile`，因为 dry-run 不生成 `.uasset`。
- 已刷新：
  - `Docs\GeneratedReports\UE58PerformanceAutomation\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAudit\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\ArtifactContract\LATEST.md`
  - `Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\LATEST.md`

验收口径调整：

- clean link 和 report-only commandlet 可用性已经通过。
- 第一轮真实 cluster dry-run 已经进入 commandlet 执行阶段；当前不是构建阻塞，也不是 commandlet class 缺失。
- 下一步首要工作不是继续查 DataLayer，而是给目标 pilot cluster 中的关卡 actor 配置 `EnvBatch.Source.Prison_S_01` 等标签，并让 Source/Proxy/Baked 传统 Streaming Level 层级/可见性证据能被 commandlet 读到。
- `DryRunCaptured` 仍必须等待：
  - 有 EnvBatch tagged source candidates。
  - 有实际 DataLayer 归属证据。
  - 有真实 Source/Proxy/Baked layer readiness。
  - 有 generated batch proxy/material/VT Atlas/MappingData 后续产物与视觉 parity。

仍未完成：

- 自动或美术确认方式给 pilot cluster 写入 `EnvBatch.*` 标签和 DataLayer 配置。
- 真实生成 `SM_BatchProxy_*`、`MI_Env_Batch_*`、`VT_Atlas_*`、`T_PropTexture_*`、`DA_MaterialBatchMap_*`。
- Batch visual MCP generated asset capture。
- Scene parity MCP side-by-side viewport PNG。
- Runtime profiling capture。
- 本地 `main` 仍落后 `origin/main`，最终提交/上传前需要处理远端差异。
更新补充：2026-06-30 10:22

材质阶段边界已按最新需求收敛：

- 当前阶段不直接制作最终地面/墙面材质网络，只提供性能分级接口、贴图采样合同、命名合同、Source/Baked 切换合同和验收口径。
- 新增规范文档：
  - `Docs/04_开发实现与系统文档/性能/UE58_材质性能分级接口与制作技术规范.md`
- 新增代码接口：
  - `FYogMaterialPerformanceTierInterface`
  - `UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier`
  - `UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForGraphicsSettings`
- 材质接口输出当前约定：
  - Epic / High：优先 Source 主材 A，允许 3 套 unique texture set，保留较完整 runtime 混合和 overlay 预算。
  - Mid：优先 bake/proxy/VT Atlas，允许 2 套 texture set、2 层 runtime blend、1 层 overlay，作为默认推荐和掌机候选验证档。
  - Low：优先 bake 后贴图或 VT Atlas，限制为 1 套 texture set、1 层 blend、0 层 dynamic overlay。
- 地面和墙面处理原则已写入规范：
  - 地面 Source 阶段可所见即所得完成多层高度混合、mesh paint/顶点色和静态贴花混合；打包阶段 bake；动态逻辑只允许在 bake 后 RVT/overlay 层处理。
  - 墙面 Source 阶段也允许混合和静态贴花，但默认不推荐 runtime RVT overlay，优先 bake 到 wall baked texture 或 VT Atlas。
- VT/non-VT 混用风险继续保留为验收项：
  - 环境 batch 默认 VT Atlas。
  - 角色、交互物、VFX、动态 actor 暂不强制 VT，也不进入环境合批。
  - 后续 profiling 必须同时记录 VT pool、non-VT streaming pool、Source texture 与 baked/atlas texture 是否重复驻留。

今日 MaterialBatch dry-run 推进状态：

- 已通过 `EnvBatchAutoTag -Apply` 给 pilot map 中 36 个 eligible static mesh actor 写入 `EnvBatch.Source.Prison_S_01_SourceProxy`。
- 最新 `Invoke-UE58MaterialBatchDryRun.ps1 -Run` 已能读取：
  - `EnvBatch actor count: 36`
  - `Manifest source found: 36`
  - `Manifest batch candidates: 36`
  - `Source/Proxy asset evidence: True`
  - `Residency risk evidence: True`
- 已修正 dry-run wrapper 的 DataLayer 验收口径，避免把“已规划 DataLayer”误判为“实际 DataLayer 已配置”。
- 当前第一阻塞点为：
  - `First blocking evidence: MissingActualDataLayerEvidence`
  - `Source/Proxy missing actual DataLayer count: 36`
- 下一步模型/场景侧应先补实际 DataLayer 归属，再继续生成 proxy/baked/VT Atlas 产物和视觉 parity 验收。

验证补充：2026-06-30 10:18

- 已执行 `BuildScripts/Automation/Invoke-UE58BuildValidation.ps1 -RepoRoot X:\Project\Dev01 -RunBuild`。
  - 最新状态：`BuildSucceeded`
  - `Latest UBT result: Result: Succeeded`
- 已执行聚焦自动化测试：
  - `Automation RunTests DevKit.Performance.Settings`
  - 发现 4 个测试：
    - `DevKit.Performance.Settings.GraphicsSettingsWidgetContract`
    - `DevKit.Performance.Settings.MakesClampedCustomSettings`
    - `DevKit.Performance.Settings.MaterialTierInterface`
    - `DevKit.Performance.Settings.TargetTierMappings`
  - 4 个测试全部 `Result={Success}`，日志结尾为 `TEST COMPLETE. EXIT CODE: 0`。
- 已刷新：
  - `Docs/GeneratedReports/UE58PerformanceAudit/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/ArtifactContract/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/LATEST.md`
  - `Docs/GeneratedReports/UE58PerformanceAutomation/LATEST.md`
- `Test-UE58PerformanceArtifactContract.ps1` 最新状态：`Passed`。
- `Invoke-UE58RequiredTests.ps1` 已补入当前机器 UE5.8 默认路径 `Z:\GZA_Software\RealityCapture\UE_5.8`，后续 required test 自动化不再需要额外传入该路径。

更新补充：2026-06-30 10:42

DataLayer 落地工具状态：

- 新增 `EnvBatchDataLayer` commandlet，用于对已经打好 `EnvBatch.Source/Proxy/Baked.*` 标签的静态环境 actor 进行真实 DataLayer 归属检查和应用。
  - 报告路径：`Docs\GeneratedReports\CommandletReports\EnvBatchDataLayerReport.md`
  - 默认目标 DataLayer：`DL_<Cluster>_<Role>`，例如本轮 pilot 为 `DL_Prison_S_01_SourceProxy_Source`。
  - 默认只对已有 DataLayer 执行 `Actor->AddDataLayer`，并保存受影响 package。
- 已修正 `MaterialBatchBuild` 的真实 DataLayer 证据读取：
  - 现在同时记录 DataLayer short name 与 instance name。
  - 这样 private DataLayer 的 GUID/object name 不会遮蔽美术和自动化约定的 `DL_<Cluster>_<Role>` 名称。
- UE5.8 commandlet 中直接创建 World Partition private DataLayer 风险较高：
  - 已验证 `UDataLayerEditorSubsystem::CreateDataLayerInstance` 路径会在 commandlet 中崩溃。
  - 已验证直接 `AWorldDataLayers::CreateDataLayer<UDataLayerInstancePrivate>()` 路径同样会崩溃。
  - 因此 `-CreateMissingDataLayer` 保留为显式 opt-in，不进入当前自动化默认路径。

当前 pilot 结果：

- `EnvBatchDataLayer -Apply` 已稳定产出报告且不再崩溃。
- 目标 map：`/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`
- 目标 cluster：`Prison_S_01_SourceProxy`
- 目标 role：`Source`
- 目标 DataLayer：`DL_Prison_S_01_SourceProxy_Source`
- 当前报告证据：
  - `Tagged actors: 36`
  - `Candidate actors: 36`
  - `Actors queued for apply: 36`
  - `DataLayer missing: True`
  - `Applied actors: 0`
- 结论：actor tag 和候选扫描已经可用，但真实 Source DataLayer 尚未创建，所以 MaterialBatch dry-run 仍必须停在 `MissingActualDataLayerEvidence`。

下一步验收顺序：

1. 通过编辑器/MCP 的安全路径创建 `DL_Prison_S_01_SourceProxy_Source`，不要在 commandlet 默认路径中自动创建。
2. 重新运行：
   - `EnvBatchDataLayer -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 -Cluster=Prison_S_01_SourceProxy -Role=Source -Apply`
3. 验收 `EnvBatchDataLayerReport.md`：
   - `DataLayer missing: False`
   - `Applied actors: 36` 或 `Already ready actors + Applied actors = 36`
4. 重新运行 `Invoke-UE58MaterialBatchDryRun.ps1 -Run`，确认 `MissingActualDataLayerEvidence` 消失。
5. 再进入 proxy/baked/VT Atlas/MappingData 生成和 batch visual/scene parity 验收。

## 2026-06-30 场景分层纠偏：传统 Streaming Level 为当前有效口径

当前项目场景使用传统流送关卡，不开启 World Partition；本轮性能分级落地不使用 DataLayer，也不把 DataLayer 作为模型、场景或材质合批的验收前提。本文前面所有把 Source/Proxy/Baked 互斥写成 DataLayer/WP 路径的段落都只保留为历史记录，后续实现和验收以本节为准。

有效技术方案：

- `LayerBackend = StreamingLevel` 是当前默认且唯一需要通过的场景分层口径。
- `MaterialBatchBuildCommandlet` 扫描真实 Map 时会加载全部 streaming sublevel，并记录每个 candidate actor/component 的 `actualStreamingLevel`、`actualLevelPackage` 和 `actualLayers`。
- `Source` actor 的 expected layer 默认等于它当前所在的 streaming level；`Proxy` 与 `Baked` 产物后续按 `SL_<Cluster>_Proxy`、`SL_<Cluster>_Baked` 或同等命名的生成/流送层组织。
- `DryRunCaptured` 要求 `Actual layer evidence=True`、`Layer evidence mode=StreamingLevel`、Source/Proxy asset readiness 和 residency evidence 同时成立。
- `EnvBatchDataLayer` commandlet 已从当前实现路径移除；后续不再创建、检查或应用 DataLayer。
- 角色、交互物、动态 actor、VFX、运行时贴花仍使用 `EnvBatch.Exclude` 或不进入 EnvBatch；它们不参与环境合批，也不进入 batch VT Atlas。

当前纠偏后的下一步：

1. 重新编译 DevKitEditor，确认删除 DataLayer commandlet 后 UHT/UBT 通过。
2. 运行 `Test-UE58PerformanceArtifactContract.ps1`，确认静态契约改为 `LayerBackend/actualLayers/actualStreamingLevel`。
3. 运行 `Invoke-UE58MaterialBatchDryRun.ps1 -Run`，让 pilot cluster 重新产出 StreamingLevel 口径的 report/manifest。
4. 刷新 `Analyze-UE58PerformanceInputs.ps1`、SubmissionGate 和 heartbeat，确认不再出现 DataLayer blocker。
