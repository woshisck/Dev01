#include "BuffFlow/Nodes/BFNode_SpawnEnemyFromContext.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "GameModes/YogGameMode.h"
#include "Mob/MobSpawner.h"

UBFNode_SpawnEnemyFromContext::UBFNode_SpawnEnemyFromContext(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Lifecycle");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Spawned")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnEnemyFromContext::ExecuteBuffFlowInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FBuffFlowLifecycleContext& Context = BFC->GetMutableLifecycleContext();
	if (Context.Type != EBuffFlowLifecycleType::Spawn || !Context.Spawner.IsValid() || !Context.EnemyClass)
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
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AEnemyCharacterBase* SpawnedEnemy = Context.Spawner->SpawnMobAtLocationWithWeapon(
		Context.EnemyClass,
		Context.SpawnTransform.GetLocation(),
		Context.EnemyWeaponDefinition.Get());
	if (!SpawnedEnemy)
	{
		if (Context.bStorySpawn)
		{
			Context.Spawner->HandleLifecycleStoryEnemySpawnFailed(Context);
		}
		else if (UWorld* World = Context.Spawner->GetWorld())
		{
			if (AYogGameMode* GameMode = World->GetAuthGameMode<AYogGameMode>())
			{
				GameMode->HandleLifecycleEnemySpawnFailed(Context);
			}
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	Context.SpawnedEnemy = SpawnedEnemy;
	Context.LifecycleTarget = SpawnedEnemy;
	BFC->SetLifecycleTarget(SpawnedEnemy);

	if (UWorld* World = SpawnedEnemy->GetWorld())
	{
		if (Context.bStorySpawn)
		{
			Context.Spawner->HandleLifecycleStoryEnemySpawned(SpawnedEnemy, Context);
		}
		else if (AYogGameMode* GameMode = World->GetAuthGameMode<AYogGameMode>())
		{
			GameMode->HandleLifecycleEnemySpawned(SpawnedEnemy, Context);
		}
	}

	TriggerOutput(TEXT("Spawned"), true);
}
