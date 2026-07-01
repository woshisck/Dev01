# UE5.8 Virtual Texture 与 VirtualTextureCollection 使用指南

版本：2026-07-01
适用：DevKit（UE5.8）
关联工具：`贴图 VT 审计`、`VTC 管理器`（DevKit 工具 → 美术资产工具）

---

## 1. 背景与目标

项目从 **T2DA + PropTexture** 的手动合批系统迁移到 **VirtualTextureCollection (VTC)**，原因：

| 项 | T2DA 旧方案 | VTC 新方案 |
|-----|-----|-----|
| 分辨率 | 必须统一 | ✅ 混合分辨率 |
| 格式 | 必须统一 | ✅ 支持格式转换 |
| 索引表 | 手动构建 `_PropTexture` | ✅ 引擎自动生成 |
| 显存 | 一次性全加载 | ✅ VT Page Cache 按需流式 |
| 材质连接 | 5 列 UV7 + Custom Node | ✅ 3 节点组合 |
| 上限 | 硬件 Texture2DArray 上限 | 单张贴图 ≤ 4K |

**VTC 是 UE 5.8 的 `Experimental` 特性**，API 可能变化，需保留 T2DA 作短期 fallback。

---

## 2. 项目 VT 全局配置

`Config/DefaultEngine.ini`：

```ini
; VT 全局开关：VirtualTextureCollection / SVT / RVT 依赖此项 (2026-07-01)
r.VirtualTextures=True
bEnableVirtualTextureOpacityMask=True
r.VirtualTexturedLightmaps=False
; VT Pool 大小（默认 64MB，项目使用中等规模 VTC，先设 128MB）
r.VT.PoolSizeInMegabyte=128
; 各向异性过滤，VT 采样质量
r.VT.AnisotropicFiltering=1
```

**修改后需要**：
1. 重启编辑器
2. 已存在的贴图**不会自动开 VT**，需要在 `贴图 VT 审计` 工具中批量开启

---

## 3. VirtualTextureCollection 架构

### 3.1 核心概念

VTC 把多张源贴图**打包到一张物理 VT 纹理** + 一个 Page Table + 一个 Index Buffer。

```
[VTC 资产]
   ├── Textures[0..N]     ← 源贴图（可混合分辨率、可混合格式）
   ├── RuntimePixelFormat ← 引擎选择的公共物理格式
   ├── bAllowFormatConversion (true) ← 允许格式转换
   └── bIsSRGB (true)     ← Physical 是否 sRGB

运行时：
   源贴图 → 引擎打包成 UDIM 布局的大 VT
        → PageTable + IndexBuffer(BlockX/Y/W/H per entry)
```

材质中通过 `CollectionIndex` 采样：
- `[TextureCollection]`：引用 VTC 资产
- `[TextureObjectFromCollection]`：给 Index 拿出一个 Texture Object
- `[TextureSample]`：普通采样，会自动走 VT 路径

### 3.2 VTC 硬性约束

| 项 | 约束 |
|-----|-----|
| 状态 | Experimental — 会有黄色警告 |
| 单张源贴图尺寸 | ≤ **4K**（`PackedCoordinateAndSize` 12-bit block） |
| 格式白名单 | DXT1/DXT5/BC4/BC5/BC6H/BC7/BGRA8/RGBA8/G8/R8G8 |
| 平台 | 需 `r.VirtualTextures=True`（SM5+） |
| Physical 格式 | 单一，混合格式会有 runtime conversion 开销 |
| Source 贴图 | 建议**不勾选** `VirtualTextureStreaming`（VTC 本身负责 VT） |

---

## 4. 工作流

### 4.1 美术贴图入库时

1. 导入贴图（DDS/PNG/TGA）
2. 尺寸控制在 **1K–4K** 之间（512 以下不用 VT，>4K 无法加入 VTC）
3. 压缩：色彩用 BC1/BC7，法线用 BC5，Mask 用 BC4/BC5
4. 打开 **DevKit 工具 → 美术资产工具 → 贴图 VT 审计**，检查新贴图状态
5. 通过后加入 VTC（下面 4.2）

### 4.2 建立 VTC（VTC 管理器）

**方式 A：从 Content Browser 一键打包**
1. Content Browser 里选中多张相关贴图（例如某个建筑套的所有 BaseColor）
2. 打开 **DevKit 工具 → 美术资产工具 → VTC 管理器**
3. 点击工具栏 `用 Content Browser 选中贴图新建 VTC`
4. VTC 会创建在 `/Game/Art/Textures/VTC/VTC_FromSelection`（可重命名）

**方式 B：追加到已有 VTC**
1. VTC 管理器左侧列表选中一个 VTC
2. Content Browser 里选中要追加的贴图
3. 点 `追加到当前 VTC`

**方式 C：手动新建空 VTC**
1. 点 `新建 VTC` → 在 `/Game/Art/Textures/VTC/` 生成 `VTC_New`
2. 打开 VTC 资产，手动拖入 Textures 数组

### 4.3 校验

- **贴图 VT 审计**：单张贴图角度的检查（VT 开没开、尺寸、格式）
- **VTC 管理器 → 全部校验**：VTC 内部一致性（空成员、超尺寸、不支持格式）

问题类型：
- `Blocked`：必须修，例如尺寸 > 4K
- `Warning`：建议改，例如 1K 图没开 VT
- `Info`：小贴图（<1K），一般不建议 VT

---

## 5. 材质中使用 VTC

### 5.1 三节点组合（标准用法）

```
[TextureCollection]  ──▶  [TextureObjectFromCollection] ──▶  [TextureSample]
       │                         │  Index (Scalar)              │  UVs
       │                         │  TextureType (Texture2D)      │  MipValue
       └── TextureCollectionObject: VTC 资产                     └── SamplerType: Virtual Color
```

**关键属性**：
- `TextureObjectFromCollection.TextureType`：`Texture 2D`
- `TextureObjectFromCollection.ConstCollectionIndex`：常量索引（用参数控制时接 Index 输入）
- `TextureSample.SamplerType`：**必须选 Virtual Color / Virtual Normal / Virtual Masks 之一**
- `TextureSample.MipValueMode`：默认或 `MipLevel`（VT 支持 Anisotropic）

### 5.2 UV 传递

VTC 内部把每张源图打包成 UDIM 的一个 tile，但材质里**仍用 0-1 的原始 UV** —— 引擎会自动把 UV 重映射到对应 tile 的 physical region。**不需要再做 tile 偏移计算**。

### 5.3 索引来源

- **静态：** 每个材质实例一个 `Const Index`
- **动态：** 从 UV7.x（现有 batch material index）或 Vertex Color 传入
- **性能分级：** 通过 `r.Yog.MaterialLightQuality` 等 CVar 分支切换 Index

---

## 6. 编辑器工具速查

菜单路径：**Tools → DevKit 工具 → 美术资产工具**

| 工具 | 功能 |
|-----|-----|
| 材质合规检查 | 贴图命名、sRGB 规则、材质分级接口（已有） |
| **贴图 VT 审计** | **VT 状态、尺寸/格式警告、VTC 兼容性、批量开关 VT** |
| **VTC 管理器** | **列出所有 VTC、新建、批量添加成员、合规校验** |

### 贴图 VT 审计 快捷操作

- 顶部 `重新扫描` — 重扫 `/Game/Art` 下所有 Texture2D
- 顶部 `对所选开启 VT` — 批量勾选 `VirtualTextureStreaming`（会保存包体）
- 筛选栏可切换 Pass/Warning/Blocked/Info 状态显示
- 底部搜索：按名称/路径过滤

### VTC 管理器 快捷操作

- 左：所有 VTC 列表，右：成员详情
- 顶部 5 个动作按钮：新建 / 从 CB 新建 / 追加 / 移除 / 校验 / 打开
- 成员列表红字 = 有兼容性问题（超 4K、格式不白名单等）

---

## 7. 已知限制与规避

| 问题 | 规避 |
|-----|-----|
| VTC 是 Experimental，API 可能变 | 保留 T2DA 系统一段时间，别一次性删掉 |
| 单张 > 4K 无法加入 | 编辑器工具会 Block 提示，缩到 4K 或以下 |
| 格式混合时有转换开销 | 尽量让一个 VTC 内的贴图用同类压缩（例如 BaseColor 全 BC1） |
| VT Pool 溢出（可见 tile 太多） | 增大 `r.VT.PoolSizeInMegabyte`，或拆分 VTC |
| Editor UI 简陋（Textures Array 手动拖） | 用本工具的批量按钮，不用直接编辑资产 |

---

## 8. 与其它系统的关系

- **RVT**：地面材质烘焙用 RVT，与 VTC 独立且**互补**（RVT 输出可作为 VTC 一员）
- **T2DA**：将逐步废弃，代码路径 `M_Env_Building_Batch` + `MaterialBatch.ush` 会保留到 VTC 全面稳定
- **性能分级**：`r.Yog.MaterialLightQuality` 等 CVar 仍可用来切材质分支，索引不同 VTC 或不同 Index

---

## 9. 后续 TODO

- [ ] 把 `M_Env_Building_Batch` 复制一份出 `M_Env_Building_VTC`，用 VTC 三节点重写
- [ ] 对现有 `T_Array_A / T_Array_N / T_Array_M / T_Array_E` 逐一拆回单独贴图后灌入 VTC
- [ ] 在 `贴图 VT 审计` 里加"一键把当前扫描结果打包成 VTC 分组建议"
- [ ] 集成 UDIM 导入工作流（Substance/Painter）
