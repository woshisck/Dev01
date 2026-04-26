# 传送门 v3 开发方案 — 已实施完成（2026-04-26）

> **此文件仅作历史索引**。原 `Docs/WorkSession/current_plan.md` 中的传送门 v3 完整方案在 2026-04-26 被功能盘点重写覆盖时未进 git，无可恢复版本。但 v3 的设计、架构、数据流、决策已完整保存在以下三处正式文档：

## v3 实际产出位置

| 内容 | 文件 |
|---|---|
| **完整流程 + 配置 + 决策表 + 跨关 Buff 确定性** | [Docs/Systems/Level/Portal_ConfigGuide.md](../../Systems/Level/Portal_ConfigGuide.md) |
| HUD 单例浮窗 WBP 布局规格 | [Docs/Systems/Level/WBP_PortalPreview_Layout.md](../../Systems/Level/WBP_PortalPreview_Layout.md) |
| 屏幕外方位箭头 WBP 布局规格 | [Docs/Systems/Level/WBP_PortalDirection_Layout.md](../../Systems/Level/WBP_PortalDirection_Layout.md) |
| 功能日志条目（核心文件 / 接口 / 已知限制） | [Docs/FeatureLog.md](../../FeatureLog.md) → `[LEVEL-006]` |

## 涉及代码

C++（已合入 main）：
- `Source/DevKit/Public/Map/Portal.h` + `Private/Map/Portal.cpp`
- `Source/DevKit/Public/UI/PortalPreviewWidget.h` + `Private/UI/PortalPreviewWidget.cpp`（新）
- `Source/DevKit/Public/UI/PortalDirectionWidget.h` + `Private/UI/PortalDirectionWidget.cpp`（新）
- `Source/DevKit/Public/UI/YogHUD.h` + `Private/UI/YogHUD.cpp`（TickPortalPreview / BlackoutFade）
- `Source/DevKit/Public/Character/PlayerCharacterBase.h`（PendingPortal）
- `Source/DevKit/Private/Character/YogPlayerControllerBase.cpp`（Interact → Portal::TryEnter）
- `Source/DevKit/Public/System/YogGameInstanceBase.h` + `.cpp`（PendingRoomBuffs / bPlayLevelIntroFadeIn）
- `Source/DevKit/Public/GameModes/YogGameMode.h` + `.cpp`（ResolveTier 静态方法）
- `Source/DevKit/Public/Data/RoomDataAsset.h`（DisplayName）

## 未完成（仍是 P0 待办）

| 项 | 位置 |
|---|---|
| WBP_PortalPreview 蓝图按 Layout 规格搭建 | [WBP_PortalPreview_Layout.md](../../Systems/Level/WBP_PortalPreview_Layout.md) |
| WBP_PortalDirection 蓝图按 Layout 规格搭建 | [WBP_PortalDirection_Layout.md](../../Systems/Level/WBP_PortalDirection_Layout.md) |
| BP_YogHUD Details → PortalPreviewClass / PortalDirectionClass 赋值 | 同上 |

## 关键决策（用户确认，记录在 memory: project_portal_design.md）

- 多门 Box 不重叠 → PendingPortal 单值即可
- 浮窗不做 LineTrace 遮挡检测
- 入门角色 0.7s 移动意图，不必抵达门
- 类型徽章颜色硬编码到 .cpp（Normal/Elite/Shop/Event → 灰/橙红/金/紫）
- 主城（HubRoom）完全不启用 HUD 引导
- Portal::Open 不留旧签名 BP 兼容包装
