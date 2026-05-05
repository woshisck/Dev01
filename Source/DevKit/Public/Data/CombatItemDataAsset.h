#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatItemDataAsset.generated.h"

class UTexture2D;
class UNiagaraSystem;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ECombatItemEffectType : uint8
{
	OilBottle    UMETA(DisplayName = "Oil Bottle"),
	ThunderStone UMETA(DisplayName = "Thunder Stone"),
	SmokeBomb    UMETA(DisplayName = "Smoke Bomb")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatItemOilConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0"))
	float OilBladeDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0"))
	float StickyDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "1"))
	int32 MaxStickyStacks = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MoveSpeedSlowPerStack = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TurnSpeedSlowPerStack = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AttackSpeedSlowPerStack = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0"))
	float FireBonusDamagePerStack = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (ClampMin = "0.0"))
	float FireBonusDamageCap = 30.0f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatItemThunderConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thunder", meta = (ClampMin = "0.0"))
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thunder", meta = (ClampMin = "0.0"))
	float Radius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thunder", meta = (ClampMin = "0.01"))
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thunder", meta = (ClampMin = "0.0"))
	float DamagePerTick = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thunder")
	bool bInterruptMinions = true;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatItemSmokeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (ClampMin = "0.0"))
	float Duration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (ClampMin = "0.0"))
	float InitialRadius = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (ClampMin = "0.0"))
	float FinalRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (ClampMin = "0.01"))
	float TickInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeBonus = 0.5f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatItemConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	ECombatItemEffectType EffectType = ECombatItemEffectType::OilBottle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0"))
	int32 MaxCharges = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0"))
	int32 InitialCharges = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0.0"))
	float Cooldown = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|VFX")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|VFX")
	TObjectPtr<UMaterialInterface> DecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Oil")
	FCombatItemOilConfig Oil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Thunder")
	FCombatItemThunderConfig Thunder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Smoke")
	FCombatItemSmokeConfig Smoke;
};

UCLASS(BlueprintType)
class DEVKIT_API UCombatItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Item")
	FCombatItemConfig Config;
};
