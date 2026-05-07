#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_GetRuneTuningValue.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Get Rune Tuning Value", Category = "BuffFlow|Rune"))
class DEVKIT_API UBFNode_GetRuneTuningValue : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune")
	FName Key;

	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune")
	float DefaultValue = 0.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune")
	FFlowDataPinOutputProperty_Float Value;

	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune")
	FFlowDataPinOutputProperty_Bool bFound;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
