// Fill out your copyright notice in the Description page of Project Settings.


#include "YogLevelScript.h"
#include <DevKit/Map/YogMapDefinition.h>

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
