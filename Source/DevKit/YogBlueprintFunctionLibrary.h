// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LatentActions.h"
#include "YogBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
class AYogCharacterBase;

UCLASS()
class UYogBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintPure, Category = Loading)
	static bool IsInEditor();


	UFUNCTION(BlueprintPure, Category = Loading)
	static FName GetDTRow(FString AssetName, int32 rowNum);

	UFUNCTION(BlueprintPure, Category = Weapon)
	static bool GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition);

	UFUNCTION(BlueprintPure, Category = Weapon)
	static bool GiveAbilityToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UYogAbilitySet* AbilitySet);


	UFUNCTION(BlueprintPure, Category = Weapon)
	static bool GiveEffectToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition);

};

class FLatentCountSeconds : public FPendingLatentAction
{

};