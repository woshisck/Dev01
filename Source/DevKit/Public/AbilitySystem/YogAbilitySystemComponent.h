#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/YogAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AbilityData.h"
#include "Data/RuneDataAsset.h"
#include "Data/StateConflictDataAsset.h"
#include "Item/Weapon/WeaponTypes.h"
#include "YogAbilitySystemComponent.generated.h"

class UYogAbilitySystemComponent;


// =========================================================
// 伤害明细结构体（DamageBreakdownWidget 使用）
// =========================================================

/**
 * 一次伤害命中的完整构成数据。
 * DamageExecution 填充后通过 OnDamageBreakdown 广播给 Widget。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FDamageBreakdown
{
	GENERATED_BODY()

	/** 来源的 Attack 属性（基础攻击力） */
	UPROPERTY(BlueprintReadOnly) float BaseAttack = 0.f;

	/** 动作系数（AttackPower，由动作 Notify 的 ActDamage 设置） */
	UPROPERTY(BlueprintReadOnly) float ActionMultiplier = 1.f;

	/** 目标减伤系数（DmgTaken，>1 表示易伤，<1 表示减伤） */
	UPROPERTY(BlueprintReadOnly) float DmgTakenMult = 1.f;

	/** 最终伤害（含暴击加成） */
	UPROPERTY(BlueprintReadOnly) float FinalDamage = 0.f;

	/** 是否暴击 */
	UPROPERTY(BlueprintReadOnly) bool bIsCrit = false;

	/** 动作名称，如 "轻击1"、"重击2"；流血/符文时为对应名称 */
	UPROPERTY(BlueprintReadOnly) FName ActionName;

	/** 伤害类型："Attack"、"Attack_Crit"、"Bleed"、"Rune_XXX" */
	UPROPERTY(BlueprintReadOnly) FName DamageType;

	/** 目标名称（调试用） */
	UPROPERTY(BlueprintReadOnly) FString TargetName;

	/** 来源Actor名称（调试用） */
	UPROPERTY(BlueprintReadOnly) FString SourceName;

	/** 伤害发生时的游戏时间（秒），由 Widget 在 HandleDamageEntry 时填入 */
	UPROPERTY(BlueprintReadOnly) float GameTime = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageBreakdown, FDamageBreakdown, Entry);

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

	// ── 韧性系统内部状态 ──────────────────────────────────────────────
	/** 连续触发受击次数（5s 无受击后归零，达到 SuperArmorThreshold 时触发霸体） */
	int32 PoiseHitCount = 0;
	FTimerHandle PoiseResetTimer;
	FTimerHandle SuperArmorTimer;
	bool bPoiseSuperArmorActive = false;

	void TriggerSuperArmorCounterAttack();
	void OnPoiseResetTimerEnd();
	void OnSuperArmorTimerEnd();

	// ── 冲刺连招保存内部状态 ───────────────────────────────────────────
	/** 当前保存的连招 Tag 集合（ApplyDashSave 注入 / ConsumeDashSave 移除） */
	FGameplayTagContainer DashSaveComboTags;
	/** 2s 后自动消费（未被攻击消费时清理） */
	FTimerHandle DashSaveExpireTimer;

	void DashSaveExpired();

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

	// ── 武器类型 Tag 守卫 ─────────────────────────────────────────────
	// 装备时挂 Weapon.Type.Melee/Ranged LooseTag，使对应类型的攻击 GA 才能激活。
	// 内部使用 SetLooseGameplayTagCount(Tag, 0) 清零避免计数残留；未来切多人时
	// 把这两个函数内部从 LooseTag 改成 ReplicatedLooseTag 即可，调用方零改动。

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ApplyWeaponTypeTag(EWeaponType Type);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ClearWeaponTypeTags();

	////////////////////////////////////////////////////////////////////////////////


	/**
	 * 受击时自动发送的 GameplayEvent Tag（供 GA_GetHit 通过 Trigger 监听）
	 * 在角色蓝图 CDO 上设置，留空则不发送事件（可用于无受击反应的对象）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	FGameplayTag HitReactEventTag;

	// ── 韧性（Poise）系统 ─────────────────────────────────────────────
	/**
	 * 动作韧性：GA_MeleeAttack 在命中 Notify 触发前设置（= AN_MeleeDamage.ActResilience），
	 * ReceiveDamage 读取后立即清零。无需手动维护，GA 自动管理。
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Poise")
	float CurrentActionPoiseBonus = 0.f;

	/**
	 * 连续触发受击多少次后进入霸体（仅对非玩家生效）。
	 * 默认 3 次。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poise")
	int32 SuperArmorThreshold = 3;

	/**
	 * 霸体持续时间（秒）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poise")
	float SuperArmorDuration = 2.f;

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

	/**
	 * 玩家伤害日志（基础版，向后兼容）。
	 * 屏幕上滚动显示最近 30 条，同时写入 Output Log，并广播 OnDamageBreakdown。
	 */
	UFUNCTION(BlueprintCallable, Category = "DamageLog")
	void LogDamageDealt(AActor* Target, float Damage, FName DamageType);

	/**
	 * 玩家伤害日志（详细版，DamageExecution 调用）。
	 * 携带完整的伤害构成数据，广播给 DamageBreakdownWidget。
	 */
	UFUNCTION(BlueprintCallable, Category = "DamageLog")
	void LogDamageDealtDetailed(AActor* Target, const FDamageBreakdown& Breakdown);

	/** 伤害明细事件，DamageBreakdownWidget 订阅此委托刷新显示 */
	UPROPERTY(BlueprintAssignable, Category = "DamageLog")
	FOnDamageBreakdown OnDamageBreakdown;


	UFUNCTION(BlueprintCallable)
	void AddActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToBlock);




	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool TryActivateRandomAbilitiesByTag(const FGameplayTagContainer& GameplayTagContainer, bool bAllowRemoteActivation = true);



	/**
	 * [Debug] 检查某个 Actor 是否已被赋予指定 GA class。
	 * 静态函数，可在任意类中调用。Actor 无 ASC 或 Class 为空时返回 false。
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities|Debug", meta = (DisplayName = "Has Ability Class (Debug)"))
	static bool DebugHasAbilityClass(AActor* Actor, TSubclassOf<UGameplayAbility> AbilityClass);

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

	// ── 冲刺连招保存 ────────────────────────────────────────────────────────
	/**
	 * 以 LooseGameplayTag 方式注入连招 Tag，使玩家冲刺后可直接接 X 招。
	 * 自动在 2s 后过期；或由 ConsumeDashSave 主动消费。
	 * GA_PlayerDash::EndAbility 在检测到处于 X-1 招位时调用。
	 */
	void ApplyDashSave(const FGameplayTagContainer& Tags);

	/**
	 * 消费（移除）之前注入的连招 Tag，并清除过期计时器。
	 * LightAtk4 / HeavyAtk4 在 ActivateAbility 时调用。
	 */
	bool ConsumeDashSave();

};
