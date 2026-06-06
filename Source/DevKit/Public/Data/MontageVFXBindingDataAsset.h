#pragma once

#include "CoreMinimal.h"
#include "Component/MontageVFXBindingComponent.h"
#include "Engine/DataAsset.h"
#include "MontageVFXBindingDataAsset.generated.h"

USTRUCT(BlueprintType)
struct DEVKIT_API FMontageVFXBindingAssetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FName SlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FMontageVFXBindingConfig Config;
};

UCLASS(BlueprintType)
class DEVKIT_API UMontageVFXBindingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FMontageVFXBindingConfig* ResolveBinding(FName SlotName) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults",
		meta = (ToolTip = "Used when no named entry matches the requested slot."))
	bool bUseFallbackConfig = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults",
		meta = (EditCondition = "bUseFallbackConfig", EditConditionHides))
	FMontageVFXBindingConfig FallbackConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	TArray<FMontageVFXBindingAssetEntry> Bindings;
};
