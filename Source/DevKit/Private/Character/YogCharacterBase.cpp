// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/RuneAttributeSet.h"
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
#include "GameplayEffect.h"
#include "Component/BufferComponent.h"
#include "Component/PropInteractComponnet.h"
#include "Data/CharacterData.h"
#include "Data/GasTemplate.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagsManager.h"



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
	PropInteractComponent = CreateDefaultSubobject<UPropInteractComponnet>(TEXT("PropInteractComponent"));

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UIWidgetComponent"));

	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
	DamageAttributeSet = CreateDefaultSubobject<UDamageAttributeSet>(TEXT("DamageAttributeSet"));
	RuneAttributeSet = CreateDefaultSubobject<URuneAttributeSet>(TEXT("RuneAttributeSet"));

	WidgetComponent->SetupAttachment(GetRootComponent());

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

UWidgetComponent* AYogCharacterBase::GetWidgetcomponent()
{
	return WidgetComponent;
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
	return AttributeStatsComponent && AttributeStatsComponent->GetStat_Health() > 0.f;
}

void AYogCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// ── 基础通用技能（路径固定，无需编辑器配置）────────────────────────────
	// 资产路径：Content/Core/Data/DA_Base_AbilitySet（GASTemplate DataAsset）
	// 填写内容：GA_Knockback / GA_HitReaction / GA_Dead 等所有角色共享的响应型 GA
	if (UGASTemplate* BaseSet = LoadObject<UGASTemplate>(nullptr, BaseAbilitySetPath))
	{
		for (const TSubclassOf<UYogGameplayAbility>& AbilityClass : BaseSet->AbilityMap)
		{
			if (AbilityClass)
			{
				GrantGameplayAbility(AbilityClass, 1);
			}
		}
	}

	// ── 角色个性化数据（属性 / 技能 / 动画层，各角色 CharacterData 单独配置）──
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

	UE_LOG(LogTemp, Log, TEXT("PostInit: AbilitySystemComponent = %p"), AbilitySystemComponent.Get());
	
	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->InitConflictTable();
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
	MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);

	if (Controller)
	{
		Controller->ResetIgnoreMoveInput();
	}
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

	// 血量减少且角色存活 → 自动触发受击动画
	// 注：此处无法可靠拿到攻击者引用，受击方向默认走 Front；
	// 若 FA 层需要精确前/后方向，可改由 FA 直接 SendGameplayEvent 并传入 Instigator
	if (Data.NewValue < Data.OldValue && IsAlive() && AbilitySystemComponent)
	{
		FGameplayEventData HitEventData;
		AbilitySystemComponent->HandleGameplayEvent(
			FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact")), &HitEventData);
	}

	if (!IsAlive() && !bIsDead)
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
	// 死亡动画播完后广播，确保监听者（敌人 BP / GameMode 等）在动画结束后再执行销毁逻辑
	OnCharacterDied.Broadcast(this);
	Destroy();
}

void AYogCharacterBase::Die()
{
	UE_LOG(LogTemp, Log, TEXT("DEATH HAPPEN, DEAD CHARACTER: %s"), *UKismetSystemLibrary::GetDisplayName(this));

	bIsDead = true;

	// 发送 Action.Dead 事件 → 触发 GA_Dead 播放死亡动画，动画结束后 GA 调用 FinishDying()
	if (AbilitySystemComponent)
	{
		FGameplayEventData EventData;
		EventData.Instigator = this;
		AbilitySystemComponent->HandleGameplayEvent(
			FGameplayTag::RequestGameplayTag(TEXT("Action.Dead")), &EventData);
	}
	// OnCharacterDied 已移至 FinishDying()，避免在动画播放前触发销毁逻辑
}

void AYogCharacterBase::InitializeComponentsWithStats(UCharacterData* characterData)
{
	//-------------------------------------
	//	movementData
	//	AttributeData
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
		}

		// 应用 GasTemplate 里的 Passive GE（如热度衰减）
		for (const TSubclassOf<UGameplayEffect> PassiveGE : characterData->GetGASTemplate()->PassiveEffect)
		{
			if (PassiveGE)
			{
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(PassiveGE, 1, Context);
				if (Spec.IsValid())
				{
					AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					UE_LOG(LogTemp, Log, TEXT("Applied Passive GE from GAS Template: %s"), *PassiveGE->GetName());
				}
			}
		}
	}

	for (const TSubclassOf<UAnimInstance> animLayer: characterData->GetDefaultAnimeLayers())
	{
		GetMesh()->GetAnimInstance()->LinkAnimClassLayers(animLayer);
		UE_LOG(LogTemp, Log, TEXT("Grant ability from GAS Template: %s"), *animLayer->GetName());
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
		if (!ensure(AttributeStatsComponent != nullptr)) return;
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
