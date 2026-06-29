# GPU 性能分析操作手册

> 适用：UE5.8，编辑器内实测 + 打包版分析。每次测试前必须切换到对应 DeviceProfile。

---

## 零、MaterialBatchBuild Commandlet 命令

> 用于离线扫描场景候选并生成合批资产计划/manifest。Git Bash 调用必须前置 `MSYS_NO_PATHCONV=1`，否则 `/Game/...` 路径会被转成 `C:/Program Files/Git/Game/...`。

```bash
# DryRun（只生成计划，不写资产）
MSYS_NO_PATHCONV=1 "D:/UE/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Self/GItGame/Dev01/DevKit.uproject" \
  -run=MaterialBatchBuild \
  -Map=/Game/Art/Map/ArtCheckLevel/GymLeve_L1 \
  -Cluster=Prison_S_03_Pillars \
  -RequireTag=EnvBatch.Source \
  -unattended -nopause
```

参数说明：

- `-Map=/Game/...` — 要扫描的关卡（commandlet 会自动 Flush 子关卡流式加载）
- `-Cluster=<Name>` — 生成资产的 Cluster 名（决定 `T2DA_*_<Cluster>` 等命名）
- `-RequireTag=<Prefix>` — 只扫描 Actor Tag 以此前缀开头的物件（默认空，扫描全部）
- `-Apply` / `-ApplyMappingOnly` / `-ApplyTextureArraysOnly` / `-ApplyPropertyTextureOnly` — 实际写资产（默认 DryRun）
- `-MaxActors=N` — 最大扫描 Actor 数（默认 2000）

输出文件：

- `Docs/GeneratedReports/CommandletReports/MaterialBatchBuildReport.md` — 人类可读报告
- `Docs/GeneratedReports/CommandletReports/MaterialBatchBuildManifest.json` — 机器可读 manifest

### Runtime Profiling Plan Commandlet

> 用于生成实测前的固定场景矩阵和记录模板，不会采集 GPU 数据，也不会写地图或资产。

```powershell
& "D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Self\GItGame\Dev01\DevKit.uproject" `
  -run=UE58RuntimeProfilingPlan `
  -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 `
  -Cluster=Prison_S_01 `
  -Camera=WideShot_A `
  -unattended -nop4 -nosplash
```

输出文件：

- `Docs/GeneratedReports/CommandletReports/UE58RuntimeProfilingPlanReport.md`
- 报告中的 `NotMeasured` 是保护标记，表示该表格等待编辑器/真机实测填数。

### 已知陷阱

1. **Git Bash 路径转换**：`/Game/...` 会被 MSYS 转成 `C:/Program Files/Git/Game/...`。所有调用必须前置 `MSYS_NO_PATHCONV=1`（PowerShell 无此问题）。
2. **空 placeholder 关卡**：部分 sublevel（如 `L1_CommonLevel_Prison_S_03_ZS`）里所有 `SM_xxx` StaticMeshActor 和 `BP_BuildingOclusionCulling_Wall` 的 `StaticMeshComponent.StaticMesh` 都是 `None`，bounds 退化。Commandlet 会全部报 `NotStaticMeshComponent`，这是关卡数据问题不是工具问题。Apply 测试请改用 `ScanAssetCandidates` 路径（去掉 `-Map=`，加 `-Root=/Game/Art/Models/...`）。
3. **Tier 字段**：当前分级使用 `Low/Medium/High/Ultra`。命令行建议显式传入 `-Tier=Medium` 或 `-Tier=Low`，避免生成资产落到旧目录。

---

---

## 一、测试前准备

### 切换到目标档位（编辑器控制台）

```
# 低档（掌机 5W）
r.SetRes 1280x720
sg.ViewDistanceQuality 0
sg.ShadowQuality 0
sg.GlobalIlluminationQuality 0
sg.ReflectionQuality 0
sg.PostProcessQuality 0
sg.TextureQuality 1
sg.EffectsQuality 0
sg.FoliageQuality 0
r.ScreenPercentage 55
t.MaxFPS 30
r.Lumen.DiffuseIndirect.Allow 0

# 中档（掌机 15W / Lumen Lite）
r.SetRes 1280x720
sg.GlobalIlluminationQuality 1
sg.ShadowQuality 1
sg.ViewDistanceQuality 1
r.ScreenPercentage 70
t.MaxFPS 60
r.Lumen.DiffuseIndirect.Allow 1
r.Lumen.FinalGatherMethod 0

# 高档（PC）
sg.GlobalIlluminationQuality 2
r.ScreenPercentage 100
t.MaxFPS 0

# 超高档（PC Ultra）
sg.GlobalIlluminationQuality 3
r.ScreenPercentage 100
t.MaxFPS 0
```

### 锁定摄像机视角

把摄像机固定在**代表场景的标准采样点**，每次测试相同位置，保证数据可比：
- 宽景点（视野内物件最多）
- 室内密集点
- 战斗压力点（同屏多特效）

---

## 二、基础性能命令

### 实时显示

| 命令 | 显示内容 | 用途 |
| --- | --- | --- |
| `stat unit` | Game/Draw/GPU/RHI ms | 最先看，判断瓶颈在 CPU 还是 GPU |
| `stat gpu` | 各 GPU pass 耗时 | GPU 细分，找最贵的 Pass |
| `stat rhi` | Draw Call 数、三角面数、显存 | 判断 Draw Call 是否下降 |
| `stat scenerendering` | Mesh Draw Commands、Visible Lights | 渲染剔除效果 |
| `stat fps` | 帧率 | 快速确认 |

### 单帧快照

```
# 最全的单帧 GPU 分析（会短暂卡顿一帧）
profilegpu

# 查看动态实例化合批效果
r.MeshDrawCommands.LogDynamicInstancingStats 1
```

`profilegpu` 结果保存在：`Saved/Profiling/GPUProfile-*.csv`

---

## 三、性能矩阵测试流程

对同一个代表场景，按以下顺序各测一次，记录数据到表格：

| 场景配置 | stat unit GPU ms | Draw Calls | 备注 |
| --- | --- | --- | --- |
| **Baseline** | | | Lumen Off，无合批 |
| **Lumen Lite Only** | | | sg.GlobalIlluminationQuality=1，无合批 |
| **Batch Only** | | | 合批代理可见，Lumen Off |
| **Batch + Lumen Lite** | | | 合批 + Lumen Lite 组合 |
| **Low 档烘培方案** | | | 地面烘培平面替代，Lumen Off |

每次测试记录：
- `stat unit` → GPU ms（取 5 秒均值）
- `stat rhi` → Mesh Draw Calls
- `stat scenerendering` → Visible Dynamic Primitives
- 是否稳定 30/60 帧

---

## 四、Lumen Lite 专项分析

Lumen 的 Pass 在 `stat gpu` 里显示为：

```
Lumen Scene Lighting     ← Surface Cache 更新，越大说明场景越复杂
Lumen Diffuse Indirect   ← Screen Probe Gather / Irradiance Field
Lumen Reflections        ← 反射 Pass（中档关闭，高档以上开启）
```

**Lumen Lite 关键 CVar 验证：**

```
# 确认 FinalGatherMethod=0（Irradiance Field，便宜的那个）
r.Lumen.FinalGatherMethod

# 确认关闭了 SDF Tracing（最贵的路径）
r.Lumen.TraceMeshSDFs.Allow

# 确认关闭了硬件光追
r.Lumen.HardwareRayTracing.HitLighting.Allowed
```

若 `Lumen Scene Lighting` 超过 4ms（掌机），优先检查：
1. Surface Cache 大小 → 降低 `r.LumenScene.SurfaceCache.AtlasSize`
2. 动态灯数量 → 减少场景动态灯
3. 场景物件密度 → 合批效果是否生效

---

## 五、合批效果验证

```
# 查看实例化合批统计（会在输出日志里打印）
r.MeshDrawCommands.LogDynamicInstancingStats 1

# 查看当前帧 Draw Call 细分
r.MeshDrawCommands.DumpInstancingStats 1

# 强制禁用合批（对比用）
r.MeshDrawCommands.UseDynamicInstancing 0
```

**预期结果：**
- 合批后 `Mesh Draw Calls`（stat rhi）应明显下降
- 如果下降不明显，检查是否所有 Source Actor 都正确标记 Tag 并切换为 Proxy

---

## 六、赛璐璐材质灯开销分析

材质灯的采样开销不在 Lumen Pass 里，在 `Base Pass` 中：

```
stat gpu 里查看：
BasePass    ← 赛璐璐材质灯采样在这里
```

检查方法：
1. `profilegpu` 单帧快照
2. 在结果里找 `BasePass` 耗时
3. 屏蔽材质灯（临时把 LightInfoCount 参数设为 0）再对比
4. 差值 = 材质灯采样开销

若材质灯占 BasePass 超过 40%，需要为掌机档降低 `LightInfoCount` 参数默认值。

---

## 七、测试结果记录模板

每次测试后填写（存入 `Docs/GeneratedReports/` 下）：

```
测试日期：
场景：
摄像机位：
档位配置：
合批状态：开/关

stat unit 结果：
  Game ms：
  Draw ms：
  GPU ms：
  RHI ms：

stat rhi 结果：
  Mesh Draw Calls：
  Triangle Count：

stat scenerendering 结果：
  Visible Dynamic Primitives：

主要瓶颈（GPU ms 最高 Pass）：
是否达标（Low=30fps / Medium=60fps）：
备注：
```

---

## 八、常见问题排查

| 现象 | 可能原因 | 排查命令 |
| --- | --- | --- |
| GPU ms 高但 Draw Call 正常 | Lumen Pass 贵 / 材质灯贵 / 特效贵 | `profilegpu` 找最贵 Pass |
| Draw Call 高，合批没效果 | Proxy Actor 未正确显示 / Tag 未设置 | 检查 Actor Tag，确认 Proxy 可见 |
| 合批后出现视觉错误 | UV7 未写入 / 材质参数名不匹配 | 检查 UV7 数据，对比材质参数名 |
| Lumen Lite 比预期贵 | FinalGatherMethod 不是 0 | `r.Lumen.FinalGatherMethod` 确认 |
| 低档仍然卡 | 动态阴影 / 动态灯没关 | `sg.ShadowQuality 0`，减少动态灯 |
