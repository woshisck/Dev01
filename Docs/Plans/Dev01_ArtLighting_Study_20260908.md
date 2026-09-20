# Dev01：策划、美术管理入口与灯光实机试验

日期：2026-09-08。状态：第一轮本地制作试验，未发布；不是最终美术验收。

后续修正：22:58 打开 RVT 测试图发生 World Memory Leaks，根因为本任务早先后台加载该 World 后留下控制台变量 p，不是灯光参数。已加固检查/切图入口并补充专项回归；详见[崩溃复盘](X:/Project/Dev01/Docs/Plans/Dev01_WorldLeak_20260908.md)。下面原有切图验证只覆盖原走廊与 Study，不能代表 RVT 路径当时已通过。

## 本轮交付

- 动作管理：新增[动作工作台方案](X:/Project/Dev01/Docs/Plans/Dev01_ActionWorkbench_DesignerWorkflow.md)，仅设计，未实现或修改冲刺。
- 贴花：新增[美术操作核查](X:/Project/Dev01/Docs/Plans/Dev01_DecalCollection_ArtistAudit_20260908.md)，说明删除入口与工具缺陷；没有替用户删除图一的 Poison。
- 灯光：真正调整并保存了 3 个独立测试地图，做过保存、返回原图、重新打开测试图及实际视口截图。
- 运行引擎是 UGS Installed Editor，不是 X:/Dev-BuildEngine 源码引擎。本轮没有修改/编译 Shader、引擎或角色材质，没有创建 CL、提交 P4/GitHub 或发布 PCB。

## 1. 现在如何打开试验

测试地图：

[L_Corridor_PhysicalStudy.umap](X:/Project/YogProject/Dev01/Content/Developers/g/LightingStudy_20260908/L_Corridor_PhysicalStudy.umap)

UE 资产路径：`/Game/Developers/g/LightingStudy_20260908/L_Corridor_PhysicalStudy`。

本轮结束时编辑器停留在这张测试图。以后重新启动编辑器，要重现完整试验，请在底部 **Cmd** 输入一次：

```text
py "X:/Project/Dev01/Tools/Lighting/open_dev01_lighting_study.py"
```

入口会先检查其他地图/资产是否有未保存工作，再打开已保存的测试图并安装临时外观预览。不自动保存当前用户工作，不重跑灯光调参，不覆盖你的后续灯光修改。

仅双击地图可以加载已保存的灯光和曝光，但不会安装本轮临时的场景分段/角色 Profile 对照，因此不保证与截图完全相同。这是研究版辅助入口，尚未做成 YogTool 美术按钮。

返回原图可正常打开原地图，或在 Cmd 执行：

```text
py "X:/Project/Dev01/Tools/Lighting/dev01_return_original_scene.py"
```

返回原图后，临时预览自动恢复。已验证角色 Profile、相关 CVar 数值均恢复，场景开关恢复到 `ProjectSetting` 优先级，不残留本次 Console 强制覆盖。每次重新进入测试图重新捕获原配置，不用旧快照覆盖你在原图的新设置。预览生效期间不要在项目设置里保存全局风格化 Profile；这轮外观参数由临时预览管理。

## 2. 本轮真实画面

下列图片由 Unreal 当前视口原生捕获，不是 AI 效果图。场景前后相机位置、方向及 FOV 一致；动态传送门粒子并非同一时刻，测试版另有一名静态角色灯光替身。

### 原场景（重新打开原图后确认恢复）

![原场景](X:/Project/YogProject/Dev01/Saved/LightingStudy/20260908/08_original_scene_verified.png)

### 第一轮物理曝光 / 自然色温试验

![场景试验](X:/Project/YogProject/Dev01/Saved/LightingStudy/20260908/06_final_scene.png)

### 角色近景

[补光前](X:/Project/YogProject/Dev01/Saved/LightingStudy/20260908/03_character_before.png) · [最终 300 lm 可选角色补光](X:/Project/YogProject/Dev01/Saved/LightingStudy/20260908/07_character_final.png)

角色对照基于同一测试光照与固定近景相机，不是把用户图二和另一个机位混作严格 A/B。这里放置的是 Miyabi_LTS42 静态姿势灯光替身，未替换玩家蓝图，未验证玩家移动时的最终效果。

## 3. 具体改了哪些灯光

目标是借鉴参考图的“低饱和冷环境 + 局部暖光源 + 深暗空间”，减少图五的大面积青蓝染色。不声称复现了暗黑破坏神 4 的真实内部渲染参数。

| 制作项 | 本轮测试值 / 做法 | 你在 UE 里调整哪里 |
| --- | --- | --- |
| 曝光基准 | Manual；启用物理相机曝光；ISO 1600、f/2.8、1/30 秒；EV100 约 3.88；曝光补偿 0 | Light_Study 子关卡的 PPV_Exposure → Exposure / Camera |
| 原有点光 | 明确使用 cd，平方反比衰减；原 cd 数值多数乘 16 是配合新曝光的艺术增亮，不是单位换算 | 点光 Details → Light / Intensity Units / Temperature |
| 暖源 | 约 2400 K；保留原灯位置，减小过强染色 | 蜡烛/壁龛附近 PointLight |
| 冷侧光 | 新建 Study_Aperture_01/02/03，6200 K；1200 / 1800 / 1200 lm；光源 300×180 cm | 对应 RectLight 的流明、宽高、位置和角度 |
| 端部光 | 原 RectLight 改为 300 cd，光源 180×450 cm；原 PointLight6 为 320 cd | 端部 RectLight / PointLight6 |
| 天光 | 白色、强度倍率 4；它是相对倍率，未校准为实测天空亮度 | Nature_SkyLight；不要把倍率当成 lux |
| 旧 Celes | 仅关闭 DataBake_Study 副本中 3 盏旧蓝图的 EnableOriginLight；未删除 Actor，未重烘共享数据纹理 | 原生灯作为本轮解析灯权威；不要重新开旧灯叠加 |
| 场景风格化 | 临时关闭普通场景的光照分段；保留角色的独立二分功能 | 由试验入口托管，不改项目 Default 配置 |
| 后期 | 对比度 1.20→1.05，饱和度 0.85→0.90；Bloom 0.25、暗角 0.25 | PPV_Dark&ColorTweak / PPV_Exposure |
| 场景描边后期 | 仅测试副本 PPV_PostMaterial 的混合权重 0；共享材质不改 | 测试 PPV_PostMaterial → Blend Weight |

场景灯全部作为 Movable / Lumen 动态试验使用，没有烘焙新的 Lightmass 数据。没有 GPU 耗时验收，不能直接把这套额外矩形灯作为正式低配置预算。

制作调节顺序建议固定为：**先固定曝光 → 调主光方向及覆盖 → 调真实光源能量与尺寸 → 调有限间接光 → 最后调后期**。不要同时用相机曝光、Celes 无单位强度、场景分段和材质乘数四处补偿同一个明暗问题。

## 4. 角色为什么黑，以及这轮怎么处理

当前 Installed Shader 的直接漫反射有明确二分：暗面会被置零。因此单纯增加主光强度，常常只让亮面更亮。物理曝光又会一起压低当前以固定辐射量定义的最低间接明度，这使原来适合非物理曝光的 0.20 底亮在新曝光下显得不足。

先做了不破坏二分开关的 Profile A/B：最低间接漫反射 0.20→0.35，底亮过渡 0.10→0.15，间接遮蔽强度 0.35→0.20，间接染色影响 1.0→0.65。单靠这组参数，实机改善有限；没有把它当成最终修复。

随后增加 **Study_CharacterFill_Optional**：300 lm，180×180 cm Rect，Channel 1，只影响接收 Channel 1 的测试角色；不产生间接光、雾散射或额外投影。500 lm 候选偏平，最后退到 300 lm。

它使用当前框架的 **Wash 美术补光**，不是严格物理灯：角色分支不走正常场景阴影/高光。它是可关闭的可读性对照，未绑定玩家跟随，不能承诺玩家走到任何暗处都已解决。没有给角色添加统一自发光，也没有关闭 Half-Lambert / 场景阴影二分。

推荐最终由程序做成一个美术可控的“角色可读性”面板：

| 你需要管理的概念 | 建议显示给美术的控制项 | 程序负责隐藏的部分 |
| --- | --- | --- |
| 二分风格 | 保留二分、分界位置、暗面比例 | Shader 功能位和打包兼容 |
| 暗处可读性 | 暗面目标明度、最大补偿、环境遮蔽影响 | 随曝光规范化，避免换场景失效 |
| 材质层次 | 皮肤 / 布料 / 金属可读性预设 | 金属与非金属不同能量处理；不能用 BaseColor 给金属假漫反射 |
| 环境融合 | 环境染色比例、角色补光强度、空间渐变 | Lighting Channel、配置 Profile ID、运行时过渡 |

现有美术入口已是 **Project Settings → Plugins → 风格化灯光**，分类为“01 基础二分”“03 多光源”“05 GI/Lumen”“06 颜色与明度”；不是要求你手动编辑隐藏 Profile 数组。但“曝光稳定的暗部保障”还不是当前版本完整实现的能力，不能只改面板名称就算完成。

特别注意：当前正在运行的 Installed Shader 底亮还使用 BaseColor；源码引擎中的金属修正并未全部进入此 Installed Build。后续框架修改必须在 X:/Dev-BuildEngine 本地编译验效，再走正式引擎与 PCB 发布链路。

## 5. 为什么还不像参考图四、六、七

本轮已收敛大面积青蓝和场景分段，但这只完成灯光基准第一版。剩余差异从实际画面可见：

- 地毯表面偏平，石材的湿润区、粗糙度变化及反射层次不足；不能仅靠灯光造出参考里的材质。
- 角色仍为现有动漫二分材质、原贴图和造型。参考图四的布料织纹、皮革/金属差异、头发层次，需要对应材质与资产制作，不是一个曝光数值。
- 当前弱补光仍有平涂倾向，黑衣细节也没有达到参考质量；需要曝光稳定且区分材质的暗部框架，再验证玩家旋转、移动与不同空间。
- 灯光尚未做 PIE 动态玩家、各画质、帧率和同事端验收；当前静态图不代替这些测试。

建议下一轮按材质类别做独立样板：干石 / 湿石 / 布料 / 皮革 / 金属，固定同一物理曝光再评审。湿润变化可结合 RVT 地表分区；动态 Poison 仍按 Deferred 管理，不应为了统一入口强制把所有效果写进 RVT。

## 6. 隔离、恢复与文件边界

测试主关卡只把 Light 和 DataBake 换成测试副本；Art、Gameplay、PLA 仍引用原子关卡，因此请只编辑带 Study 后缀的灯光内容，不在此图顺手移动共享场景物件。

原地图及子关卡 6 个 umap、已有 Light_BuiltData 共 7 个文件有本轮开始时备份，并已 SHA256 比对未变。原有 P4 opened 仍只有用户的 Light_BuiltData 编辑项，未创建 pending CL，没有待 resolve。本轮未保存它。

本地证据目录：[LightingStudy/20260908](X:/Project/YogProject/Dev01/Saved/LightingStudy/20260908)。重要证据：baseline.json、details.json、isolation.json、physical_settings.json、save_result.json、verify_original.json、verify_study.json；OriginalFiles 保存原文件副本。不要用早期 01/02 截图做最终 A/B，它们处于相机/曝光调整中。

辅助脚本位于 [Tools/Lighting](X:/Project/Dev01/Tools/Lighting)，当前被项目既有 /Tools/* Git 忽略规则排除，只是本地研究工具；本轮没有擅自修改忽略规则或强制提交。正式协作发布需另行确定工具入库白名单，并把研究地图放入受控 P4 测试路径。现在同事不会自动收到这套试验。

脚本通过 Python 语法检查，主要编辑/保存/复开流程已在当前 Installed Editor 实际执行。本轮没有 C++ 或 Shader 源码变更，因此没有执行源码引擎编译，也没有冒充完成游戏内和发行验收。
