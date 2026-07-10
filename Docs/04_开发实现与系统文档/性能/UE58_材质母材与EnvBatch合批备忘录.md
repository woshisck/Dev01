# UE5.8 材质母材与 EnvBatch 合批备忘录

更新时间：2026-07-02

本文记录当前约定，用于后续制作性能测试关卡、材质母材和自动化工具时对齐范围。

## 目标

后续材质体系拆成三类母材：

| 母材 | 作用对象 | 主要职责 |
| --- | --- | --- |
| 地面 RVT 母材 | 地形、地面大面 | 写入/读取 RVT，承接地表混合、贴花烘焙、泥土/湿度/遮蔽等效果。 |
| Building 母材 | 墙面、建筑、大块结构 | 支持 Mesh Paint、手动实例化、UDIM 或大面材质规则；接收 RVT 信息做接地、污渍、湿边、AO 等增强。 |
| Prop 母材 | 静态道具、可合批小物件 | 支持普通 Texture Collection/UDIM 或后续集合贴图；接收 RVT 信息做环境融合；为 Prop.Batched 预留合批 index 入口。 |

Building 和 Prop 都需要能接收 RVT 信息，但 RVT 是效果增强，不是所有物件贴图合批的唯一方案。

## 内容分类

| 分类 | 是否打 EnvBatch | 主要技术 |
| --- | --- | --- |
| 地形 | 否 | RVT。 |
| 墙面、建筑 | 默认否，必要时打 `Building.Instance` 做组织/审计 | 手动实例化、Mesh Paint、UDIM、建筑母材。 |
| 静态实例化物件 | 是，`Prop.Instance` | ISM/HISM、Texture Collection 或同模型多贴图管理。 |
| 静态合批物件 | 是，`Prop.Batched` | 几何合并、Texture Collection/UDIM 贴图集合、合批材质。 |

不要把整个关卡直接作为一个合批组。组名应表达区域、类型和处理方式，例如：

```text
EnvBatch.Source.<LevelName>.Prop.Batched.TC-A.01
```

## 贴图集合方向

### Texture Collection

普通 `UTextureCollection` 更适合当前大量独立小物件的集合化需求：

- 每个物件继续提交普通 unique 贴图。
- 工具链根据命名规则、材质参数和 EnvBatch tag 收集贴图。
- 自动生成或维护 Texture Collection Data Asset。
- 材质通过外部 index 采样对应集合成员。
- 适合尺寸不完全一致、来源分散、物件边界清晰的 Prop。

Texture Collection 不是 UE 自动判断合批范围。“哪些贴图进哪个 collection、哪个 Actor 属于哪个集合、哪个材质对应哪个 index”仍由项目工具生成。

### UDIM

UDIM 更适合手工规划的大面积连续表面：

- 建筑墙体、大块连续结构、特殊 hero asset 可以考虑。
- 美术需要更明确的 UV/UDIM 规划。
- 不适合把大量零散小物件硬塞进同一套 UDIM。

### RVT

RVT 当前只作为地面主路径，以及 Building/Prop 的环境信息来源：

- 地形写入 RVT。
- Building/Prop 读取 RVT 信息用于接地、融合、污渍、湿边、AO 或风格化增强。
- 不把 RVT 当作普通物件贴图集合工具。

## 合批 index 承载

默认不再把 index 写入 UV7。原因是大多数模型只应保留必要通道，额外补到 8 套 UV 会增加顶点数据体积，也会让资产规范变复杂。

当前优先级：

1. 第二 UV 通道的可用分量。
   - 用户约定中“2U”用于 Mesh Paint 或合批辅助。
   - Prop.Batched 通常不使用 Mesh Paint，因此可以优先占用。
2. VertexColor.A 作为兜底。
   - 对只需少量 index 的物件足够。
   - 注意顶点色精度通常是 8-bit，不能承载过大的集合 index。
3. UV7 只作为历史方案或特殊工具链 fallback，不作为默认策略。

后续工具需要在扫描阶段判断：

- 模型是否已有第二 UV。
- 第二 UV 是否被 Mesh Paint 或其他系统占用。
- 顶点色是否存在、是否被美术效果占用。
- 当前 batch 需要的 index 数量是否超过 VertexColor.A 可承载范围。

## EnvBatch Source Tag

命名格式：

```text
EnvBatch.Source.<LevelName>.<Prop|Building>.<Instance|Batched>.<TextureCollectionGroup>.<Serial>
```

示例：

```text
EnvBatch.Source.L1_CommonLevel_corridor_01b.Prop.Instance.TC-A.01
EnvBatch.Source.L1_CommonLevel_corridor_01b.Prop.Batched.TC-A.01
EnvBatch.Source.L1_CommonLevel_corridor_01b.Prop.Batched.TC-A.02
EnvBatch.Source.L1_CommonLevel_corridor_01b.Building.Instance.TC-B.01
```

规则：

- 这是 Actor Tags，不是 GameplayTags。
- 每个 Actor 只能保留一个 `EnvBatch.Source.*`。
- `Prop.Batched` 是自动合批的主要候选。
- `Building.*` 先用于组织、审计和人工策略，不默认进入自动合批。
- `Instance` 表示优先走实例化或集合贴图，不代表几何合并。
- `Batched` 表示后续可进入几何合并和合批材质。
- 同一个完整 Source Tag 是模型 merge 组边界；`TextureCollectionGroup` 是 SharedProp 贴图集合父组，允许 `TC-A.01`、`TC-A.02` 等多个 merge 组共享一组按通道拆分的 Texture Collection。
- 后续自动 Texture Collection 命名按通道拆分，例如 `TC_<Level>_<TextureCollectionGroup>_SharedProp_BaseColor`、`TC_<Level>_<TextureCollectionGroup>_SharedProp_Normal`、`TC_<Level>_<TextureCollectionGroup>_SharedProp_ORM`、`TC_<Level>_<TextureCollectionGroup>_SharedProp_MaterialLight`。

## 后续工具链接口

后续自动化应拆成几个阶段：

1. 扫描 Actor Tags。
2. 校验 Actor 是否静态、是否适合合批、是否包含可用 StaticMesh。
3. 按完整 Source Tag 分模型 merge 组，并按 `LevelName + TextureCollectionGroup + 通道` 分贴图集合父组。
4. 收集材质参数和贴图命名。
5. 生成 Texture Collection/UDIM 计划。
6. 判断 index 写入策略：第二 UV 或 VertexColor.A。
7. 生成合批材质实例或材质参数映射。
8. 对 Prop.Batched 执行几何合并或生成代理资产。
9. 输出报告，再由人审查后决定是否替换关卡引用。

当前阶段只完成 Actor Tags 标记入口和规范备忘录；不直接替换关卡资产。
