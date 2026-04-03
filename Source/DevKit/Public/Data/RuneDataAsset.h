#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "CharacterData.h"
#include "RuneDataAsset.generated.h"

class UFlowAsset;


/**
 * 单条属性修改器 —— DA 里的"属性字典"条目
 * 在编辑器中可直接下拉选择属性、操作类型、填写数值
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FRuneAttributeModifier
{
    GENERATED_BODY()

    /** 要修改的属性（下拉选择，如 BaseAttributeSet.Attack） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FGameplayAttribute Attribute;

    /** 操作类型：Additive / Multiplicative / Override */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

    /** 数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    float Value = 0.f;
};


USTRUCT(BlueprintType)
struct DEVKIT_API FRuneShape
{
    GENERATED_BODY()

    // 相对于 Pivot 点的格子偏移列表，X=列偏移，Y=行偏移
    // 例：L形 = {(0,0),(0,1),(0,2),(1,2)}
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
    TArray<FIntPoint> Cells;

    int32 GetCellCount() const { return Cells.Num(); }

    // 顺时针旋转90度，返回新的 FRuneShape
    FRuneShape Rotate90() const;
};



USTRUCT(BlueprintType)
struct DEVKIT_API FRuneInstance
{
    GENERATED_BODY()

    // --- 显示信息 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FName RuneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    TObjectPtr<UTexture2D> RuneIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText RuneDescription;

    // --- 形状 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    FRuneShape Shape;

    // --- 数值效果（属性字典） ---
    // 在 DA 编辑器里直接配置：选属性、选操作类型、填数值
    // 运行时会自动构建 GE 并 Apply，不需要预制 GE 资产
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FRuneAttributeModifier> AttributeModifiers;

    // --- 行为效果 ---
    // 非数值效果的 GE（如击退、持续伤害等需要 Granted Ability 的 GE）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TSubclassOf<UGameplayEffect> BehaviorEffect;

    // --- BuffFlow 可视化逻辑 ---
    // 可选：引用一个 FlowAsset，符文激活时自动启动 Flow
    // 用于复杂条件逻辑（如：命中5次后触发、血量低于30%时...）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TObjectPtr<UFlowAsset> BuffFlowAsset;

    // --- 运行时数据 ---
    UPROPERTY(BlueprintReadWrite)
    int32 Level = 1;

    // 每个实例唯一标识，用于查找和移除
    UPROPERTY(BlueprintReadWrite)
    FGuid RuneGuid;
};

UCLASS(BlueprintType)
class DEVKIT_API URuneDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
    FRuneInstance RuneTemplate;

    // 从模板创建实例（自动生成 Guid）
    FRuneInstance CreateInstance() const;
};