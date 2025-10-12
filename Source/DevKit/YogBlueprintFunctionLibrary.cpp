// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBlueprintFunctionLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "Animation/YogAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <DevKit/Buff/Aura/AuraBase.h>
#include <DevKit/Buff/Aura/AuraDefinition.h>
#include "System/YogGameInstanceBase.h"

UYogBlueprintFunctionLibrary::UYogBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYogBlueprintFunctionLibrary::IsInEditor()
{
	return GIsEditor;
}


FName UYogBlueprintFunctionLibrary::GetDTRow(FString AssetName, int32 rowNum)
{
	FString result;
	result.Append(AssetName);
	result.Append(TEXT("Lvl_"));
	result += FString::Printf(TEXT("%u"), rowNum);

	return FName(result);
}

void UYogBlueprintFunctionLibrary::GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition, UWeaponData* WeaponData)
{
	check(WorldContextObject && ReceivingChar && WeaponDefinition && WeaponData);

		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
	
		WeaponDefinition->SetupWeaponToCharacter(World, AttachTarget, ReceivingChar);

		WeaponData->GrantAbilityToOwner(ReceivingChar);

}

bool UYogBlueprintFunctionLibrary::EquipWeapon(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, AWeaponInstance* weaponInst)
{
	if (WorldContextObject)
	{

		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		//TArray<FYogAbilitySet_GameplayAbility> GrantedGameplayAbilities;

		if (weaponInst)
		{
			for (TSubclassOf<UYogGameplayAbility> gp : weaponInst->WeaponMoves)
			{
				ReceivingChar->GrantGameplayAbility(gp, 0);
			}

			return true;
		}




		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}


}

bool UYogBlueprintFunctionLibrary::GiveAbilityToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UYogAbilitySet* AbilitySet)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		//TArray<FYogAbilitySet_GameplayAbility> GrantedGameplayAbilities;

		for (FYogAbilitySet_GameplayAbility GameAbilitySet : AbilitySet->GrantedGameplayAbilities)
		{
			ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
		}
		
		return true;
	}
	else
	{
		return false;
	}
}
bool UYogBlueprintFunctionLibrary::GiveEffectToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition)
{
	return false;
}

UAnimSequence* UYogBlueprintFunctionLibrary::GetCurrentSlotAnimation(USkeletalMeshComponent* MeshComp, FName SlotName)
{
	if (!MeshComp || !MeshComp->GetAnimInstance()) return nullptr;

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();

	if (!MontageInstance || !MontageInstance->Montage) return nullptr;

	// Get the current position in the montage
	float CurrentPosition = MontageInstance->GetPosition();

	// Find the slot track
	const FSlotAnimationTrack* SlotTrack = MontageInstance->Montage->SlotAnimTracks.FindByPredicate(
		[SlotName](const FSlotAnimationTrack& Track) { return Track.SlotName == SlotName; });

	if (SlotTrack)
	{
		// Find which segment is currently playing
		const FAnimSegment* CurrentSegment = SlotTrack->AnimTrack.GetSegmentAtTime(CurrentPosition);
		if (CurrentSegment && CurrentSegment->GetAnimReference())
		{
			return Cast<UAnimSequence>(CurrentSegment->GetAnimReference());
		}
	}

	return nullptr;
}

UAnimSequence* UYogBlueprintFunctionLibrary::SetSequenceRootMotion(UAnimSequence* sequence, bool enableRoot)
{
	sequence->bRootMotionSettingsCopiedFromMontage = false;
	sequence->EnableRootMotionSettingFromMontage(enableRoot ,  ERootMotionRootLock::RefPose);
	return sequence;
}

EYogCharacterState UYogBlueprintFunctionLibrary::GetCharacterState(AYogCharacterBase* character)
{

	if (character)
	{
		return character->CurrentState;
	}
	else
	{
		return EYogCharacterState::Idle;
	}
}

bool UYogBlueprintFunctionLibrary::LaunchCharacterWithDist(AYogCharacterBase* character, FVector Direction, float Distance, bool bHorizontalOnly, bool bVerticalOnl)
{
	if (character)
	{
		float Speed = FMath::Sqrt(2.f * character->GetCharacterMovement()->GetGravityZ() * Distance);
		character->GetCharacterMovement()->Launch(Direction * Speed);
		return true;
	}
	else
	{
		return false;
	}

}


bool UYogBlueprintFunctionLibrary::SpawnAura(UObject* WorldContextObject, AYogCharacterBase* character, UAuraDefinition* auraDefinition)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		TSubclassOf<AAuraBase> AuraSpawnClass = auraDefinition->AuraClass;

		//FActorSpawnParameters SpawnParams;
		//SpawnParams.Owner = character; // Set owner if needed
		//SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


		AAuraBase* newAura = World->SpawnActorDeferred<AAuraBase>(AuraSpawnClass, FTransform::Identity, character);


		newAura->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		newAura->AttachToActor(character, FAttachmentTransformRules::KeepRelativeTransform);

		return true;
	
	}
	else
	{
		return false;
	}




}

FVector2D UYogBlueprintFunctionLibrary::PerpendicularClockWise(FVector2D vector)
{
	return FVector2D(vector.Y, -vector.X);
}

FVector2D UYogBlueprintFunctionLibrary::PerpendicularAntiClockWise(FVector2D vector)
{
	return FVector2D(-vector.Y, -vector.X);
}

bool UYogBlueprintFunctionLibrary::TargetLocIsInTriangle(FVector targetLoc, FVector pointA, FVector pointB, FVector pointC)
{
	//https://www.baeldung.com/cs/check-if-point-is-in-2d-triangle

	FVector Normal = FVector(0, 0, -1);

	FVector vec_BA = pointA - pointB;
	FVector vec_CB = pointB - pointC;
	FVector vec_AC = pointC - pointA;

	FVector vec_Bp = targetLoc - pointB;
	FVector vec_Cp = targetLoc - pointC;
	FVector vec_Ap = targetLoc - pointA;


	UE_LOG(LogTemp, Warning, TEXT("FVector::CrossProduct(vec_BA, vec_Bp): %f, %f, %f"), FVector::CrossProduct(vec_BA, vec_Bp).X, FVector::CrossProduct(vec_BA, vec_Bp).Y, FVector::CrossProduct(vec_BA, vec_Bp).Z);
	UE_LOG(LogTemp, Warning, TEXT("FVector::CrossProduct(vec_BA, vec_Bp): %f, %f, %f"), FVector::CrossProduct(vec_CB, vec_Cp).X, FVector::CrossProduct(vec_CB, vec_Cp).Y, FVector::CrossProduct(vec_CB, vec_Cp).Z);
	UE_LOG(LogTemp, Warning, TEXT("FVector::CrossProduct(vec_BA, vec_Bp): %f, %f, %f"), FVector::CrossProduct(vec_AC, vec_Ap).X, FVector::CrossProduct(vec_AC, vec_Ap).Y, FVector::CrossProduct(vec_AC, vec_Ap).Z);

	if (FVector::DotProduct(Normal, FVector::CrossProduct(vec_BA, vec_Bp)) > 0 &&
		FVector::DotProduct(Normal, FVector::CrossProduct(vec_CB, vec_Cp)) > 0 &&
		FVector::DotProduct(Normal, FVector::CrossProduct(vec_AC, vec_Ap)) > 0)
	{ 
		return true;
	}
	else
	{
		return false;
	}

}

bool UYogBlueprintFunctionLibrary::DrawDebugTriangle(UObject* WorldContextObject, FVector pointA, FVector pointB, FVector pointC)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		FColor lineColor = FColor(1, 0, 0);
		DrawDebugLine(World, pointA, pointB, lineColor, false, 15.f, 0, 5.f);
		return true;
	}
	else
	{
		return false;
	}

}


FVector UYogBlueprintFunctionLibrary::FunctionFromPoint(UObject* WorldContextObject, FVector pointA, FVector pointB)
{
	int x1 = pointA.X;
	int y1 = pointA.Y;

	int x2 = pointB.X;
	int y2 = pointB.Y;



	float A = y2 - y1;
	float B = x1 - x2;
	float C = x2 * y1 - x1 * y2;

	return FVector(A, B, C);
}

float UYogBlueprintFunctionLibrary::DistFromPointToLine(UObject* WorldContextObject, FVector pointA, FVector Coef)
{
	float A = Coef.X;
	float B = Coef.Y;
	float C = Coef.Z;
	float x = pointA.X;
	float y = pointA.Y;

	float Distance = abs((A * x + B * y + C) / FMath::Sqrt(pow(A, 2) + pow(B, 2)));
	return Distance;
}

TArray<FYogTriangle> UYogBlueprintFunctionLibrary::MakeTriangleArray(UObject* WorldContextObject, TArray<FVector> point_array, FVector Playerloc)
{
	TArray<FYogTriangle> result;
	for (int i = 0; i < point_array.Num(); i++)
	{
		if (i > 0)
		{
			FVector current_Point = point_array[i];
			FVector last_Point = point_array[i - 1];
			FYogTriangle triangle = FYogTriangle(last_Point, current_Point, Playerloc);
			result.Add(triangle);
		}
	}

	return result;
}

bool UYogBlueprintFunctionLibrary::DrawTriangle(UObject* WorldContextObject, FYogTriangle triangle)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		FColor lineColor = FColor(1, 0, 0);

		DrawDebugLine(World, triangle.PointA, triangle.PointB, lineColor, false, 15.f, 0, 5.f);
		DrawDebugLine(World, triangle.PointB, triangle.PointC, lineColor, false, 15.f, 0, 5.f);
		DrawDebugLine(World, triangle.PointC, triangle.PointA, lineColor, false, 15.f, 0, 5.f);
		return true;
	}
	else
	{
		return false;
	}
}

UYogGameInstanceBase* UYogBlueprintFunctionLibrary::GetYogGameInstance(UObject* WorldContextObject)
{

	return  Cast<UYogGameInstanceBase>(WorldContextObject->GetWorld()->GetGameInstance());

}


float UYogBlueprintFunctionLibrary::DistFromPointToTriangle(UObject* WorldContextObject, FVector target_point, FYogTriangle triangle)
{
	// FVector pointA, FVector pointB, FVector pointC
	FVector pointA = triangle.PointA;
	FVector pointB = triangle.PointB;
	FVector pointC = triangle.PointC;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		// Compute vectors
		FVector AB = pointB - pointA;
		FVector AC = pointC - pointA;
		FVector AP = target_point - pointA;

		// Compute dot products
		float d1 = FVector::DotProduct(AB, AP);
		float d2 = FVector::DotProduct(AC, AP);

		// Check if target_point is vertex region outside pointA
		if (d1 <= 0.0f && d2 <= 0.0f)
			return FVector::Dist(target_point, pointA);

		// Check if target_point is in vertex region outside pointB
		FVector BP = target_point - pointB;
		float d3 = FVector::DotProduct(AB, BP);
		float d4 = FVector::DotProduct(AC, BP);
		if (d3 >= 0.0f && d4 <= d3)
			return FVector::Dist(target_point, pointB);

		// Check if target_point is in edge region of AB, if so return distance to AB
		float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
			float v = d1 / (d1 - d3);
			FVector Closesttarget_point = pointA + v * AB;
			return FVector::Dist(target_point, Closesttarget_point);
		}

		// Check if target_point is in vertex region outside pointC
		FVector CP = target_point - pointC;
		float d5 = FVector::DotProduct(AB, CP);
		float d6 = FVector::DotProduct(AC, CP);
		if (d6 >= 0.0f && d5 <= d6)
			return FVector::Dist(target_point, pointC);

		// Check if target_point is in edge region of AC, if so return distance to AC
		float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
			float w = d2 / (d2 - d6);
			FVector Closesttarget_point = pointA + w * AC;
			return FVector::Dist(target_point, Closesttarget_point);
		}

		// Check if target_point is in edge region of BC, if so return distance to BC
		float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
			float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			FVector BC = pointC - pointB;
			FVector Closesttarget_point = pointB + w * BC;
			return FVector::Dist(target_point, Closesttarget_point);
		}

		// target_point is inside face region. Compute distance to plane.
		FVector Normal = FVector::CrossProduct(AB, AC).GetSafeNormal();
		float Distance = FVector::DotProduct(target_point - pointA, Normal);
		return FMath::Abs(Distance);
	}
	else
	{
		return BIG_NUMBER;
	}
}



