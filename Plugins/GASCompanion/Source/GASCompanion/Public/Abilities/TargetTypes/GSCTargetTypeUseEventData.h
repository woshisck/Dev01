// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GSCTargetType.h"
#include "GSCTargetTypeUseEventData.generated.h"

/** Trivial target type that pulls the target out of the event data */
UCLASS(NotBlueprintable)
class GASCOMPANION_API UGSCTargetTypeUseEventData : public UGSCTargetType
{
    GENERATED_BODY()
public:
    // Constructor and overrides
    UGSCTargetTypeUseEventData() {}

    /** Uses the passed in event data */
    virtual void GetTargets_Implementation(AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};
