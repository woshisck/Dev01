// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GSCAbilityQueueNotifyState.generated.h"

/**
 * Use this notify state to open and close the ability queue window for your montage.
 *
 * If this montage is triggered from within a Gameplay Ability, any ability that is failing to
 * activate during this window will be queued for activation when the current active one ends.
 */
UCLASS(meta = (DisplayName = "AbilityQueueWindow"))
class GASCOMPANION_API UGSCAbilityQueueNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	/**
	 * If true, enable queueing of all abilities, otherwise only allow Abilities that are defined in AllowedAbilities array.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	bool bAllowAllAbilities = false;

	/**
	 * The set of Abilities that can be queued for this window (has no effect if bAllowAllAbilities is set to true)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	TArray<TSubclassOf<UGameplayAbility>> AllowedAbilities;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;
};
