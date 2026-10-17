# Dev01 开发、构建与数据工作流

维护日期：2026-09-20。版本号应从现场预检获取。

## 统一入口

- 主入口在编辑器中：`YogTool → 开发工作台`，或工具栏的“开发工作台”。这是可停靠的原生 Slate 面板。
- `YogTool → 构建、发布与数据整理` 打开同一 UE 面板的发布页，不再启动外部 GUI。
- 编辑器外的 `Dev01_Workflow.bat` 仅作必须关闭 UE 才能构建时的辅助入口，不是主工作台。
- 工具、制作顺序、输入/产出与本机路径统一在 `Build/Dev01WorkflowCatalog.json`。
- 现场报告：`Saved/WorkflowHub/Reports`；运行日志：`Saved/WorkflowHub/Runs`；归档：`Saved/WorkflowHub/Archive`。

## UE5 风格与使用方式

界面采用 UE5 Starship 默认配色：背景 `#151515`、面板 `#242424`、分区 `#2F2F2F`、选中/主操作 `#0070E0`。原生面板直接使用 `FAppStyle` / `FStyleColors`，随编辑器主题变化；独立工作台使用同一套默认深色视觉语义。保持扁平、少边框，不使用渐变和装饰阴影。

- 原生面板左侧按材质、地表、场景、玩法分组，支持关键词搜索；底部进入“发布与数据整理”。
- 选中制作步骤后，右侧分别说明准备内容和产出。顶部明确当前是源码引擎还是 Installed Build。
- 发布页提供“检查模块匹配”“发布只读预检”“扫描重复日志”和“建立本地资产分类集合”，结果直接显示在面板内，详情默认折叠。
- 检查异步执行，运行期间禁止重复点击；失败不会显示成完成。不自动提交或上传，构建和发布的关闭编辑器/人工验收边界会明确显示。
- 原生只读检查共用 `workflow_core.py`，本机默认 `C:/Python313/python.exe`；其他机器在 `Saved/WorkflowHub/settings.json` 配置 `pythonExecutable`。缺少环境会显示原因，不会悄悄调用其他 Python 或执行发布。
- 独立辅助窗口另提供 `Ctrl+F`、开发副本路径设置和关闭编辑器后的构建执行。它不替代 UE 内的制作工具。

## 两套引擎的二进制隔离

主工作区 `X:/Project/YogProject/Dev01` 保持 UGS 已发布引擎与对应 PCB。源码引擎只能编译独立开发副本，不能再直接编译主工作区，否则项目和插件 DLL 会覆盖 UGS 产物而造成 Missing Modules。

本机开发副本为 `X:/Dev-ProjectBuilds/Dev01-Workflow`。它不是新的 P4 workspace；源码、Shaders、配置、Binaries 和 Intermediate 独立复制，Content 通过 junction 引用原资产。项目即使没有自定义 Shader 文件，也必须保留 `Shaders` 目录供 DevKitShaders 注册。**二进制隔离不等于资产写隔离**：本轮验证不保存任何资产；需要修改资产仍应明确其 P4 归属。

工作台“连接与路径”设置独立副本后，源码构建和启动都使用该目录；主目录、主目录内子目录及指回主目录的 Binaries 会被拒绝。副本不会自动同步源码；更新前应审查文件白名单，避免覆盖副本的在途修改。`prepare_development_copy.ps1` 只允许创建全新目标，遇到现有目录立即停止。

开发副本的 `Dev01Development.json` 标记让原生面板的发布检查使用主工作区配置，不会误把开发副本当作 UGS 安装。新版 UE 内工作台只有在项目源码提交、云端编译和匹配 PCB 发布完成后才进入团队 UGS 版本；仅有本地开发版本不代表已经发布。

## UGS 报错的本次诊断（2026-09-20）

`Saved/Logs/DevKit.log` 记录了引擎 BuildId `792eec68-5f82-4e2f-852a-66474164b377` 与项目/插件 `4b62cf96-fa6d-4dad-a6dd-a57b299a0955` 不匹配。原因是同一主工作区被源码引擎编译过，不是 Engine 未安装。

现场 UGS 记录项目 / Code CL130、PCB #27（P4 CL131）。本轮已用 P4 摘要 `BA565187BC627FD5AAFC8C5090178811` 校验下载包，备份后恢复 49 个项目/插件二进制文件。原文件与前后 SHA-256 清单位于 `Saved/WorkflowHub/Recovery/Restore-20260920-211500/manifest.json`。恢复后 12 处 Engine / target / 项目 / 插件 BuildId 均为 `792eec68-5f82-4e2f-852a-66474164b377`。

恢复未修改 Source、Content、Engine 或 P4 提交状态。不能只改 `.modules` 的 BuildId。`restore_ugs_pcb.ps1` 默认仅校验，需显式 `-Apply` 才恢复，并保存原文件；再次恢复前仍须核对 opened / pending、目标 PCB、编辑器进程和发布摘要。模块匹配和自动化启动通过不能替代同事端完整 UGS 冷下载/效果验收。

## 制作流程

| 工作流 | 顺序 | 权威数据 |
| --- | --- | --- |
| 材质与灯光 | RVT 材质库 → 材质合规 → NoVT 检查 → Texture Collection → 风格化自发光 | 原材质/纹理资产与库配置 |
| 地表与贴花 | 地表物件库 → 贴花放置 → 关卡 RVT / 烘焙 | `Collection.Records`；派生组件不替代放置记录 |
| 场景搭建与优化 | 地图包 → 关卡数据 → 模型检查 → EnvBatch 分组 → 合批审查 | RoomData / CampaignData / Actor Tag / BatchedAsset |
| 角色、战斗与玩法 | 角色 → 武器 → 敌人 → 卡牌/BuffFlow → 数值 → 成长 → 剧情 → 调试 | 各系统原有 DataAsset 与配置 |

工具按需创建，切换步骤保留本次工具状态。步骤序号是制作导航，不是自动验收勾选；每一步以工具自身报告和实际效果为准。贴花步骤调用已有编辑模式，战斗日志保留已有独立窗口的对象生命周期。原工具页 ID 和入口保留在“高级 / 独立工具”，旧布局仍可打开。

这里合并的是编辑入口和制作顺序，未重写渲染算法，也未合并不同业务资产的数据结构。模型/材质/贴花仍由各自原工具执行具体操作。

LookDev 工作台入口为 `YogTool → 开发工作台 → 材质与灯光 → LookDev 关卡工作台`，高级 / 独立工具也保留同名入口。旧页 ID `DevKitCharacterLookDev` 保留兼容已有布局。只有编译并发布匹配 PCB 后，UGS 用户才能看到新界面；仅同步源码不代表已交付。

打开工作台默认打开 `/Game/Art/Map/LookDev/LookDevMain`。上半区选择通用 Review、Character、Building、Prop 后点击唯一的“加载关卡”按钮；内容以子关卡形式加载，选中的 Review 成为当前编辑关卡。四个 Review 初始为空，请在其子关卡内自行填充并正常保存。现有美术 Gym 与掉落/战斗 Gym 从同一选择框独立打开，保留它们原有子关卡和灯光。

下半区逐项列出九个灯光子关卡，点击立即切换显示/隐藏，不另设应用按钮；切灯光不会切换 Review，也不会覆盖已制作内容。灯光文件位于 `/Game/Art/Map/LookDev/LightDev`，Review 位于 `ReviewLevel`。同文件夹新增普通非 World Partition 地图后，重开工作台即可发现。Gym 使用自身灯光，工作台的灯光按钮在 Gym 中禁用；加载任意 Review 可返回主关卡。也可通过控制台命令 `DevKit.LookDev.Open` 打开相同入口，供代码验证和工具接入。

九套灯光使用原生物理单位、平方反比衰减、已说明的光源尺度及固定曝光，依据与初始数值见 [LookDev 灯光方案](Dev01_LookDev_Lighting.md)。它们仍是待美术标定的方案，不是已验收游戏效果，不修改角色 Shader、材质参数、引擎或原有生产地图。完整 GI、角色运行时 LookVolume 与正式游戏镜头仍需在真实关卡验收。自动建图仅创建缺失资产，已有地图不会被重建覆盖。

## 构建与发布

### 光影精度与角色投影工作台（2026-10-10）

#### 独立 NPR 金属管线（2026-10-11）

##### PBR / 风格化块面模式与实时混合（2026-10-12，当前行为以下节为准）

###### 远景基础色块与暗部强化（2026-10-12，最新调整）

- 同一角色金属美术面板新增实时“基础色块大小”（0–1，默认0.30），扩大风格化端点的微表面坡度分布；像素导数提供最低覆盖，不设置相机距离淡出、不扩张金属遮罩或角色轮廓。基础尺寸0取消美术扩展；PBR模式或混合0不使用此扩展。
- 默认宽度倍率由1提高至1.5，高光由2.4提高至3.2，亮芯由3.5提高至4.5。宽度继续读取粗糙度并允许材质覆写；混合仍默认0.5。粗糙度控制柔化和衰减，金属F0、实际光强/光色、结构扫动、VSM可见度仍保留。
- 原“暗面强度”只是反射增益，现隐藏该旧兼容字段，改为独立“暗部强化”（0–1，默认0.35）：只压低风格化金属亮块之外的真实镜面尾部，不降低亮块/亮芯，不改原生环境反射、角色非金属、曝光或整屏对比。它不是额外AO、自阴影图或阴影补色。全局默认及Lighting Profile覆写均实时生效，只有显式保存按钮才写配置。
- Spec审计：当前父材质以MixMap.R经SpecIntensity写入Specular；旧抑制/展宽仅作用于非金属，完整金属不受此控制。角色明暗分界实际由T_Normal.B / Diffuse Bias和光照方向驱动；Metallic取MixMap.B、Roughness取G、金属F0取BaseColor。没有重排旧贴图通道或保存材质资产，新增实际DX12风格化金属Spec=0/1不变性对照。
- 实现复用Profile的12×float4槽位，仅增加着色算术，不增加绘制Pass、邻域采样、Kuwahara或人为环境灯带。对目标机的GPU成本仍需profiling；任何屏幕效果都不能保证在角色/金属完全小于像素、被遮挡或不可见时保持细节。

本轮验收（开发副本，不代表UGS发布）：`Saved/ShadowSplitBuild/20261012-014224-build.log`增量编译成功。`Saved/CharacterShadowValidation/20261012-014441/Automation/index.json`中UI、运行时上传、全局面板、性能与两项实际阴影渲染共10项通过（3项带已有警告）；金属测试当时被外部MCP旧会话错误日志标为失败，渲染断言未失败。已在无MCP干扰的独立DX12进程重跑金属对照，`20261012-015442/Automation/index.json`为1项通过、0失败、0警告，不将受干扰的原报告算作全绿。

- 150像素、全风格化端点下，基础尺寸0 / 0.3 / 0.6的相对高光覆盖约8.50% / 16.16% / 62.52%；基础尺寸0.3在300 / 150像素下约16.55% / 16.18%。该指标按测试球有效区域、各自峰值25%计数，不能当作整个人物的金属占比。
- 暗部强化0→0.8的有效尾部样本5196个，线性亮度均值0.003218→0.001972，亮块平均误差0.000167；Spec=0/1金属输出RGB8平均差0.0431，低于1级阈值。真实光强、金属F0/金属色、粗糙度柔化、PBR/风格化混合与原生GGX对照均通过。
- 本轮6个项目源码文件主目录/开发副本哈希一致；307个LookDev及角色父材质资产哈希与清单不变；两份DefaultGame.ini哈希不变。源引擎开发编辑器已打开LookDevMain和渲染工作台，读取确认基础尺寸0.3、暗部0.35、宽度1.5、高光3.2、亮芯4.5、混合0.5、默认Profile继承；`r.Shadow.Virtual.Enable=1`。没有提交P4/Git、保存关卡/材质或发布UGS。此前的验证记录描述旧默认值，不作为本次验收结果。

- 渲染工作台 → 二分光影设置 → 角色金属美术参数 → **金属模式与混合**：提供“PBR 响应”和“风格化块面”两个互斥模式。默认选择风格化块面，混合强度0.5；0为上一轮PBR响应，1为完整块面，0.25/0.5/0.75可连续预览中间效果。PBR模式暂不使用混合强度，但切回时保留数值。
- PBR端点保持上一轮原生GGX加美术对比增益；块面端点把相同真实镜面的相对GGX分布整理为宽亮带和亮芯，再在线性光照空间混合。不是整屏后处理对比度、固定白色圆盘或自发光补光。两端沿用实际光强/光色、F0/菲涅耳、法线/视角、原生面积光与阴影；粗糙度仍使峰值降低、范围扩大、边缘柔化。块面是美术重塑，不声称严格能量守恒。
- 模式和强度均实时上传，无需Apply。全局默认可由所选Lighting Profile的“独立覆写此Profile”覆盖；材质原有宽度/颜色/贴图覆写继续优先。只作用于自定义角色管线中已启用金属风格的金属像素，不改非金属、场景BRDF、VSM、曝光、反射alpha或已保存材质/关卡。
- 复用已退役环境灯带的GPU数据通道，不增加Pass、纹理邻域采样或Kuwahara。PBR/0强度走早退路径；块面混合增加少量着色算术，目标机GPU耗时仍须实测。原有12×float4 Profile ABI不变，旧配置缺少新字段时读取默认风格化/0.5。
- 测试方式：在同一LookDev主光、相机、材质下对比PBR与块面0/0.5/1；默认宽度倍率1，粗糙度0.05/0.5/0.95检查宽度、峰值和柔和变化。关闭主光应同时消失，移动主光/旋转物体应沿结构扫动；正常游戏镜头按角色150–300像素检查。修改只预览；需要持久化时再点“保存金属与反射配置”。

验证：开发副本编译 `Saved/ShadowSplitBuild/20261012-002610-build.log` 成功。首轮回归发现完整块面模式中等粗糙度的边缘衰减偏硬，增加粗糙度驱动柔化幅度后，最终DX12 / SM6报告 `Saved/CharacterShadowValidation/20261012-003815/Automation/index.json` 共11项通过、0失败、0未运行；3项测试带既有资源读取/控制台优先级警告，无Shader编译错误。截图目录 `Saved/CharacterMetallic/B14C842847B467A23D249EABFFF429D7` 共63张（含诊断遮罩）：0混合与PBR金属像素平均RGB8差为0；完整块面与PBR差4.923；0.5/0.25混合的线性RGB误差0.000179/0.000161；非金属差0.001。完整块面模式粗糙度0.05/0.5/0.95的线性峰值0.437/0.067/0.012、相对高光覆盖2.12%/8.47%/100%、归一化最大边缘梯度0.703/0.377/0.037，确认保留弱化与柔化趋势。材质覆写、F0、零光、结构扫动、Bloom、真实角色150/300像素与阴影回归亦通过；数值与截图不代替美术最终验收。

执行前后306项LookDev资产及角色父材质（合计307项）SHA-256一致，无新增/删除；主工作区与开发副本DefaultGame.ini未变，6个源码镜像文件一致。源引擎/项目/CelesLight模块BuildId匹配。已重开源码开发副本，现场读取确认风格化模式、混合0.5、Profile0继承且启用、反射alpha0.3/0.7及VSM开启。修改前备份在开发副本 `Saved/MetalModeBlend-20261012`；未保存测试资产、未提交或发布Git/P4/UGS。

##### 粗糙金属的衰减与柔化扩散（2026-10-11，覆盖下文仅扩大宽度的响应）

- 按用户补充要求，粗糙度响应改为直接调用原生 `DefaultLitBxDF` 金属镜面，沿用 GGX NDF、Smith 可见性、Schlick F0/菲涅耳、面积光积分/能量归一化和多次散射补偿。越粗糙自然越宽、峰值越低、过渡越柔和；删除固定亮度圆盘、小硬亮芯，以及本轮尚未验收的自定义指数扩散/峰值衰减公式。
- 默认宽度倍率1读取材质原始 Roughness；宽度控制现在缩放 GGX 微表面 `alpha = Roughness²`，有效粗糙度为 `clamp(sqrt(Roughness² × 宽度倍率), 0.04, 1)`，并保留原生 View.MinRoughness。材质显式宽度优先，仍遵循 GGX 的宽度/峰值联动，不再独立放大亮斑。原有高光/亮芯强度只作为真实镜面的对比增益，亮芯形状读取相对 GGX NDF；亮暗面控件也改为反射增益，不给纯金属额外添加漫反射/自发光底色。对比增强是美术增益，整套效果不声称严格能量守恒。
- 不加屏幕模糊、Kuwahara、纹理邻域采样或额外 Pass，不修改原生环境反射、非金属、VSM、曝光或已保存材质/关卡。高光仍由真实光强、结构法线、视角、F0 和投影驱动；没有新增距离剔除。复用原生 BRDF 会改变着色运算，未宣称目标机零耗时或已完成性能验收。
- DX12 粗糙度回归使用相同150像素球体、相同主光和继承宽度，临时排除 GI/Film/Bloom，分别验证高光范围变宽、线性峰值下降和归一化边缘梯度减小。新增中性美术参数与独立 DefaultLit 材质的0.05/0.5/0.95三组真实像素对照，检查物理 GGX 一致性；锐利/扫动/Bloom 回归使用0.25粗糙度抛光球，真实角色贴图不变。金铜银 F0 采样用0.5粗糙度避免窄峰饱和；数值回归不代替美术验收。

验证：最终开发副本增量编译 `Saved/ShadowSplitBuild/20261011-235047-build.log` 成功。DX12 / SM6 报告 `Saved/CharacterShadowValidation/20261011-234113/Automation/index.json` 的10项渲染、上传和工作台测试通过；剩余1项旧“非PBR金属”界面文字断言更新后，在 `20261011-235156/Automation/index.json` 与上传回归一起复测，2项均通过、0失败。渲染报告的3项既有控制台优先级/资产读取警告仍保留，无 Shader 编译错误。金属捕获 `Saved/CharacterMetallic/C363EEAF4FF8B5070AD1F79AC10BA0B7` 共52张（含诊断遮罩）；专项粗糙度/F0夹具使用临时10-lux、30度物理面积方向光使 SDR 峰值不饱和，没有修改用户灯光。粗糙度0.05 / 0.5 / 0.95的线性峰值0.391 / 0.067 / 0.012、相对高光覆盖2.27% / 10.88% / 100%、归一化最大边缘梯度0.858 / 0.104 / 0.051。中性增益与独立 DefaultLit 的金属区域 RGB8 平均差均小于0.05；金铜银黑 F0、材质宽度/颜色/贴图覆写、真实角色150/300像素、零光、结构扫动、原生 Bloom 和阴影回归通过。当前306项 LookDev资产及角色父材质（合计307项）执行前后 SHA-256 一致，无新增或删除；未保存测试材质、地图或配置，未提交或发布 Git/P4/UGS。改动前备份：开发副本 `Saved/RoughMetalDiffusion-20261011-223849`。

查看方式：渲染工作台 → 二分光影设置 → 角色受光的开关现名为“金属 GGX 对比增强（默认 Profile 0）”；角色金属美术参数中的“GGX 高光与对比”保留高光倍率、亮芯倍率和材质优先的宽度倍率。默认宽度1不重映射原始粗糙度；美术增益只增强真实镜面，不生成固定亮盘。

##### 真实环境反射与粗糙度高光宽度（2026-10-11，覆盖下文旧亮带说明）

- **人工环境灯带已移除**：旧路径以固定世界方向生成亮带，用实际 GI / 粗糙反射亮度照亮，却只保留 20% 原生反射；本轮删除生成函数、间接合成替代分支及亮带导数计算，不再用固定形状替代环境信息。环境镜面保留原生 Lumen / 反射捕获的实际辐射、F0、粗糙度与 BRDF，再应用既有反射强度及非金属/金属 alpha 0.3/0.7。已确认的直接块状高光、光强驱动亮芯、结构扫动与原生 Bloom 保留。
- **默认高光宽度随材质粗糙度变化**：有效默认宽度为 `clamp(全局或 Profile 宽度倍率 × lerp(0.25, 1.75, Roughness), 0.1, 3)`。相同倍率下，光滑金属更窄，粗糙金属更宽；块状高光与亮芯共同遵循该宽度。边缘柔和度继续读取 Roughness，并保留远景像素抗锯齿。材质显式 `MetalHighlightWidth > 0` 使用原有绝对宽度编码，优先于默认映射，不再额外乘粗糙度倍率。
- 渲染工作台移除“艺术环境亮带”整组及强度、宽度、方向控件。“默认高光宽度倍率 · 随粗糙度（材质可覆写）”仍可实时调整。旧 `EnvironmentStrength / EnvironmentWidth / BandDirection` 仅保留序列化加载兼容、不可编辑、不参与 GPU 上传；12×float4 ABI 保持不变、退役通道上传0，不重写材质或关卡。

验证：开发副本增量编译 `Saved/ShadowSplitBuild/20261011-215926-build.log` 成功。DX12 / SM6 报告 `Saved/CharacterShadowValidation/20261011-220101/Automation/index.json` 共11项通过、0失败、0未运行（3项含既有控制台优先级 / 资源读取警告），无 Shader 编译错误。金属捕获目录 `Saved/CharacterMetallic/06A0977245DCB7AA569D59B4F4D84DAA` 共46张含诊断图；相同150像素球体、相同真实主光、排除 GI / Film / Bloom 后，粗糙度0.05 / 0.5 / 0.95的高光覆盖分别13.34% / 38.84% / 61.02%，线性峰值约0.288 / 0.288 / 0.280，确认宽度变化而非曝光增亮。材质宽度/颜色/贴图覆写、金铜银黑 F0、零光、扫动、Bloom、角色和阴影回归通过；视觉风格仍需美术检查。LookDev 295项地图/资产及角色父材质 SHA-256 与执行前一致；未保存测试资产、关卡或配置，未提交 / 发布 UGS。源码备份：`Saved/NativeMetalEnvironment-20261011`。

##### 角色材质高光覆写（2026-10-11，优先于下文旧柔和度说明）

- `M_CharacterMaster` 的 **NPR Metal** 分组新增 `Override Metal Highlight Width` / `Metal Highlight Width`（0.1–3），以及 `Override Metal Highlight Color` / `Metal Highlight Color` / `Use Metal Highlight Color Texture` / `T_MetalHighlightColor`。两项覆写默认关闭；高光和亮芯颜色读取原生 `GBuffer.SpecularColor`（`ComputeF0` 的金属度工作流）并按原生 Schlick 菲涅耳计算，不再固定掺入 32% 白色。金属度1时 F0 来自 BaseColor，非金属镜面 F0 来自 Specular。覆写颜色/贴图表示线性 RGB F0、范围 0–1，之后仍参与菲涅耳及真实灯光计算；不重染金属本体或原生环境反射。
- 原有材质输出增加 `MetalHighlightWidth` 和 `MetalHighlightColor` 输入。宽度 0 继承全局/Profile；颜色负 RGB 继承金属色。可接标量、颜色参数或 RGB 贴图，不占用阴影偏移、GI 平滑法线和 Profile 数据。独立 `EdgeSoftness` 控件退役：边缘柔和度读取材质 Roughness，并保留像素抗锯齿下限。
- 高光和亮芯均乘实际灯光颜色、强度、衰减、可见性与投影，不引入固定自发光/曝光补偿。测试数值恢复为已确认的默认值：宽度倍率 1、块状倍率 2.4、亮芯倍率 3.5、白色色调、非金属/金属环境反射 alpha 0.3/0.7。环境亮带已退役；不改变关卡、贴图和各实例原有材质输入。
- DX12 延迟、非 Nanite 角色通过原生可选 GBufferF 材质数据 Pass 承载逐像素覆写。宽度 7-bit 编码，RGB 8-bit；材质静态关闭覆写时不请求该附加绘制。启用覆写会增加角色网格绘制/纹理采样成本，未完成目标机 GPU 性能验收。依赖平台各向异性材质辅助通道支持及 `r.AnisotropicMaterials=1`；Forward、移动、Nanite 或关闭辅助通道时仍用默认效果。本次未增加距离淡出，也未改动 VSM 远距保护或场景管线。

验证包含同一 150 像素金属样本的继承/宽/窄、颜色/四色贴图、平滑/粗糙、低光/零光真实视口读回。数值测试验证数据路由，不代替美术验收。资产补丁 `-run=StylizedCharacterMaterialSetup -PatchMetalOverrides -Apply` 仅增补上述父材质新输入，拒绝覆盖未知连接；不要使用该命令旧的全量 `-Apply` 路线重建既有角色材质。

本轮验证：源引擎及开发副本编译成功（最终增量 `20261011-202445`）；DX12 报告 `Saved/CharacterShadowValidation/20261011-202604/Automation/index.json` 共 11 项成功、0 失败（其中 3 项带既有资源读取 / 控制台优先级警告），无 Shader 编译错误。33 张金属捕获包含 150 像素样本及约 300 像素的真实 CellCaptain 材质实例对照；实际金属区域 815 个采样像素的 RGB8 平均差为 73.99，确认不是仅有参数写入、画面无变化。默认父材质不请求附加数据绘制；实际 CellCaptain 走非 Nanite 网格代理。LookDev 当前 295 项地图/资产与本轮执行前 SHA-256 一致；只增补角色父材质，未保存测试实例、地图或配置，未提交 / 发布 UGS。

金属颜色与角色滤镜修正后的验证（2026-10-11）：开发副本增量编译 `20261011-205924` 成功；金/铜/银/黑 F0 专项 DX12 测试 `20261011-210041` 成功。最终报告 `Saved/CharacterShadowValidation/20261011-210436/Automation/index.json` 共 11 项成功、0 失败、0 未运行（3 项带既有控制台优先级警告），无 Shader 编译错误。38 张金属捕获覆盖 150/300 像素、材质宽度/颜色/贴图覆写、光强、粗糙度、扫动、Bloom 和真实 CellCaptain；正常视图中金色块状高光仍保留，专项测试排除 Film/GI/反射后确认高光 RGB 比例跟随 F0，而非固定白色。原生菲涅耳掠射角及 Film 高亮压缩仍可能使颜色变浅，此次不修改曝光或色调映射。执行前后 LookDev 296 项地图/资产 SHA-256 一致，角色父材质 SHA-256 亦未变；未保存任何测试材质、地图或配置，未提交 / 发布 UGS。修改前源码备份在开发副本 `Saved/MetalF0RemoveKuwahara-20261011`。

查看方式：打开 CellCaptain 使用的材质实例，在 **NPR Metal** 中开启宽度覆写，对比 0.2 / 2.5；开启颜色覆写后可用色板，或再开启颜色贴图并指定 RGB 贴图。边缘随原有 Roughness 变化；调低 / 关闭 LookDev 主光可检查高光与亮芯同步减弱。测试后关闭两项覆写即可恢复默认继承，不需要改回整套材质。

##### 美术控制与场景反射扩展（2026-10-11，覆盖下文旧控件说明）

- 渲染工作台 → 二分光影设置 → **03 角色金属美术参数**：亮暗面强度、随粗糙度的默认块状高光宽度倍率/强度、亮芯强度、高光原生色板。修改实时上传；全局默认可被材质选用的 Lighting Profile 单独覆写，材质逐像素宽度/颜色优先级最高。只作用于模式 3 且启用 NPR 的角色金属，不修改场景 BRDF、材质资产或 Bloom 设置。
- **02 角色受光**以两个 0–1 Alpha 代替环境反射复选框，默认非金属 0.3 / 金属 0.7。实际系数为 `lerp(NonMetalAlpha, MetalAlpha, Metallic)`，乘在原生角色间接镜面上；直接高光与漫反射 GI 不受此 Alpha 影响。保存角色受光或保存金属与反射配置均保存 Alpha。
- **场景 Kuwahara 已移除（2026-10-11）**：现场实际 Mode 为0；开启8方向、强度1同机位检查后，整体画面改善仍不明显。此前专用场景金属捕获的4/8方向 RGB8 平均差约0.38/0.47，而强度0的时域基线差约0.44，未证明有足够可见收益。因此按用户要求移除4/8方向面板、强度、配置字段、CVar、Shader参数与25点邻域采样代码；普通场景继续使用原生材质粗糙度及 Lumen 反射。该滤镜原本只覆盖 DefaultLit 金属间接镜面，角色被排除、粗糙度≤0.04的镜面绕过，所以不能期待整幅场景产生明显绘画效果。
- 性能工作台的 **VSM 远镜头精度 / 方向光 Bias**是 float：降低可提高相同相机距离下的精度；既有角色远距保留仍开启。新增 **角色接收 / 自投影偏移 · cm**（默认0，范围-2到2）：沿角色法线移动 VSM 接收采样点，不移动投向地面的阴影，也不改变物体几何。正负方向可能导致漏光或阴影痤疮，应小幅调整。原生 VSM 接收外部投影与自投影共用遮罩，因此该滑条不是“只改自投影”开关。
- **角色 Kuwahara 也已移除（2026-10-11，金属颜色修正轮）**：删除 9 点稀疏四象限滤波、邻域兼容性 GBuffer 读取、粗糙/追踪反射额外采样、方差权重及邻域亮度重映射；不是仅关闭面板。Lumen 原生反射、降噪与 Roughness 混合、直接块状高光、结构扫动、原生 Bloom 和 0.3/0.7 受光 Alpha 保留；人工世界亮带在本轮后续也已退役。删除的是额外计算，没有目标机 GPU 计时或帧率收益保证。

场景滤镜移除后的验证：独立开发副本 `20261011-182639` 源引擎编译成功；DX12 自动化 `Saved/CharacterShadowValidation/20261011-182944/Automation/index.json` 共11项成功、0失败，含22张角色金属/NPR/美术参数捕获及阴影回归。4项测试带控制台优先级或既有资产读取警告，无 Shader 编译错误或断言失败。LookDev 插件45项地图/资产与执行前 SHA-256 一致、无新增或删除；未保存材质/地图，未提交或发布 UGS。源码移除前备份位于开发副本 `Saved/RemoveSceneKuwahara-20261011`。

新增字段复用 Profile 已退役颜色槽 0–3，保留 12×float4 ABI，旧颜色数据不复活；旧材质无需重写。保存只改相应配置键和每个 Profile 的新美术字段，保留其他字段、未知数据和重复 Profile 索引，不自动保存材质/地图。

验证记录：开发副本 `20261011-164146` 构建成功。DX12 报告 `Saved/CharacterShadowValidation/20261011-162638/Automation/index.json` 中金属/受光 Alpha/美术参数/VSM 等 9 项通过；剩余两项旧界面编号及退役槽位断言更新后，在 `20261011-165828-FinalTwo/Automation/index.json` 复测通过（0 失败）。金属测试包含 150/300 像素远景、4/8 方向切换、强度0、粗糙度0镜面绕过、美术参数极值及 Alpha 0/1 的真实 DX12 捕获；不代表目标平台 GPU 性能或所有材质美术效果已经验收。新增接收偏移已编译并纳入参数契约，非零偏移的场景视觉验收仍需人工小幅调整检查。

模式 2 的 GGX 倍增在实际远景角色上的可读性仍不足，因此增加 `r.StylizedCharacterLighting.MetalStyle 3`（默认）。模式 0/1/2 保留供回退与实时对照，2–3 的小数值可混合。只处理 `StylizedCharacterLit` 的金属部分，不改变普通场景 / 非金属、VSM、曝光或色调映射。

- 每个材质通过既有 `Lighting Profile` 选择是否启用 `bEnableNPRMetal`，使用 profile 表原空闲的 `11.w`，不扩展 GBuffer。该选择只读取材质自身 profile，不受摄像机的全局 profile override 偷换。当前 Default / Profile 0 默认开启以预览现有角色；新增 profile 默认关闭。关闭后模式 3 回退为模式 2。
- 直射金属使用独立的艺术色调、亮暗块面、宽高光和小型 HDR 核心，替换对应的 GGX 金属贡献。复用原生面积光的积分输入，保留灯光颜色、距离衰减、Rect 源纹理 / barn door、可见性、阴影和高光功能开关；不是按时间扫动或屏幕曝光补偿。
- 环境镜面直接使用原生 Lumen / 反射捕获，不再生成世界空间艺术色带。现有反射强度、alpha 及间接遮蔽仍生效；直接 NPR 高光的形状属于非能量守恒美术表达，不等同于物理测量金属。
- 不跨表面扩张金属遮罩、不重写原始贴图 / 资产、不强制全角色 LOD0。小于像素或被几何 LOD 删除的信息不能从屏幕缓冲恢复；现有 VSM 角色远距保护保持开启。
- 没有新增全屏 Pass；金属像素增加解析着色和面积光数据计算，沿用既有反射滤波。没有目标设备 GPU 成本保证，Steam Deck / Switch 需单独测量。

验证使用临时舞台，精确拟合 300 / 150 像素角色投影，并保存实际 Metallic 遮罩，仅在金属区域报告对比 / 高光覆盖 / 锐边等诊断。包含低光、主光转动、Bloom、物理零光（功能开关仍启用）、非金属与 profile opt-out 对照。数值门槛只验证路径和隔离性；实际角色的材质辨识、块面形状和运动稳定性需要目检，不以球体效果或整幅平均差替代验收。

本轮验证：C++ 增量构建 `Saved/ShadowSplitBuild/20261011-023133-status.json` 成功；DX12 / SM6 的 NPR-only 渲染测试 `Saved/CharacterShadowValidation/20261011-023312/Automation/index.json` 成功，18 个变体与 BaseColor / Metallic 诊断图存于 `Saved/CharacterMetallic/42015AA040035BD6B0E78FA5863449FF`。清理前序高亮帧的 Lumen 历史后，物理零光帧均值 / 峰值均为 0；灯光旋转使高光质心移动约 24.7 像素，原生 Bloom 产生可测外轮廓光晕。没有以人工增亮或放宽黑场断言通过测试。

工作台入口：渲染工作台 → 二分光影设置 → **02 角色受光开关** → **非 PBR 金属（默认 Profile 0）**。修改实时预览，显式保存才持久化；保存只修改已有第一项 Profile 的新开关，保留其他参数、未知字段和重复 Profile 的顺序。

控件 / GPU Profile 上传回归 `Saved/CharacterShadowValidation/20261011-024606/Automation/index.json` 两项成功。开发编辑器使用的 Engine 与项目 BuildId 均为 `c41828fc-2fa2-4d5e-b29e-253cf6e46406`。本轮未提交或发布 GitHub / P4 / UGS。

只读资产审计还发现：当前 `M_CharacterMaster` 的 BaseColor 已是 `T_Color + 0.2`（线性空间），Metallic 已接 `Round`；这与 2026-10-10 的直接纹理连接记录不同。源色贴图仍为暗皮革，额外的 `+0.2` 会在光照前抬灰暗部。本轮没有删除该已有资产修改，也没有保存任何材质 / 关卡。证据为 `Saved/Logs/NPR-SourceColorAudit-20261011.log`；材质最终视觉验收应考虑这个输入差异。

#### 俯视远景金属可读性（2026-10-11）

在上一版金属基础上新增默认俯视档；以 1080p 下角色约 150–300 像素高为视觉目标，不通过全局曝光或后处理对比度提亮角色。

- `r.StylizedCharacterLighting.MetalStyle` 现在支持 `0` 原生金属、`1` 上版近景风格、`2` 俯视块面（默认），可实时切换或使用中间值混合。没有增加独立全屏后处理或角色自发光。
- 俯视档采用较宽的 GGX 承载高光和有限宽度的块面边缘，在像素着色路径按屏幕导数做边缘抗锯齿；实际法线、视线和光源方向决定高光扫动，不使用时间动画。金属自身仍沿用真实光源积分、距离衰减、材质本色及阴影遮蔽。
- 提升高光块面与暗反射的对比，收紧金属接收阴影的过渡，但不改变 VSM 投影方向、贴图分配、原生漫反射 GI 或非金属 / 场景材质。亮度输出保持 HDR 线性值，由现有 UE Bloom 处理；最终显示仍为 SDR。Bloom 必须在当前视口和场景后处理中启用，不强制覆盖关卡的 Bloom 强度。
- 远景 Mip 会平均小金属遮罩，俯视档让风格化权重更早介入，但不改写原 Metallic/F0，也不跨皮革、深度或法线边界扩大金属区域。完全非金属像素不受影响。
- 金属着色没有按相机距离关闭的分支。既有 `r.Shadow.Virtual.Character.PreserveCasters=1` 继续默认保留 `StylizedCharacterLit` 非 Nanite 角色的远距离、小半径及粗页投影；不取消全场景剔除、不强制全角色 LOD0。视锥、遮挡、灯光范围、画质档以及低于一像素的信息量仍有各自边界，不能承诺无限距离维持相同细节。

验证增加俯视实际 CellCaptain、远景小金属球、主光旋转、原生 Bloom 开关和无光黑色检查；源码默认游戏镜头为 2300 cm / FOV 50，另保留 1150 cm 对照。测试只使用临时舞台，不保存共享资产。构建 `20261011-003742` 已通过，BuildId 为 `975fff0e-6a9f-439e-8f58-08ec649aa7a0`；真实 DX12 回归 `Saved/CharacterShadowValidation/20261011-004017/Automation/index.json` 共 11 项全部成功（3 项有既有警告），包含远距 VSM 缓存 / 非缓存投影回归，无新增引擎 Shader 编译错误。

30 张真实捕获位于开发副本 `Saved/CharacterMetallic/8F5962A641E5E489FF02ACA6859270B1`。23 米默认游戏距离的 CellCaptain 已按屏幕大小目检；其稀疏金属在主体 ROI 的模式 1/2 平均 RGB8 差为 1.149，近景为 1.239。小金属球模式差为 13.15，主光旋转 55 度后的差为 31.94；20000 lux、固定曝光下原生 Bloom 轮廓外正增量为 9.15；关闭光照和反射后金属内部均值为 0。非金属模式 0/2 差为 1.247（时序容差 2），关闭风格化的原生一致性差为 0.929（容差 3）。这些数据确认路径生效，不替代艺术效果、连续运动稳定性或其他 Blueprint 镜头的验收。未发布 GitHub / P4 / UGS，目标设备 GPU 成本尚未测量。

#### 角色风格化金属：反射块面与 HDR 高光（2026-10-10）

此节保留首版实施记录；其中 9 点 Kuwahara 和邻域亮度 Ramp 已在 2026-10-11 金属颜色修正轮删除，以前述当前实现为准。

本轮在上一次原生金属修复之上增加美术化处理，不再以“开启风格化时仍与 DefaultLit 完全相同”为目标。依据 [终末地风格化 PBR 分析](https://zhuanlan.zhihu.com/p/2012135042818778255)、[金属材质表达](https://zhuanlan.zhihu.com/p/634173928) 及用户提供的反射 Kuwahara / HDR 补偿截图，保留金属本色、提高可辨认的高光反光、整理反射细节。文章中的独立材质示例没有阴影采样，不能直接照搬到最终渲染器。

- 只作用于 `StylizedCharacterLit` 的 Metallic 区域；非金属、其他场景着色模型、漫反射二分、GI、VSM、曝光和色调映射不变。
- 在 Lumen 间接光合成内，对粗糙反射和追踪反射做共享区域权重的 9 点稀疏 Kuwahara。四象限采用相对方差软选择，拒绝跨深度、法线、金属度、粗糙度、底色和 Profile 的采样；保留两个独立输出以维持原生 AO / 粗糙度交叉混合。
- 反射使用保持色相的解析亮度 Ramp 与有限增益，补偿降频损失；对低反射率金属再作最多 2 倍的保色增益，不修改原 BaseColor 贴图或增加漫反射底色。直接金属高光适度收紧并补强 HDR 核心，仍乘真实灯光颜色、衰减和阴影可见度。零入射光不会产生额外自发光。HDR 指内部线性亮度，最终仍为 SDR。
- 默认启用。内部 A/B / 性能退路为 `r.StylizedCharacterLighting.MetalStyle 0`（上轮原生金属）或 `1`（本轮风格化），实时生效；复用原参数向量空位，不改变材质/GBuffer ABI，不增加一批工作台调试控件。
- 首版反射滤波覆盖常规 deferred GBuffer + Lumen ScreenProbeGather；SSR、Substrate 和特殊 irradiance-field 降采样组合不使用此滤波。没有新增全屏 Pass，但有效金属像素增加邻域纹理读取，不能当作零成本；尚未做 Steam Deck / Switch 的 GPU 性能验收。
- 不改写角色原有 Metallic / Roughness 贴图，不把皮革布料变成金属。旧 `ReflectionKuwaharaMode` / `Strength` 字段仍是已退役兼容数据，不是本轮开关。

真实 RHI 测试保留关闭风格化时的原生基线，并新增粗糙金属开关、非金属不变性、实际 CellCaptain 对照及直接 / 间接反射隔离捕获。源码构建 `20261010-234156` 成功；引擎、项目及插件模块的 BuildId 均为 `e77d0b1f-fb0e-42c0-b235-a259fb96b9bd`。最终 DX12 回归 `Saved/CharacterShadowValidation/20261010-235028/Automation/index.json` 的 11 项测试全部成功（其中 3 项带既有警告），失败 / 未运行均为 0。

20 张金属捕获位于开发副本 `Saved/CharacterMetallic/458CFF6845B216D264CDD6BEBBDE94F5`。固定机位主体区域的平均 RGB8 差：关闭风格化后的原生一致性为 0.965（阈值 3）；粗糙金属风格化开关为 14.68；非金属开关为 1.233（时序容差 2）；实际 CellCaptain 为 0.747（金属仅占局部，已目检头笼、扣件与饰边）；隔离直接高光 / 间接反射的开关差分别为 19.37 / 13.51。数值只证明路径生效，不等于审美验收。阴影及二分回归同时通过。项目既有 `M_SlashTrail_Base` 空纹理参数警告未在此轮处理；没有新增引擎 Shader 编译错误，没有保存测试资产或发布 GitHub / P4 / UGS，尚未完成目标机 GPU 性能测量。

#### 角色金属镜面与二分漫反射解耦（2026-10-10，后续修复）

上一轮只恢复了 BxDF 内部的金属能量分配，后续 `DeferredLightingCommon.ush` 仍让金属高光乘以角色二分亮面遮罩，并与漫反射共用灯光强度 / 颜色压缩。默认 `DirectLightIntensityInfluence=0.5` 会削弱金属的实际入射光能量；硬亮面遮罩还会截掉本来应该存在的掠射高光。这不是曝光或金属度贴图未连接的问题。

现将 **金属部分的直接镜面贡献** 按材质 Metallic 权重独立累加：保持原生 GGX、Roughness、真实灯光颜色 / 强度和原生投影可见度，不经过漫反射 Ramp / 亮面裁切 / 强度归一化。仍尊重高光启用状态、灯光 Specular Scale、角色 Profile 高光强度以及接收投影设置。非金属部分继续使用既有角色二分逻辑，不改场景其他着色模型，不更改 VSM，也不新增补光、曝光、绘制 Pass 或额外纹理采样。环境镜面颜色的旧去色控制仅作用于非金属部分，金属保留完整反射色。

只读审计实际 CellCaptain 的三个材质槽，父材质均为 `/YogArt_Material/MasterMaterial/Character/M_CharacterMaster`：BaseColor 接 `T_Color.RGB`；Metallic / Roughness / Specular 分别接 `T_MixMap.B/G/R`；MixMap 的 sRGB 均关闭。身体和配件中金属度不低于 0.8 的源贴图像素平均 Roughness 分别约 0.499、0.502，因此修复后仍是原贴图定义的偏哑光金属，不将布料或皮革强行变成金属。该统计是源贴图像素统计，不是屏幕面积统计；没有改写任何资产。

金属真实 RHI 测试现增加原生 DefaultLit 对照：使用瞬时材质实例只覆盖 Shading Model，保留同一实际角色的所有原材质输入；金属球则使用相同 Metallic=1 / Roughness=0.2 / Specular=0.5。对照期间仅在测试进程禁用场景二分，避免原生对照也被场景高光重塑，退出后恢复。修复前 `20261010-225103` 的金属球主体区域平均 RGB8 差为 8.70148。新版要求降至 3 以内（时序容差），并继续验证材质金属度、粗糙度、环境反射及 VSM。

验证完成：`Saved/ShadowSplitBuild/20261010-230109-status.json` 编译退出 0；`Saved/CharacterShadowValidation/20261010-230200/Automation/index.json` 的 11 项测试全部 Success（8 项无警告、3 项带既有警告，零失败）。金属球与原生差降为 **0.248005**；全金属 Specular=0/1 差为 0.2981，Roughness .2/.65 差 60.07，环境反射开关差 30.53。真实角色及球体 10 张截图位于 `Saved/CharacterMetallic/60E2151E40EF4E06BEE6C6B2AE2738A7`；测试将用于断言的实际视口读回直接保存为 PNG，避免全局截图请求被其他视口消费。VSM 的 18 张回归截图及二分光影回归全部完成。引擎、target、项目和 9 个插件的模块清单 BuildId 均为 `f28e819d-0cc9-42ae-aa92-d05c9ab34e47`。

验证边界：没有 Shader 管线编译失败；但项目原有特效材质 `M_SlashTrail_Base` 因三个纹理参数为 NULL 编译失败并回退默认材质，该问题在本次引擎修改前的基线日志也存在，未纳入本次修改。其他警告是既有控制台优先级 / 配置读取测试日志。没有保存地图或角色材质、没有发布 GitHub / P4 / UGS；仍需用户对开发编辑器中的最终材质观感验收，没有目标机 GPU 性能测量。

#### 当前确认效果：原生 VSM 与角色金属修复（2026-10-10）

本节优先于下方历史状态。按照用户最新截图，角色阴影拆分固定关闭，半程向量偏好保持开启但不参与渲染；这两项不再作为工作台或项目设置中的编辑开关。接收投影、Lumen 漫反射 GI、材质环境反射默认开启，额外细碎遮蔽关闭。独立细节自阴影的历史勾选保留但控件禁用；当前接收投影和自投影仍由原生 VSM 提供，不增加角色专用阴影图。旧配置即使保存拆分为 true，常规设置上传也不能重新开启实验路径；内部渲染回归测试仍可显式设置 CVar 验证旧实现。

环境反射是天空、反射捕获、SSR / Lumen 的间接镜面贡献，并不是曝光或额外补光。金属的漫反射接近零，依赖材质本色控制的镜面反射、粗糙度与环境内容体现质感，因此保留该项。实际灯光和反射内容、原材质贴图仍会影响最终外观。

角色 Shader 修复：`StylizedCharacterLitBxDF` 不再使用完整 BaseColor 作为金属漫反射，而采用原生 `DiffuseColor = BaseColor * (1 - Metallic)`；旧 MixMap / Specular 的高光压低和展宽仅作用于非金属部分，全金属保留原始 Roughness 与 GGX 响应。非金属的既有二分形状不变。历史上可选的暗部漫反射保底同样遵循金属能量分配，不重新启用该功能。不修改场景其他着色模型，不转换或保存材质资产。

暗部交界颜色退役：旧实现确实连接到渲染器，但只是很窄的直射漫反射乘色，不处理 GI、反射或完整遮挡区；对二分后已归零的暗面、纯金属漫反射几乎无可见效果。现移除工作台控件和 Shader 消费代码，所有旧 GPU 槽上传中性值；兼容序列化字段仍保留但不可编辑，旧 Blueprint API 标记弃用并无操作，避免旧资产加载断链。未增加替代补色或曝光处理。

本次在独立开发副本完成 C++ 编译，最新 `20261010-221201` 退出 0。引擎、项目 target 和 11 份模块清单 BuildId 一致，为 `f28e819d-0cc9-42ae-aa92-d05c9ab34e47`。最新 DX12 自动化 `Saved/CharacterShadowValidation/20261010-221258/Automation/index.json` 共 11 项全部成功、零失败，进程退出 0；4 项带控制台优先级、测试读取非文件路径 `Game` 或既有运行日志警告，没有 Shader 编译错误或断言失败。验证覆盖控件、兼容数据、金属度 / 粗糙度 / 环境反射与原生 VSM 回归，没有保存地图或材质，没有提交或发布 UGS。

`DevKitEditor.CharacterMaterial.MetallicRender` 本次逐槽读回真实 CellCaptain 身体、披风、配件三项材质，均为 `M_CharacterMaster` 父材质及 `StylizedCharacterLit`。使用同一瞬时角色 Shader 球对照：全金属 Specular 输入 0/1 的主体区域平均 RGB8 差为 0.349（容许时序误差 2）；粗糙度 .2/.65 差 47.47，金属 / 非金属差 90.10，环境反射开 / 关差 30.30，表明关键参数实际进入渲染输出。六张图片位于 `Saved/CharacterMetallic/9D55B78A40F9BDE2C640F591F3CF69A1`。另保留 18 张阴影回归与 3 张二分 / 退役颜色对照，实际 VSM 路径及远景阴影保留测试均成功。该结果不代表所有角色 Actor 的覆盖材质已审计，也不替代用户的最终视觉验收或目标机 GPU profiling。

#### 阴影拆分恢复制作与实时预览修复（2026-10-10，实现与验证）

本节更新优先于下方早先的延后记录。两篇文章已通过 Firecrawl 完整读取：文章一提供 RenderTarget / SceneCapture 原型及场景、角色自身遮挡的分离思路，不是可直接复制的完整 UE 渲染器补丁；文章二说明暗面、细节自阴影、场景物件投影的区别及半程向量方案。本次采用其拆分思路接入渲染器，不新增 SceneCapture Actor，也不保存地图或材质。

当前状态：源码已写入，C++ 编译成功。13:02 独立后台续编在 14:03:50 成功退出（2837 actions）；20:27:37 引擎修复增量编译在 20:28:13 成功退出（8 actions），最新 20:37:05 测试增量编译在 20:37:34 成功退出（4 actions），退出码均为 0，日志在开发副本 `Saved/ShadowSplitBuild/20261010-202737-*` 和 `20261010-203705-*`。最新引擎、项目 target、项目及九个插件模块清单的 BuildId 均为 `f28e819d-0cc9-42ae-aa92-d05c9ab34e47`，11 份模块清单无不一致项。此前发现 VSM 平台依赖未满足，早先截图不能用于验收；修正后独立 D3D12 运行已明确检查实际 `UseVirtualShadowMaps()` 为 true。最新验证 `Saved/CharacterShadowValidation/20261010-203809/Automation/index.json` 三项自动化全部通过、零失败（其中两项带预期优先级/旧运行警告），进程退出码 0。独立接收图、自阴影图已有非空 caster 并完成画面对照；GPU 成本、动画和模块化骨骼角色仍未验收。新增角色阴影拆分默认关闭并标为实验开关，原生 VSM 为正常和失败回退路径；尚未发布 UGS。

实际路径根因：UE 5.8 `UseVirtualShadowMaps()` 依赖 `DoesPlatformSupportNanite()`，而项目及四档设备预设均曾设置 `r.Nanite.ProjectEnabled=0`。这会让 `r.Shadow.Virtual.Enable=1` 仍回退传统阴影。现仅将项目级 shader / 平台能力打开为 1，保留 `r.Nanite=0`，不转换或启用 Nanite 几何资产。工作台诊断与运行测试改为检查实际 `UseVirtualShadowMaps()`，VSM 路径不可用时明确报错，不再仅凭开关为 1 判定生效。项目能力更改需重启并重编 shader；DX11/SM5 不能使用此 VSM 路径。Low 档继承 UE 的动态阴影关闭策略，属于明确的画质策略而非距离剔除，本次未扩大修改该策略。

- 场景与角色投向场景：继续使用真实灯光方向的 VSM，半程向量不影响地面投影。
- 场景投向角色：沿真实方向光建立角色接收图，排除该角色附件组。
- 角色细节自阴影：单独绘制角色附件组，可用真实灯光方向或 `normalize(L + V)`；通过接收者 stencil 限定到所属角色，再仅替换 `StylizedCharacterLit` 的方向光遮罩。
- 首版范围：SM5 桌面延迟渲染、非 Nanite 角色、方向光、附件光照组。方向光之外、Forward、Substrate、SceneCapture、立体视图，以及角色/有效遮挡物为 Nanite 或分配失败时保持原生 VSM。每张图上限 512×512；场景接收图沿光线向上搜索 200 m。尚未实现空间索引和数量预算，不能宣称已经适合大量敌人的正式运行。
- 功能只提供拆分、独立自阴影、半程向量开关；不添加阴影颜色补光，不重新启用旧 HZB 屏幕空间近似。

性能工作台源码修复：选中档位的数值控件即时写入 CVar 并请求视口重绘；其他列仍为预设草稿。选档、选预设和 Default 立即预览。写入优先级可覆盖项目 ini，但保留命令行/控制台的更高优先级，并回读实际值提示未应用项。诊断按钮不再限定 LookDevMain，可作用于当前关卡的透视编辑器视口；PIE 不由编辑器按钮控制。

新增角色投影保留开关和原生小投影剔除阈值入口，位于性能工作台的“VSM / 角色投影保留与小物件剔除”。角色识别来自实际 `StylizedCharacterLit` 材质，不依赖资产名；绕过非 Nanite VSM 的远级联距离、小屏幕半径和粗页细节剔除，不取消正常视锥、灯光范围或模型 LOD 选择，不会同时绘制多个 LOD。UE 原生 `r.Shadow.Virtual.UseFarShadowCulling` 构造默认值为 1，本次只豁免受保护角色，不全局关闭场景距离筛选。另补齐方向光早期 `FilterPrimitiveForShadows` 的 CPU 半径剔除豁免，保留原生主视图尺寸筛选；此前只改后段 AddSubject/GPU 会导致局部灯有效而方向光仍消失。

距离保留验收：最新同机位 A/B 分别测试 `Cache=0` 和 `Cache=1`，保持 `ForceInvalidateDirectional=0`、原生小物件半径阈值 0.1、FarShadow 标志关闭，明确锁定 `LookDev Candidate Key (lux)` 方向灯；仅在未保存的测试舞台关闭 Fill 投影、DistanceFieldShadowing 与 ContactShadows，结束恢复 CVar，不改变生产配置。VSM 遮罩读回蓝色阴影像素：无缓存保留关闭 108、开启 1524；开启缓存后热切换为关闭 113、开启 1525。开启图可见正常角色地面投影，无需移动角色或重开关卡；不再只靠截图存在判通过。18 张截图位于开发副本 `Saved/CharacterShadowSplit/6FA8912648D6990AD07271BC1EDC42F0`。这证明该静态角色与方向灯 fixture 的距离/小投射物豁免及缓存开启时的热切换，不代表无限距离、所有平台或所有材质的保证。

自动化：`DevKit.Workflow.GraphicsQualityCloudContract`（实际 spinbox 回调、ini 优先级、非选中列、Clipmap 端点修复与恢复）；`DevKit.Workflow.CloudLightingFinish.CharacterShadowEnvironmentControls`（七个实时开关且无数字调试，同时回读新增拆分 CVar）；`DevKitEditor.CharacterShadowSplit.RenderSmoke`（需要独立真实 RHI 编辑器和 `-Unattended -Dev01CharacterShadowSplitRender`）。后者生成原生 VSM、真实方向拆分、半程向量、关闭自阴影、远景、场景遮挡物、两个角色、同机位角色保留关闭/开启（含缓存热切换），以及 VSM 遮罩 / LumenScene 诊断的十八张截图，并检查实际 VSM 平台路径。截图存在仍不替代视觉正确性与 GPU profiling。

续编中的代码审查补充：独立 per-object 阴影原生写 BA 通道，合成显式映射到方向光 RG；多视口保留原遮罩；独立图不走 Capsule 路由；禁用 stencil 时回退原生 VSM；模块化角色先归一化到附件光照组根节点，避免重复建图。受保护角色的 VSM 粗页缓存失效同步绕过小物件限制，并为其触及的动态页使用一次性失效标记，不全局关闭粗页缓存优化。另修正了“图已分配但没有 caster”的问题：实际 VSM 下，原生传统 ShadowDepth mesh processor 会过滤 GPUScene 网格，CSM 缓存命令也可能不存在；独立小图明确允许所有 mesh 类型并自行构建命令，不改场景 VSM 的 mesh 选择策略。真实运行已确认 environment / self 图均 `allocated=1 hasSubjects=1`，开关自阴影有画面差异，场景遮挡物仍能投到角色，半程向量不改变地面 VSM 方向。

没有进行 Git 提交、P4 提交、云端构建或 UGS 发布。主工作区资产保持不变；编译项目为 `X:\Dev-ProjectBuilds\Dev01-Workflow`，可编辑引擎为 `X:\Dev-BuildEngine`。

构建续接注意：诊断请求使用单调递增 token，渲染线程只读取，不写回 CVar。独立 self 图先加入 caster 后登记 receiver，避免原生 preshadow 规则将自身 caster 排除。两项修复已进入成功构建及真实 VSM 运行；后续仍需 GPU 预算、骨骼/模块化角色和动画验证，不将默认关闭的实验入口当作正式完成的效果。

#### 早先记录（历史状态，以上节为准）

`YogTool → 性能工作台 → 画质分级` 中的原生参数表保留 Low / Medium / High / Epic、Default、Apply 与本机预设，阴影方法固定 VSM，不提供传统 SM 切换。参数下方回读实际 CVar；未注册或被高优先级覆盖会在 Apply 结果中报告。旧十行预设保留模型/材质调节，但不重新启用其中已经退役的阴影数值；新增参数从合并 Scalability 默认值补齐。

- VSM：方向光、移动方向光、局部光与移动局部光分辨率 Bias；最近/最远 Clipmap 层级；物理页容量；方向光/局部光柔影射线及每射线采样数。负 Bias 在同一相机距离提高分辨率，页容量只防止页池溢出；Source Angle / Radius 才控制光源尺寸造成的柔影宽度。Clipmap 最远层级必须不小于最近层级。
- Lumen：探针预算、屏幕探针间距、每探针射线分辨率、反射降采样。参数仅在对应 GI/反射路径开启时生效，关卡后处理质量还会参与最终分辨率计算；不重写 Lumen GI 算法。
- 诊断：在当前 LookDev 视口切换 VSM 遮罩、层级/Mip、缓存以及 Lumen 总览、Scene、Surface Cache、光照复杂度；恢复 Lit 返回正常显示。诊断不保存资产或修改曝光。

`渲染工作台 → 灯光 → 二分光影设置` 的角色组精简为四项实时开关：原生 VSM 接收投影、Lumen 漫反射 GI、材质环境反射、可选细碎遮蔽。只对 `StylizedCharacterLit` 生效；原生 VSM 自投影和接收其他物件投影当前仍共用遮罩，未完成独立投影拆分。移除了工作台中的角色 Profile 倍率、颜色补光数值和高光/质感/阴影试调组。保存开关只写这四个全局键，不再重写整张 Profile 表。

已撤回 GI 色相保底补光改动，并默认关闭旧补光、角色细碎遮蔽和附加 HZB 屏幕空间自阴影。原生 Lumen / VSM / 材质反射保持开启。关闭角色遮蔽并不取消整个场景的 AO pass，不能据此宣称 GPU 性能提升；需要实际 profiling。旧屏幕空间追踪不是截图中的角色专用半程向量阴影图，不将其改名为已实现的方案。专用阴影图与 VSM 的集成、角色材质高光/反射参数接口仍待实现与视觉验收；不保存地图或材质资产。

#### 延后制作记录：投影拆分与角色自阴影（2026-10-10）

状态：用户明确要求延后。当前只交付上面的控件清理和补色撤回，不增加新阴影图、不把未实现的效果显示为可用开关；没有进行云端发布。

已确认的制作范围：

- 场景继续使用 VSM，不恢复全局传统 Shadow Maps。用户已接受为角色增加独立的小型阴影图及其额外绘制成本。
- 区分角色投向场景、场景投向角色、角色自身投影；不能只在已经合并的 VSM 遮罩上叠加颜色或屏幕空间遮挡来冒充拆分。
- 角色独立自阴影采用参考图中的半程光照/视线方向，另建投影矩阵与阴影图；仅投射到所属角色，不能让其随视角变化的投影落到场景上。正常方向的场景投影仍保留。
- 工作台只提供功能开关，固定的算法参数留在实现内部。角色高光倍率、环境反射倍率移到角色 Shader / 材质实例参数；这部分也尚未实现。
- 默认不启用角色细碎遮蔽等小收益附加效果。是否节省 GPU 成本要用 profiling 验证，不用开关名称推断。

待继续的依据与验收：

- 阴影拆分参考：[文章一](https://zhuanlan.zhihu.com/p/2008887309844640748)；自阴影参考：[文章二](https://zhuanlan.zhihu.com/p/20592939601)。本次普通读取返回 403，Chrome 插件返回 `nodeRepl.fetch request failed`，内置浏览器读取超时；尚未核对完整正文代码。继续时先重连浏览器或读取用户提供的 PDF / 正文。
- 当前 `SM_CellCaptain_01` 的三个材质槽继承 `/YogArt_Material/MasterMaterial/Character/M_CharacterMaster`，实际着色模型为 `StylizedCharacterLit`，已查槽位 `LightingProfile=0`；不是 Unlit 导致这些控件无效。
- LookDev 要分别验证三种投影和开关的关闭回退，覆盖角色转身、移动相机、遮挡物、多个角色及主光/局部光；检查不会跨角色接收错误自阴影，核对正常 VSM 投影不随相机方向改变。
- 验证独立图分辨率/数量预算、非 Nanite 与骨骼网格、屏幕外遮挡、D3D12 Shader 编译和 GPU 时间。文章中的具体适用范围尚待核对，不把这段记录当成已经完成的管线设计。

本次清理验证：源引擎 + 独立开发副本 `DevKitEditor Win64 Development` 编译成功；D3D12 下 `DevKit.Workflow.CloudLightingFinish` 与 `CelesLight.LumenLayers` 共 9 项测试通过、0 失败（1 项有旧 Kuwahara CVar 查询警告）。报告位于开发副本 `Saved/Automation/CharacterShadowCleanup-20261010/index.json`。启动还发现双手剑材质缺贴图输入、斩击拖尾材质空贴图警告；后者在修改前预览日志中也存在，未在本次清理中修改这些资产。测试通过不是新投影方案或美术效果的验收。

1. 点击“只读预检”：核对 P4 opened / pending / resolve、项目和引擎 head、固定 Engine CL、PCB、BuildId。
2. 在 `X:/Dev-BuildEngine` 中开发引擎，关闭 UE 后用“编译开发副本”编译设置中的独立项目目录。不会覆盖主工作区的 UGS 预编译模块。
3. 用“打开开发副本”确认材质、Shader、场景和模块加载。验收是人工步骤，工作台不会从“编译成功”推断效果合格。
4. 项目代码变更走独立项目 CL；不发布引擎。引擎变更先运行流水线自检，再生成 Installed Build。
5. Installed Build 启动器返回只是 `started`。它输出的 OutLog / ExitFile 才是后台构建结果；退出码 0 后继续引擎发布。
6. 引擎发布交接：核对 `X:/Dev-EnginePublishStage/UE_5.8_Dev01/Engine` 的 source-free 内容，提交 Engine CL，验证 BuildId，再更新 stream 的显式 `@EngineCL` 和 release marker。
7. 使用已配置 SSH 公钥的 Host，点击“读取云端状态”。云端发布统一走 `C:/BuildAgent/Dev01/dev01_auto_ugs_build.ps1`。点击“云端构建并发布 PCB”是明确的发布动作，会再次执行现场预检。
8. 核对 completed_build.json 的 ProjectCL / CodeCL / EngineCL / BuildId / 输出哈希，校验 PCB，再用 UGS 同步和启动验证。

源码构建与源码编辑器启动使用现有 `Global\Dev01BinaryReleasePipeline` 互斥锁，避免在 Installed Build 后台仍运行时修改/读取构建产物。工作台同一时间只执行一个操作；构建前也会检查普通编辑器和命令行编辑器进程。若预检发现待 resolve 文件，发布被阻止。

**旧引擎发布入口的适用边界：** `X:/Dev-BuildPipeline/Dev01/start_dev01_engine_publish.ps1` 对应的发布器依赖旧 `Prepared PCB`，`anchor_dev01_engine_release.ps1` 使用 `.json` marker，而当前项目现场使用 `.txt` marker。不能把旧入口串接到云端生产流程并称为“一键发布”。工作台将此阶段保留为明确的人工交接，须在下一次引擎更新任务中统一该契约后再开放自动执行。

发布脚本、实际计划任务和云端状态以 `Docs/Dev01_Cloud_Build_Automation_Runbook.md` 的最新维护记录和实时读取为准。仅配置 SSH Host，密码和票据不写进工作流配置。

## 数据整理与恢复

本次建立统一分类，不移动 `.uasset` / `.umap`，因此不会破坏软引用或地图依赖。模型、材质、贴图、贴花、角色、战斗、剧情的编辑入口已经归组；已有业务记录仍由原工具维护。

工作台中的“整理资产：建立本地分类集合”按 Asset Registry 读取 `/Game` 和项目插件，创建 `Dev01WF_Materials`、`Textures`、`Meshes`、`Effects`、`Levels`、`GameplayData`、`Other`、`Redirectors_Review` 本地集合。集合只保存资产引用，重复运行补充成员，不删除其他集合或移动资产。Redirector 只进入审查集合，不能仅凭该集合直接删除。

可自动整理的数据严格限定为 `Saved/Logs`、`Saved/Codex`、`Saved/Automation` 中的文本生成物：

- 扫描 SHA-256，保留同组最新副本。
- 只选取七天前的重复文件，跳过 latest/current、票据、凭据、锁文件、备份、恢复记录、链接和 junction。
- 归档前再次核对源文件和保留副本的哈希。
- 每次归档先写 manifest 再移动文件；保留原相对路径。
- “恢复归档”按 manifest 恢复，并拒绝覆盖现有文件。
- 归档仍占空间；本阶段整理目录，不永久删除数据。

Content、Plugins、Engine、Saved/Autosaves、SaveGames、Screenshots、正式构建输出、发布包、Docs/GeneratedReports 都保留。需要清理正式资产时，应另走 Asset Registry 依赖检查与编辑器迁移流程。

命令行只读入口：

```powershell
python BuildScripts/Workflow/workflow_core.py --project . inventory
python BuildScripts/Workflow/workflow_core.py --project . preflight
python BuildScripts/Workflow/workflow_core.py --project . buildids
python BuildScripts/Workflow/workflow_core.py --project . cleanup-preview
```

确认候选后，`archive-duplicates` 会重新扫描并归档符合规则的全部候选；`restore --manifest <manifest.json>` 恢复对应归档。GUI 可选择部分候选。

## 维护约定

新增工具先归属现有工作流；增加目录项、输入/产出和 widget factory，避免继续堆积工具栏按钮。业务数据模型和发布契约分别维护，工作流面板只负责组织、导航和调用已有能力。

本次整理不自动提交 P4/Git、不更新 stream、不上传引擎或 PCB。普通预检可以在有未提交工作的环境中运行；发布阶段必须先明确 opened/pending 的归属。

本机可直接用已打包的 `BuildScripts/Workflow/dist/Dev01Workflow.exe`。该 EXE 与 `build/`、`__pycache__/` 属于本机生成物，已加入忽略规则；并不自动随 P4 代码发布。其他机器可用 Python 3.13+ 运行入口，或在单独批准的工具发布任务中分发 EXE。重新打包执行 `BuildScripts/Workflow/build_exe.ps1`（需要 PyInstaller）。

回归验证：`python -m unittest -v test_workflow test_workflow_ui`（在 `BuildScripts/Workflow` 内运行）；UE 自动化过滤器为 `DevKit.Workflow` 和 `DevKitEditor.Toolbar`。

## 上传前本地验证记录（2026-09-20）

- 源码引擎下的独立副本编译通过；UI 增量构建使用 `-NoEngineChanges`，禁止覆盖现有引擎二进制。
- 工作流/安全/UI Python 测试 19 项通过，含“BuildId 相同但 DLL 缺失仍阻止启动”。
- UE 测试 4 项通过：目录、工具栏/旧入口、原生页面、原生面板实际调用主工作区模块检查。
- 原生页面 PNG 是 Slate 组件实际渲染，不是网页概念图：开发副本 `Saved/WorkflowHub/Reports/NativeWorkbench.png` 与 `NativeOperations.png`。组件验收在正常 D3D12 模式运行；本机 `-RenderOffscreen` 会触发既有 Wintab/NullApplication 初始化崩溃，因此未采用该启动模式，也未修改引擎输入/渲染实现。
- UGS 主工作区恢复后，Installed Build + 项目完成启动，工具栏测试 1 项通过：主工作区 `Saved/WorkflowHub/Reports/UGS-Restored.log`。没有执行新同事 workspace 冷下载验收。
- 开发副本测试记录：`Saved/WorkflowHub/Reports/NativeTests-Final/index.json`；主工作区保留 `NativeBuild-Final.log`。
- 以上是上传前的本地验收快照，当时团队 PCB #27 尚不包含工作台。正式发布状态须另外核对项目 CL、云端 `completed_build.json` 和匹配 PCB revision，不能从本地测试结果推断。
