// Fill out your copyright notice in the Description page of Project Settings.


#include "YogLevelScript.h"
#include <DevKit/Map/YogMapDefinition.h>
#include "Portal.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

void AYogLevelScript::PreInitializeComponents()
{
	UE_LOG(LogTemp, Warning, TEXT("Start pre initialize component"));
	
	FWorldDelegates::OnPostWorldCreation.AddUObject(this, &AYogLevelScript::OnLevelLoaded);

	
}

void AYogLevelScript::LevelSetup(UYogMapDefinition& MapDefine)
{
	
}

void AYogLevelScript::OnLevelLoaded(UWorld* LoadedWorld)
{
	LevelStart.Broadcast();
	/*LevelSetup(this->Mapdefinition);*/
}

void AYogLevelScript::BeginPlay()
{
	Super::BeginPlay();
	// Array to store found actors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		for (AActor* portalActor : FoundActors)
		{
			APortal* portal = Cast<APortal>(portalActor);
			for (const FPortalEntry& level_portal : this->Mapdefinition->LevelPortals)
			{
				
				int gate_index = level_portal.GateIndex;
				
				if (gate_index == portal->Index)
				{
					portal->NextLevels = level_portal.NextLevels;
				}

			}
			//for(const int& portal_index : this->Mapdefinition.levelportals)
			//int portal_index = this->Mapdefinition->LevelPortals
		}
		

	}

}