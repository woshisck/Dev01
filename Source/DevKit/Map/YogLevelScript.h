// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "YogLevelScript.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()
protected:

	virtual void PreInitializeComponents() override;

	virtual void EnableInput(class APlayerController* PlayerController) override;
	virtual void DisableInput(class APlayerController* PlayerController) override;
};
