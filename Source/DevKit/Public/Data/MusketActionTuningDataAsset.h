#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MusketActionTuningDataAsset.generated.h"

UCLASS(BlueprintType, Blueprintable, DisplayName = "Musket Action Tuning")
class DEVKIT_API UMusketActionTuningDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Attack", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float LightDamageMultiplier = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Attack", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float LightHalfAngleDeg = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "0.3", ClampMax = "5.0"))
	float HeavyChargeTime = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "1.0", ClampMax = "90.0"))
	float HeavyStartHalfAngle = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "0.5", ClampMax = "45.0"))
	float HeavyEndHalfAngle = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "50.0"))
	float HeavyStartRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "50.0"))
	float HeavyEndRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "0.1"))
	float HeavyBaseDamageMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Attack", meta = (ClampMin = "0.1"))
	float HeavyFullChargeMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint Attack", meta = (ClampMin = "1.0", ClampMax = "90.0"))
	float SprintHalfFanAngle = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint Attack", meta = (ClampMin = "0.1"))
	float SprintDamageMultiplier = 0.6f;
};
