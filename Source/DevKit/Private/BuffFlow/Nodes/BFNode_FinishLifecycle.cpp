#include "BuffFlow/Nodes/BFNode_FinishLifecycle.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameModes/YogGameMode.h"
#include "Mob/MobSpawner.h"
#include "TimerManager.h"

UBFNode_FinishLifecycle::UBFNode_FinishLifecycle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Lifecycle");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_FinishLifecycle::ExecuteBuffFlowInput(const FName& PinName)
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		FBuffFlowLifecycleContext& Context = BFC->GetMutableLifecycleContext();
		BFC->RequestLifecycleFinish();
		if (Context.Type == EBuffFlowLifecycleType::Spawn && !Context.bSpawnFinalized)
		{
			if (Context.bStorySpawn && Context.Spawner.IsValid())
			{
				Context.Spawner->HandleLifecycleStoryEnemySpawnFailed(Context);
			}
			else if (UWorld* World = GetWorld())
			{
				if (AYogGameMode* GameMode = World->GetAuthGameMode<AYogGameMode>())
				{
					GameMode->HandleLifecycleEnemySpawnFailed(Context);
				}
			}
		}

		if (bFinishDyingOnDeathLifecycle && Context.Type == EBuffFlowLifecycleType::Death)
		{
			if (AYogCharacterBase* DyingCharacter = Context.DyingCharacter.Get())
			{
				TWeakObjectPtr<AYogCharacterBase> WeakDyingCharacter = DyingCharacter;
				if (UWorld* World = DyingCharacter->GetWorld())
				{
					World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(
						DyingCharacter, [WeakDyingCharacter]()
						{
							if (WeakDyingCharacter.IsValid())
							{
								WeakDyingCharacter->FinishDying();
							}
						}));
				}
			}
		}

		if (Context.Type == EBuffFlowLifecycleType::Spawn)
		{
			if (AActor* Owner = BFC->GetOwner())
			{
				TWeakObjectPtr<AActor> WeakOwner = Owner;
				if (UWorld* World = Owner->GetWorld())
				{
					World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(
						Owner, [WeakOwner]()
						{
							if (WeakOwner.IsValid())
							{
								WeakOwner->Destroy();
							}
						}));
				}
			}
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
