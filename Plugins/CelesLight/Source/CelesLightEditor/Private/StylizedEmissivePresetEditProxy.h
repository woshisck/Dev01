#pragma once

#include "CoreMinimal.h"
#include "StylizedEmissiveModelLibrary.h"
#include "StylizedEmissivePresetEditProxy.generated.h"

/** Transient property panel used by the artist-facing library tool. */
UCLASS(Transient)
class UStylizedEmissivePresetEditProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "1. 预设")
	FName PresetId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "1. 预设")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "1. 预设", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, Category = "2. 模型")
	bool bUseMesh = true;

	UPROPERTY(EditAnywhere, Category = "2. 模型", meta = (EditCondition = "bUseMesh", EditConditionHides))
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "2. 模型", meta = (EditCondition = "bUseMesh", EditConditionHides))
	TObjectPtr<UMaterialInterface> EmissiveMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "2. 模型", meta = (EditCondition = "bUseMesh", EditConditionHides))
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "3. 灯光")
	EStylizedEmissiveLightingOutput LightingOutput = EStylizedEmissiveLightingOutput::StylizedMaterial;

	UPROPERTY(EditAnywhere, Category = "3. 灯光")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "3. 灯光", meta = (ClampMin = "0.0", UIMax = "10000.0"))
	float Intensity = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "3. 灯光", meta = (ClampMin = "0.0", UIMax = "100.0", EditCondition = "bUseMesh", EditConditionHides))
	float EmissiveIntensity = 20.0f;

	UPROPERTY(EditAnywhere, Category = "3. 灯光", meta = (ClampMin = "1.0", UIMax = "5000.0"))
	float AttenuationRadius = 1000.0f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "4. 高级")
	bool bFillLight = false;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "4. 高级", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMin = 0.0f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "4. 高级", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMax = 1.0f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "4. 高级")
	float SpecularOffset = 0.0f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "4. 高级")
	int32 EffectType = 0;

	void LoadFromEntry(const FStylizedEmissiveModelEntry& Entry);
	void WriteToEntry(FStylizedEmissiveModelEntry& Entry) const;
};
