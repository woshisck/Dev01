#include "Component/PlayerSpecialAttackComponent.h"

UPlayerSpecialAttackComponent::UPlayerSpecialAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerSpecialAttackComponent::BeginPlay()
{
	Super::BeginPlay();
	BroadcastSpecialAttackChanged();
}

void UPlayerSpecialAttackComponent::SetSpecialAttack(USpecialAttackDataAsset* InSpecialAttack)
{
	(void)InSpecialAttack;
	BroadcastSpecialAttackChanged();
}

bool UPlayerSpecialAttackComponent::UseSpecialAttack()
{
	OnSpecialAttackUseFailed.Broadcast(FText::FromString(TEXT("Deprecated special attack component; use active skills")));
	return false;
}

void UPlayerSpecialAttackComponent::BroadcastSpecialAttackChanged() const
{
	OnSpecialAttackChanged.Broadcast(GetSlotView());
}
