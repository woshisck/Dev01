// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Character/YogPlayerControllerBase.h"
#include "Camera/YogCameraPawn.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Buff/Aura/AuraBase.h"
#include "SaveGame/YogSaveGame.h"
#include "UObject/ConstructorHelpers.h"
#include "Data/GASTemplate.h"
#include "Item/ItemSpawner.h"


APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{


	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

}

void APlayerCharacterBase::SetOwnCamera(AYogCameraPawn* cameraActor)
{

	CameraPawnActor = cameraActor;
}

AYogCameraPawn* APlayerCharacterBase::GetOwnCamera()
{

	return CameraPawnActor;
}


void APlayerCharacterBase::Die()
{
	Super::Die();

}

void APlayerCharacterBase::ItemInteract(const AItemSpawner* item)
{

	TArray<AActor*> actor_overlap_list;
	TSubclassOf<AItemSpawner> class_filter;
	this->GetCapsuleComponent()->GetOverlappingActors(actor_overlap_list, AItemSpawner::StaticClass());
	for (AActor* Actor : actor_overlap_list)
	{
		if (Actor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Overlapping Actor: %s"), *Actor->GetName());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("item"));
}

void APlayerCharacterBase::HeatChanged(const FOnAttributeChangeData& Data)
{

	float Health = Data.NewValue;
	float percent = Health / BaseAttributeSet->GetMaxHeat();


	OnHeatUpdate.Broadcast(percent);

}

void APlayerCharacterBase::MaxHeatChanged(const FOnAttributeChangeData& Data)
{
	float MaxHeat = Data.NewValue;
	OnMaxHeatUpdate.Broadcast(MaxHeat);
}


void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	//check(AttributeStatsComponent);
	//check(AbilitySystemComponent);


	//GetASC()->InitAbilityActorInfo(this, this);
	//if (GasTemplate != nullptr)
	//{
	//	for (TSubclassOf<UYogGameplayAbility> ablity_class : GasTemplate->PassiveMap)
	//	{
	//		//TODO: confirm about the inputID
	//		GetASC()->K2_GiveAbility(ablity_class, 0, 0);
	//	}
	//	for (TSubclassOf<UYogGameplayAbility> ablity_class : GasTemplate->AbilityMap)
	//	{
	//		GetASC()->K2_GiveAbility(ablity_class, 0, 0);
	//	}
	//	for (const TSubclassOf<UYogGameplayEffect> effect_class : GasTemplate->PassiveEffect)
	//	{
	//		//TODO: fix bug when the ower is null
	//		//UYogAbilitySystemComponent* asc = this->GetASC();
	//		//FGameplayEffectContextHandle Context = asc->MakeEffectContext();
	//		//FGameplayEffectSpecHandle SpecHandle = asc->MakeOutgoingSpec(effect_class, 0, Context);
	//		//
	//		//if (SpecHandle.IsValid())
	//		//{
	//		//	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	//		//	asc->ApplyGameplayEffectSpecToSelf(*Spec);
	//		//}
	//	}
	//}
}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

void APlayerCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();


}


