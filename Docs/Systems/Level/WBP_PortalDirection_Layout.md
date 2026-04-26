# WBP_PortalDirection 布局规格

> 用于"传送门方位指引 HUD overlay" — 关卡结算后在屏幕边缘画箭头指向所有开启 Portal。
> 父类：`UPortalDirectionWidget`（`Source/DevKit/Public/UI/PortalDirectionWidget.h`）

---

## 类信息

| 项目 | 内容 |
|------|------|
| WBP 名 | `WBP_PortalDirection` |
| 父类（C++） | `UPortalDirectionWidget` |
| 加入方式 | `AYogHUD::ShowPortalGuidance` 内 `CreateWidget + AddToViewport(Z=14)` |
| 配置入口 | `BP_YogHUD` → Details → `Portal Direction Class = WBP_PortalDirection` |
| 触发 | EnterArrangementPhase 末尾启用；玩家进入任意门 Box（`PendingPortal != null`）→ 全箭头自动隐藏 |

---

## 控件层级（树形）

```
CanvasPanel（RootCanvas [BindWidget]，Full Screen）
```

> 全屏 Canvas 即可。所有箭头单元（Image + Label）由 C++ `RebuildArrowUnits` 在 `SetActive(true)` 时动态创建，添加到 `RootCanvas`。
> WBP 中**不需要预放任何子控件**。

---

## 各控件详细属性

### CanvasPanel（RootCanvas）

#### Slot（根，无父）
此为根 Canvas，无 Slot 配置。

#### 自身属性
- 默认即可，不需要额外设置
- 可见性由 C++ 通过 `SetActive(bool)` 整体控制（true → HitTestInvisible，false → Collapsed）

---

## Details 配置（C++ 字段，在 WBP Details 面板填）

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `ArrowTexture` | UTexture2D* | （空，需填）| 箭头贴图，约定**顶点朝上 = -Y 方向**为 0° 基准（与 EnemyArrow 一致） |
| `ArrowSize` | float | 36 | 箭头像素尺寸 |
| `EdgeMargin` | float | 80 | 距屏幕边缘留白 |
| `OnScreenShrink` | float | 100 | 屏幕内可视判定向内缩减；越大越早画箭头 |
| `ArrowProjectionZOffset` | float | 80 | 投影点相对门 Pivot 的 Z 抬升（cm） |
| `ArrowAngleOffset` | float | 90 | 贴图朝向补偿角（顶点朝上时填 90） |
| `ArrowColor` | LinearColor | (0.95, 0.85, 0.55, 0.92) | 箭头颜色（RGBA 线性） |

> ArrowTexture 推荐复用现有 EnemyArrow 用的箭头贴图（如 `T_ArrowTriangle`），避免重复美术。

---

## BindWidget 汇总

### 必须（BindWidget）

| C++ 变量名 | 控件类型 | WBP 控件名 | 说明 |
|---|---|---|---|
| `RootCanvas` | CanvasPanel | `RootCanvas` | 根 Canvas，C++ 在此动态添加箭头单元 |

### 可选

无。

---

## 颜色速查

| 元素 | Hex | Linear RGBA | 说明 |
|---|---|---|---|
| 默认箭头色 | #F2D98C α=0.92 | (0.95, 0.85, 0.55, 0.92) | `ArrowColor` 字段；C++ 直接应用到 Image ColorAndOpacity |
| 标签文字（普通房） | #F2D98C | (0.95, 0.85, 0.55, 0.92) | C++ 按 Room.Type 静态映射（Normal=米黄）|
| 标签文字（精英房） | #D85A4A | (0.85, 0.35, 0.29, 0.92) | |
| 标签文字（商店房） | #D9B047 | (0.85, 0.69, 0.28, 0.92) | |
| 标签文字（事件房） | #7A5BC9 | (0.48, 0.36, 0.79, 0.92) | |

---

## 注意事项

- **WBP 中不要预放任何子控件** — 全部由 C++ 动态生成，预放会与动态生成的箭头单元混在一起，难管理
- 设计这个 Widget 时**不要勾选** "Is Variable" 之外的特殊选项；保持 RootCanvas 为最纯净的全屏 Canvas
- 箭头单元结构（C++ 内部，作参考，不需 WBP 配置）：
  ```
  CanvasPanelSlot（HBox 单元，Size=220×ArrowSize，Alignment=0.5/0.5，AutoSize=false）
    └── HorizontalBox
        ├── SizeBox（Width/Height = ArrowSize）
        │   └── Image（箭头，ColorAndOpacity = ArrowColor）
        └── TextBlock（房间名，颜色按 Room.Type 静态映射）
  ```
- 箭头每帧只在"门屏幕外 + 玩家未进任何 Portal Box"时显示，其他情况自动 Collapse
