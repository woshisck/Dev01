// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterBase.h"



#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Inventory/InventoryManagerComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"
#include <DevKit/Player/YogPlayerControllerBase.h>
#include "../Camera/YogCameraPawn.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "../Buff/Aura/AuraBase.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{
	EnemyCloseDist = 100.0f;


}

void APlayerCharacterBase::SetOwnCamera(AYogCameraPawn* cameraActor)
{

	CameraPawnActor = cameraActor;
}

AYogCameraPawn* APlayerCharacterBase::GetOwnCamera()
{

	return CameraPawnActor;
}

void APlayerCharacterBase::SetPrepareItem(AActor* actor)
{
	temp_Item_prepare = actor;
	UE_LOG(LogTemp, Warning, TEXT("temp_Item_prepare set: %s"), *temp_Item_prepare->GetName());
}

void APlayerCharacterBase::DropPrepareItem()
{
	temp_Item_prepare = nullptr;
}

AActor* APlayerCharacterBase::GetPrepareItem()
{
	return temp_Item_prepare;
}

void APlayerCharacterBase::SpawnAura()
{

	//UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	//USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();

	//TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
	//FName Socket = WeaponActorInst.AttachSocket;
	//FTransform Transform = WeaponActorInst.AttachTransform;

	//FVector Location = TargetCharacter->GetActorLocation();
	//FRotator Rotation = FRotator::ZeroRotator;
	//FActorSpawnParameters SpawnParams;
	//SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	//AYogCameraPawn* CameraActorPawn = GetWorld()->SpawnActor<AYogCameraPawn>(CameraPawnClass, Location, Rotation, SpawnParams);


	AAuraBase* aura = GetWorld()->SpawnActorDeferred<AAuraBase>(Aura, FTransform::Identity, this);
	//AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
	aura->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
	//aura->SetActorRelativeTransform(Transform);
	aura->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	//ReceivingChar->Weapon = NewActor;


}



void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (CharacterMovementDataTable)
	{
		static const FString ContextString(TEXT("Character movement Data Lookup"));
		FName RowName(TEXT("TripleC_Lvl_1")); // Name of the row you want to access

		FCharacterMovementData* MovementData = this->CharacterMovementDataTable->FindRow<FCharacterMovementData>(FName(TEXT("CharacterMoveLvl_1")), ContextString, true);

		if (MovementData)
		{

			UYogCharacterMovementComponent* MovementComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());
			MovementComp->MaxWalkSpeed = MovementData->MaxWalkSpeed;
			MovementComp->GroundFriction = MovementData->GroundFriction;
			MovementComp->MaxAcceleration = MovementData->MaxAcceleration;
			MovementComp->RotationRate = MovementData->RotationRate;
		}

	}




	//if (AbilitySystemComponent) {

	//	HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AYogCharacterBase::HealthChanged);
	//	MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogCharacterBase::MaxHealthChanged);
	//	BaseDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBaseDMGAttribute()).AddUObject(this, &AYogCharacterBase::BaseDMGChanged);
	//	WeaponDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetWeaponDMGAttribute()).AddUObject(this, &AYogCharacterBase::WeaponDMGChanged);
	//	BuffAmplifyChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBuffAmplifyAttribute()).AddUObject(this, &AYogCharacterBase::BuffAmplifyChanged);
	//}

	this->CurrentState = EYogCharacterState::Idle;

}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

