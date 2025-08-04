// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../Character/PlayerCharacterBase.h"
#include "../Item/Item.h"
#include "TriggerItemInterface.generated.h"

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemInteractEndDelegate, APlayerCharacterBase*, Character);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemInteractStartDelegate, APlayerCharacterBase*, Character, AActor*, actor);


// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UTriggerItemInterface : public UInterface
{
	GENERATED_BODY()
};




class DEVKIT_API ITriggerItemInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void ITriggerItemStart();
	virtual void ITriggerItemEnd();
};
