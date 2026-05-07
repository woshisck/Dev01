#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SpawnRuneAreaProfile.generated.h"

class URuneCardEffectProfileDA;

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Rune Area Profile", Category = "BuffFlow|Profile"))
class DEVKIT_API UBFNode_SpawnRuneAreaProfile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Profile")
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

	UPROPERTY(EditAnywhere, Category = "Profile|Position", meta = (ToolTip = "Optional data pin. If linked, this world location overrides profile area position calculation."))
	FFlowDataPinInputProperty_Vector SpawnLocationOverride;

	UPROPERTY(EditAnywhere, Category = "Profile|Position", meta = (ToolTip = "Optional data pin. If linked, this world rotation overrides profile area facing."))
	FFlowDataPinInputProperty_Rotator SpawnRotationOverride;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	bool ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const;
};
