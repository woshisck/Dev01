// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "YogLevelScript.generated.h"

class UYogMapDefinition;

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()
protected:

	virtual void PreInitializeComponents() override;
	void LevelSetup(UYogMapDefinition& MapDefine);
	


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UYogMapDefinition> Mapdefinition;
};
