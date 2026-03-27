// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Character/YogPlayerControllerBase.h"
#include "Item/Weapon/WeaponInstance.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Component/HitBoxBufferComponent.h"
#include "System/YogGameInstanceBase.h"

#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "Component/GameEffectComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Component/BufferComponent.h"
#include "Data/CharacterData.h"


AYogCharacterBase::AYogCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = CreateDefaultSubobject<UYogAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeStatsComponent = CreateDefaultSubobject<UAttributeStatComponent>(TEXT("AttributeStatComponent"));
	GameEffectComponent = CreateDefaultSubobject<UGameEffectComponent>(TEXT("GameEffectComponent"));
	InputBufferComponent = CreateDefaultSubobject<UBufferComponent>(TEXT("InputBufferComponent"));
	CharacterDataComponent = CreateDefaultSubobject<UCharacterDataComponent>(TEXT("CharacterDataComponent"));


	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
	DamageAttributeSet = CreateDefaultSubobject<UDamageAttributeSet>(TEXT("DamageAttributeSet"));


	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);


	//MovementComponent setup
	UYogCharacterMovementComponent* YogMoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());

	CurrentState = EYogCharacterState::Idle;
	PreviousState = EYogCharacterState::Idle;

	CurrentWeaponState = EWeaponState::Unequipped;

	UE_LOG(LogTemp, Log, TEXT("Constructor: AbilitySystemComponent = %p"), AbilitySystemComponent.Get());


}



int32 AYogCharacterBase::GetStatePriority(EYogCharacterState State)
{
	switch (State) {
	case EYogCharacterState::Dead: return 120;
	case EYogCharacterState::Stun: return 100;
	case EYogCharacterState::GetHit:  return 80;
	case EYogCharacterState::Action: return 60;
	case EYogCharacterState::Move: return 40;
	case EYogCharacterState::Idle: return 20;
	default: return 0;
	}

}

UYogAbilitySystemComponent* AYogCharacterBase::GetASC() const
{
	return Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent());
	//UAbilitySystemComponent* comp = GetAbilitySystemComponent();
	//if (comp)
	//{
	//	UYogAbilitySystemComponent* result = Cast<UYogAbilitySystemComponent>(comp);
	//	return result;
	//}
	//return nullptr;
}



bool AYogCharacterBase::IsAlive() const
{
	return true;
}

void AYogCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	UCharacterData* pCharacterData = CharacterDataComponent->GetCharacterData();
	if (pCharacterData != nullptr)
	{
		InitializeComponentsWithStats(pCharacterData);
	}

}

void AYogCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

void AYogCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	//-------------------------------
	//	only setup the component 
	//-------------------------------

	UE_LOG(LogTemp, Log, TEXT("PostInit: AbilitySystemComponent = %p"), AbilitySystemComponent.Get());
	
	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	//const UCharacterData* pCharacterData = DataCacheComponent->GetCharacterData();
	//// In case data is not set up we initialize it now ( for instance spawning player )
	//if (pCharacterData == nullptr)
	//{
	//	pCharacterData = DataCacheComponent->InitializeCharacterData();
	//}


}

void AYogCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AYogCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);




	//const FMovementData& moveData = CharacterData->GetMovementData();
	//const FYogBaseAttributeData& characterData = CharacterData->GetBaseAttributeData();
	//BaseAttributeSet->Init(CharacterData);

	HealthChangedDelegateHandle = GetASC()->GetGameplayAttributeValueChangeDelegate(BaseAttributeSet->GetHealthAttribute()).AddUObject(this, &AYogCharacterBase::HealthChanged);
	MaxHealthChangedDelegateHandle = GetASC()->GetGameplayAttributeValueChangeDelegate(BaseAttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogCharacterBase::MaxHealthChanged);




}

void AYogCharacterBase::UnPossessed()
{
	Super::UnPossessed();
}

void AYogCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

}

void AYogCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

UBufferComponent* AYogCharacterBase::GetInputBufferComponent()
{
	return InputBufferComponent;
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


void AYogCharacterBase::GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel)
{
	check(AbilitySystemComponent);

	if (AbilitySystemComponent && AbilityToGrant)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityToGrant, AbilityLevel);
		AbilitySystemComponent->GiveAbility(AbilitySpec);

	}

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
	UYogCharacterMovementComponent* MoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());

}


void AYogCharacterBase::UpdateCharacterState(EYogCharacterState newState)
{
	//check if previous state 
	int32 previous_state_priority = GetStatePriority(PreviousState);
	int32 update_state_priority = GetStatePriority(newState);

	if (previous_state_priority > update_state_priority)
	{
		return;
	}
	else
	{
		EYogCharacterState state_before;
		state_before = PreviousState;
		PreviousState = CurrentState;

		CurrentState = newState;
		OnCharacterStateUpdate.Broadcast(state_before, newState);
	}

}



void AYogCharacterBase::HealthChanged(const FOnAttributeChangeData& Data)
{

	float Health = Data.NewValue;
	float percent = Health / AttributeStatsComponent->GetStat_MaxHealth();


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


	UYogGameInstanceBase* YogGameInstance = Cast<UYogGameInstanceBase>(GetGameInstance());
	OnCharacterDied.Broadcast(this);

}

void AYogCharacterBase::InitializeComponentsWithStats(UCharacterData* characterData)
{
	//-------------------------------------
	//	movementData
	//	AttributeData
	//	AbilityData
	//	GasTemplate
	//	DefaultAnimeLayer
	//-------------------------------------

	const FMovementData* movementData = characterData->GetMovementData();
	if (movementData != nullptr)
	{
		InitializeMovement(movementData);
	}

	const FYogBaseAttributeData* attributeData = characterData->GetBaseAttributeData();
	if (attributeData != nullptr)
	{
		InitializeStats(attributeData);
	}

	if (characterData->GetGASTemplate() != nullptr)
	{
		for (const TSubclassOf<UYogGameplayAbility> abilityTemp : characterData->GetGASTemplate()->AbilityMap)
		{
			GrantGameplayAbility(abilityTemp, 1);
			UE_LOG(LogTemp, Log, TEXT("Grant ability from GAS Template: %s"), *abilityTemp->GetName());
			//UE_LOG(LogTemp, Log, TEXT("AbilitySystemComponent: %p"), AbilitySystemComponent.Get());
			//if (AbilitySystemComponent)
			//{
			//	UE_LOG(LogTemp, Log, TEXT("ApplyGASFromTemplate: %s"), *characterData->GetGASTemplate()->GetName());
			//	AbilitySystemComponent->ApplyGASFromTemplate(characterData->GetGASTemplate());
			//}
			//else
			//{
			//	UE_LOG(LogTemp, Warning, TEXT("AbilitySystemComponent is null when applying GAS Template"));
			//}
		}
	}
}

void AYogCharacterBase::InitializeMovement(const FMovementData* movementData) const
{
	if (movementData)
	{
		//check(GetCharacterMovement() != nullptr)
		UYogCharacterMovementComponent* MoveComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());
		MoveComp->MaxWalkSpeed = movementData->MaxWalkSpeed;
		MoveComp->MaxWalkSpeedCrouched = movementData->GroundFriction;
		MoveComp->BrakingDecelerationWalking = movementData->BreakingDeceleration;
		MoveComp->MaxAcceleration = movementData->MaxAcceleration;
		MoveComp->RotationRate = movementData->RotationRate;
	}
}

void AYogCharacterBase::InitializeStats(const FYogBaseAttributeData* attributeData) const
{
	if (attributeData != nullptr)
	{
		check(AttributeStatsComponent != nullptr)
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetAttackAttribute(), attributeData->Attack);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetAttackPowerAttribute(), attributeData->AttackPower);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetMaxHealthAttribute(), attributeData->MaxHealth);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetMaxHeatAttribute(), attributeData->MaxHeat);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetShieldAttribute(), attributeData->Shield);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetAttackSpeedAttribute(), attributeData->AttackSpeed);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetAttackRangeAttribute(), attributeData->AttackRange);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetSanityAttribute(), attributeData->Sanity);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetMoveSpeedAttribute(), attributeData->MoveSpeed);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetDodgeAttribute(), attributeData->Dodge);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetResilienceAttribute(), attributeData->Resilience);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetResistAttribute(), attributeData->Resist);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetDmgTakenAttribute(), attributeData->DmgTaken);

		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetCrit_RateAttribute(), attributeData->Crit_Rate);
		AttributeStatsComponent->OverrideAttribute(BaseAttributeSet->GetCrit_DamageAttribute(), attributeData->Crit_Damage);

	}

}


EYogCharacterState AYogCharacterBase::GetCurrentState()
{
	return this->CurrentState;
}

EWeaponState AYogCharacterBase::GetWeaponState()
{
	return this->CurrentWeaponState;
}

void AYogCharacterBase::GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetActiveAbilitiesWithTags(AbilityTags, ActiveAbilities);
	}
}
