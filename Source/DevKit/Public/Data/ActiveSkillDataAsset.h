#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Engine/DataAsset.h"
#include "ActiveSkillDataAsset.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct DEVKIT_API FActiveSkillConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Skill")
	FName SkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Skill")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Skill")
	TSubclassOf<UYogGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Skill", meta = (ClampMin = "0.0"))
	float Cooldown = 120.0f;
};

UCLASS(BlueprintType)
class DEVKIT_API UActiveSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Active Skill")
	FActiveSkillConfig Config;
};
