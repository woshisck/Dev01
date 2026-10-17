# LookDev 灯光关卡与定标依据

LookDev 的编辑器代码和关卡归属 `Plugins/YogLookDev`，主关卡为 `/YogLookDev/LookDevMain`。评审内容位于 `ReviewLevel`；四套灯光位于 `LightDev`，包括中性环境，以及 L1、L2、L3 各一个标准环境。插件单向依赖 YogArt、YogArt_Material、YogArt_VFX、YogAnimSource、YogSFX、YogFrontLib，因此 Review 关卡可以直接引用这些内容插件的资产；不反向依赖项目 DevKitEditor，也不创建插件间相互依赖。

入口为 `YogTool → LookDev 光照对比`，也可从渲染工作台打开；兼容原命令 `DevKit.LookDev.Open` 和 Tab ID `DevKitCharacterLookDev`。面板仅保留 Review 加载、灯光场景切换、通用设置，以及类似 Env. Light Mixer 的方向光/天空光双列参数，不再提供相机、渲染诊断或本机备注。切换只控制子关卡，不重新生成已保存的灯光。

中性场景继续使用原来的 `AssetsPreview_BP` 模型、HDRI、背景材质及校准物，路径改为 `/YogLookDev/Assets/AssetsPreview_BP`。其灯光控制迁入原生 `AYogLookDevLightingRig`，默认 **MidContrast、背景补偿 0、曝光补偿 0、Grounded 开、Calibrator 开**；方向光 2.3 lux、白色，天空光 1.2、白色，间接光和体积散射强度均为 1。Low/Mid/High 分别使用原蓝图的 `(0.8, 2.5)`、`(2.3, 1.2)`、`(7, 0.42)` 主光/天空光强度组合。重新选择档位会恢复该档强度与方向，普通刷新、加载和撤销不累加旋转。面板修改保存在灯光关卡的原生属性中。

原蓝图的 Raytracing Shadow、Enable Transmission、Use Hard Ware Raytracing、Enable Checker、Turn Lighting 控制和对应执行图已移除；旧 PBR Checker 组件禁用。LookDev 不再写入全局光追 CVar，也不会修改角色材质或角色渲染算法。YogLookDev 仅启用于 Editor 目标，模块类型为 Editor；正式 Cook/Stage 内容排除仍需打包验证。不要直接把插件根目录添加到 `DirectoriesToNeverCook`：当前引擎会因此创建重叠的资产引用域，破坏插件依赖检查。

迁移使用 `YogLookDevMigrate` commandlet，默认只读预检，`-Apply` 才执行；执行前备份 45 个源包并记录哈希，通过 Unreal AssetTools 移动资产，保留旧路径 redirector，限制保存范围为 LookDev。`-VerifyOnly` 应在新进程中检查保存后的结果。迁移前不要运行 Setup 的 `-Apply`：初始化现在会检查整个地图集合，若缺失的新图仍有旧包或重定向对应，立即拒绝创建，以免用空图替换原有内容。无人值守自动打开面板也不会创建缺失的主关卡。不可对全项目执行重定向清理。旧 `/Game/LookDev/DataPrepViewMain` 不属于本次迁移范围。

四个评审关卡保留迁移前已保存的内容，可继续由美术填充。需要留意评审中心约为 `(0, 0, 100)` cm：初始灯光围绕这里布置，放入更大的 building 或 prop 后，可直接在相应灯光子关卡中移动和扩大光源。

## 单位与规则

- Directional 主光使用 **lux**；Point / Rect 使用 **lumens**。Point 开启平方反比衰减；Rect 使用原生面积光分布。衰减半径 2000 cm 是影响范围限制，不用于人为拉平距离衰减。[Epic 物理灯光单位说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-physical-lighting-units-in-unreal-engine)
- 主、辅灯光的 Diffuse Scale、Specular Scale、Indirect Lighting Intensity 均为 1；Inverse Exposure Blend 为 0。阴影可读性由有明确位置、面积和通量的辅光提供，不通过材质自发光、降低高光能量或改变贴图来补亮。
- 辅光是一块 2.4×2.4 m Rect，位于 `(-200, 200, 240)` cm，朝向评审中心。它是环境反射或侧窗的可编辑近似。在还没有建筑、墙面与实测 HDR 的初始空关卡中，这个来源比任意天空倍数容易核对。
- 当前不添加 `T_GreyAmbient` SkyLight。该贴图缺少此项目的绝对亮度定标，不能把强度倍数称为场景真实照度。后续加入测量或定标后的 HDR 时，需要记录亮度依据，并减少相应的近似辅光，避免重复计算环境贡献。
- 下表 L1-L3 使用一个不受画面亮暗影响的 Manual 曝光；ISO 100、光圈 f/4、曝光补偿 0。`CameraShutterSpeed = 2^EV100 / 16`（该属性单位为 1/s）。中性关卡继续使用其既有默认曝光与灯光设置。[Epic 曝光说明](https://dev.epicgames.com/documentation/unreal-engine/auto-exposure-in-unreal-engine)
- 灯光颜色采用项目当前的线性 RGB；保留灯光原生色温默认值，未启用或调整色温。后处理不覆盖白平衡、Tint 或全局调色。

这些规则约束灯光输入和曝光，不能据此宣称所有角色材质本身已满足能量守恒。旧 Unlit / 自定义采样材质仍由自身算法决定响应；Default Lit 和定制角色材质都需在同一套地图中实际检查。

## 四套初始灯光

数值是依据光源类型、尺寸、距离和参考图方向选择的**可编辑设计起点**，并非从绘画中测出的照度，也尚未作为全关卡最终灯光签核。

| 方案 | 主光及空间依据 | 环境辅光 | 固定 EV100 | 验收重点 |
| --- | --- | --- | --- | --- |
| 中性环境，灯光基准 | 原 AssetsPreview 场景；Mid 主光 2.3 lux、Sky 1.2 | 原灰色 HDRI，面板可编辑 | Manual，补偿 0；保留原相机曝光标定 | 中灰、金属和布料在同一光照下比较 |
| L1 / 标准 | 暖色 Point，1600 lm，距评审中心 1.90 m，发光半径 8 cm | 冷青 320 lm，作为石壁/远处开口反射 | 3 | 近处火光塑形、脸和暗色布料的层次 |
| L2 / 标准 | 橙金 Rect，16000 lm，2.4×4 m，作为燃烧立面；距中心约 3.79 m | 冷灰 1800 lm，作为烟尘与街道反射 | 5 | 橘红烟尘氛围；衣料色差、火场高光细节 |
| L3 / 标准 | 暖白 Rect，9000 lm，1.6×2.4 m，作为高窗焦点 | 蓝紫 2000 lm，作为侧窗/冷色石壁反射 | 6 | 银饰、浅衣保留高光层次；蓝紫圣域和暖白焦点 |

火光近/远的白光解析估算分别为约 35 lx 和 2.9 lx，使用 `E = Φ / (4π r²)`，`r` 以米计。这个估算用于核对距离趋势；未计入颜色、遮挡、角度、有限光源和衰减半径截断。Rect 的照度随发光面积、方向和距离变化，不能把 lumens 直接等同于角色表面的 lux。最终照度与过曝判断应在实际关卡里用灰卡和 Pixel Inspector 核对。

地牢的雾密度 0.008；城市为 0.025；教堂为 0.012。它们是 UE ExponentialHeightFog 的视觉起点，**不代表测量得到的烟尘消光系数**。画板只提供颜色/气氛方向；完成真实场景后再核对能见距离、角色附近层次和背景深度。当前雾关卡也不包含额外火焰、粒子或燃烧建筑。

## 美术如何定稿

1. 在上半区选择通用、角色、building 或 prop，点击“加载关卡”，在对应 ReviewLevel 子关卡填充内容并保存。
2. 在下半区点击灯光名称，即可切换唯一可见的灯光子关卡。检查时保持编辑器视口使用 Game Settings 曝光，以读取灯光关卡的固定 EV100。
3. 定标时可自行放置线性反射率 0.18 的灰卡、深色布料、白色非金属和粗糙/光滑金属参考。它们属于 ReviewLevel 内容，切换灯光时保持不动。
4. 优先调整明确来源的灯光位置、尺寸和通量；保留同一关卡家族的固定曝光做明暗比较。确认材质不死黑、亮部未丢失纹理后，再进行场景色彩定稿。
5. 保存调整后的 LightDev 子关卡。再次点击灯光会加载该保存版本；只有缺失的初始关卡会按代码中的方案生成。

L1–L3 初始方案数据见 `Plugins/YogLookDev/Source/YogLookDevEditor/Private/Tools/CharacterLookDev/DevKitCharacterLookDevPresets.cpp`（Revision 2）。中性场景控制见同模块 `Private/LookDev/YogLookDevLightingRig.cpp`。生成后，美术保存的地图资产就是灯光编辑结果；更新预设源码不会自动覆盖它们。

## 本地迁移验收（2026-10-11）

- 已将 45 个包（35 个 uasset、10 个 umap）迁至 `/YogLookDev`，旧根目录保留 45 个重定向包。新进程 `-VerifyOnly` 确认目标数量、原生蓝图父类、移除旧执行图、默认值和主关卡 8 条新路径引用。
- 独立源码引擎开发副本 `X:\Dev-ProjectBuilds\Dev01-Workflow` 编译成功；12 项逻辑测试和 1 项 DX12 原生面板渲染测试通过。已有导航/动画蓝图警告仍保留，未改动相关资产。900×900、640×900 面板截图位于该副本 `Saved/Automation/LookDevWorkbench`。
- 两轮最终验收均对旧重定向及新资产共 90 个文件做 SHA-256 前后对比，无改写、无新增。报告位于该副本 `Saved/LookDevPluginMigration-20261011/Acceptance-Logic-20261011-174222`、`Acceptance-Panel-20261011-174346`。
- 原始资产备份及哈希清单位于该副本 `Saved/LookDevMigration/20261011-093050-2CB3F6C9`。旧测试自动生成的 9 张占位地图另行归档在 `Saved/LookDevPluginMigration-20261011/GeneratedPlaceholderMaps-20261011-155125`，未作为迁移结果使用。
- 尚未提交 P4、发布 PCB/UGS 或验证正式 Cook/Stage。使用新的资产前需要加载本次编译的 YogLookDevEditor 模块；不要用尚未包含新插件模块的旧 UGS 二进制打开它们。
