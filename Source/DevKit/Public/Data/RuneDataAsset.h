#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterData.h"
#include "RuneDataAsset.generated.h"


// Source/DevKit/Public/Rune/RuneData.h

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

    // --- 效果 ---
    // 激活时施加的 GameplayEffect（继承自 UYogGameplayEffect）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TSubclassOf<UGameplayEffect> ActivationEffect;

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