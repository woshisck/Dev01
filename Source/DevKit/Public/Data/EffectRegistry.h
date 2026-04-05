#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EffectRegistry.generated.h"

class URuneDataAsset;

/**
 * 全局效果注册表 DataAsset
 * 策划创建 DA_EffectRegistry，将 GameplayTag 映射到对应的 URuneDataAsset
 * BFNode_AddEffect 节点查询此表，实现"填 Tag 即触发效果"的设计目标
 */
UCLASS(BlueprintType)
class DEVKIT_API UEffectRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Tag → RuneDataAsset 映射表。策划填写 Tag（如 Buff.Bleed），系统自动找到对应 DA */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EffectRegistry")
	TMap<FGameplayTag, TObjectPtr<URuneDataAsset>> EffectMap;

	/** 根据 Tag 查找 RuneDataAsset，未找到返回 nullptr */
	UFUNCTION(BlueprintPure, Category = "EffectRegistry")
	URuneDataAsset* FindEffect(FGameplayTag EffectTag) const;
};
