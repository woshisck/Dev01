# Character LookDev UGS 接入记录

## 当前交付状态

2026-10-04 最新设计：按用户要求，以可保存的 Review / LightDev 子关卡工作台取代临时角色舞台。范围为 `Content/Art/Map/LookDev/LookDevMain`、四个空 Review、九个物理单位灯光子关卡及两段式 UI；现有 Gym 仅独立打开。用户先选择本机验收，随后明确要求“上传”：本次发布白名单为 CL148 的32个文件，只提交该 CL 并单次触发云端编译、PCB 发布及既有飞书完成通知，不更新 Engine 或 GitHub。上传前状态仍为 pending，云端/PCB 成功必须另以完成凭据和 archive verify 确认。保留用户验收时在 LookDevMain 中保存的 Plane；未修改引擎、SP、角色材质或既有生产关卡。

本机验证：canonical Installed Build 隔离工程编译成功；十四个地图已通过真实 UE 保存生成。新测试 `DevKitEditor.LookDevLevels.*` 三项及 `DevKitEditor.LookDevWorkbench.*` 两项全部通过，包含36组互斥切换、编辑目标保持、隐藏未保存内容保持、dirty状态保持、重复初始化文件哈希不变及原生 Slate 900×900 / 640×900 截图。最终报告位于 `X:/Dev-ProjectBuilds/Dev01-LookDev-20261002/Saved/Automation/LookDevPersistent-Final-20261004/index.json`，截图位于其 `Saved/Automation/LookDevWorkbench`。独立首次创建测试在 `X:/Dev-ProjectBuilds/Dev01-LookDev-Bootstrap-20261004` 通过 `-Apply -VerifyOpen`，确认同进程创建十四地图并立即打开 Main，默认通用 Review + Neutral。`DevKit.LookDev.Open` 已注册，可通过代码打开同一窗口；本机交互验收使用隔离工程，Content junction 指向主项目，保存 Review 内容会保存到主项目，Binaries / Intermediate 仍独立。

测试启动注意：此 Installed Build 使用 `-RenderOffscreen` 时，StylusInputWintab 在启动阶段触发 FNullApplication 析构访问冲突；去掉该启动模式后正常原生 RHI 测试成功。该冲突不是工作台测试失败，未修改引擎或禁用插件。隔离工程模块 BuildId 为 `59f42fc9-c192-48bc-a496-34c5b5f7923e`；不得将其 DLL 复制到使用正式 BuildId `792eec68-5f82-4e2f-852a-66474164b377` 的 UGS 主工作区。

上传前复核（2026-10-05）：隔离工程重新编译退出0、Target up to date；按用户已保存的 Main 再跑五项测试全部成功，0 failed / notRun / warnings，十四个地图运行前后 SHA-256 无变化。报告 `X:/Dev-ProjectBuilds/Dev01-LookDev-20261002/Saved/Automation/LookDevPersistent-Upload-20261004/index.json`。CL148 为14源码、14地图、3文档、1 Catalog，无待 resolve；default 中无关 Corridor Light BuiltData 保持 opened、不提交。Git 工作树仍有独立未提交改动，不参与本次发布。

上一版临时舞台源码已发布于 CL146，云端正式 PCB 为 `++Dev01+main-Editor.zip#31` / P4 CL147 / CodeCL146 / EngineCL76 / BuildId `792eec68-5f82-4e2f-852a-66474164b377`，校验通过。它不含本次持久化工作台；下方临时舞台和旧编译记录仅作历史证据，不代表当前设计。

入口：YogTool → 开发工作台 → 材质与灯光 → LookDev 关卡工作台。
备用入口：YogTool → 高级 / 独立工具 → 美术资产工具 → LookDev 关卡工作台。

首次初始化仅创建缺失地图，绝不覆盖美术后续填充的内容。上半区选择后点击“加载关卡”，下半区点击灯光立即切换。灯光方案和目录见 `Docs/Dev01_LookDev_Lighting.md`；使用说明见 `Docs/Dev01_Workflow_Guide.md`。原有地图创建器及它的生产地图映射保留原样。

## 历史：临时舞台验证与边界（本次设计已替代）

- Workflow Python / UI 回归 20 项通过。
- 独立副本中 `-Module=DevKitEditor -NoEngineChanges` 定向 C++ 编译成功，160 个 action，退出码 0；日志 `X:/Dev-ProjectBuilds/Dev01-LookDev-20261002/LookDev_Build_20261002.log`。它基于本机既有源码快照和源码引擎，不代表云端 CL140 / Installed Build 编译成功。
- 完整独立项目编译已通过：使用 canonical Installed Build，`DevKitEditor Win64 Development -NoEngineChanges -NoUBA -UsePrecompiled`，944 actions，1283 秒，退出码 0；日志 `X:/Dev-ProjectBuilds/Dev01-LookDev-20261002/LookDev_Full_Installed_20261002.log`。这不替代云端正式 Engine @76 的编译验证。
- UE 自动化 8 项通过，覆盖旧 CombatItem、Workflow、Toolbar、LookDev PresetContract 与 StageSmoke。报告 `X:/Dev-ProjectBuilds/Dev01-LookDev-20261002/Saved/Automation/LookDevContracts-20261002/index.json`。
- RenderSmoke 已生成并验证 Neutral / L1_Torch / L2_Fire / L3_Sanctuary 四张真实 PNG（1165×787）；该测试只检查截图交付，不检验角色受光或美术效果。报告 `X:/Dev-ProjectBuilds/Dev01-LookDev-20261002/Saved/Automation/LookDevRender-20261002/index.json`。实图发现角色明显偏黑，视觉验收未通过，暂不发布 LookDev。
- 未完成主 UGS 编辑器的新菜单验收或 UGS 冷下载，主工作区未替换本地编译 DLL。
- 新建独立编译副本：`X:/Dev-ProjectBuilds/Dev01-LookDev-20261002`。Binaries / Intermediate 独立，Content 为只读测试用途的共享 junction；不保存资产。
- 候选灯光仍待美术标定。四份报告确认 Elian 网格默认 Body / Hair / Face 均为 `M_MasterCharacterMAT` 派生的 Unlit 材质，未使用 `StylizedCharacterLit`；其旧数据灯路径引用 `MF_CelesLightInfo` / `MF_CelesLightLit`。原生候选灯不能代替旧 CaptureBox / 数据纹理灯光，也不能通过拉高曝光或自动换材质掩盖。需确认游戏中正式角色及其实际材质覆盖，再做角色受光验收。
- 已修复 DirectionalLight 的默认根变换与传入变换相乘问题：Neutral 主灯从错误的 pitch −86° / scale 2.5 恢复为预设 −40° / scale 1。仅对工具生成的 Key / Fill 设置最终绝对变换，不改变其他 actor、引擎 Shader、材质或曝光策略。
- 修复后再次增量编译通过（5 actions / 16.30 秒）；8 项 UE 自动化全部通过，包括全部 9 预设的 Key / Fill 位置、旋转、缩放断言。RenderSmoke 再次生成四张实际 PNG，JSON 证实主灯变换正确；Elian 仍黑，符合未改其旧 Unlit 材质的边界。报告分别在 `Saved/Automation/LookDevContracts-20261002-LightTransform` 与 `Saved/Automation/LookDevRender-20261002-LightTransform`，编译日志 `LookDev_LightTransform_20261002.log`。上述路径均相对于独立副本，不是主 UGS 工作区。
- 正式角色选择待确认：默认 `B_GameMode` 依赖 `B_PlayerOne`，后者引用 Elian，暂未找到新版基础材质覆盖；存档还可能覆盖角色类，不能仅由默认配置推断实际玩家。`/Game/Developers/g/SM_CellGuard_011` 可作为新版材质的静态技术样本，不代替正式玩家、骨骼自阴影或动画验收。历史 Miyabi 场景报告存在混合 Unlit 槽和未保存调整，也不能直接当作正式基准。
- 不使用源码引擎编译主 UGS 工作区，也不手工替换其 DLL / BuildId。
- 主 UGS 工作区已加载的 `UnrealEditor-DevKitEditor.dll` 与 CL144 构建输出哈希一致，但它不含本地待提交的 LookDev 文件。正式 Engine / 项目模块 BuildId 为 `792eec68-5f82-4e2f-852a-66474164b377`。不能用本地独立构建 BuildId `59f42fc9-c192-48bc-a496-34c5b5f7923e` 的 DLL 替换 UGS 输出。

## 已解决的 CL140 构建阻塞与当前发布状态

云端 `C:/BuildAgent/Dev01/logs/auto_ugs_build_20261002_103901.log:1142` 曾显示 CL140 构建失败：

`MontagePhaseTagDriver.cpp(1,1): fatal error C1083: AbilitySystem/AbilityTask/MontagePhaseTagDriver.h` 缺失。

随后还有 `AWeaponInstance.PlayJustComboVFX`、`UWeaponDefinition.JustComboVFX` 等缺失接口。2026-10-03 的 P4 CL144 回退了 CL140 中不完整的 `Source/DevKit/Private` 变更，保留旧 CombatItem 路径；2026-10-04 云端已成功编译 CL144。

当前服务器凭据：`completed_build.json` 为 `complete`，ProjectCL / CodeCL 144、EngineCL 76、BuildId `792eec68-5f82-4e2f-852a-66474164b377`；PCB #30 已在 P4 CL145 提交，说明为 `[CL 00000144] Cloud-built DevKitEditor`，`p4 verify -q` 无错误。CL144/PCB #30 不包含本地 LookDev 代码。

## 后续发布顺序

### 2026-10-02 战斗兼容历史记录

用户已授权处理 CL140 编译依赖并继续发布。复核发现还存在不能隐含在编译修复中的 CombatItem 玩法分岔，因此目前仍未提交或发布。

- 无歧义的头文件/API 缺失可追溯至已提交 Git 对象 `82c527949`（Montage Phase / ASC）、`260b45511`（GameplayCue 标签反应）、`0c4e87503`（JustComboVFX）、`ce252c3b1`（有效攻击距离）；不应复制脏 Git 工作树中的 ActionRules 修改。
- CL140 的 GA_MeleeAttack.cpp、GA_PlayMontage.cpp、DamageAttributeSet.cpp 与 Git `ca66c230a` 的移除旧 CombatItem 版本一致，AreaActor.cpp 也已删除；但组件、UI、数据结构和 AreaActor.h 仍是父提交版本。只补声明无法解决这套不一致的链接关系。
- 完成 `ca66c230a` 配套迁移会移除内置油瓶/雷石/烟雾效果、次数与冷却，改为 ActivationTag 触发 GA 后消耗道具；需要连同 GameplayEvent.CombatItem.Throw、道具资产和 GA 路径一起验证，不能仅凭编译宣称玩法正确。
- `X:/Dev-ProjectBuilds/Dev01-UGS-CL140-Repair-20261002` 的已编译候选采用 `ca66c230a^` 回退四个文件的策略，保留旧道具。这是另一种行为选择，不能直接当纯编译补丁提交。
- 用户随后明确选择：先保留旧 CombatItem，等同事在 GitHub 完成新道具方案后再接入。CL140 已是提交历史，不能重新提交或改写；旧道具兼容修复已在本地准备并验证，但本轮暂不创建或提交新 CL。后续发布必须重新复核提交范围，不把 P4 修复自动推回 GitHub，也不启用 Code Mirror。
- 最小兼容修复白名单为 16 文件：11 个配套接口/实现、CharacterTag.ini 的三个阶段标签、四个旧 CombatItem 实现文件。排除额外的 MontagePhaseFlowComponent 与角色挂载，排除脏 Git ActionRules/LuaRules；不修改资产或引擎。
- 四个旧道具文件的不可变来源为 `f5fe71eb3dde51eacb42a6210eb123f151985c02`（`ca66c230a^`）。保留原次数、冷却、油瓶/雷石/烟雾、油刃命中和油火额外伤害；暂不引入 ActivationTag 单次消耗迁移。
- 隔离修复来源、SHA256 和恢复记录：`X:/Dev-ProjectBuilds/Dev01-LookDev-Compatibility-Audit-20261002/final-manifest.json`。主项目定向同步前的源文件备份在同目录 `MainBeforePort`；用户的贴花、灯光 BuiltData 与 UGS 二进制不进入提交白名单。
- `pre-submit-files.json` 是灯光变换修复前的只读快照，不可直接作为后续提交放行依据；Service.cpp、CharacterLookDevTests.cpp 与本文档已继续修改，发布前必须重新生成并审阅白名单及哈希。
- 源码引擎完整编译被 `-NoEngineChanges` 拦截（现有 NetCore 重建需求），未放开引擎写入。随后改用 canonical Installed Build 编译独立副本；本机引擎 BuildId `59f42fc9-c192-48bc-a496-34c5b5f7923e` 不等于正式 P4 Engine @76，云端正式验证仍不可省略。

### CL140 最小兼容修复白名单（16 文件）

- `Source/DevKit/Public/AbilitySystem/AbilityTask/MontagePhaseTagDriver.h`
- `Source/DevKit/Public/AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h`
- `Source/DevKit/Public/AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h`
- `Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h`
- `Source/DevKit/Public/AbilitySystem/Abilities/YogTargetType_Melee.h`
- `Source/DevKit/Public/Character/YogCharacterBase.h`
- `Source/DevKit/Private/Character/YogCharacterBase.cpp`
- `Source/DevKit/Public/Item/Weapon/WeaponDefinition.h`
- `Source/DevKit/Public/Item/Weapon/WeaponInstance.h`
- `Source/DevKit/Private/Item/Weapon/WeaponInstance.cpp`
- `Source/DevKit/Public/Data/TagReactionDataAsset.h`
- `Source/DevKit/Private/AbilitySystem/Abilities/GA_MeleeAttack.cpp`
- `Source/DevKit/Private/AbilitySystem/Abilities/GA_PlayMontage.cpp`
- `Source/DevKit/Private/AbilitySystem/Attribute/DamageAttributeSet.cpp`
- `Source/DevKit/Private/Actors/CombatItemAreaActor.cpp`
- `Config/Tags/CharacterTag.ini`

### 提交与发布门禁

1. 由战斗代码负责人补齐 CL140 编译依赖，或明确授权处理该独立阻塞。
2. 重新检查 opened / pending / resolve，复核本次精确文件白名单；保留其他光照、贴花和战斗改动。
3. 等待同事 GitHub 工作完成并复核接入选择；确认正式角色材质和 LookDev 验收结果后，再提交独立 CL。截图文件生成成功不构成角色效果验收。
4. 只启动一次云端 `dev01_auto_ugs_build_with_feishu.ps1` 包装入口，保留既有飞书通知和管线锁；不直接并行调用 build / publish，也不启用被禁用的调度任务。
5. 核对 completed_build.json 为 complete、新 PCB 的 CodeCL / EngineCL / BuildId 与 SHA-256，执行 p4 verify；再关闭编辑器后由 UGS 下载匹配包。
6. 实际确认上述入口存在且能打开，再标记已交付。不要让美术手工复制 DLL 或点击 Missing Modules 的 Yes。

## 历史：临时舞台精确文件白名单（当前发布以 CL148 describe 为准）

- `Source/DevKitEditor/Private/Tools/CharacterLookDev/DevKitCharacterLookDevPresets.cpp`
- `Source/DevKitEditor/Private/Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h`
- `Source/DevKitEditor/Private/Tools/CharacterLookDev/DevKitCharacterLookDevService.cpp`
- `Source/DevKitEditor/Private/Tools/CharacterLookDev/DevKitCharacterLookDevService.h`
- `Source/DevKitEditor/Private/Tools/CharacterLookDev/SDevKitCharacterLookDevWidget.cpp`
- `Source/DevKitEditor/Private/Tools/CharacterLookDev/SDevKitCharacterLookDevWidget.h`
- `Source/DevKitEditor/Private/Tests/CharacterLookDevTests.cpp`
- `Source/DevKitEditor/Private/Tests/CharacterLookDevRenderTests.cpp`
- `Source/DevKitEditor/DevKitEditor.cpp`
- `Source/DevKitEditor/DevKitEditor.Build.cs`
- `Source/DevKitEditor/Private/Tools/Workflow/SDevKitWorkflowWidget.cpp`
- `Build/Dev01WorkflowCatalog.json`
- `Source/DevKitEditor/Private/Tests/DevKitWorkflowTests.cpp`
- `Source/DevKitEditor/Private/Tests/DevKitEditorToolbarTests.cpp`
- `BuildScripts/Workflow/test_workflow_ui.py`
- `Docs/Dev01_Workflow_Guide.md`
- `Docs/Plans/Dev01_LookDev_UGS_Handoff_20261002.md`
