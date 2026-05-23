#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Water/WaterInteractionTypes.h"
#include "WaterInteractionBlueprintLibrary.generated.h"

UCLASS()
class DEVKIT_API UWaterInteractionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Water", meta = (WorldContext = "WorldContextObject"))
	static int32 AddWaterImpulseAtLocation(
		const UObject* WorldContextObject,
		FVector WorldLocation,
		float Radius,
		float Strength,
		EWaterImpulseType Type,
		FVector Direction,
		float MassScale);

	UFUNCTION(BlueprintCallable, Category = "Water")
	static bool AddWaterImpulseToActor(
		AActor* WaterActor,
		FVector WorldLocation,
		float Radius,
		float Strength,
		EWaterImpulseType Type,
		FVector Direction,
		float MassScale);
};
