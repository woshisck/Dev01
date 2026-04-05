#include "AbilitySystem/Abilities/RuneGameplayAbility.h"
#include "Data/RuneDataAsset.h"

URuneDataAsset* URuneGameplayAbility::GetGrantingDA() const
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!Spec)
		return nullptr;

	return Cast<URuneDataAsset>(Spec->SourceObject.Get());
}
