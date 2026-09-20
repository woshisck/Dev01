# 贴花与地表物件：美术操作核查

日期：2026-09-08。范围：用户图一的绿色 Poison 延迟投射贴花，以及当前 Collection 制作工具。本文是代码核查与操作建议，不代表已完成交互删除、撤销或性能验收。

## 先说结论

绿色贴花属于 `DecalCollection_RVTTest_Ground` 中的一条放置记录。右侧的 `Derived_Deferred_…` 是系统生成的显示组件，不是需要美术维护的原始数据。**不要删除整个 Collection，也不要删除 Poison 材质或资产库定义来清除这一处贴花。**

当前工具有删除功能，但“右侧选中了组件”和“工具选中了一个场景实例”不是同一个状态；面板又缺少显眼的删除按钮，因此确实容易出现“看到选中、却无法删除”的体验。

## 图一：安全删除这一处贴花

以下是现有代码支持的操作路径；尚未在用户当前关卡上实际执行。正式操作前应先在关卡副本验证，避免覆盖当前未保存的制作内容。

1. 如果“画笔放置”前有 `●`，先点击它关闭画笔，防止点击视口时新增贴花。
2. 确认当前 Collection 是 `RVTTest_Ground`，点击“编辑当前 Collection”。
3. 在**视口**点击绿色贴花靠近中心的位置。只点击资产库的 Poison 缩略图，或右侧组件树里的 `Derived_Deferred_…`，不等于选中了工具内部的实例。
4. 向下查看左面板“当前实例 / 直接调整”，必须显示 **1 个实例**，且资产/材质对应 Poison。如果仍为 0，先停止，不要删除整个 Actor。
5. 让焦点留在视口，按 `Delete`。此命令应只删除当前 Record，并重建显示组件；预期记录总数减少 1，其他地表物件不变。
6. 点击底部“应用并退出”，再保存对应的测试关卡。点击“取消”会恢复进入本次编辑会话时的记录快照，因此已删除的贴花会重新出现。

如果上述路径无法可靠选中，不要反复尝试删除右侧派生组件。应先补齐下面的实例列表与显式删除入口。

“恢复原始来源”也不是删除按钮：它用于曾经从旧 Actor 接管的内容，会重新显示旧来源并禁用 Collection 替代记录。

## 已有证据与尚待验证

| 项目 | 当前结论 |
| --- | --- |
| Poison 的显示方式 | 从截图组件命名及源码可确认是 Collection 派生的真实 `UDecalComponent`，不能把“删不掉”直接解释为 RVT 缓存残影。 |
| 删除入口 | `ProcessEditDelete` 已实现按当前 Record GUID 删除，然后重建派生组件。 |
| 为什么截图操作可能无效 | 代码未把普通 Details 组件选择转换为 Mode 的 Record 选择；没有有效 Record 时 Delete 被安全吞掉。与截图相符，但尚未交互复现。 |
| 原始记录与派生组件 | Record 是保存真值；派生组件可重建。直接改派生组件可能在重建后丢失。 |
| Git/P4 代码一致性 | 核查的 EdMode、Widget、Collection Actor 三个 `.cpp` 忽略行尾差异后内容一致。 |
| Undo / Redo | 有事务和会话取消快照，但本轮未执行 Delete→Undo→Redo 的视口验收。现有四项自动化不覆盖这条交互链。 |
| 性能 | 本轮未采集 GPU、RVT 页更新或编辑器耗时，下面的性能项是代码可见风险，不是已测量的瓶颈结论。 |

## 确认的工具缺口

| 优先级 | 美术遇到的问题 | 代码依据与建议 |
| --- | --- | --- |
| P0 | 明明选中右侧组件，却删不掉；不知道到底选的是资产、批次还是一个实例 | 删除只认 Mode 内部记录。增加“场景实例”列表与“删除当前实例”按钮，显示将删除的名称和数量；没有实例时给明确原因，不静默吞命令。 |
| P0 | 大贴花、旋转贴花或重叠贴花可能难选、误选 | Deferred 拾取采用未缩放 DecalSize 推导的球半径和射线距离，没有精确处理投射体旋转、世界缩放或遮挡。改为精确投射体拾取，并支持重叠候选选择。 |
| P1 | 修改当前实例材质后，接下来像是没有选中任何东西 | 材质修改会重建全部代理，但 Deferred 选择仍保存旧组件弱引用。应以 GUID 保存选择，重建后重新定位代理。 |
| P1 | 面板标为“有效/可放置”，但实际配置不完整 | Deferred 校验只检查材质非空，不检查材质 Domain；RVT 目标必需标志也未纳入有效性门禁。加入明确中文错误，例如“需要 Deferred Decal 材质”“缺少地表 RVT”。 |
| P1 | 未来若按实例设置湿度、颜色等参数，可能填了也不生效 | CustomData 写入前未配置 ISM 的 NumCustomDataFloats，底层越界返回 false，结果未检查。先补齐参数布局及测试，再向美术开放参数表。 |
| P2 | 大批量刷地面时操作成本可能越来越高 | 每次放置/删除会重建整个 Collection。改为变化记录/批次增量更新，并按关卡区域分组；实施前后需测量。 |

不建议先让美术学习更多内部 Tag 或派生组件名称。这些问题应由工具选择模型、数据校验和操作反馈解决。

## RVT 与 Deferred 的真实边界

- **Collection 是统一制作入口，不是把所有效果自动烘进 RVT。** RVT 平面、RVT 地表物件、网格贴花、普通地表物件和 Deferred 使用不同渲染后端。
- 图一 Poison 仍然每条记录对应一个真实 Deferred 投射组件；收进 Collection 不代表获得 ISM 合批收益。
- 网格批次当前按 **资产定义对象 + 单实例材质覆盖** 分组。不同资产即使使用相同 Mesh/材质，也不一定合并；大量复制材质变体或“Bake 为独立批次”会增加批次。
- RVT 显示物件在编辑器中会保留源网格，运行时再根据画质决定是否投射化。编辑器 `G` 视图不能代替实际 High/Mid/Low 运行验收。
- 引擎移除 RVT Writer 图元时已有 Bounds 脏区刷新。当前工具却常重建整个 Collection，可能扩大更新范围；不能把所有异常都归因于“RVT 完全不刷新”。
- 静态污渍、地面混合可以继续采用 RVT；需要实时投射、独立动画/发光的效果可保留 Deferred 或其他动态 Overlay。转换后端应是显式、可预览、可回退的制作操作，不应自动替美术更换效果。

## 建议的美术管理入口

同一个窗口分成三个日常区域，后台细节折叠：

| 区域 | 美术看到什么 | 常用操作 |
| --- | --- | --- |
| 资产库：选用什么 | 缩略图、中文名称、适用表面、静态/动态类型、配置状态 | 拖放、画笔、创建变体 |
| 场景实例：已经放了什么 | 可搜索名称、所在 Collection/区域、位置、显示状态；可按 Poison 等资产筛选 | 定位、单选/多选、临时隔离、复制、隐藏、删除 |
| 当前实例：只改这一处 | 变换、投射尺寸、允许的材质参数、来源与影响范围 | 调整、重置、应用、删除当前实例 |
| 后台审计：按需展开 | RVT/Deferred 类型、代理数、批次数、校验错误、预计受影响区域 | 排查配置、定位错误，不直接编辑派生组件 |

选中列表、视口或右侧来源入口，都应指向同一条 Record；删除资产定义、删除场景实例、清空整个 Collection 必须是三个不同命令。右侧派生组件应只读，并提供“定位来源实例”。

实施顺序：**先解决选择与删除 → 再补齐校验、撤销和材质修改后的选择保持 → 最后做批量操作与性能优化。**

最低验收：在隔离关卡上确认单个 Poison 删除只减少一条记录；Undo/Redo 同时恢复记录和画面；取消恢复会话快照；应用、保存并重新打开后结果一致；修改材质后仍能继续选择同一实例；重叠、旋转、放大投射均能可靠选中。

## 程序定位入口

以下路径均位于 P4 项目工作区，便于程序对照；本文未修改这些代码。

- 删除与安全吞键：[DevKitDecalCollectionEdMode.cpp:636](X:/Project/YogProject/Dev01/Source/DevKitEditor/Private/Tools/DecalCollection/DevKitDecalCollectionEdMode.cpp:636)
- Deferred 选择解析：[DevKitDecalCollectionEdMode.cpp:989](X:/Project/YogProject/Dev01/Source/DevKitEditor/Private/Tools/DecalCollection/DevKitDecalCollectionEdMode.cpp:989)
- Deferred 拾取：[DevKitDecalCollectionEdMode.cpp:1017](X:/Project/YogProject/Dev01/Source/DevKitEditor/Private/Tools/DecalCollection/DevKitDecalCollectionEdMode.cpp:1017)
- 当前实例面板：[SDevKitDecalCollectionWidget.cpp:891](X:/Project/YogProject/Dev01/Source/DevKitEditor/Private/Tools/DecalCollection/SDevKitDecalCollectionWidget.cpp:891)
- 全量重建与派生生命周期：[DevKitDecalCollectionActor.cpp:182](X:/Project/YogProject/Dev01/Source/DevKit/Private/Surface/DevKitDecalCollectionActor.cpp:182)
- CustomData 写入：[DevKitDecalCollectionActor.cpp:784](X:/Project/YogProject/Dev01/Source/DevKit/Private/Surface/DevKitDecalCollectionActor.cpp:784)
- Deferred 独立代理：[DevKitDecalCollectionActor.cpp:817](X:/Project/YogProject/Dev01/Source/DevKit/Private/Surface/DevKitDecalCollectionActor.cpp:817)
- 资产有效性校验：[DevKitDecalAsset.cpp:28](X:/Project/YogProject/Dev01/Source/DevKit/Private/Surface/DevKitDecalAsset.cpp:28)
- 现有测试范围：[DecalCollectionTests.cpp:10](X:/Project/YogProject/Dev01/Source/DevKit/Private/Tests/DecalCollectionTests.cpp:10)
