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
class AAuraBase;
class UAuraDefinition;
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

	UFUNCTION(BlueprintPure, Category = "Animation|Montage")
	static UAnimSequence* GetCurrentSlotAnimation(USkeletalMeshComponent* MeshComp, FName SlotName);


	UFUNCTION(BlueprintPure, Category = "Animation|Montage")
	static UAnimSequence* SetSequenceRootMotion(UAnimSequence* sequence, bool enableRoot);

	UFUNCTION(BlueprintPure, Category = "YogCharacterBase")
	static EYogCharacterState GetCharacterState(AYogCharacterBase* character);

	UFUNCTION(BlueprintPure, Category = "YogCharacterBase")
	static bool LaunchCharacterWithDist(AYogCharacterBase* character, FVector Direction, float Distance, bool bHorizontalOnly, bool bVerticalOnl);

	UFUNCTION(BlueprintPure, Category = "YogCharacterBase")
	static bool SpawnAura(UObject* WorldContextObject, AYogCharacterBase* character, UAuraDefinition* auraDefinition);

	UFUNCTION(BlueprintPure, Category = "Math")
	static FVector2D PerpendicularClockWise(FVector2D vector);

	UFUNCTION(BlueprintPure, Category = "Math")
	static FVector2D PerpendicularAntiClockWise(FVector2D vector);

	UFUNCTION(BlueprintPure, Category = "Math")
	static bool TargetLocIsInTriangle2D(FVector targetLoc, FVector pointA, FVector pointB, FVector pointC);



};

class FLatentCountSeconds : public FPendingLatentAction
{

};