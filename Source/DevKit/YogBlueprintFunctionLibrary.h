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
class UYogWorldSubsystem;
class UYogGameInstanceBase;

USTRUCT(BlueprintType)
struct FYogTriangle
{
	GENERATED_BODY()
	FYogTriangle()
		:PointA(FVector(0, 0, 0)), PointB(FVector(0, 0, 0)), PointC(FVector(0, 0, 0))
	{
	}
	FYogTriangle(FVector a, FVector b, FVector c)
		:PointA(a), PointB(b), PointC(c)
	{
	}

	// What kind of actor to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PointA;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PointB;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PointC;

};

UCLASS()
class UYogBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintPure, Category = Loading)
	static bool IsInEditor();
	
	UFUNCTION(BlueprintPure, Category = Loading)
	static FName GetDTRow(FString AssetName, int32 rowNum);

	////////////////////////////////////////////////// Weapon Ability //////////////////////////////////////////////////
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static void GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition, UWeaponData* WeaponData);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	static void EquipWeapon(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponData* WeaponData);


	UFUNCTION(BlueprintCallable, Category = Weapon)
	static void GiveAbilityToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UAbilityData* AbilityData);


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
	static bool TargetLocIsInTriangle(FVector targetLoc, FVector pointA, FVector pointB, FVector pointC);


	UFUNCTION(BlueprintCallable, Category = "Math")
	static bool DrawDebugTriangle(UObject* WorldContextObject, FVector pointA, FVector pointB, FVector pointC);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static float DistFromPointToTriangle(UObject* WorldContextObject, FVector target_point, FYogTriangle triangle);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static FVector FunctionFromPoint(UObject* WorldContextObject,FVector pointA, FVector pointB);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static float DistFromPointToLine(UObject* WorldContextObject, FVector pointA, FVector Coef);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static TArray<FYogTriangle> MakeTriangleArray(UObject* WorldContextObject, TArray<FVector> point_array, FVector Playerloc);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static bool DrawTriangle(UObject* WorldContextObject, FYogTriangle triangle);

	UFUNCTION(BlueprintCallable, Category = "System")
	static UYogGameInstanceBase* GetYogGameInstance(UObject* WorldContextObject);


	template<typename T>

	static T* GetSubsystem(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		UWorld* World = WorldContextObject->GetWorld();
		if (World)
		{
			return nullptr;
		}

		// Try different subsystem types
		if (T::StaticClass()->IsChildOf<UGameInstanceSubsystem>())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				return GameInstance->GetSubsystem<T>();
			}
		}
		else if (T::StaticClass()->IsChildOf<UWorldSubsystem>())
		{
			return World->GetSubsystem<T>();
		}
		else if (T::StaticClass()->IsChildOf<ULocalPlayerSubsystem>())
		{
			// Need a player context for this
			return nullptr;
		}

		return nullptr;
	}

};

class FLatentCountSeconds : public FPendingLatentAction
{

};