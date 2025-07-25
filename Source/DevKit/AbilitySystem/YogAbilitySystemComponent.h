#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/YogAbilityTypes.h"
#include "Abilities/GameplayAbility.h"

#include "YogAbilitySystemComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceivedDamageDelegate, UYogAbilitySystemComponent*, SourceASC, float, Damage);


class UYogGameplayAbility;
struct FGameplayTag;
struct FYogGameplayEffectContainer;

UCLASS()
class DEVKIT_API UYogAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this empty's properties
	UYogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Uses a gameplay effect to add the specified dynamic granted tag.
	void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);

	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);

	/** Gets the ability target data associated with the given ability handle and activation info */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);


	//////////////////////////////////Gameplay Tag//////////////////////////////////
	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTag(FGameplayTag Tag, int32 Count);

	UFUNCTION(BlueprintCallable)
	void AddGameplayTag(FGameplayTag Tag, int32 Count);
	////////////////////////////////////////////////////////////////////////////////

	FReceivedDamageDelegate ReceivedDamage;
	virtual void ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage);
	

	UFUNCTION(BlueprintCallable)
	void AddActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToBlock);

	UFUNCTION(BlueprintCallable)
	void RemoveActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToUnblock);

	UFUNCTION(BlueprintCallable)
	UYogGameplayAbility* GetCurrentAbilityClass();

	UFUNCTION(BlueprintCallable)
	void LogAllGrantedAbilities();

	UFUNCTION(BlueprintCallable)
	void GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UYogGameplayAbility*>& ActiveAbilities);
	
	UFUNCTION()
	void OnAbilityActivated(UYogGameplayAbility* ActivatedAbility);

	UFUNCTION()
	void OnAbilityEnded(const FAbilityEndedData& EndedData);

	/** Map of gameplay tags to gameplay effect containers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffer")
	TMap<FGameplayTag, FYogGameplayEffectContainer> EffectContainerMap;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UYogGameplayAbility> CurrentActiveAbility;
	

	UPROPERTY(BlueprintReadOnly)
	FGameplayAbilitySpecHandle CurrentAbilitySpecHandle;



	UFUNCTION(BlueprintCallable)
	void SetAbilityRetriggerable(FGameplayAbilitySpecHandle Handle, bool bCanRetrigger);



};
