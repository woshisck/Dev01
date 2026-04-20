#include "BuffFlow/Nodes/BFNode_AreaDamage.h"
#include "BuffFlow/Actors/BFAreaDamageZone.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"
#include "Types/FlowDataPinResults.h"
#include "Engine/World.h"
#include "TimerManager.h"

UBFNode_AreaDamage::UBFNode_AreaDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	Delay          = FFlowDataPinInputProperty_Float(0.f);
	Duration       = FFlowDataPinInputProperty_Float(5.f);
	Radius         = FFlowDataPinInputProperty_Float(300.f);
	DamageAmount   = FFlowDataPinInputProperty_Float(10.f);
	DamageInterval = FFlowDataPinInputProperty_Float(1.f);

	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Cancel")) };
	OutputPins = { FFlowPin(TEXT("Spawned")), FFlowPin(TEXT("Completed")),
	               FFlowPin(TEXT("Cancelled")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_AreaDamage::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Cancel"))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DelayTimerHandle);
		}

		if (ABFAreaDamageZone* Zone = SpawnedZone.Get())
		{
			Zone->OnExpiredDelegate.RemoveAll(this);
			Zone->Destroy();
			SpawnedZone.Reset();
		}

		TriggerOutput(TEXT("Cancelled"), true);
		return;
	}

	if (!DamageEffect)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (!AreaActorClass)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const float ResolvedDelay = ResolveFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_AreaDamage, Delay), Delay);

	if (ResolvedDelay <= 0.f)
	{
		SpawnArea();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	World->GetTimerManager().ClearTimer(DelayTimerHandle);
	World->GetTimerManager().SetTimer(DelayTimerHandle, [this]()
	{
		SpawnArea();
	}, ResolvedDelay, false);
}

void UBFNode_AreaDamage::SpawnArea()
{
	AYogCharacterBase* Owner = GetBuffOwner();
	if (!Owner)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const float ResolvedRadius         = ResolveFloat(GET_MEMBER_NAME_CHECKED(UBFNode_AreaDamage, Radius), Radius);
	const float ResolvedDuration       = ResolveFloat(GET_MEMBER_NAME_CHECKED(UBFNode_AreaDamage, Duration), Duration);
	const float ResolvedDamageAmount   = ResolveFloat(GET_MEMBER_NAME_CHECKED(UBFNode_AreaDamage, DamageAmount), DamageAmount);
	const float ResolvedDamageInterval = ResolveFloat(GET_MEMBER_NAME_CHECKED(UBFNode_AreaDamage, DamageInterval), DamageInterval);

	const FVector SpawnLocation = Owner->GetActorLocation() + LocationOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABFAreaDamageZone* Zone = World->SpawnActor<ABFAreaDamageZone>(
		AreaActorClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (!Zone)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	Zone->InitZone(Cast<ACharacter>(Owner), ResolvedRadius, ResolvedDuration,
	               ResolvedDamageAmount, ResolvedDamageInterval, DamageEffect);
	Zone->OnExpiredDelegate.AddDynamic(this, &UBFNode_AreaDamage::OnAreaExpired);
	SpawnedZone = Zone;

	TriggerOutput(TEXT("Spawned"), false);
}

void UBFNode_AreaDamage::OnAreaExpired()
{
	SpawnedZone.Reset();
	TriggerOutput(TEXT("Completed"), true);
}

void UBFNode_AreaDamage::Cleanup()
{
	if (DelayTimerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DelayTimerHandle);
		}
	}

	if (ABFAreaDamageZone* Zone = SpawnedZone.Get())
	{
		Zone->OnExpiredDelegate.RemoveAll(this);
		Zone->Destroy();
		SpawnedZone.Reset();
	}

	Super::Cleanup();
}

float UBFNode_AreaDamage::ResolveFloat(const FName& MemberName,
                                        const FFlowDataPinInputProperty_Float& Default) const
{
	FFlowDataPinResult_Float Result = TryResolveDataPinAsFloat(MemberName);
	return (Result.Result == EFlowDataPinResolveResult::Success)
		? Result.Value
		: Default.Value;
}
