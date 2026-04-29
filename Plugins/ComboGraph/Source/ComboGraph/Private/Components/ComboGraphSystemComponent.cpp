// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Components/ComboGraphSystemComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/ComboGraphNativeAbility.h"
#include "Utils/ComboGraphBlueprintLibrary.h"
#include "Utils/ComboGraphUtils.h"
#include "ComboGraphLog.h"

void UComboGraphSystemComponent::OnRegister()
{
	Super::OnRegister();
}

void UComboGraphSystemComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UComboGraphSystemComponent::BeginPlay()
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent::BeginPlay - Owner: %s, Role: %s"), *GetNameSafe(GetOwner()), *UEnum::GetValueAsString(GetOwnerRole()))
	Super::BeginPlay();

	SetupAbilitySystemComponent();
}

void UComboGraphSystemComponent::SetupAbilitySystemComponent()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphSystemComponent::SetupAbilitySystemComponent - Owner Actor is invalid"))
		return;
	}

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent::SetupAbilitySystemComponent - Owner: %s"), *GetNameSafe(GetOwner()))
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent::SetupAbilitySystemComponent - OwnerRole: %s"), *UEnum::GetValueAsString(GetOwnerRole()))

	UAbilitySystemComponent* ASC = UComboGraphBlueprintLibrary::GetAbilitySystemComponentFromActor(OwnerActor);
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent::SetupAbilitySystemComponent - Existing ASC: %s"), *GetNameSafe(ASC))
	if (!ASC)
	{
		// Actor doesn't have an ASC, spawn and register one here
		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent: spawning UComboGraphSystemComponent.."));
		ASC = NewObject<UAbilitySystemComponent>(OwnerActor, TEXT("AbilitySystemComponent_Spawned_ComboGraph"));
		ASC->SetIsReplicated(true);
		// TODO: Consider adding BP exposed ReplicationMode property to configure this
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
		ASC->RegisterComponent();
	}

	if (ASC)
	{
		if (ASC->IsOwnerActorAuthoritative())
		{
			const FGameplayAbilitySpec AbilitySpec(UComboGraphNativeAbility::StaticClass());
			ASC->GiveAbility(AbilitySpec);

			CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent::SetupAbilitySystemComponent - Give Combo ability to %s"), *GetNameSafe(GetOwner()))
		}
		else
		{
			CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphSystemComponent::SetupAbilitySystemComponent - No Authority, No Ability"), *GetNameSafe(GetOwner()))
		}
	}
}
