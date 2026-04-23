#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/RuneDataAsset.h"
#include "AltarDataAsset.generated.h"

USTRUCT(BlueprintType)
struct DEVKIT_API FAltarSacrificeEntry
{
	GENERATED_BODY()

	// 玩家通过献祭可获得的符文
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<URuneDataAsset> GrantedRune;

	// 向玩家展示的代价描述文字（代价逻辑由 GrantedRune 的 FA 执行）
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText CostDescription;
};

UCLASS()
class DEVKIT_API UAltarDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 献祭符文候选池（三选一随机抽取）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar|Sacrifice")
	TArray<FAltarSacrificeEntry> SacrificeRunePool;

	// 净化功能开关
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar")
	bool bPurificationEnabled = true;

	// 献祭功能开关
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar")
	bool bSacrificeEnabled = true;
};
