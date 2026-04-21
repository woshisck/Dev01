# WBP 布局配置规范文档 — 标准格式

> 本文档定义 Claude 在提供任何 UMG Widget Blueprint 布局规格时必须遵循的格式标准。  
> 目的：开发者可直接按表格在 UE Editor 中一次性配置完成，无需反复询问。

---

## 1. 文档结构模板

每份 WBP 布局规格文档必须包含以下小节（按顺序）：

```
## 类信息
## 控件层级（树形）
## 各控件详细属性（一控件一表格）
## BindWidget 汇总表
## 颜色速查表
## 注意事项（可选）
```

---

## 2. 类信息节

```markdown
## 类信息

| 项目 | 内容 |
|------|------|
| WBP 名 | `WBP_XXX` |
| 父类（C++） | `UXxxWidget`（或 UserWidget / CommonActivatableWidget） |
| 加入方式 | CreateWidget + AddToViewport(Z=N) / CommonUI Stack / HUD 子控件 |
| 配置入口 | BP_HUD → Details → XxxClass = WBP_XXX |
```

---

## 3. 控件层级节

用缩进树形表示父子关系，每行格式：

```
控件类型（控件名，Slot简述，关键属性摘要）
```

示例：
```
CanvasPanel（根，Full Screen）
└── SizeBox（MainBox，中心锚，780×420）
    └── Border（BG，#1A1A24 Alpha=0.97，Pad=24/20）
        └── VerticalBox（VStack，HFill VFill）
            ├── TextBlock（TitleText [BindWidget]，字号22 Bold，居中）
            └── Button（BtnConfirm [BindWidget]，Auto，Pad=24/10）
                └── TextBlock（BtnLabel [BindWidget]，字号15，White）
```

规则：
- 方括号内标注 `[BindWidget]` 或 `[BindWidgetOptional]`
- Slot简述写父容器决定的布局类型（Canvas/HBox/VBox/Overlay/SizeBox）
- 关键属性摘要：尺寸 / 颜色 / 字号中最重要的1-2项

---

## 4. 各控件详细属性节

**每个控件单独一个表格**，表格包含该控件所有非默认属性。按以下分类填写：

### 4.1 Slot 属性（取决于父容器类型）

#### Canvas Panel Slot
| 属性 | 说明 | 示例值 |
|------|------|------|
| Anchors Min/Max | 锚点范围（0=左上，1=右下） | Min(0.5,0.5) Max(0.5,0.5)=中心点锚 |
| Position X / Y | 相对于锚点的偏移（px） | 0 / 0 |
| Size X / Y | 控件像素尺寸 | 780 / 420 |
| Alignment X / Y | 自身对齐点（0=左/上，0.5=中，1=右/下） | 0.5 / 0.5 |
| bAutoSize | 是否随内容自动缩放 | false |
| Z Order | 叠层顺序 | 0 |

常用 Anchors 预设：
- **全屏**：Min(0,0) Max(1,1)，Position=0/0，Size不填（由Anchors撑开）
- **中心点**：Min(0.5,0.5) Max(0.5,0.5)，Alignment=0.5/0.5
- **左上固定**：Min(0,0) Max(0,0)，Alignment=0/0
- **底部拉伸**：Min(0,1) Max(1,1)，Alignment=0/1

#### HorizontalBox / VerticalBox Slot
| 属性 | 说明 | 示例值 |
|------|------|------|
| Size Rule | Auto 或 Fill | Fill |
| Fill Coefficient | Fill 时的比例权重 | 1.0 |
| Padding L/T/R/B | 内边距（px） | 0/0/0/0 |
| H Alignment | 水平对齐（Fill/Left/Center/Right） | Fill |
| V Alignment | 垂直对齐（Fill/Top/Center/Bottom） | Center |

#### Overlay Slot
| 属性 | 说明 |
|------|------|
| Padding L/T/R/B | 内边距 |
| H Alignment | 水平对齐 |
| V Alignment | 垂直对齐 |

#### SizeBox（作为父容器时，子控件Slot同HBox）

SizeBox 自身属性：
| 属性 | 说明 |
|------|------|
| Override Width | 强制宽度（px），不填=由内容决定 |
| Override Height | 强制高度（px） |
| Min Desired Width/Height | 最小期望尺寸 |
| Max Desired Width/Height | 最大期望尺寸 |

---

### 4.2 通用外观属性

| 属性 | 类型 | 说明 |
|------|------|------|
| Color and Opacity | RGBA（线性） | 控件整体透明度与色调；纯白=不影响子控件颜色 |
| Visibility | 枚举 | Visible / Hidden / Collapsed / HitTestInvisible / SelfHitTestInvisible |
| Render Opacity | float 0-1 | 仅影响渲染，不影响布局（Collapsed仍占位） |
| Render Transform | Scale/Rotation/Translation | 纯视觉变换，不影响布局 |
| Tool Tip | Text | 鼠标悬停提示 |

---

### 4.3 控件类型专属属性

#### Image
| 属性 | 说明 |
|------|------|
| Brush → Image | 纹理资产（留空=白色1×1默认Brush） |
| Brush → Draw As | Image / Box / Border / None |
| Brush → Image Size | 固有尺寸（影响Auto布局），建议与实际贴图一致 |
| Brush → Tint | 颜色叠加（RGBA线性，(1,1,1,1)=原色） |
| Color and Opacity | 最终颜色 = Tint × ColorAndOpacity，C++`SetColorAndOpacity`改的是这个 |
| Mirror → Flip Horizontal/Vertical | 镜像翻转 |

#### TextBlock
| 属性 | 说明 |
|------|------|
| Text | 初始文本 |
| Font → Typeface | Regular / Bold / Italic 等 |
| Font → Size | 字号（px）；项目常用：12/13/15/18/22 |
| Color and Opacity | 字体颜色（RGBA线性） |
| Justification | Left / Center / Right |
| Auto Wrap Text | 勾选后按控件宽度自动换行 |
| Wrap Text At | 固定换行宽度（px），与Auto Wrap二选一 |
| Min Desired Width | 最小布局宽度 |
| Shadow Color | 投影颜色（Alpha=0=无投影） |

#### RichTextBlock（额外属性）
| 属性 | 说明 |
|------|------|
| Text Style Set | 项目富文本样式 DA（如 `DT_RichTextStyles`） |
| Decorator Classes | 图标等内联装饰类 |

#### Button
| 属性 | 说明 |
|------|------|
| Style → Normal / Hovered / Pressed / Disabled | 四态Brush |
| Style → Normal Padding | 内容区内边距（影响子控件位置） |
| Content Padding | 同上（优先级更高） |
| Is Focusable | 手柄/键盘焦点可落到此按钮，通常=true |
| Click Method | OnRelease（默认）/ OnMouseDown / PreciseClick |
| 绑定 | C++：`Button->OnClicked.AddDynamic(this, &::Func)` |

#### Border
| 属性 | 说明 |
|------|------|
| Brush → 背景 | 纹理或纯色（Draw As=Image/Border） |
| Brush Tint | 颜色（常直接用此控制背景色） |
| Content Padding L/T/R/B | 内容与边框的间距 |
| H/V Alignment | 内容对齐 |

#### Overlay
- 子控件按添加顺序叠层（后加=在上）
- 每个子控件设自己的 Overlay Slot Alignment

#### UniformGridPanel
| 属性 | 说明 |
|------|------|
| Slot Padding | 每格四周内边距（统一） |
| Min Desired Slot Width/Height | 最小格子尺寸（px） |
| 子控件 Slot → Row / Column | 所在行列（0起始） |
| 子控件 Slot → H/V Alignment | 格内对齐 |

#### ScrollBox
| 属性 | 说明 |
|------|------|
| Orientation | Vertical / Horizontal |
| Scroll Bar Visibility | Auto / Always / Never |
| Always Show Scrollbar Track | 是否始终显示轨道 |
| Scrollbar Thickness | 滚动条宽度（px） |

#### ProgressBar
| 属性 | 说明 |
|------|------|
| Percent | 初始进度（0.0-1.0） |
| Bar Fill Type | Left to Right / Right to Left / Fill from Center |
| Style → Background/Fill Image | 背景/前景Brush |
| Fill Color and Opacity | 进度条前景色 |

---

## 5. BindWidget 汇总表格式

```markdown
## BindWidget 汇总

### 必须（BindWidget）
| C++ 变量名 | 控件类型 | WBP 控件名（一字不差）| 说明 |
|---|---|---|---|
| `TitleText` | TextBlock | `TitleText` | 标题，C++ 写入 |
| `BtnConfirm` | Button | `BtnConfirm` | C++ 绑 OnClicked |

### 可选（BindWidgetOptional）
| C++ 变量名 | 控件类型 | WBP 控件名 | 缺失时行为 |
|---|---|---|---|
| `PageHint` | TextBlock | `PageHint` | 跳过，不显示页数 |
```

规则：
- C++ 变量名 = WBP 控件名（大小写完全一致）
- 必须控件若名字不对，PIE 时编辑器会报错并崩溃

---

## 6. 颜色速查表格式

```markdown
## 颜色速查

| 元素 | Hex | Linear RGBA |
|---|---|---|
| 弹窗背景 | #1A1A24 | (0.10, 0.10, 0.14, 0.97) |
| 主要文字 | #ECECEC | (0.93, 0.93, 0.93, 1.0) |
| 次要文字 | #888888 | (0.53, 0.53, 0.53, 1.0) |
| 按钮 Normal | #3A3A4A | (0.23, 0.23, 0.29, 1.0) |
```

> UE Editor 颜色输入框使用线性空间（非 Gamma），#ECECEC ≠ (0.93,0.93,0.93) 在非线性情况下有差异，建议直接用线性值或在 Editor 用色盘选色后记录。

---

## 7. 注意事项节（可选）

用于记录：
- WBP 中必须手动操作的步骤（如绑定动画、勾选某个隐藏选项）
- 与其他 WBP 的依赖关系
- 已知的编辑器 Bug 及绕过方式

---

## 8. 颜色转换速查

| Hex | Linear R | Linear G | Linear B |
|-----|----------|----------|----------|
| #FF | 1.000 | — | — |
| #CC | 0.800 | — | — |
| #AA | 0.667 | — | — |
| #88 | 0.533 | — | — |
| #66 | 0.400 | — | — |
| #44 | 0.267 | — | — |
| #22 | 0.133 | — | — |
| #1A | 0.102 | — | — |
| #0A | 0.039 | — | — |

---

## 9. 本项目常用尺寸约定

| 元素 | 典型值 |
|------|------|
| 背包格子尺寸 | 64px（StyleDA.CellSize） |
| 格子间距 | 2px（StyleDA.CellPadding） |
| 弹窗宽度 | 780px（TutorialPopup） |
| 弹窗高度 | 420px（TutorialPopup） |
| 正文字号 | 15px |
| 标题字号 | 18-22px |
| 提示文字字号 | 12-13px |
| 按钮内边距 | L=24 T=10 R=24 B=10 |
| 面板内边距 | 16-24px |
