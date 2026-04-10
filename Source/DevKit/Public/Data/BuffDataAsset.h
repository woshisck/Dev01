#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "BuffDataAsset.generated.h"

/**
 * UBuffDataAsset — 关卡词条 Buff 配置
 *
 * 命名规范：DA_Buff_<效果名>，例：DA_Buff_ArmorBreak、DA_Buff_Frenzy
 *
 * 关卡结算时从 RoomDataAsset.BuffPool 随机选取若干条目，
 * 施加给本关刷出的所有敌人。
 */
UCLASS(BlueprintType)
class DEVKIT_API UBuffDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Buff 名称（UI 展示用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
	FName BuffName;

	// Buff 说明（UI 展示用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
	FText BuffDescription;

	// 实际施加给敌人的 GameplayEffect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
	TSubclassOf<UGameplayEffect> BuffEffect;
};
