#pragma once

#include "YogAbilityTypes.h"
#include "YogTargetType.generated.h"

class AYogCharacterBase;
class AActor;
struct FGameplayEventData;


UCLASS(Blueprintable, meta = (ShowWorldContextPin))
class DEVKIT_API UYogTargetType : public UObject
{
	GENERATED_BODY()

public:
	// Constructor and overrides
	UYogTargetType() {}

	/** Called to determine targets to apply gameplay effects to */
	UFUNCTION(BlueprintNativeEvent)
	void GetTargets(int HitboxIndex, AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const;
};

/** Trivial target type that uses the owner */
UCLASS(NotBlueprintable)
class DEVKIT_API UYogTargetType_UseOwner : public UYogTargetType
{
	GENERATED_BODY()

public:
	// Constructor and overrides
	UYogTargetType_UseOwner() {}

	/** Uses the passed in event data */
	virtual void GetTargets_Implementation(int HitboxIndex, AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};

/** Trivial target type that pulls the target out of the event data */
UCLASS(NotBlueprintable)
class DEVKIT_API UYogTargetType_UseEventData : public UYogTargetType
{
	GENERATED_BODY()

public:
	// Constructor and overrides
	UYogTargetType_UseEventData() {}

	/** Uses the passed in event data */
	virtual void GetTargets_Implementation(int HitboxIndex, AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};
