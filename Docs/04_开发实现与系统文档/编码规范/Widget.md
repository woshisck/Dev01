# UI Widget 规范

> Claude 在创建 C++ Widget 类或提供 WBP 布局规格前必须阅读。  
> WBP 布局标准格式见 [WBP_LayoutSpec_Standard.md](../系统/UI/WBP_LayoutSpec_Standard.md)

---

## 核心规则

1. **Claude 只写 C++ 类**，不创建 WBP 资产（那是用户在引擎里做的）
2. **Claude 提供 WBP 布局规格文档**，必须包含完整属性表格（尺寸/颜色/锚点/Slot 全部写明）
3. **BindWidget 名称必须与 WBP 控件名完全一致**（大小写敏感）
4. **颜色走 DA 或 C++ SetColorAndOpacity**，不在 WBP 里硬编码颜色
5. CommonUI 激活控件继承 `UCommonActivatableWidget`，普通 UI 继承 `UUserWidget`

---

## 分工

### Claude 写：
- C++ 类（.h + .cpp），`UPROPERTY(meta=(BindWidget))` 声明所有控件引用
- 数据绑定逻辑（OnPropertyChanged / NativeConstruct / NativeTick）
- 事件响应函数（按钮点击、属性变化广播）
- **WBP 布局规格文档**（完整参数表格，用户按此在编辑器搭建）

### 用户在引擎里做：
- 新建 WBP，Parent Class 选 Claude 写的 C++ 类
- 按布局规格文档搭建控件层级
- 将控件命名与 BindWidget 名匹配
- 设置 Anchors / Alignment / Padding / Size（按文档参数）
- 配置输入绑定（InputAction / CommonUI Action）

---

## BindWidget 声明规范

```cpp
// 控件绑定（名称必须和 WBP 里的控件名完全一致）
UPROPERTY(meta = (BindWidget))
TObjectPtr<UTextBlock> AmmoText;

UPROPERTY(meta = (BindWidget))
TObjectPtr<UHorizontalBox> BulletContainer;

// 可选绑定（WBP 里可以没有）
UPROPERTY(meta = (BindWidgetOptional))
TObjectPtr<UImage> ReloadIcon;
```

---

## WBP 布局规格输出标准

Claude 必须按以下格式输出每一个 WBP：

```
## WBP_XxxWidget

**Parent Class**: UXxxWidget（C++ 类名）
**Canvas 尺寸**: 400 × 120

### 控件层级
CanvasPanel
└─ [Image] Background        Anchor: 全屏撑满  Size: 400×120
   └─ [HorizontalBox] BulletContainer
      Padding: 8,4,8,4  Alignment: 左中
      └─ [Image] BulletIcon_0  Size: 24×24

### 控件属性表

| 控件 | 属性 | 值 |
|---|---|---|
| Background | Brush Color | #1A1A2E / A=0.85 |
| BulletContainer | Spacing | 4 |
| BulletIcon_0 | Image | T_BulletIcon |

### BindWidget 汇总
| C++ 变量名 | WBP 控件名 | 类型 |
|---|---|---|
| BulletContainer | BulletContainer | UHorizontalBox |
```

---

## 颜色约定

| 用途 | 参考值 |
|---|---|
| 背景暗色 | `#1A1A2E` A=0.85 |
| 边框高光 | `#FFFFFF` A=0.3–0.6 |
| 激活/高亮 | 从 `DA_BackpackStyle.ActiveColor` 读取 |
| 文字主色 | `#F0E6D3` |
| 文字次色/灰 | `#888888` |

---

## CommonUI 模式

```cpp
// CommonActivatableWidget 子类
UCLASS()
class UXxxWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()
public:
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    // 激活/停用通过 ActivateWidget() / DeactivateWidget() 控制
};
```

普通 HUD 类不需要继承 CommonUI，直接继承 `UUserWidget`。

---

## 属性变化响应模式

```cpp
// 监听 GAS Attribute 变化（在 NativeConstruct 绑定）
void UXxxWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (AXxxCharacter* Char = Cast<AXxxCharacter>(GetOwningPlayerPawn()))
    {
        Char->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(
            UXxxAttributeSet::GetAmmoAttribute())
            .AddUObject(this, &UXxxWidget::OnAmmoChanged);
    }
}

void UXxxWidget::OnAmmoChanged(const FOnAttributeChangeData& Data)
{
    // 更新显示
}
```

---

## 常见踩坑

| 问题 | 解决 |
|---|---|
| BindWidget 编译报错 | 检查 WBP 里控件名与 C++ 变量名大小写是否完全一致 |
| NativeConstruct 时 GetOwningPlayerPawn() 为 null | 延迟到 NativeOnActivated 或 OnVisibilityChanged |
| WBP 折叠时留白 | 背景控件必须在同一个容器内，不能放在 Canvas 最外层 |
| Slate 崩溃（运行时 NewObject Widget） | Widget 只能在 NativeConstruct 阶段创建，不能在 GA/GE 回调里创建 |
