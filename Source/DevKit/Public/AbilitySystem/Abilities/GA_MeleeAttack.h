#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Animation/AN_MeleeDamage.h"
#include "Component/CombatDeckComponent.h"
#include "Data/EnemyData.h"
#include "Data/RuneDataAsset.h"
#include "Data/WeaponComboConfigDA.h"
#include "GA_MeleeAttack.generated.h"

class UMontageConfigDA;
class UMontageAttackDataAsset;
class UAbilityTask_ApplyRootMotionMoveToForce;

/**
 * 閫氱敤杩戞垬鏀诲嚮 GA锛堟晫浜?+ 鐜╁鍧囧彲浣跨敤锛夈€?
 *
 * 绛栧垝宸ヤ綔娴侊細
 * 1. 鏂板缓 Blueprint GA锛孭arent Class 閫?GA_MeleeAttack
 * 2. Class Defaults 鈫?AbilityTags 濉啓瀵瑰簲 Tag锛堥』涓庤鑹?CharacterData.AbilityData 琛ㄦ牸琛?Key 涓€鑷达級
 * 3. 鐜╁ GA锛氬嬀閫?bRequireCommit锛孲tatBeforeATKEffect 濉啓 GE_StatBeforeATK
 *    鏁屼汉 GA锛氫繚鎸侀粯璁わ紙bRequireCommit=false锛孲tatBeforeATKEffect 鐣欑┖锛?
 * 4. 娓呯┖ Event Graph锛堟墍鏈夋祦绋嬪凡鍦?C++ 涓疄鐜帮級
 *
 * 钂欏お濂囧拰鍛戒腑妗嗚嚜鍔ㄤ粠 瑙掕壊.CharacterData.AbilityData.AbilityMap[AbilityTags[0]] 璇诲彇锛?
 * 鏃犻渶鍦?GA 涓婂崟鐙厤缃€?
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_MeleeAttack : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MeleeAttack();

	/**
	 * 鏄惁鍦ㄦ縺娲绘椂璋冪敤 CommitAbility锛堟墸闄ゆ秷鑰?+ 瑙﹀彂鍐峰嵈锛夈€?
	 * 鐜╁杩戞垬 GA 濉?true锛涙晫浜鸿繎鎴?GA 淇濇寔 false锛堥粯璁わ級銆?
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	bool bRequireCommit = false;

	/**
	 * 婵€娲绘椂鏂藉姞鍒?Self 鐨?鏀诲嚮鍓嶆憞"GameplayEffect锛堜緥濡?GE_StatBeforeATK锛夈€?
	 * EndAbility 鏃惰嚜鍔ㄧЩ闄わ紝鏃犻渶鎵嬪姩璋冪敤銆?
	 * 鐜╁杩戞垬 GA 濉啓锛涙晫浜?GA 鐣欑┖銆?
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSubclassOf<UGameplayEffect> StatBeforeATKEffect;

	/**
	 * 鎶€鑳芥甯哥粨鏉熸椂锛堥潪 Cancel/Interrupt锛夋柦鍔犵殑"鏀诲嚮鍚庢憞"GameplayEffect锛堜緥濡?GE_StatAfterATK锛夈€?
	 * 璇?GE 鑷韩甯?Duration锛岃嚜鍔ㄥ埌鏈燂紝鏃犻渶鎵嬪姩绉婚櫎銆?
	 * 鐜╁杩戞垬 GA 濉啓锛涙晫浜?GA 鐣欑┖銆?
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSubclassOf<UGameplayEffect> StatAfterATKEffect;

	virtual FActionData GetAbilityActionData_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECardRequiredAction GetCombatDeckActionType() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	bool IsCombatDeckComboFinisher() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Melee")
	bool HasConfiguredAttackData() const;

	const UMontageAttackDataAsset* GetConfiguredAttackData() const { return ActiveComboAttackData; }

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	void SetNextActivationFromDashSave(bool bFromDashSave);

private:
	/** GE_StatBeforeATK 鐨勬縺娲诲彞鏌勶紝EndAbility 鏃惰嚜鍔ㄧЩ闄?*/
	FActiveGameplayEffectHandle StatBeforeATKHandle;

	/**
	 * 涓嶄娇鐢?UPROPERTY 鏄洜涓?AnimNotify 涓嶅弬涓?GC锛岀敓鍛藉懆鏈熼殢钂欏お濂囪祫浜с€?
	 */
	TObjectPtr<UAN_MeleeDamage> CachedDamageNotify;

	UPROPERTY()
	TObjectPtr<UMontageConfigDA> ActiveMontageConfig;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveComboNode;

	UPROPERTY()
	TObjectPtr<UMontageAttackDataAsset> ActiveComboAttackData;

	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> EnemyLungeTask;

	UPROPERTY()
	FEnemyAIAttackRuntimeContext ActiveEnemyAttackContext;

	/**
	 * StatAfterATK 浼樺厛鐢ㄦ鍊硷紙浠ｈ〃鏈€鍚庝竴鍑伙級锛屾湭鍛戒腑杩囨椂 fallback 鍒?CachedDamageNotify銆?
	 * EventData.OptionalObject 鏄?const UObject*锛屾晠鐢ㄥ師濮?const 鎸囬拡锛汚nimNotify 涓嶅弬涓?GC銆?
	 */
	const UAN_MeleeDamage* LastFiredDamageNotify = nullptr;

	bool bCombatDeckCardResolvedThisActivation = false;
	bool bCombatDeckFromDashSave = false;
	bool bNextActivationFromDashSave = false;
	bool bActiveComboNodeValid = false;
	bool bComboContinued = true;
	bool bExitedComboState = false;
	int32 CombatDeckHitResolveCounter = 0;
	bool bHasStatBeforeAttributeSnapshot = false;
	float LocalPreStatBeforeAttack = 0.f;
	float LocalPreStatBeforeAttackPower = 0.f;
	float LocalStatBeforeAttackDelta = 0.f;
	float LocalStatBeforeAttackPowerDelta = 0.f;
	FTimerHandle ComboWindowOpenTimerHandle;
	FTimerHandle ComboWindowCloseTimerHandle;
	FDelegateHandle CanComboTagHandle;
	float AbilityActivationTime = 0.f;

	UPROPERTY()
	FCombatCardResolveResult ActiveCombatCardResult;

	FGuid ActiveAttackGuid;
	int32 ActiveComboIndex = 0;
	FGameplayTagContainer ActiveComboTags;

	/** Finds the first AN_MeleeDamage notify on the selected montage. */
	static UAN_MeleeDamage* GetFirstDamageNotify(UAnimMontage* Montage);

	void ScheduleNodeComboWindow(UAnimMontage* Montage, float PlayRate);
	void TryStartEnemyRadialLunge();
	void OpenNodeComboWindow();
	void CloseNodeComboWindow();

	AActor* PreviewFirstCombatDeckHitTarget(const FGameplayEventData& EventData) const;
	void PrimeCombatDeckHitContext(const FGameplayEventData& EventData);

	void TryResolveCombatDeckOnHit();
	FCombatCardResolveResult ResolveCombatDeck(ECombatCardTriggerTiming TriggerTiming);

	UFUNCTION()
	void OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);
};
