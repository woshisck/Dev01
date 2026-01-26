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
struct FYogCollisionAnnulus
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Center = FVector(0,0,0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float inner_radius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float outer_radius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float start_angle_degree = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float end_angle_degree = 0;


};

USTRUCT(BlueprintType)
struct FYogCollisionCircle
{
	GENERATED_BODY()
	FYogCollisionCircle()
		:Radius(0), Offset(FVector(0, 0, 0))
	{
	}
	FYogCollisionCircle(float a, FVector b)
		:Radius(a), Offset(b)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Offset = FVector(0, 0, 0);

};

USTRUCT(BlueprintType)
struct FYogCollisionSquare
{
	GENERATED_BODY()
	FYogCollisionSquare()
		:Width(0), Length(0)
	{
	}
	FYogCollisionSquare(float a, float b)
		:Width(a), Length(b)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Length = 0;
};

USTRUCT(BlueprintType)
struct FYogCollisionTriangle
{
	GENERATED_BODY()
	FYogCollisionTriangle()
		:PointA(FVector(0, 0, 0)), PointB(FVector(0, 0, 0)), PointC(FVector(0, 0, 0))
	{
	}
	FYogCollisionTriangle(FVector a, FVector b, FVector c)
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
	GENERATED_BODY()
public:

	UYogBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
	}

	UFUNCTION(BlueprintPure, Category = Loading)
	static bool IsInEditor();
	
	UFUNCTION(BlueprintPure, Category = Loading)
	static FName GetDTRow(FString AssetName, int32 rowNum);

	UFUNCTION(BlueprintCallable, Category = "GAS")
	static void PrintGameplayTagContainer(FGameplayTagContainer gameplaytag_container);

	UFUNCTION(BlueprintCallable, Category = "YogMath")
	static TArray<int> RandomSplitInteger(int Total, int NumberParts);

	UFUNCTION(BlueprintCallable)
	static TArray<AYogCharacterBase*> GetAllActorsFromRange(UObject* WorldContextObject, FVector SphereCenter, float SphereRadius, TSubclassOf<AYogCharacterBase> EnemyClass);

	UFUNCTION(BlueprintCallable)
	static TArray<AActor*> ASync_GetAllActorsFromRange(UObject* WorldContextObject, const FVector& Center, float Radius, TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable)
	static void FindAllCharacters(UObject* WorldContextObject, TArray<AYogCharacterBase*>& OutEnemies);



	////////////////////////////////////////////////// Weapon Ability //////////////////////////////////////////////////
	UFUNCTION(BlueprintCallable, Category = Weapon)
	static void GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition, UWeaponData* WeaponData);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	static void EquipWeapon(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, AWeaponInstance* WeaponInstance);


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

	////////////////////////////////////////////////// Debug //////////////////////////////////////////////////
	UFUNCTION(BlueprintCallable, Category = "Math")
	static void DrawDebugAnnulusSector(UObject* WorldContextObject, FVector Center, float InnerRadius, float OuterRadius, float StartAngleDeg, float EndAngleDeg, FColor Color);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static bool DrawDebugTriangle(UObject* WorldContextObject, FVector pointA, FVector pointB, FVector pointC);


	////////////////////////////////////////////////// Collision //////////////////////////////////////////////////
	UFUNCTION(BlueprintCallable, Category = "Math")
	static float DistFromPointToTriangle(UObject* WorldContextObject, FVector target_point, FYogCollisionTriangle triangle);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static float DistFromPointToAnnulus(UObject* WorldContextObject, FVector target_point, FYogCollisionAnnulus annulus);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static float DistFromPointToSquare(UObject* WorldContextObject, FVector target_point, FYogCollisionSquare square);


	UFUNCTION(BlueprintCallable, Category = "Math")
	static FVector DistFromPointToPoint(UObject* WorldContextObject,FVector pointA, FVector pointB);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static float DistFromPointToLine(UObject* WorldContextObject, FVector pointA, FVector Coef);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static TArray<FYogCollisionTriangle> MakeTriangleArray(UObject* WorldContextObject, TArray<FVector> point_array, FVector Playerloc);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static bool DrawTriangle(UObject* WorldContextObject, FYogCollisionTriangle triangle);

	UFUNCTION(BlueprintCallable, Category = "System")
	static UYogGameInstanceBase* GetYogGameInstance(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Effect")
	static TArray<FYogApplyEffect> MergeApplyEffects(const TArray<FYogApplyEffect>& SourceArray);

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