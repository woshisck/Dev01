// Fill out your copyright notice in the Description page of Project Settings.


#include "YogCharacterBase.h"
#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Inventory/InventoryManagerComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"

#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>

AYogCharacterBase::AYogCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	InventoryManagerComponent = ObjectInitializer.CreateDefaultSubobject<UInventoryManagerComponent>(this, TEXT("InventoryComponent"));

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	//HealthSet = CreateDefaultSubobject<UYogHealthSet>(TEXT("HealthSet"));
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));




	//TODO: Dead Tag hardcode define
	DeadTag = FGameplayTag::RequestGameplayTag(FName("Status.Dead"));
	

	//MovementComponent setup
	UYogCharacterMovementComponent* YogMoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());


	bAbilitiesInitialized = false;
}

UYogAbilitySystemComponent* AYogCharacterBase::GetASC() const
{
	return Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent());
}



bool AYogCharacterBase::IsAlive() const
{
	return GetHealth() > 0.0f;
}

void AYogCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent) {

		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AYogCharacterBase::HealthChanged);
		MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogCharacterBase::MaxHealthChanged);
		BaseDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBaseDMGAttribute()).AddUObject(this, &AYogCharacterBase::BaseDMGChanged);
		WeaponDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetWeaponDMGAttribute()).AddUObject(this, &AYogCharacterBase::WeaponDMGChanged);
		BuffAmplifyChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBuffAmplifyAttribute()).AddUObject(this, &AYogCharacterBase::BuffAmplifyChanged);
	}
}

void AYogCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

UAbilitySystemComponent* AYogCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 AYogCharacterBase::GetCharacterLevel() const
{
	return CharacterLevel;
}

void AYogCharacterBase::UpdateMoveable(const bool IsMoveAble)
{


	this->bCanMove = IsMoveAble;
	if (IsMoveAble)
	{
		this->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}
	else
	{
		this->GetCharacterMovement()->DisableMovement();
	}
	OnCharacterCanMoveUpdate.Broadcast(IsMoveAble);
}



float AYogCharacterBase::GetHealth() const
{
	if (AttributeSet)
	{
		return AttributeSet->GetHealth();
	}

	return .4444f;
}

float AYogCharacterBase::GetMaxHealth() const
{
	if (AttributeSet)
	{
		return AttributeSet->GetMaxHealth();
	}

	return .5555f;
}

void AYogCharacterBase::GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel)
{
	check(AbilitySystemComponent);

	if (AbilitySystemComponent && AbilityToGrant)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityToGrant, AbilityLevel);
		AbilitySystemComponent->GiveAbility(AbilitySpec);
		//AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	//if (GetLocalRole() == ROLE_Authority && !bAbilitiesInitialized)
	//{
	//	for (TSubclassOf<UYogGameplayAbility>& StartupAbility : GameplayAbilities)
	//	{
	//		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, GetCharacterLevel(), INDEX_NONE, this));
	//	}
	//}
	//bAbilitiesInitialized = false;
}




void AYogCharacterBase::PrintAllGameplayTags(const FGameplayTagContainer& TagContainer)
{
	
	for (const FGameplayTag& Tag : TagContainer)
	{
		// Print each tag to the log
		UE_LOG(LogTemp, Log, TEXT("Gameplay Tag: %s"), *Tag.ToString());
	}

	if (TagContainer.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Gameplay Tags present in the container."));
	}
}



void AYogCharacterBase::HealthChanged(const FOnAttributeChangeData& Data)
{

	float Health = Data.NewValue;
	float percent = Health / GetMaxHealth();


	OnCharacterHealthUpdate.Broadcast(percent);
	UE_LOG(LogTemp, Log, TEXT("Health Changed to: %f"), Health);
	if (!IsAlive() && !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		
		Die();
		
	}

}

void AYogCharacterBase::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	float MaxHealth = Data.NewValue;

}

void AYogCharacterBase::BaseDMGChanged(const FOnAttributeChangeData& Data)
{
	float BaseDMG = Data.NewValue;
}

void AYogCharacterBase::WeaponDMGChanged(const FOnAttributeChangeData& Data)
{
	float WeaponDMG = Data.NewValue;
}

void AYogCharacterBase::BuffAmplifyChanged(const FOnAttributeChangeData& Data)
{
	float BuffAmplify = Data.NewValue;
}


void AYogCharacterBase::FinishDying()
{
	//TODO: add death event and animation montage later
	Destroy();
}

void AYogCharacterBase::Die()
{
	UE_LOG(LogTemp, Log, TEXT("DEATH HAPPEN, DEAD CHARACTER: %s"), *UKismetSystemLibrary::GetDisplayName(this));

	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->Velocity = FVector(0);

	OnCharacterDied.Broadcast(this);
	AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	else
	{
		FinishDying();
	}


}

void AYogCharacterBase::GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetActiveAbilitiesWithTags(AbilityTags, ActiveAbilities);
	}
}
