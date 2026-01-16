#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/YogAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "DevKit/Data/AbilityData.h"

#include "YogAbilitySystemComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceivedDamageDelegate, UYogAbilitySystemComponent*, SourceASC, float, Damage);




class UYogGameplayAbility;
struct FGameplayTag;
struct FYogGameplayEffectContainer;
struct FWeaponSaveData;



USTRUCT(BlueprintType,Blueprintable)
struct DEVKIT_API FYogContainerEffect 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EYogEffectTarget target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int effect_level;

};


UCLASS(BlueprintType)
class DEVKIT_API UYogAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:


	//ASAP: NEED TO MODIFIER
	UPROPERTY(BlueprintReadOnly)
	TArray<FGameplayAbilitySpec> WeaponAbilities;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGameplayAbilitySpec> GeneralAbilities;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGameplayAbilitySpec> PassiveAbilities;



	// Sets default values for this empty's properties
	UYogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Uses a gameplay effect to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable)
	void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);

	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable)
	void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);




	/** Gets the ability target data associated with the given ability handle and activation info */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);


	UFUNCTION(BlueprintCallable)
	FGameplayAbilitySpecHandle GrantAbility(TSubclassOf<UYogGameplayAbility> ability_class);


	UFUNCTION(BlueprintCallable)
	void RemoveAbility(TSubclassOf<UYogGameplayAbility> ability_class);


	//////////////////////////////////Gameplay Tag//////////////////////////////////
	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTag(FGameplayTag Tag, int32 Count);

	UFUNCTION(BlueprintCallable)
	void AddGameplayTag(FGameplayTag Tag, int32 Count);

	////////////////////////////////////////////////////////////////////////////////


	UPROPERTY(BlueprintAssignable, Category = "DamageTaken")
	FReceivedDamageDelegate ReceivedDamage;

	virtual void ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage);
	

	UFUNCTION(BlueprintCallable)
	void AddActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToBlock);




	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool TryActivateRandomAbilitiesByTag(const FGameplayTagContainer& GameplayTagContainer, bool bAllowRemoteActivation = true);



	UFUNCTION(BlueprintCallable)
	void RemoveActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToUnblock);

	UFUNCTION(BlueprintCallable)
	UYogGameplayAbility* GetCurrentAbilityClass();

	UFUNCTION(BlueprintCallable)
	void LogAllGrantedAbilities();

	UFUNCTION(BlueprintCallable)
	TArray<FAbilitySaveData> GetAllGrantedAbilities();

	UFUNCTION(BlueprintCallable)
	void GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UYogGameplayAbility*>& ActiveAbilities);
	
	UFUNCTION()
	void OnAbilityActivated(UYogGameplayAbility* ActivatedAbility);

	UFUNCTION()
	void OnAbilityEnded(const FAbilityEndedData& EndedData);

	/** Map of gameplay tags to gameplay effect containers */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffer", meta = (ForceInlineRow))
	TMap<FGameplayTag, FYogGameplayEffectContainer> EffectContainerMap;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	//TSubclassOf<UGameplayEffect> GameplayEffect;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	//int EffectLevel;

	UFUNCTION(BlueprintCallable)
	void AddEffectToContainer(FGameplayTag gameplayTag, TSubclassOf<UGameplayEffect> GameplayEffect, int effect_level);

	UFUNCTION(BlueprintCallable)
	void RemoveEffectFromContainer(FGameplayTag gameplayTag, TSubclassOf<UGameplayEffect> GameplayEffect, int effect_level);

	//Test For Future for dumping 
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffer")
	//TArray<FYogEffectPorperty> YogEffects;


	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UYogGameplayAbility> CurrentActiveAbility;
	

	UPROPERTY(BlueprintReadOnly)
	FGameplayAbilitySpecHandle CurrentAbilitySpecHandle;



	UFUNCTION(BlueprintCallable)
	void SetAbilityRetriggerable(FGameplayAbilitySpecHandle Handle, bool bCanRetrigger);



};
