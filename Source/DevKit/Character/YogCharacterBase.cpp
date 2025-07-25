// Fill out your copyright notice in the Description page of Project Settings.


#include "YogCharacterBase.h"
#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Inventory/InventoryManagerComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"
#include <DevKit/Player/YogPlayerControllerBase.h>
#include <DevKit/Item/Weapon/WeaponInstance.h>
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "../Component/HitBoxBufferComponent.h"

#include "../AbilitySystem/Abilities/Passive_YogAbility.h"

AYogCharacterBase::AYogCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	//InventoryManagerComponent = ObjectInitializer.CreateDefaultSubobject<UInventoryManagerComponent>(this, TEXT("InventoryComponent"));
	HitboxbuffComponent = ObjectInitializer.CreateDefaultSubobject<UHitBoxBufferComponent>(this, TEXT("HitBoxBufferComponent"));
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
	UAbilitySystemComponent* comp = GetAbilitySystemComponent();
	if (comp)
	{
		UYogAbilitySystemComponent* result = Cast<UYogAbilitySystemComponent>(comp);
		return result;
	}
	return nullptr;
	
	
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
	
	if (DT_Attribute)
	{
		FYogAttributeData* pYogAttributeData = DT_Attribute->FindRow<FYogAttributeData>(TEXT("Default_Attribute"), TEXT(""));
		
		if (pYogAttributeData != nullptr)
		{
			AttributeSet->InitHealth(pYogAttributeData->Health);
			AttributeSet->InitMaxHealth(pYogAttributeData->MaxHealth);
			AttributeSet->InitBaseDMG(pYogAttributeData->BaseDMG);
			AttributeSet->InitWeaponDMG(pYogAttributeData->WeaponDMG);
			AttributeSet->InitBuffAmplify(pYogAttributeData->BuffAmplify);
			AttributeSet->InitDMGAbsorb(pYogAttributeData->DMGAbsorb);
			AttributeSet->InitActResist(pYogAttributeData->ActResist);
		}
	}



	//if (YogAttributeInfo)
	//{
	//	static const FString ContextString(TEXT("Character attribute Data Lookup"));
	//	//FName RowName(TEXT("TripleC_Lvl_1")); // Name of the row you want to access

	//	FYogAttributeData* AttributeData = this->YogAttributeInfo->FindRow<FYogAttributeData>(FName(TEXT("Default_Attribute")), ContextString, true);

	//	if (AttributeData)
	//	{

	//		AttributeSet->InitHealth(AttributeData->Health);
	//		AttributeSet->InitMaxHealth(AttributeData->MaxHealth);
	//		AttributeSet->InitBaseDMG(AttributeData->BaseDMG);
	//		AttributeSet->InitWeaponDMG(AttributeData->WeaponDMG);
	//		AttributeSet->InitBuffAmplify(AttributeData->BuffAmplify);
	//		AttributeSet->InitDMGAbsorb(AttributeData->DMGAbsorb);
	//		AttributeSet->InitActResist(AttributeData->ActResist);

	//	}
	//	

	//}

}

void AYogCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

UAbilitySystemComponent* AYogCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}



void AYogCharacterBase::UpdateCharacterMovement(const bool IsMovable)
{
	this->bMovable = IsMovable;
	//TODO: can not disable movement component, will disable montage root motion as well
	//if (IsMovable)
	//{
	//	this->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	//}
	//else
	//{
	//	
	//	this->GetCharacterMovement()->DisableMovement();
	//}
	OnCharacterCanMoveUpdate.Broadcast(IsMovable);
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

void AYogCharacterBase::DisableMovement()
{
	UYogCharacterMovementComponent* LyraMoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());
	LyraMoveComp->StopMovementImmediately();
	LyraMoveComp->DisableMovement();


	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

}

void AYogCharacterBase::EnableMovement()
{
	UYogCharacterMovementComponent* LyraMoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());

}


void AYogCharacterBase::SetCharacterState(EYogCharacterState newState, FVector movementInput)
{
	
	EYogCharacterState previousState = CurrentState;

	CurrentState = newState;
	OnCharacterStateUpdate.Broadcast(newState, movementInput);

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
	UE_LOG(LogTemp, Display, TEXT("Character FinishDying"));
	//Destroy();
}

void AYogCharacterBase::Die()
{
	UE_LOG(LogTemp, Log, TEXT("DEATH HAPPEN, DEAD CHARACTER: %s"), *UKismetSystemLibrary::GetDisplayName(this));

	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->Velocity = FVector(0);

	OnCharacterDied.Broadcast(this);
	AbilitySystemComponent->AddLooseGameplayTag(DeadTag);

	if (APlayerController* PC = Cast<APlayerController>(this->GetController()))
	{
		DisableInput(PC);
	}


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
