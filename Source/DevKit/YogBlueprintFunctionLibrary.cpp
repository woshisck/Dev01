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

bool UYogBlueprintFunctionLibrary::GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
		
		
		for (FWeaponActorToSpawn& WeaponActorInst : WeaponDefinition->ActorsToSpawn)
		{
			TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
			FName Socket = WeaponActorInst.AttachSocket;
			FTransform Transform = WeaponActorInst.AttachTransform;


			AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
			//AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(Transform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
			ReceivingChar->Weapon = NewActor;

		}


		for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
		{
			for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
			{
				ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
			}
		}
		if (WeaponDefinition->WeaponLayer)
		{
			UAnimInstance* AnimInstance = ReceivingChar->GetMesh()->GetAnimInstance();
			AnimInstance->LinkAnimClassLayers(TSubclassOf<UAnimInstance>(WeaponDefinition->WeaponLayer));
		}
		
		return true;
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

bool UYogBlueprintFunctionLibrary::TargetLocIsInTriangle2D(FVector targetLoc, FVector pointA, FVector pointB, FVector pointC)
{
	FVector Normal = FVector(0, 0, -1);

	FVector vec_BA = pointA - pointB;
	FVector vec_CB = pointB - pointC;
	FVector vec_AC = pointC - pointA;

	FVector vec_Bp = targetLoc - pointB;
	FVector vec_Cp = targetLoc - pointC;
	FVector vec_Ap = targetLoc - pointA;

	//FVector::DotProduct(FVector::CrossProduct(vec_BA, vec_Bp))
	//FVector::DotProduct(FVector::CrossProduct(vec_CB, vec_Cp))
	//FVector::DotProduct(FVector::CrossProduct(vec_AC, vec_Ap))



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
