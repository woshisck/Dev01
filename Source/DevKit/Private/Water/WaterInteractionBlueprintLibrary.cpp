#include "Water/WaterInteractionBlueprintLibrary.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Water/WaterInteractionReceiver.h"

int32 UWaterInteractionBlueprintLibrary::AddWaterImpulseAtLocation(
	const UObject* WorldContextObject,
	FVector WorldLocation,
	float Radius,
	float Strength,
	EWaterImpulseType Type,
	FVector Direction,
	float MassScale)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0;
	}

	TArray<AActor*> WaterActors;
	UGameplayStatics::GetAllActorsWithInterface(World, UWaterInteractionReceiver::StaticClass(), WaterActors);

	int32 ReceiverCount = 0;
	for (AActor* WaterActor : WaterActors)
	{
		if (!WaterActor)
		{
			continue;
		}

		if (IWaterInteractionReceiver::Execute_CanReceiveWaterImpulse(WaterActor, WorldLocation, Radius))
		{
			IWaterInteractionReceiver::Execute_AddWaterImpulse(WaterActor, WorldLocation, Radius, Strength, Type, Direction, MassScale);
			++ReceiverCount;
		}
	}

	return ReceiverCount;
}

bool UWaterInteractionBlueprintLibrary::AddWaterImpulseToActor(
	AActor* WaterActor,
	FVector WorldLocation,
	float Radius,
	float Strength,
	EWaterImpulseType Type,
	FVector Direction,
	float MassScale)
{
	if (!WaterActor || !WaterActor->GetClass()->ImplementsInterface(UWaterInteractionReceiver::StaticClass()))
	{
		return false;
	}

	if (!IWaterInteractionReceiver::Execute_CanReceiveWaterImpulse(WaterActor, WorldLocation, Radius))
	{
		return false;
	}

	IWaterInteractionReceiver::Execute_AddWaterImpulse(WaterActor, WorldLocation, Radius, Strength, Type, Direction, MassScale);
	return true;
}
