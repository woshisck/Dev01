#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UAnimInstance;
class UAnimMontage;
class UYogAbilitySystemComponent;

/**
 * Holds a phase tag on the ASC matching the montage section under the playhead, so
 * UMontagePhaseFlowComponent can react to attack phases.
 *
 * Section boundaries are the animation itself, so a retime cannot desync them the way authored time
 * values do. Every ability task that plays a montage owns one of these and must Clear() it on
 * destruction: there is no NotifyEnd to fall back on, so an interrupted attack would otherwise hold
 * its phase tag forever.
 */
class FMontagePhaseTagDriver
{
public:
	/** Swaps the held tag when the playhead crosses into a section that maps to a different phase. */
	void Update(UAnimInstance& AnimInstance, UAnimMontage* Montage, UYogAbilitySystemComponent* ASC);

	/** Releases the held tag. Safe to call when nothing is held. */
	void Clear(UYogAbilitySystemComponent* ASC);

private:
	FName CurrentSection = NAME_None;
	FGameplayTag CurrentPhaseTag;
};
