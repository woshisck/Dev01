// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DevKit/Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"

#include "DevKit/AbilitySystem/Abilities/YogGameplayAbility.h"
#include <DevKit/Player/YogPlayerControllerBase.h>
#include <DevKit/Item/Weapon/WeaponInstance.h>
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "DevKit/Component/HitBoxBufferComponent.h"
#include "System/YogGameInstanceBase.h"

#include "DevKit/AbilitySystem/Abilities/Passive_YogAbility.h"

AYogCharacterBase::AYogCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	HitboxbuffComponent = ObjectInitializer.CreateDefaultSubobject<UHitBoxBufferComponent>(this, TEXT("HitBoxBufferComponent"));
	AttributeStatsComponent = ObjectInitializer.CreateDefaultSubobject<UAttributeStatComponent>(this, TEXT("AttributeStatComp"));

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);


	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
	DamageAttributeSet = CreateDefaultSubobject<UDamageAttributeSet>(TEXT("DamageAttributeSet"));
	AdditionAttributeSet = CreateDefaultSubobject<UAdditionAttributeSet>(TEXT("AdditionAttributeSet"));




	//MovementComponent setup
	UYogCharacterMovementComponent* YogMoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());

	CurrentState = EYogCharacterState::Idle;
	PreviousState = EYogCharacterState::Idle;
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
}

void AYogCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

void AYogCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CharacterData)
	{
		const FMovementData& moveData = CharacterData->GetMovementData();
		const FYogBaseAttributeData& characterData = CharacterData->GetBaseAttributeData();
		BaseAttributeSet->Init(CharacterData);
	}
	if (AbilitySystemComponent) 
	{
		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAttributeSet->GetHealthAttribute()).AddUObject(this, &AYogCharacterBase::HealthChanged);
		MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogCharacterBase::MaxHealthChanged);
	}

}

UAbilitySystemComponent* AYogCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AYogCharacterBase::GetGroundSlope(float length)
{
	FVector result = FVector();
	FVector GroundVec = FVector(0,0,-1);

	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() + GroundVec * length;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	bool isHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility, // Collision channel
		TraceParams);
	if (isHit)
	{
		result = FVector::CrossProduct(HitResult.ImpactNormal, GetActorRightVector());
	}

	return result;
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
	if (BaseAttributeSet)
	{
		return BaseAttributeSet->GetHealth();
	}

	return .4444f;
}

float AYogCharacterBase::GetMaxHealth() const
{
	if (BaseAttributeSet)
	{
		return BaseAttributeSet->GetMaxHealth();
	}

	return .5555f;
}

float AYogCharacterBase::GetAtkDist() const
{
	//if (BaseAttributeSet)
	//{
	//	return BaseAttributeSet->GetAtkDist();
	//}
	return .6666f;
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


void AYogCharacterBase::UpdateCharacterState(EYogCharacterState newState, FVector movementInput)
{
	//check if previous state 
	PreviousState = CurrentState;

	CurrentState = newState;
	OnCharacterStateUpdate.Broadcast(newState, movementInput);

}



void AYogCharacterBase::HealthChanged(const FOnAttributeChangeData& Data)
{

	float Health = Data.NewValue;
	float percent = Health / GetMaxHealth();


	OnCharacterHealthUpdate.Broadcast(percent);
	UE_LOG(LogTemp, Log, TEXT("Health Changed to: %f"), Health);
	if (!IsAlive())
	{
		
		Die();
		
	}

}

void AYogCharacterBase::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	float MaxHealth = Data.NewValue;

}



void AYogCharacterBase::FinishDying()
{
	UE_LOG(LogTemp, Display, TEXT("Character FinishDying"));
	//Destroy();
}

void AYogCharacterBase::Die()
{
	UE_LOG(LogTemp, Log, TEXT("DEATH HAPPEN, DEAD CHARACTER: %s"), *UKismetSystemLibrary::GetDisplayName(this));

	//GetCharacterMovement()->GravityScale = 0;
	//GetCharacterMovement()->Velocity = FVector(0);


	UYogGameInstanceBase* YogGameInstance = Cast<UYogGameInstanceBase>(GetGameInstance());
	YogGameInstance->MapStateCount.MonsterSlayCount++;

	UE_LOG(LogTemp, Log, TEXT("Current Monster Slay Count: %i"), YogGameInstance->MapStateCount.MonsterSlayCount);

	OnCharacterDied.Broadcast(this);


	//AbilitySystemComponent->AddLooseGameplayTag(DeadTag);

	//if (APlayerController* PC = Cast<APlayerController>(this->GetController()))
	//{
	//	DisableInput(PC);
	//}


	//if (DeathMontage)
	//{
	//	PlayAnimMontage(DeathMontage);
	//}
	//else
	//{
	//	FinishDying();
	//} 

}


void AYogCharacterBase::GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetActiveAbilitiesWithTags(AbilityTags, ActiveAbilities);
	}
}
