// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "YogLevelScript.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLevelStart);

class UYogMapDefinition;

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MonsterKillCountTarget;

protected:

	virtual void PreInitializeComponents() override;
	void LevelSetup(UYogMapDefinition& MapDefine);
	


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UYogMapDefinition> Mapdefinition;

	UPROPERTY(BlueprintAssignable)
	FLevelStart LevelStart;

	void OnLevelLoaded(UWorld* LoadedWorld);

	virtual void BeginPlay();

};
