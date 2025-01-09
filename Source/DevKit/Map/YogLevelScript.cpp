// Fill out your copyright notice in the Description page of Project Settings.


#include "YogLevelScript.h"
#include <DevKit/Map/YogMapDefinition.h>

void AYogLevelScript::PreInitializeComponents()
{
	UE_LOG(LogTemp, Warning, TEXT("Start pre initialize component"));
}

void AYogLevelScript::LevelSetup(UYogMapDefinition& MapDefine)
{
}
