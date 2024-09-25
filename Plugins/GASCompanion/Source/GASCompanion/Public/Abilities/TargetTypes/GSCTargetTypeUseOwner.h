// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GSCTargetType.h"
#include "GSCTargetTypeUseOwner.generated.h"

/** Trivial target type that uses the owner */
UCLASS(NotBlueprintable)
class GASCOMPANION_API UGSCTargetTypeUseOwner : public UGSCTargetType
{
    GENERATED_BODY()

public:
    // Constructor and overrides
    UGSCTargetTypeUseOwner() {}

    /** Uses the passed in event data */
    virtual void GetTargets_Implementation(AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};
