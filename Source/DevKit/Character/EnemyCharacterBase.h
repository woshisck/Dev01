// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogCharacterBase.h"

#include "EnemyCharacterBase.generated.h"


/**
 * 
 */
UCLASS()
class DEVKIT_API AEnemyCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
public:

	AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
