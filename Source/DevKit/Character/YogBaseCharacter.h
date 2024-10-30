// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"

#include "YogBaseCharacter.generated.h"

/**
 * 
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "Yog Character class basement"))
class DEVKIT_API AYogBaseCharacter : public AModularCharacter
{
	GENERATED_BODY()
public:
	AYogBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
