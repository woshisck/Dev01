#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Water/WaterInteractionTypes.h"
#include "WaterInteractionReceiver.generated.h"

UINTERFACE(BlueprintType)
class DEVKIT_API UWaterInteractionReceiver : public UInterface
{
	GENERATED_BODY()
};

class DEVKIT_API IWaterInteractionReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Water")
	bool CanReceiveWaterImpulse(FVector WorldLocation, float Radius) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Water")
	void AddWaterImpulse(
		FVector WorldLocation,
		float Radius,
		float Strength,
		EWaterImpulseType Type,
		FVector Direction,
		float MassScale);
};
