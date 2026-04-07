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
#include "Component/BackpackGridComponent.h"
#include "BuffFlow/BuffFlowComponent.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{
	BackpackGridComponent = CreateDefaultSubobject<UBackpackGridComponent>(TEXT("BackpackGridComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));
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

void APlayerCharacterBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (AItemSpawner* Spawner = Cast<AItemSpawner>(OtherActor))
	{
		OverlappingSpawner = Spawner;
	}
}

void APlayerCharacterBase::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (OtherActor == OverlappingSpawner)
	{
		OverlappingSpawner = nullptr;
	}
}

UBackpackGridComponent* APlayerCharacterBase::GetBackpackGridComponent()
{
	return BackpackGridComponent;
}

void APlayerCharacterBase::AddRuneToInventory(const FRuneInstance& Rune)
{
	PendingRunes.Add(Rune);
}

void APlayerCharacterBase::AddGold(int32 Amount)
{
	Gold = FMath::Max(0, Gold + Amount);
	OnGoldChanged.Broadcast(Gold);
}

void APlayerCharacterBase::InitDashChargeSystem()
{
	DashChargeCount = MaxDashChargeCount;
	GetWorldTimerManager().SetTimer(DashChargeRegenTimer, [this]()
	{
		if (DashChargeCount < MaxDashChargeCount)
		{
			DashChargeCount++;
		}
		if (DashChargeCount >= MaxDashChargeCount)
		{
			GetWorldTimerManager().ClearTimer(DashChargeRegenTimer);
		}
	}, DashChargeRegenInterval, true);
}

void APlayerCharacterBase::ShutdownDashChargeSystem()
{
	DashChargeCount = 0;
	GetWorldTimerManager().ClearTimer(DashChargeRegenTimer);
}

void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 将 BackpackGridComponent 与 ASC 关联，使符文激活时可施加 GE
	if (BackpackGridComponent && GetAbilitySystemComponent())
	{
		BackpackGridComponent->InitWithASC(GetAbilitySystemComponent());
	}

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


