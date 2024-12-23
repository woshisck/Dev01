// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "YogGameMode.generated.h"


class UYogGameRule;
/**
 * 
 */
UCLASS()
class DEVKIT_API AYogGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
	

public:
	AYogGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void OnExperienceLoaded(const UYogGameRule* CurrentExperience);

};
