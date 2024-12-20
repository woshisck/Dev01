// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>


#include "YogPlayerControllerBase.generated.h"


/**
 * 
 */
UCLASS()
class DEVKIT_API AYogPlayerControllerBase : public AModularPlayerController
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ASC")
	UYogAbilitySystemComponent* GetYogAbilitySystemComponent() const;
};
