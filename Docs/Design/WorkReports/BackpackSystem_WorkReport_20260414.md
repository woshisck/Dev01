# 背包与符文激活系统 工作报告 2026-04-14

> 最后更新：2026-04-14

---

## 本次完成

- **`UBackpackScreenWidget` C++ 基类**：新建，覆盖背包 UI 全部数据逻辑
  - 格子状态查询（GetCellVisualState / IsCellInActivationZone / IsCellOccupied）
  - 符文操作（SelectRuneFromList / ClickCell / RemoveRuneAtSelectedCell / MoveRune）
  - 委托绑定（自动响应 OnRunePlaced / OnRuneRemoved / OnRuneActivationChanged）
  - 双来源符文列表（PendingRunes 三选一来源 + AvailableRunes 固定展示库）
- **符文移动**：ClickCell 内扩展"选中格子 → 点击空格子 → MoveRune"逻辑
- **三选一 → 自动入格**：修改 `AddRuneToInventory`，优先自动寻位放入 BackpackGridComponent，背包满时才退入 PendingRunes
- **初始关卡兜底**：YogGameMode 新增 `FallbackLootPool`，`GenerateLootOptions` 无 ActiveRoomData 时自动使用

---

## 新增 / 修改的文件

| 文件 | 类型 | 说明 |
|---|---|---|
| `Source/DevKit/Public/UI/BackpackScreenWidget.h` | 新建 | 背包 UI 蓝图基类 |
| `Source/DevKit/Private/UI/BackpackScreenWidget.cpp` | 新建 | 背包 UI 逻辑实现 |
| `Source/DevKit/Public/GameModes/YogGameMode.h` | 修改 | 新增 FallbackLootPool 属性 |
| `Source/DevKit/Private/GameModes/YogGameMode.cpp` | 修改 | GenerateLootOptions 加兜底分支 |
| `Source/DevKit/Private/Character/PlayerCharacterBase.cpp` | 修改 | AddRuneToInventory 自动寻位放格子 |

---

## 关键决策

**Q：为什么符文选完直接入格子，而不是先放 PendingRunes 再手动放？**  
A：展示场景下玩家不会区分"待放置列表"和"已在格子"，直接入格体验更流畅。背包满时才 fallback 到 PendingRunes，不丢失数据。

**Q：为什么 BackpackScreenWidget 用 C++ 基类而不是纯蓝图？**  
A：格子状态计算（激活区判断、Guid 匹配、PendingRunes 消耗）在蓝图里连线极繁琐且难维护。C++ 基类封装数据层，蓝图只处理视觉刷新（颜色、图标、文字），分工清晰。

**Q：为什么 AvailableRunes 不消耗，PendingRunes 消耗？**  
A：`AvailableRunes` 是 Details 面板配置的固定展示库，用于测试和演示，反复放置是预期行为。`PendingRunes` 是玩家三选一获得的实际符文，放进格子后应从待放列表消失。

---

## 遗留问题

- [ ] `WBP_BackpackScreen` 蓝图尚未制作（需今晚完成）
- [ ] 多格符文（Shape.Cells 超过 1 格）的网格显示未实现，展示版本固定 1×1
- [ ] 背包 UI 没有动画（打开/关闭淡入淡出），展示版本直接切显示状态
- [ ] `WBP_LootSelection` 内部实现需确认是否已连线

---

## 下次计划

- 验证完整链路：拾取物 → 三选一 → 自动入格 → 背包 UI 显示 → 热度激活
- 展示后根据玩家反馈决定是否需要拖拽放置
- 多格符文形状显示（Shape 支持）
