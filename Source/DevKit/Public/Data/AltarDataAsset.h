#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/RuneDataAsset.h"
#include "AltarDataAsset.generated.h"

UENUM(BlueprintType)
enum class ESacrificeOfferingCostType : uint8
{
	SacrificeDeckCard UMETA(DisplayName = "Sacrifice Deck Card"),
	AttackUpDamageTakenUp UMETA(DisplayName = "Attack Up / Damage Taken Up"),
	CritRateDownCritDamageUp UMETA(DisplayName = "Crit Rate Down / Crit Damage Up"),
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESacrificeOfferingCostType CostType = ESacrificeOfferingCostType::SacrificeDeckCard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float PrimaryMagnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float SecondaryMagnitude = 0.0f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FSacrificeOfferingCostState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESacrificeOfferingCostType CostType = ESacrificeOfferingCostType::AttackUpDamageTakenUp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DmgTakenDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CritRateDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CritDamageDelta = 0.0f;
};

UCLASS()
class DEVKIT_API UAltarDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar|Sacrifice")
	TObjectPtr<URuneDataAsset> EventSacrificeRune;

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
