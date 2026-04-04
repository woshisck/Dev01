#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EffectRegistry.generated.h"

class UYogBuffDefinition;

/**
 * 全局效果注册表 DataAsset
 * 策划创建 DA_EffectRegistry，将 GameplayTag 映射到对应的 UYogBuffDefinition
 * BFNode_AddEffect 节点查询此表，实现"填 Tag 即触发效果"的设计目标
 */
UCLASS(BlueprintType)
class DEVKIT_API UEffectRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Tag → BuffDefinition 映射表。策划填写 Tag（如 Buff.Bleed），系统自动找到对应 DA */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EffectRegistry")
	TMap<FGameplayTag, TObjectPtr<UYogBuffDefinition>> EffectMap;

	/** 根据 Tag 查找 BuffDefinition，未找到返回 nullptr */
	UFUNCTION(BlueprintPure, Category = "EffectRegistry")
	UYogBuffDefinition* FindEffect(FGameplayTag EffectTag) const;
};
