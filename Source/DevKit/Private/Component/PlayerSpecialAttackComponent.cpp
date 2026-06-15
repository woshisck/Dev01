#include "Component/PlayerSpecialAttackComponent.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"

UPlayerSpecialAttackComponent::UPlayerSpecialAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerSpecialAttackComponent::BeginPlay()
{
	Super::BeginPlay();
	SetSpecialAttack(DefaultSpecialAttack);
}

void UPlayerSpecialAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CooldownRemaining <= 0.0f)
	{
		return;
	}

	const float PreviousCooldown = CooldownRemaining;
	CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - FMath::Max(0.0f, DeltaTime));
	if (!FMath::IsNearlyEqual(PreviousCooldown, CooldownRemaining))
	{
		BroadcastSpecialAttackChanged();
	}
}

void UPlayerSpecialAttackComponent::SetSpecialAttack(USpecialAttackDataAsset* InSpecialAttack)
{
	SpecialAttackAsset = InSpecialAttack;
	RuntimeConfig = InSpecialAttack ? InSpecialAttack->Config : FSpecialAttackConfig();
	CooldownRemaining = 0.0f;
	GrantSpecialAttackAbility();
	BroadcastSpecialAttackChanged();
}

bool UPlayerSpecialAttackComponent::HasSpecialAttack() const
{
	return SpecialAttackAsset && RuntimeConfig.AbilityClass && RuntimeConfig.Montage;
}

bool UPlayerSpecialAttackComponent::UseSpecialAttack()
{
	if (!SpecialAttackAsset || !RuntimeConfig.AbilityClass)
	{
		OnSpecialAttackUseFailed.Broadcast(FText::FromString(TEXT("No special attack equipped")));
		return false;
	}

	if (CooldownRemaining > 0.0f)
	{
		OnSpecialAttackUseFailed.Broadcast(FText::FromString(TEXT("Special attack cooling down")));
		return false;
	}

	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	if (!ASC)
	{
		OnSpecialAttackUseFailed.Broadcast(FText::FromString(TEXT("Missing ability system")));
		return false;
	}

	if (ASC->IsPlayerActionMontageLocked())
	{
		OnSpecialAttackUseFailed.Broadcast(FText::FromString(TEXT("Special attack locked until combo window")));
		return false;
	}

	GrantSpecialAttackAbility();
	const bool bActivated = ASC->TryActivateAbilityByClass(RuntimeConfig.AbilityClass, true);

	if (!bActivated)
	{
		OnSpecialAttackUseFailed.Broadcast(FText::FromString(TEXT("Cannot activate special attack")));
		return false;
	}

	CooldownRemaining = FMath::Max(0.0f, RuntimeConfig.Cooldown);
	const FSpecialAttackSlotView UsedSlot = GetSlotView();
	OnSpecialAttackUsed.Broadcast(UsedSlot);
	BroadcastSpecialAttackChanged();
	return true;
}

void UPlayerSpecialAttackComponent::ClearCooldown()
{
	if (CooldownRemaining <= 0.0f)
	{
		return;
	}

	CooldownRemaining = 0.0f;
	BroadcastSpecialAttackChanged();
}

FSpecialAttackSlotView UPlayerSpecialAttackComponent::GetSlotView() const
{
	FSpecialAttackSlotView View;
	View.SpecialAttackId = RuntimeConfig.SpecialAttackId;
	View.DisplayName = RuntimeConfig.DisplayName;
	View.Icon = RuntimeConfig.Icon;
	View.CooldownRemaining = CooldownRemaining;
	View.CooldownDuration = RuntimeConfig.Cooldown;
	View.bEquipped = SpecialAttackAsset != nullptr;
	return View;
}

void UPlayerSpecialAttackComponent::GrantSpecialAttackAbility()
{
	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	if (!ASC || !RuntimeConfig.AbilityClass)
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == RuntimeConfig.AbilityClass)
		{
			return;
		}
	}

	ASC->GiveAbility(FGameplayAbilitySpec(RuntimeConfig.AbilityClass, 1, INDEX_NONE, this));
}

void UPlayerSpecialAttackComponent::BroadcastSpecialAttackChanged() const
{
	OnSpecialAttackChanged.Broadcast(GetSlotView());
}

APlayerCharacterBase* UPlayerSpecialAttackComponent::GetPlayerOwner() const
{
	return Cast<APlayerCharacterBase>(GetOwner());
}

UYogAbilitySystemComponent* UPlayerSpecialAttackComponent::GetOwnerYogASC() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? Cast<UYogAbilitySystemComponent>(OwnerActor->FindComponentByClass<UAbilitySystemComponent>()) : nullptr;
}
