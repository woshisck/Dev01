# 未完成工作清单（工作树快照）

| 项 | 内容 |
|------|------|
| 适用范围 | 当前 `main` 分支未提交的改动（HEAD = `5dc9feb0`） |
| 适用人群 | 程序 |
| 配套文档 | [FeatureLog](FeatureLog.md) · [任务看板](PM/TASKS.md) |
| 最后更新 | 2026-09-22 |

工作树里有两条互不相关的在飞工作：**A** 交互系统重构（含 2026-09-22 的交互提示统一，代码完成、已编译、未提交），**B** RGame 工程的 GPU 崩溃分析（dump 已解码、未定位）。另有 **C** 提交前需要剔除的构建产物。

---

## A. INTERACT-001 蓄力交互重构

**状态**：C++ 改动完整，`FeatureLog` 已记录为完成，但**整套改动尚未提交**。`Source/` 下 25 个文件变更（含删除的 `InteractHoldFeedback.h`），外加未跟踪的新文件 `YogInteractable.h` 与改动过的 `IA_Interact.uasset`。

代码侧已确认完成的部分：六个 `Pending*` 单值槽已合并为 `OverlappingInteractables` 数组，六个交互物（WeaponSpawner / RewardPickup / AltarActor / ShopActor / Portal / HubFacilityActor）全部实现 `IYogInteractable`，`ResolveInteractTarget` 与 `CommitInteract` 的 if-chain 已删除，手写 `PlayerTick` 计时已换成 IA_Interact 上的 `UInputTriggerHold`。仓库内已无 `Pending*` 残留引用（`YogHUD.cpp` 里的同名符号是局部变量）。

### 待办

| # | 事项 | 状态 | 细节 |
|---|------|------|------|
| A1 | 提交改动 | ⬜ 待办 | 源文件改动 + `IA_Interact.uasset` + 新增 `Public/Character/YogInteractable.h` + 删除 `Public/Character/InteractHoldFeedback.h`，需一次提交，避免头文件删除与引用改动分离 |
| A2 | `M_InteractHoldRing.uasset` 悬空 | 🟡 代码已接，待重编译验证 | 材质本身是完整可用的径向进度环（`MD_UI` / `BLEND_Translucent`；Custom HLSL 输出 `float2(ring, filled)`；参数 `Percent` / `InnerRadius 0.36` / `OuterRadius 0.46` + 两个颜色向量；Emissive ← Lerp、Opacity ← 环遮罩均已连）。已在 `UInteractPromptWidget` 接入，详见下方 A2 实现说明 |
| A3 | 复核 `IA_Interact` 参数 | ✅ 已确认 | 2026-09-21 经 MCP 读运行中编辑器：`HoldTimeThreshold=0.6`、`bIsOneShot=true`、`bAffectedByTimeDilation=false`、`ActuationThreshold=0.5`，仅挂 1 个 `InputTriggerHold`，`ValueType=Boolean`。与文档一致，无需改动 |
| A4 | 代码注释语言违规 | ✅ 已修 | `YogInteractable.h` 类注释与 `PlayerCharacterBase.h` 的 `OverlappingInteractables` 注释已改写为英文。注：本次改动**顺带重写过的旧中文注释**（`Portal.cpp` / `RewardPickup.cpp` / `YogHUD.cpp` / `WeaponSpawner.h` / `PortalDirectionWidget.h`）保持原样 —— 属仓库既有风格，未纳入本次范围 |
| A5 | 重跑编译与自动化测试 | ✅ 已验证 | 先确认运行中的编辑器加载的就是重构后的二进制（`InteractHoldDuration` 已读不到，而同可读性级别的 `DefaultMappingContext` 正常读出），再跑 16 项交互相关测试（Interact / InteractPromptWidget / Portal ×4 / PortalPreviewWidget ×2 / RewardPickup ×2 / WeaponSpawner / AltarMenuWidget / StoryEncounter ×2 / GameMode / PortalPreview 编辑器）→ **16 通过 0 失败**。此后的源码改动只有 A4 的注释，不影响编译 |
| A6 | 蓝图侧未验证 | ✅ 已排除 | `git show HEAD` 确认旧六个 `Pending*` 全部是裸 `UPROPERTY()`，无 `BlueprintReadWrite` / `BlueprintReadOnly`，蓝图从来无法引用，不存在 BP 断链风险 |

### A2 实现说明（2026-09-21 接入）

交互浮窗一直是用 `SetWidgetClass(UInteractPromptWidget::StaticClass())` 直接实例化**原生类**的，没有任何 WBP，布局全由 `BuildFallbackLayout()` 在 C++ 里搭。所以环形材质无法「由策划在 WBP 里绑」，只能在 C++ 按路径加载 —— 这与仓库既有做法一致（`YogGameMode.cpp`、`PlayerCommonInfoWidget.cpp`、`WeaponSpawner.cpp` 等都有硬编码 `/Game/UI/Playtest_UI/...` 路径）。

蓄力进度的显示逻辑原先在**三个 widget 里各写了一份**完全相同的代码（clamp + `SetPercent` + 可见性），这轮一并收口到共享实现。

| 项目 | 内容 |
|------|------|
| 新增文件 | `Source/DevKit/Public/UI/InteractHoldRing.h`、`Source/DevKit/Private/UI/InteractHoldRing.cpp` |
| 共享接口 | `YogInteractHoldRing::BuildRing(WidgetTree, DiameterPx)` 按路径加载材质并建 `UImage`（材质缺失返回 null）；`YogInteractHoldRing::ApplyProgress(Ring, Bar, Normalized)` 统一 clamp / 可见性，环与条哪个存在就驱动哪个 |
| 三处调用方 | `UInteractPromptWidget`（原生布局，`BuildRing` + `ApplyProgress`）、`URuneRewardFloatWidget`、`UWeaponFloatWidget`（均为 WBP 布局，只调 `ApplyProgress`）。三个 `SetHoldProgress` 现在都是一行 |
| 进度驱动 | `UImage::GetDynamicMaterial()` 取**每实例** MID 后写 `Percent`。必须走 MID，否则所有交互物共用同一材质参数、进度互相串台 |
| WBP 绑定 | 两个浮窗 widget 新增 `HoldProgressRing`（`UImage`，`BindWidgetOptional`）；WBP 里按此名放 Image 即生效，没放则沿用原行为 |
| 已完成的 WBP 编辑 | `WBP_WeaponFloat`：已通过 MCP 在 `FloatCard` 下加 Image `HoldProgressRing`，挂 `M_InteractHoldRing`、40×40、默认 Collapsed，CanvasPanelSlot 锚点底部居中（anchor 0.5/1.0、alignment 0.5/0、offset top 8、zOrder 10），**绝对定位不影响原有排版**；已 Compile + Save。材质现在有了真实引用方 |

#### 发现的既有缺口（不是本次改动引入）

| 交互物 | 现状 |
|------|------|
| Portal / Altar / Shop / HubFacility | 用原生 `UInteractPromptWidget`，自建布局，**一直有**蓄力进度显示 |
| WeaponSpawner | `WBP_WeaponFloat` 的继承绑定里 `HoldProgressBar` 是 `None`（声明了但从未在 WBP 里摆放），所以**从来没显示过**蓄力进度。本次加的 `HoldProgressRing` 是这里的第一个蓄力反馈 |
| RewardPickup | `BP_RewardPickup.RuneFloatWidgetClass` = **None**，且全工程没有任何 `URuneRewardFloatWidget` 的 WBP 子类（资产注册表 + 二进制扫描双向确认）。即浮窗根本没被创建，`SetInteractHoldProgress` 一直空转。**要在这里看到环，得先做一个符文浮窗 WBP 并填进 `RuneFloatWidgetClass`** —— 属新 UI 制作，未动 |

| ⚠️ 待办 | 说明 |
|------|------|
| 必须一起提交 | `Content/UI/Playtest_UI/Interact/M_InteractHoldRing.uasset` 仍是未跟踪文件；C++ 加载失败会静默退回线性条，只提交代码不提交材质的话别人看不到环也不会报错 |
| 需重编译 | 新增反射属性 + 新增 .cpp，Live Coding 不生效，必须 UBT 重编译 `DevKitEditor`。**环形效果在重编译前仍未经过任何图形验证** |

### 本次重构未覆盖（已知限制，非阻塞）

| 事项 | 说明 |
|------|------|
| `CanInteract()` 覆写不全 | 仅 `RewardPickup`（`!bPickedUp && IsPickupAllowed()`）与 `WeaponSpawner`（`bEnabledByFirstRunTutorialState && !bPickedUp`）覆写；Altar / Shop / Portal / Facility 仍只靠 Register / Unregister 的时机 gate，阶段切换时玩家站在体积内的边界情况未统一收口 |
| 单目标自定义蓄力时长能力丧失 | 时长现由 InputAction 统一持有；想让单个交互物用不同时长只能另建 InputAction（原 `IInteractHoldFeedback::GetInteractHoldDuration` 无实现者，属死代码，已随文件删除） |
| Portal Stage C 钩子仍空 | `Portal.cpp:476` / `Portal.cpp:489` 的 `TODO(Stage C)` 未处理；`YogHUD::NotifyPlayerInPortalRange` 仍是空实现，浮窗目前靠 `TickPortalPreview` 每帧自己挑 Target |

---

## B. RGame 工程 GPU 崩溃（Nsight Aftermath）

**状态**：dump 已成功解码，崩溃位置已缩到 Pass 级别，**未定位到具体 shader、未验证修复**。

注意：这份 dump **不是本工程的**。`CommandLine` 显示崩溃进程是 `D:\P4\RGameEditor\RGame\RGame.uproject`（`GameName=UE-RGame`），只是分析工作发生在本目录。

### 相关文件（全部未跟踪，位于仓库根目录）

| 文件 | 说明 |
|------|------|
| `D3D12.0.2026.09.21-10.31.06.nv-gpudmp` | 原始 Aftermath dump，627 KB |
| `gpudmp_crashcontext.xml` | UE 崩溃上下文（`CrashType=GPUCrash`） |
| `gpudmp_decoded.json` | 解码产物 |
| `decode_aftermath.py` | 解码脚本，`ctypes` 直调 `GFSDK_Aftermath_Lib.x64.dll` |

### 已得结论

| 项 | 值 |
|------|------|
| Device state | `Error_DMA_PageFault`，`Engine reset occurred = true`，adapter 未 reset |
| Fault address | `0x6f00074f26b30580`（高位形态不像合法 VA，倾向于已释放 / 越界的描述符或 buffer） |
| Shader | `compute_01`，Compute，93952 字节，hash `106427061617616069` |
| 环境 | UE 5.8.1 Editor（Development）· RTX 4070（AD104-A）· 驱动 610.88 · Win11 26200 · 崩溃发生在启动后 107 秒 |
| Aftermath marker 栈 | `ShadowDepths` → `RenderVirtualShadowMaps(Nanite)` → `FVirtualShadowMapArray::BuildPageAllocation` → `Nanite::DrawGeometry` → `InstanceCulling` / `NoOcclusionPass` |

即：page fault 落在 **虚拟阴影贴图（VSM）页分配 / Nanite 阴影渲染** 的 compute 路径上。

### 待办

| # | 事项 |
|---|------|
| B1 | 把 `compute_01` 对应到具体 shader —— 需要 RGame 的 shader symbol / PDB，或开 `r.Shaders.Symbols=1` 重跑复现 |
| B2 | 用 VSM / Nanite 相关 cvar 做二分验证（先关 VSM 看是否消失），确认是引擎侧问题还是 RGame 的内容 / 渲染改动触发 |
| B3 | 确认是否可复现 —— 目前只有单次 dump，无复现步骤记录 |
| B4 | `decode_aftermath.py` 里引擎 DLL 路径与 dump 文件名为硬编码，复用前需改为参数 |
| B5 | ✅ 已处理：4 个文件已加进 `.gitignore`（`*.nv-gpudmp` + 三个根目录文件），不再出现在 `git status`。文件仍在磁盘上，分析完可自行删除；若结论有保留价值，另写一份报告进 `验收与报告/` |

---

## C. 提交前清理

以下改动是构建 / 环境产物，不应随功能改动一起提交。**注意：`.gitignore` 里早已写了 `.vsconfig`（重复 6 次）与 `*.modules`，但这 5 个文件是「已跟踪」状态，`.gitignore` 对已跟踪文件无效**，所以它们会一直以 modified 出现。要彻底止血需要 `git rm --cached` 把它们脱离跟踪并提交一次 —— 该操作会让其他人 pull 后本地副本被删除，属共享状态改动，需先确认。短期做法是提交时不要 stage 这几个文件。

| 文件 | 说明 |
|------|------|
| `.vsconfig` | Visual Studio 组件清单，本机生成 |
| `Plugins/CommonLoadingScreen/Binaries/Win64/UnrealEditor.modules` | 构建产物 |
| `Plugins/ElectronicNodes/Binaries/Win64/UnrealEditor.modules` | 同上 |
| `Plugins/GameFeatures/CountDownTime/Binaries/Win64/UnrealEditor.modules` | 同上 |
| `Plugins/ModularGameplayActors/Binaries/Win64/UnrealEditor.modules` | 同上 |

文档侧改动（`02_玩法设计与调研/` 三篇、`Portal_ConfigGuide`、两篇 WBP Layout、`WeaponSystem_Technical`、`FeatureLog`）是 A 的配套更新，随 A1 一起提交即可。
