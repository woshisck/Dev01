#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YogPerformanceAuthoringData.generated.h"

USTRUCT(BlueprintType)
struct DEVKIT_API FYogMaterialTextureNamingRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FName ChannelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FName CanonicalParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	TArray<FString> AcceptedSuffixes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	bool bSRGB = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material", meta = (ClampMin = "1"))
	int32 RecommendedMaxTextureSize = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	bool bRequirePowerOfTwo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	bool bPreferVirtualTextureForBatchedEnvironment = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FString CompressionIntent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FString Packing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FString Notes;
};

UCLASS(BlueprintType)
class DEVKIT_API UYogMaterialTextureNamingConvention : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FString Schema = TEXT("DevKit.MaterialTextureNamingConvention.v1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	FString RuleUsage = TEXT("Material texture tools classify artist-provided textures into canonical channels. Ordinary scene model, building, and prop textures default to NoVT Texture2D. Only ground Runtime Virtual Texture assets use the VT system; texture grouping should use ordinary TextureCollection.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Material")
	TArray<FYogMaterialTextureNamingRule> Rules;
};
