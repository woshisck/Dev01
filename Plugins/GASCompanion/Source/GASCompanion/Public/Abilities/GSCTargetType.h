// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GSCTargetType.generated.h"

/**
* Class that is used to determine targeting for abilities
*
* It is meant to be blueprinted to run target logic
*
* This does not subclass GameplayAbilityTargetActor because this class is never instanced into the world
*
* This can be used as a basis for a game-specific targeting blueprint .If your targeting is more complicated
* you may need to instance into the world once or as a pooled actor
*/
UCLASS(Blueprintable, meta = (ShowWorldContextPin))
class GASCOMPANION_API UGSCTargetType : public UObject
{
	GENERATED_BODY()

	public:
	// Constructor and overrides
	UGSCTargetType() {}

	/** Called to determine targets to apply gameplay effects to */
	UFUNCTION(BlueprintNativeEvent)
    void GetTargets(AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const;
};
