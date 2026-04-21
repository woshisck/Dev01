# DataAsset 规范

> Claude 在设计新的 DataAsset 类型或添加字段前必须阅读。

---

## 核心规则

1. **DA 是策划的填表工具**，C++ 只定义字段类型，**不填默认值作为业务配置**
2. **命名约定**：资产文件名 `DA_` 前缀，C++ 类名无前缀约定（UXxxDataAsset）
3. **软引用 vs 硬引用**：引用大资产（Mesh / Texture / Montage）用 `TSoftObjectPtr`，避免启动时全部加载
4. **颜色字段**用 `FLinearColor`，不用 `FColor`（精度问题）
5. **DA 不存逻辑**，只存数据。逻辑在 C++ 或 FA 里

---

## 分工

### Claude 写：
- DA C++ 类（继承 `UPrimaryDataAsset` 或 `UDataAsset`）
- 字段定义（UPROPERTY + 清晰 Category 分组 + Tooltip）
- 读取 DA 字段的 C++ 代码

### 用户在引擎里做：
- 右键 Content Browser → Miscellaneous → Data Asset → 选择 C++ 类
- 填写所有字段
- 在 BP / Character Details 里将 DA 引用赋给对应 UPROPERTY

---

## DA 类标准结构

```cpp
UCLASS()
class DEVKIT_API UXxxDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    // ── 分类清晰，方便策划在编辑器里找到 ──────────

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float BaseDamage = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "Visual")
    FLinearColor GlowColor = FLinearColor(1.f, 0.5f, 0.f);

    // 软引用大资产（蒙太奇/贴图/Mesh）
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> AttackMontage;

    // 数组字段
    UPROPERTY(EditDefaultsOnly, Category = "Abilities")
    TArray<FGameplayTag> GrantedAbilities;
};
```

---

## UPrimaryDataAsset vs UDataAsset

| 基类 | 适用场景 |
|---|---|
| `UPrimaryDataAsset` | 有资产 ID 概念、需要 Asset Manager 管理的（如符文 DA、武器 DA） |
| `UDataAsset` | 简单配置表、全局共享参数（如 BackpackStyleDataAsset） |

---

## 现有 DA 类速查

| DA 类 | 资产名示例 | 用途 |
|---|---|---|
| `UBackpackStyleDataAsset` | `DA_BackpackStyle` | 热度颜色、激活区颜色、背包格透明度 |
| `UWeaponDefinition` | `DA_Weapon_Musket` | 武器类型、弹药上限、GA Tag 列表 |
| `UCharacterDataAsset` | `DA_PlayerOne` | 角色属性表、AbilityData Map（GA Tag→蒙太奇） |
| `UBuffDataAsset` | `DA_Buff_Fire` | 关卡 Buff 池条目 |
| `UCampaignDataAsset` | `DA_Campaign_Act1` | 关卡顺序、传送门分支配置 |
| `URuneDataAsset` | `DA_Rune_1001` | 符文 ID、名称、图标、FA 引用 |

---

## 字段 Category 约定

| Category | 内容 |
|---|---|
| `Combat` | 伤害、CD、充能次数、韧性 |
| `Visual` | 颜色、粒子、材质引用 |
| `Animation` | 蒙太奇、AnimBP |
| `UI` | 图标、显示文本、颜色 |
| `Abilities` | GA Tag 列表、GE 引用 |
| `Progression` | 等级、解锁条件 |
