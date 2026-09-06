# 贴花与地表物件编辑器（M1）

本阶段把贴花编辑从单一 RVT Surface Library 扩展为按关卡/区域管理的 Collection 外壳。

## 数据契约

- `UDevKitDecalAsset` 是统一资产定义，包含稳定 Definition GUID、Backend、Usage Tags、Placement Profile 和按质量/平台的 Representation 表。
- `FDevKitDecalPlacementRecord` 是实例的唯一权威数据，包含稳定 Instance GUID、资产引用、世界变换、Deferred 投射尺寸、Custom Data、Tags 和启用状态；显式接管的 Deferred 或兼容 ISM 批次记录还保存来源 Actor/组件，便于恢复而不删除旧数据。
- `ADevKitDecalCollectionActor` 每个 Level/区域一个，拥有 `UDevKitDecalCollectionComponent`。渲染组件只能由 Records 派生，不能反过来成为作者数据。
- 旧 `UDevKitRVTSurfaceAsset` 通过 `LegacyRVTAsset` 兼容，不会在 M1 自动改写已有 RVT 资产。

## 编辑入口

- UE Editor Mode：`贴花与地表物件`，会出现在 Selection Mode 下拉中。
- Nomad 面板：`YogTool → 贴花与地表物件`。
- 面板默认提供管理、放置、资产、审计四个工作区入口；旧 `RVT 地表物件库` 保留为兼容入口。
- 选择 Collection Actor 后点击 `Edit 当前 Collection` 进入模式。模式只允许 Collection Actor 被选择/编辑，避免误改场景中的未接管贴花。

## 会话语义

- 进入模式记录当前 Collection 的轻量 Records 快照。
- 每次后续工具操作应使用正常 UE Transaction，并在数据/派生组件修改前调用 `Modify()`。
- `应用并退出` 只校验并重建派生渲染入口，不自动保存地图。
- `取消` 恢复进入时 Records 快照，不重置全局 Undo，也不静默覆盖其他对象。

## 单实例编辑与选择性能

- 普通 Selection Mode 不创建派生 ISM 的逐实例 HitProxy；因此只选中 Collection Actor 或浏览场景时，不会为全部记录建立 191 个可选元素。
- 进入 `贴花与地表物件` Mode 后，模式只为当前 Collection 临时打开派生 ISM 的 per-instance HitProxy，并取消整组 Collection Actor 的选择高亮。点击一个网格后可直接使用 W/E/R；`UDevKitDecalCollectionISMComponent` 会把 UE 的实例变换回写到对应 `FDevKitDecalPlacementRecord`，而不是只改临时渲染组件。
- 派生组件隐藏在 Details 的作者数据之外；Records 仍是唯一持久化来源。模式会把删除/复制拦截成 Record 操作并重建派生组件，避免“渲染实例存在但作者记录不存在”。Deferred 贴花使用投射范围的精确射线选择，再复用同一套 W/E/R 事务回写 Record。
- 资产卡的 `画笔放置` 会进入显式地表画笔状态：在视口左键拖动，只有命中可碰撞表面且距上一落点达到面板设置的间距（25–1000cm，默认 100cm）时才新增 Record；一整笔画使用同一个 Undo 事务。再次点击资产卡的画笔按钮即可退出，不会劫持普通单实例 W/E/R。
- 变换期间使用普通 UE Transaction；重建或 Undo 后会重新建立映射，Mode 仍保持可编辑状态。

## PCG 的边界

- PCG 适合作为批量生成、刷点、区域规则和散布输入；不适合作为这套手工贴花的唯一作者数据。PCG 重新生成会改变点顺序/生命周期，难以保证当前 Collection 需要的稳定 Instance GUID、跨 RVT/静态网格/延迟贴花的混合记录和逐实例 Undo。
- 推荐流程是 `PCG 生成 → 预览/审计 → 接管(Bake/Adopt)到 Collection Records → 在 Mode 中单实例调整`。接管后 Records 脱离生成图即可独立编辑；需要重新生成时由用户显式覆盖/合并，不做静默重算。
- 因此当前卡顿问题应先由 Mode 的选择隔离和按需 HitProxy 解决，而不是把 Collection 直接改成 PCG 生成结果。后续可增加 PCG 接管器，但不需要修改 UE 引擎。

## RVT_TEST 接入结果

- `/Game/Developers/g/L1_Corridor_01a_RVT_Test/LevelAsset/L1_Corridor_01a_RVT_Test` 已创建 `DecalCollection_RVTTest_Ground`。
- 重载校验：16 个旧 RVT Surface 控制器、191 个源网格实例，对应 191 条网格 Collection Records、16 个派生 RVT ISM 批次和 191 个派生网格实例全部一致；另有 1 条受 Collection 管理的真实 Deferred Decal Record/Component（总 Records=192）。FloorBrick 批次恢复使用 `MI_L1_Corridor_01a_FloorBrick_RVT_Test`，不会以 `WorldGridMaterial` 调试材质替代美术预览。
- 旧控制器资产仍保留作为回退源，但测试关卡中的旧渲染源已隐藏；Collection 派生渲染从 Records 重建。需要回退时恢复旧源可见性即可，不应同时显示两套结果。

## 后续里程碑

1. M1：完成 Mode、Collection、统一资产、RVT/ISM 适配器和当前 RVT_TEST 的兼容记录导入。
2. M2（当前部分完成）：Mesh Decal、Static Mesh Overlay 和 Deferred Decal 后端已接入；审计会显示未接管候选；Deferred 与兼容 ISM 批次支持明确 Adopt/Restore，并已有基础地表画笔。仍待通用静态网格接管、批量 Convert/Finalize、Lasso/Reapply/Fill 等完整画刷工具。
3. M3：接入 Niagara/PCG/Spline Bake、Recovery Journal、预算、Multi-User 锁和完整 Cook 规则。

Deferred Decal 在未完成专用 renderer 前仍保持每条组件独立的渲染代理；Collection 负责组织、选择和生命周期，不把它误报为 ISM 合批。
