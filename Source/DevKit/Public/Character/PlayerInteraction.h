// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerInteraction.generated.h"

class APlayerCharacterBase;

UINTERFACE(MinimalAPI)
class UPlayerInteraction : public UInterface
{
	GENERATED_BODY()
};

class DEVKIT_API IPlayerInteraction
{
	GENERATED_BODY()

public:
	virtual void OnPlayerBeginOverlap(APlayerCharacterBase* Player) = 0;
	virtual void OnPlayerEndOverlap(APlayerCharacterBase* Player) = 0;
};
