#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameplayEffectTypes.h"
#include "DataCacheComponent.generated.h"


USTRUCT(BlueprintType)
struct FAnimationUseCache
{
	GENERATED_BODY()

    TMap<FGameplayTag, UAnimMontage*> AnimationMapCache;

};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UDataCacheComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UDataCacheComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;


};
