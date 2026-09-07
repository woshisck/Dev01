# Dev01：物理单位灯光创作与风格化协作方案

日期：2026-09-08。性质：基于当前文件的只读技术评估与待批准方案，未修改灯光、地图、材质、项目设置或引擎。本次没有打开关卡评图，也没有编译、性能采样或完成 A/B 验收。

## 结论

**可以继续用物理数据驱动灯光，而且适合作为团队的统一创作入口。** 推荐保留 UE 的 Directional / Point / Spot / Rect Light，把物理光强、颜色/色温、形状、位置、阴影作为场景权威；固定一套曝光基线；把角色明暗分层、可读性补偿放在角色材质与共享 CharacterLightingProfile 层。

目前不是“引擎不支持物理单位”，而是**物理灯之后叠加了场景分段、角色强度归一化、最低亮度补偿，以及另一套 Celes 材质数据灯**。这些控制面叠加后，同一个 Intensity 数值不再有单一、直观的画面含义。数据驱动可以保留，但“数据灯”的数据应该驱动真实 LightComponent，而不是把未经单位换算的 Intensity 当成另一套通用光能。

这套方案不要求把角色改成写实 PBR；它要求把“场景里的光是什么”和“角色如何艺术化地响应这束光”分开管理。

## 1. 本次确认的代码与版本边界

- 项目源代码与配置：`X:\Project\Dev01`。
- P4/UGS 项目：`X:\Project\YogProject\Dev01`。
- 唯一可编辑引擎源码：`X:\Dev-BuildEngine`。
- 本次核对的 `Config/DefaultGame.ini`、Celes 设置应用、CelesPointLight、CaptureBox、StylizedEmissiveLight 在 Git 与 P4 本地副本中内容一致（忽略 CRLF 差异）。
- 当前实际类名是 `UStylizedLightingSettings`，配置节为 `/Script/CelesLightRuntime.StylizedLightingSettings`。本次搜索未找到 `YogStylizedLightingSettings` 类定义。
- 本次先前的分发核对为项目 CL127 / Engine CL76 / PCB #26、CL128；这是时间点快照，不是后续发布的固定目标。

**源码引擎和当前 Installed Build 的 Shader 并不完全相同。** 本次逐文件 SHA256 比较：

| Engine/Shaders/Private 下的文件 | 源码与 UGS Installed 是否相同 | 影响 |
| --- | --- | --- |
| DeferredLightingCommon.ush | 相同 | 本文角色直接光归一化与全局直接光分段分析适用于两份文件 |
| StylizedCharacterLighting.ush | 相同 | 材质 Profile、半兰伯特坐标、角色曝光公式一致 |
| ShadingModels.ush | 不同 | 源码角色漫反射使用 DiffuseColor；Installed 仍使用 BaseColor |
| DiffuseIndirectComposite.usf | 不同 | 源码最低亮度用 DiffuseColor 与能量项；Installed 使用 BaseColor |
| StylizedLumenLighting.ush | 不同 | 源码增加 GI 亮度分段函数；Installed 没有该函数 |

因此不能把源码中新的 PBR 修正或 GI 上限处理，描述成所有同事当前已看到的效果。**同一个 BuildId 也不足以证明 Shader 内容一致。** 评图必须记录 EngineCL、项目/PCB CL、相关 Shader 版本以及运行的编辑器路径。

## 2. 原生物理灯仍然存在

| 创作灯种 | 当前本地源码证据 | 建议的团队用法 |
| --- | --- | --- |
| Directional | `DirectionalLightComponent.cpp:1509` 标明 Lux 语义；原生方向光路径保留 | 用 lux 记录照度基准，颜色或色温另列 |
| Point | `PointLightComponent.cpp:127` 起保留 candela / lumen / nits / EV 换算；lumen 有球面立体角换算 | 明确 lm 或 cd，启用平方反比衰减，不把两者数值直接比较 |
| Spot | `SpotLightComponent.cpp:79` 起保留单位换算及聚光几何处理 | 单位、锥角、光源尺寸一起作为预设，不只存一个强度数字 |
| Rect | `RectLightComponent.cpp:179` 起保留物理单位换算 | 单位与宽高/光源形状一起管理，适合窗光/面光源 |

完整路径前缀：`X:\Dev-BuildEngine\Engine\Source\Runtime\Engine\Private\Components\`。

这些灯不需要先变成 CelesPointLight 才能影响角色。引擎 `DeferredLightingCommon.ush:464` 对局部光使用 `bRadialLight` 与 `LOCAL_MULTI_LIGHTS` 开关，当前项目已开启局部多灯。新增的 `StylizedCharacterLightMode` 位于原生 `ULightComponent`，普通灯也能提供 Toon / Wash 等角色响应标记。

但有两个细节必须明确：

1. `ULocalLightComponent` C++ 默认 `IntensityUnits=Unitless`，`ACelesPointLight` 构造函数只设置 Intensity=5000，没有显式选择 lm/cd。已有地图可能另存单位覆盖；本次没有逐灯加载检查，因此不能宣称当前所有灯已经用物理单位。
2. “物理单位可用”不等于所有响应完全物理正确。原生 AttenuationRadius 本身是性能用的影响范围截断；角色二分明暗、底亮和艺术高光也是有意的非物理处理。

## 3. 为什么当前灯光数值不够直观

### 3.1 场景 PBR 也被全局风格化影响，不只是角色

`X:\Project\Dev01\Config\DefaultGame.ini:133` 起当前设置：

- `bEnableStylizedLumenLighting=True`
- `BandCount=5`、`BandSoftness=0.18`、`GlossInfluence=0.65`
- `DirectBlend=1`、`IndirectBlend=1`、`IndirectMaxLuminance=2`

`X:\Dev-BuildEngine\Engine\Shaders\Private\DeferredLightingCommon.ush:431` 明确对 **非 StylizedCharacter** 材质的直接漫反射和高光乘分段函数。`StylizedLumenLighting.ush:20` 起使用 Roughness/GlossInfluence 改写 N·L 曲线，再离散成明暗带。因此普通环境材质也不是未改动的连续 PBR 响应；不能仅靠更换一种灯恢复标准调光手感。

源码引擎还在 `StylizedLumenLighting.ush:88` 起将 GI 转回 scene-referred 亮度，除以 `IndirectMaxLuminance`、saturate、分段后再乘回。当前 Blend=1 时，该分段支路会限制高于基准的亮度差异。**这段新增 GI 公式在当前 Installed Shader 中不存在**；Installed 的间接风格化仍可经 Lumen 场景灯光注入路径发生，不能把两套实现混为一谈。

### 3.2 角色直接光强做了“归一化与物理亮度混合”

当前 Profile（`DefaultGame.ini:152`）：`DirectLightIntensityInfluence=0.5`、`DirectLightColorInfluence=0.5`。

`DeferredLightingCommon.ush:524` 起核心公式可简化为：

```text
L = 当前渲染灯光颜色的亮度
NormalizedColor = LightColor / max(L, epsilon)
ControlledLuminance = lerp(1, L, IntensityInfluence)
ControlledColor = lerp(white, NormalizedColor, ColorInfluence) * ControlledLuminance
```

当 Influence=0.5 时是 `0.5 + 0.5L`，不是“严格按物理强度缩放到 50%”；弱光附近有归一化基底。这里的 L 是渲染器阶段的灯光量，不是界面上未经换算的 lumen 数值。其余距离/面积衰减仍在原生支路中，不能说所有距离衰减已被取消。

如果想把灯光数字重新作为可预测的调节入口，A/B 基线应先令 IntensityInfluence=1、ColorInfluence=1，再决定艺术化保留多少。**只改这两个值仍不是完整 DefaultLit**：角色依然有二分遮罩、材质 Diffuse Bias、独立高光和阴影响应。

### 3.3 暗部补光、遮蔽与独立调色有多层控制

当前 Profile 还配置 `CharacterBaseFill=0.2`、`IndirectOcclusionStrength=0.35`、底亮受 AO 影响 0.25，且开启 DarkColorFloor。这样做有角色可读性的价值，但角色处于真实暗处时不会只由场景光决定明暗。

- 源码 `DiffuseIndirectComposite.usf:755` 起用 PBR DiffuseColor 和能量项构造最低间接漫反射。
- Installed 对应文件 `:742` 仍用 BaseColor，金属表面也可能得到这层假漫反射。源码已针对这个边界修正，但本次没有发布。
- 当前 `bEnableCharacterTone=False`，CharacterExposure=0、Contrast=1；不能把当前问题直接归因于角色独立曝光已开启。
- 若以后打开角色 Tone，`StylizedCharacterLighting.ush:84` 使用 `exp2(Exposure)` 和亮度幂次 Contrast，会再形成一层独立调色。
- `DefaultEngine.ini:70` 扩展曝光范围；`:73,75` 设置局部曝光亮暗对比 0.8。`DefaultScalability.ini:245` 在最低档关闭 LocalExposure，其余档位开启。不同画质档的最终显示可能不同。

本次没有检查关卡后处理体、相机运行时曝光覆盖和视口预览曝光，因此还不能判断“当前画面过亮/过灰”具体有多少来自曝光。

### 3.4 同一原生灯种还可能选了不同的角色响应模式

`DeferredLightingCommon.ush:472` 起：Wash 只取亮度，忽略场景投影，并去掉角色直接高光；Toon 走二分阴影遮罩；Rim 模式当前代码明确归零（保留枚举并不代表运行效果仍在）。

多人协作时必须把 Mode 显示在灯光清单里。否则一盏“同样强度的点光”对角色无投影、无高光或完全无直接响应，会被误判为 lux/lumen 或材质异常。

### 3.5 Celes 数据灯与物理灯目前不是同一种数据合同

- `ACelesPointLight` 内部确实有 `UPointLightComponent`。但它还把原始 `Intensity * LightIntensityMultiply`、原始颜色、半径和 SmoothStep 写入 Celes 数据。原生灯与数据支路不是一份已统一单位的辐射量。
- `ACelesLightCaptureBox::AddNativePointLightData`（`CelesLightCaptureBox.cpp:293`）直接复制 PointLightComponent.Intensity；数据结构没有保存 IntensityUnits。`HighestIntensity` 排序也直接按这个数排序。**数值相同的 lm 与 cd 不应这样等同处理。** 它没有构造完整的 Directional/Spot/Rect 物理几何数据合同，不能拿这份数据纹理替代 UE 全灯种照明。
- `AStylizedEmissiveLight` 没有解析式 LightComponent，`GetLight_Implementation()` 返回 nullptr。`Intensity` 写材质光数据，`EmissiveIntensity` 写可见模型的 HDR Emissive，两者都不能直接叫作“流明”。可见模型启用 Native Lumen 时可参与发光 GI，但 DataOnly 没有真实发光几何，也不等价于投影/高光完备的点灯。
- `CelesLightReceiveComponent.cpp:80` 的自动改写 Mesh 材质函数当前是空实现，数据由 CaptureBox 提供。本次未追踪每个材质资产是否采样数据纹理，**不声称当前所有角色都有“双重照明”**；只确认这两种创作接口并存且单位含义不同。

上述插件路径前缀：`X:\Project\Dev01\Plugins\CelesLight\Source\CelesLightRuntime\Private\Actors\`；ReceiveComponent 位于相邻 `Private\Components\`。

## 4. 建议的协作结构（尚未实施）

### 4.1 物理灯权威层

允许继续“用数据放灯”：建立经过批准的灯具预设/数据资产，驱动原生 LightComponent，保存 **类型、单位、强度、色温或颜色、尺寸、锥角、平方反比开关、阴影策略、影响通道、性能等级**。

灯具预设不暗中改全局曝光、不同时再乘一套隐形强度倍率。创建灯时显式选单位。局部灯的 `InverseExposureBlend` 基线为 0，只有独立标记的玩法提示灯例外；本机引擎 `LocalLightComponent.h:31` 也提醒不要把曝光补偿用于主要场景灯。

Celes 数据纹理灯保留为明确标注的“艺术补光/材质效果”，不承担场景主照明、真实投影或照度标定。若确有统一数据要求，应未来扩展数据合同并进行单位/灯种几何换算，而不是把现有原始数字改名为 lm。

### 4.2 单一曝光基线

基准关卡固定同一相机与手动曝光/固定 EV100，统一白平衡、色调映射和后处理版本。最终游戏可以有自动曝光，但要有明确测光范围、目标和过渡策略，不允许每个美术视口以个人预览曝光作为验收依据。

初次标定关闭局部曝光补偿；待物理基线确认后再作为显示层效果 A/B 加回，并按实际高/中/低画质档验证。不要通过反复加灯和降低曝光去相互抵消。

### 4.3 角色风格化层

默认保留当前角色的材质方向，不做全项目切换。用独立 Profile 的实验分支比较：物理光强/光色影响为 1、直接漫反射/反射/GI 倍率为 1、角色 Tone 关闭、底亮先设 0，再逐项加回。

二分明暗、Diffuse Bias、MixMap 高光和可读性底亮归角色负责人管理；场景灯作者不通过全局角色参数补救单一关卡。环境 PBR 基线关闭全局 `StylizedLumenLighting`，之后若确需环境分段，再明确建立一个风格化场景档，而不是默认把所有环境都纳入同一不可见改写。

### 4.4 版本化共享 LightingProfile

项目已经有 `CharacterLightingProfiles` 数组，最多 8 档，材质用数字索引选择；`StylizedCharacterLookSubsystem.cpp:14` 起还允许相机所在 LookVolume 覆盖。先利用现有结构，不立即另造重叠系统。

建议建立一个团队维护的 LightingProfile 合同：

- 场景灯具预设、曝光基线、角色 Profile、LookVolume 优先级、画质档和引擎/Shader 版本成组记录。
- Profile 名称与索引固定；新增可追加，不随意调整数组顺序，否则旧材质索引会指向不同外观。
- 明确谁能改全局配置、谁能改角色材质、谁能改关卡灯；跨层改动必须附同机位前后图与原因。
- 地图/灯具资产/材质/Profile 资产走 P4；代码和批准文本按项目同步白名单处理。不要再依赖“只在某人的 Git Content 里存在”的公共灯光资源。
- 发布通过源引擎编译与本地验收 → Installed Build → 固定 EngineCL → 匹配 PCB → 同事端验收。禁止把 Installed Engine 当源码改。

## 5. 建议的最小 A/B 验证

以下为待执行实验，不是当前效果结论；无需先全图换灯。

建立一个隔离验证关卡或获准的测试副本，放 18% 中性灰参照、粗糙介质球、金属球、实际玩家/敌人，使用真正的俯视战斗相机，保留地板、阴影、轮廓与技能预警的可读性。固定引擎构建、分辨率、曝光、白平衡、画质、灯位与材质；一次只改一个因素。

| 阶段 | 只改变什么 | 要回答的问题 |
| --- | --- | --- |
| A：原生参照 | DefaultLit 参照物、关闭全局分段、固定曝光；一盏带单位的原生灯 | 标准灯的强度与距离响应是否符合预期？ |
| B：角色物理输入 | 保持灯不变；角色 Intensity/ColorInfluence=1、Tone 关闭、BaseFill=0，其余角色造型路径保留 | 非线性来自角色响应，还是场景灯/曝光？ |
| C：艺术回加 | 逐一恢复二分、底亮、AO减弱、色彩抑制；最后测试场景分段与局部曝光 | 哪个代价换来的可读性是团队需要的？ |

每盏灯测试 0.5× / 1× / 2× 强度；Point/Spot 在不接近源尺寸与半径截断的范围测 1m/2m/4m；在关闭 GI 的隔离直接光检查中，平方反比只针对适用局部灯、固定角度等条件，不能要求 Directional 也按距离衰减。lumen 与 candela 比较需先做等效换算。

记录线性/scene-referred 光照读数与最终屏幕截图两份证据：色调映射后的截图不会简单“亮两倍”，不能拿显示像素值直接判定光强换算错误。另录实际角色移动进出阴影、多个弱灯叠加、黑暗/有色光、金属表面、LookVolume 切换与低画质 LocalExposure 关闭场景。

验收标准：同一预设在两位同事的同版本 UGS 工作区有一致输入与可复现画面；原生参照物响应可解释；角色艺术差异可归因到有限 Profile 参数；不以丢失投影/金属假漫反射换取无意的亮度；记录目标硬件的 GPU 阶段耗时与 P95/P99，避免用更多数据灯或解析灯盲目填暗。

## 6. 推荐先后顺序

1. 先确定一个对照关卡、实际游戏相机和固定曝光，记录现有灯的单位/Mode，不修改生产地图。
2. 完成 A/B，判断保留哪种角色风格化，而不是先把全部现有灯换类型。
3. 整理统一灯具预设与共享 Profile 合同，优先修复单位歧义和跨人版本差异。
4. 如需把源码已有的金属/底亮修正带给同事，单独走引擎发布链；本报告不触发编译或发布。

本报告唯一产物为该 Markdown 文档。实现、批量迁移、引擎发布以及任何生产画面变更均需另行安排。
