// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBaseCharacter.h"
#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>

AYogBaseCharacter::AYogBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	//HealthSet = CreateDefaultSubobject<UYogHealthSet>(TEXT("HealthSet"));
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));
}



bool AYogBaseCharacter::IsAlive() const
{
	return GetHealth() > 0.0f;
}

void AYogBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent) {

		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AYogBaseCharacter::HealthChanged);
		MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogBaseCharacter::MaxHealthChanged);
		BaseDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBaseDMGAttribute()).AddUObject(this, &AYogBaseCharacter::BaseDMGChanged);
		WeaponDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetWeaponDMGAttribute()).AddUObject(this, &AYogBaseCharacter::WeaponDMGChanged);
		BuffAmplifyChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBuffAmplifyAttribute()).AddUObject(this, &AYogBaseCharacter::BuffAmplifyChanged);
	}
}

void AYogBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

UAbilitySystemComponent* AYogBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}



float AYogBaseCharacter::GetHealth() const
{
	if (AttributeSet)
	{
		return AttributeSet->GetHealth();
	}

	return .04f;
}

void AYogBaseCharacter::AddStartupGameplayAbilities()
{
}


void AYogBaseCharacter::HandleDamage(float DamageAmount, const FHitResult& HitInfo, const FGameplayTagContainer& DamageTags, AYogBaseCharacter* InstigatorPawn, AActor* DamageCauser)
{
	OnDamaged(DamageAmount, HitInfo, DamageTags, InstigatorPawn, DamageCauser);
}

void AYogBaseCharacter::HandleHealthChanged(float DeltaValue, const FGameplayTagContainer& EventTags)
{
	OnHealthChanged(DeltaValue, EventTags);
}


void AYogBaseCharacter::HandleMoveSpeedChanged(float DeltaValue, const FGameplayTagContainer& EventTags)
{
	
}

void AYogBaseCharacter::HealthChanged(const FOnAttributeChangeData& Data)
{
	float Health = Data.NewValue;
}

void AYogBaseCharacter::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	float MaxHealth = Data.NewValue;

}

void AYogBaseCharacter::BaseDMGChanged(const FOnAttributeChangeData& Data)
{
	float BaseDMG = Data.NewValue;
}

void AYogBaseCharacter::WeaponDMGChanged(const FOnAttributeChangeData& Data)
{
	float WeaponDMG = Data.NewValue;
}

void AYogBaseCharacter::BuffAmplifyChanged(const FOnAttributeChangeData& Data)
{
	float BuffAmplify = Data.NewValue;
}
