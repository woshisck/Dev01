// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatureAction_AddSpawnObject.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"


void UGameFeatureAction_AddSpawnObject::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);
	Reset();
}

void UGameFeatureAction_AddSpawnObject::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	Reset();
}

void UGameFeatureAction_AddSpawnObject::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = nullptr;
#if WITH_EDITOR
	if (GIsEditor)
	{
		if (GPlayInEditorID == -1)
		{
			FWorldContext* WorldContext = GEditor->GetPIEWorldContext(1);
			if (WorldContext != nullptr)
			{
				World = WorldContext->World();
			}
			else
			{
				if (UGameViewportClient* Viewport = GEngine->GameViewport)
				{
					World = Viewport->GetWorld();
				}
			}
		}
		else
		{
			FWorldContext* WorldContext = GEditor->GetPIEWorldContext(GPlayInEditorID);
			if (WorldContext == nullptr)
			{
				World = nullptr;
			}
			World = WorldContext->World();
		}
	}
	else
	{
		World = GEngine->GetCurrentPlayWorld(nullptr);
	}
#else
	World = GEngine->GetCurrentPlayWorld(nullptr);

	/*UWorld* World = GEditor->GetEditorWorldContext().World();*/
#endif

	UWorld* World = WorldContext.World();

	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	if ((World != nullptr) && World->IsGameWorld())
	{
		
		for (const FSpawningWorldActorsEntry& Entry : ActorsList)
		{
			if (!Entry.TargetWorld.IsNull())
			{
				UWorld* TargetWorld = Entry.TargetWorld.Get();
				if (TargetWorld != World)
				{
					// This system is intended for a specific world (not this one)
					continue;
				}
			}

			for (const FSpawningActorEntry& ActorEntry : Entry.Actors)
			{
				UE_LOG(LogTemp, Warning, TEXT("ActorEntry:"));
				AActor* NewActor = World->SpawnActor<AActor>(ActorEntry.ActorType, ActorEntry.SpawnTransform);
				SpawnedActors.Add(NewActor);
			}
		}
	}
}

void UGameFeatureAction_AddSpawnObject::Reset()
{
	for (TWeakObjectPtr<AActor>& ActorPtr : SpawnedActors)
	{
		if (ActorPtr.IsValid())
		{
			ActorPtr->Destroy();
		}
	}
}