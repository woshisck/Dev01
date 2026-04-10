#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/YogAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AbilityData.h"
#include "Data/RuneDataAsset.h"
#include "Data/StateConflictDataAsset.h"
#include "YogAbilitySystemComponent.generated.h"

class UYogAbilitySystemComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceivedDamageDelegate, UYogAbilitySystemComponent*, SourceASC, float, Damage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceiveHitResultDelegate, class UYogAbilitySystemComponent*, SourceASC, bool, HitResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDealtDamageDelegate, UYogAbilitySystemComponent*, TargetASC, float, Damage);

/** 造成暴击时在 Source ASC 广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCritHitDelegate, UYogAbilitySystemComponent*, TargetASC, float, Damage);

/** 冲刺成功时在 Owner ASC 广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDashExecutedDelegate);

/** 击杀目标时在 Source ASC 广播，携带目标 Actor 和死亡位置 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKilledTargetDelegate, AActor*, Target, FVector, DeathLocation);





class UYogGameplayAbility;
struct FGameplayTag;
struct FWeaponSaveData;






UCLASS(BlueprintType)
class DEVKIT_API UYogAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:



	// Sets default values for this empty's properties
	UYogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	UPROPERTY(BlueprintReadWrite)
	TMap<FGameplayTag, FActionData> AbilityMap;

	UPROPERTY(BlueprintReadWrite)
	TMap<FGameplayTag, FPassiveActionData> PassiveMap;


	UFUNCTION(BlueprintCallable)
	void ApplyAbilityData(UAbilityData* abilityData);


	// Uses a gameplay effect to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable)
	void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);

	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable)
	void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);




	/** Gets the ability target data associated with the given ability handle and activation info */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);



	// =========================================================
	// 状态冲突系统
	// =========================================================

	/**
	 * 状态冲突规则表（DataAsset）
	 * 推荐在角色蓝图 CDO 上统一赋值（EditDefaultsOnly），
	 * 或在 GameInstance / GameData 中持有一份全局引用后调用 InitConflictTable()。
	 * 不赋值时系统静默跳过，不会崩溃。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StateConflict")
	TObjectPtr<UStateConflictDataAsset> ConflictTable;

	/**
	 * 手动触发冲突表初始化（可在 InitAbilityActorInfo 之后或 BeginPlay 中调用）。
	 * 内部会自动构建 O(1) 查找用的 ConflictMap。
	 */
	UFUNCTION(BlueprintCallable, Category = "StateConflict")
	void InitConflictTable();

	/** 运行时替换冲突表并重新初始化（热更新用） */
	UFUNCTION(BlueprintCallable, Category = "StateConflict")
	void SetConflictTable(UStateConflictDataAsset* NewTable);

protected:
	// OnTagUpdated override：tag 变化时自动查表执行 block / cancel
	virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists) override;

private:
	// 预处理后的查找表（ActiveTag → Rule），由 InitConflictTable() 构建
	TMap<FGameplayTag, FStateConflictRule> ConflictMap;

	// 阻断分类表（BlockCategory → 触发该阻断的 State Tags），从 DA 复制
	TMap<FGameplayTag, FGameplayTagContainer> BlockCategoryMap;

	// 反向索引（StateTag → 所属阻断分类列表），OnTagUpdated O(1) 查找
	TMap<FGameplayTag, TArray<FGameplayTag>> StateToBlockCategories;

	// 防止 OnTagUpdated 递归（BlockAbilitiesWithTags 内部也会触发 tag 变化）
	bool bProcessingConflict = false;

public:
	//////////////////////////////////Gameplay Tag//////////////////////////////////
	UFUNCTION(BlueprintCallable)
	void GetOwnedGameplayTag();

	UFUNCTION(BlueprintCallable)
	TMap<FGameplayTag, int32> GetPlayerOwnedTagsWithCounts();

	UFUNCTION(BlueprintCallable)
	void PrintPlayerOwnedTagsWithCounts(TMap<FGameplayTag, int32> TagCounts);

	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTagWithCount(FGameplayTag Tag, int32 Count);

	UFUNCTION(BlueprintCallable)
	void AddGameplayTagWithCount(FGameplayTag Tag, int32 Count);



	////////////////////////////////////////////////////////////////////////////////


	/**
	 * 受击时自动发送的 GameplayEvent Tag（供 GA_GetHit 通过 Trigger 监听）
	 * 在角色蓝图 CDO 上设置，留空则不发送事件（可用于无受击反应的对象）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	FGameplayTag HitReactEventTag;

	UPROPERTY(BlueprintAssignable, Category = "DamageTaken")
	FReceivedDamageDelegate ReceivedDamage;

	UPROPERTY(BlueprintAssignable)
	FReceiveHitResultDelegate ReceiveHitResult;

	UPROPERTY(BlueprintAssignable, Category = "DamageDealt")
	FDealtDamageDelegate DealtDamage;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FCritHitDelegate OnCritHit;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FDashExecutedDelegate OnDashExecuted;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FKilledTargetDelegate OnKilledTarget;

	virtual void ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage);
	

	UFUNCTION(BlueprintCallable)
	void AddActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToBlock);




	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool TryActivateRandomAbilitiesByTag(const FGameplayTagContainer& GameplayTagContainer, bool bAllowRemoteActivation = true);



	UFUNCTION(BlueprintCallable)
	void RemoveActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToUnblock);

	UFUNCTION(BlueprintCallable)
	//DEPRECATED, DONT USE THIS
	UYogGameplayAbility* GetCurrentAbilityInstance();

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

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UYogGameplayAbility> CurrentActiveAbility;
	

	//UPROPERTY(BlueprintReadOnly)
	//FGameplayAbilitySpecHandle CurrentAbilitySpecHandle;



	UFUNCTION(BlueprintCallable)
	void SetAbilityRetriggerable(FGameplayAbilitySpecHandle Handle, bool bCanRetrigger);


	// 移除之前 Apply 的符文 GE
	UFUNCTION(BlueprintCallable, Category = "Rune")
	void RemoveRuneModifiers(FActiveGameplayEffectHandle Handle);

};
