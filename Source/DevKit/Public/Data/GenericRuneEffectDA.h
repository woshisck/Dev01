#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GenericRuneEffectDA.generated.h"

class UTexture2D;
class UFlowAsset;

/**
 * 通用符文效果（如击退/燃烧/冰冻等）
 * 多个符文可引用同一个效果定义，集中维护描述文案与图标
 */
UCLASS(BlueprintType)
class DEVKIT_API UGenericRuneEffectDA : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<UTexture2D> Icon;

	/**
	 * 关联的 BuffFlow 资产（仅作为信息归档，系统不会自动启动）
	 * 策划可在此挂载该效果的实现 FA，便于一处管理"效果说明 + 效果实现"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<UFlowAsset> EffectFlow;
};
