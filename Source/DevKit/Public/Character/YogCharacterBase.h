// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/Attribute/RuneAttributeSet.h"
#include "AbilitySystem/Abilities/YogTargetType.h"
#include "AbilitySystem/GameplayEffect/YogGameplayEffect.h"

#include "AbilitySystemInterface.h"
#include "Data/CharacterData.h"
#include "Component/AttributeStatComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "YogCharacterBase.generated.h"



enum class EHitStopMode : uint8;
enum class EHitStopScope : uint8;

class AYogPlayerControllerBase;
class UGASTemplate;
class UGameEffectComponent;
class UCharacterDataComponent;
class UBufferComponent;
class URuneDataAsset;

UENUM(BlueprintType)
enum class EYogCharacterState : uint8
{
	Idle					UMETA(DisplayName = "Idle"),
	Move					UMETA(DisplayName = "Move"),
	Action				UMETA(DisplayName = "Action"),
	GetHit					UMETA(DisplayName = "GetHit"),
	Stun					UMETA(DisplayName = "Stun"),
	Dead					UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Unequipped		UMETA(DisplayName = "Unequipped"), 
	Equipping		UMETA(DisplayName = "Equipping"), 
	Equipped		UMETA(DisplayName = "Equipped"), 
	Firing			UMETA(DisplayName = "Firing"), 
	Reloading		UMETA(DisplayName = "Reloading")
};




class UItemInstance;
class UYogAbilitySystemComponent;
class UHitBoxBufferComponent;
class UYogGameplayAbility;
class AItemSpawner;
class AWeaponInstance;
class UCharacterData;
class UPropInteractComponnet;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AYogCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterHealthUpdateDelegate, float, HealthPercent, float, DamageTaken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterMoveableDelegate, const bool, Moveable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterVelocityDelegate, const FVector, Velocity);





DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterStateDelegate, EYogCharacterState, StateBefore, EYogCharacterState, StateAfter);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponStateDelegate, EWeaponState, StateBefore, EWeaponState, StateAfter);


/**
 * 
 */
UCLASS()
class DEVKIT_API AYogCharacterBase : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


public:
	AYogCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;

	//---------------------------------------
	//	Components
	//---------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCharacterDataComponent> CharacterDataComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPropInteractComponnet> PropInteractComponent;

	UPROPERTY()
	TObjectPtr<UBaseAttributeSet> BaseAttributeSet;

	UPROPERTY()
	TObjectPtr<UDamageAttributeSet> DamageAttributeSet;

	UPROPERTY()
	TObjectPtr<URuneAttributeSet> RuneAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> WidgetComponent;



	UAttributeStatComponent* GetAttributeStatsComponent() const;
	UGameEffectComponent* GetGameEffectComponent() const;
	UBufferComponent* GetInputBufferComponent() const;
	UCharacterDataComponent* GetCharacterDataComponent() const;


	UWidgetComponent* GetWidgetcomponent();

	UFUNCTION(BlueprintCallable, Category = "Character")
	UYogAbilitySystemComponent* GetASC() const;


	UFUNCTION()
	UBufferComponent* GetInputBufferComponent();

	UFUNCTION(BlueprintCallable)
	int32 GetStatePriority(EYogCharacterState State);


	//--------------------------------------------
	//	Data table for all character
	//--------------------------------------------

	// 基础通用技能集路径（代码固定，无需编辑器配置）
	// 对应 Content 资产：Content/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial
	static constexpr const TCHAR* BaseAbilitySetPath = TEXT("/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial");

	//---------------------------------------
	//	近战伤害默认配置（子类在蓝图 Class Defaults 中填写，无需每个 EffectContainerMap 重复配置）
	//---------------------------------------

	/** 近战攻击默认使用的 TargetType（决定打击范围和目标筛选）。子类 Blueprint 填写对应 B_TT_Enemy / B_TT_Player。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Melee")
	TSubclassOf<UYogTargetType> DefaultMeleeTargetType;

	/** 近战攻击默认施加的 GameplayEffect（通常为 GE_WeaponHitDamage）。子类 Blueprint 填写。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Melee")
	TSubclassOf<UYogGameplayEffect> DefaultMeleeDamageEffect;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	//TObjectPtr<UAbilityData> AbilityData;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	//TObjectPtr<UGASTemplate> GasTemplate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAttributeStatComponent> AttributeStatsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UGameEffectComponent> GameEffectComponent;






	//DELEGATE DEFINE
	UPROPERTY(BlueprintAssignable,Category = "Character|Attributes")
	FCharacterDiedDelegate OnCharacterDied;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterHealthUpdateDelegate OnCharacterHealthUpdate;



	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterMoveableDelegate OnCharacterCanMoveUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Character|Movement")
	FCharacterVelocityDelegate OnCharacterVelocityUpdate;


	UPROPERTY(BlueprintAssignable, Category = "Character|State")
	FCharacterStateDelegate OnCharacterStateUpdate;



	UFUNCTION(BlueprintCallable)
	EYogCharacterState GetCurrentState();

	UFUNCTION(BlueprintCallable)
	EWeaponState GetWeaponState();


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	void GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities);



	UFUNCTION(BlueprintCallable, Category = "Character|Abilities")
	void GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel);


	UFUNCTION(BlueprintCallable, Category = "Character|Debug")
	void PrintAllGameplayTags(const FGameplayTagContainer& TagContainer);



	///** Map of gameplay tags to gameplay effect containers */
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character|Buff")
	//TMap<FGameplayTag, FYogGameplayEffectContainer> BufferMap;



	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable)
	FVector GetGroundSlope(float length);

	UFUNCTION(BlueprintCallable)
	void UpdateCharacterMovement(const bool IsMovable);

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void DisableMovement();

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void EnableMovement();

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void UpdateCharacterState(EYogCharacterState newState);


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMovable;

	// 当前连击节是否命中了目标（GA_MeleeAttack::OnEventReceived 设置，AN_EnemyComboSection 读取后重置）
	bool bComboHitConnected = false;

	// AN_MeleeDamage notify 存入的附加命中 Rune，GA_MeleeAttack::OnEventReceived 触发到命中目标后清空
	TArray<TObjectPtr<URuneDataAsset>> PendingAdditionalHitRunes;

	// AN_MeleeDamage 存入的 HitStop 覆盖参数，BFNode_HitStop 读取后清空
	struct FPendingHitStopOverride
	{
		bool bActive = false;
		EHitStopMode Mode{};
		EHitStopScope Scope{};
		float FrozenDuration = 0.f;
		float SlowDuration = 0.f;
		float SlowRate = 0.3f;
		float CatchUpRate = 2.0f;
	};
	FPendingHitStopOverride PendingHitStopOverride;
	void ConsumePendingHitStop(const TArray<AActor*>& HitActors);

	// AN_MeleeDamage 存入的命中事件 Tag，GA_MeleeAttack 命中目标时广播后清空
	TArray<FGameplayTag> PendingOnHitEventTags;

	/**
	 * 命中目标时触发附加符文效果的入口（由 GA_MeleeAttack::OnEventReceived 调用）。
	 * 在 Blueprint 中重写：启动 RuneDA 的 FlowAsset，或执行其他符文激活逻辑。
	 * @param RuneDA    要触发的符文 DataAsset
	 * @param Instigator 发起攻击的角色
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character|Combat")
	void ReceiveOnHitRune(URuneDataAsset* RuneDA, AActor* AttackInstigator);
	virtual void ReceiveOnHitRune_Implementation(URuneDataAsset* RuneDA, AActor* AttackInstigator);


	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBufferComponent> InputBufferComponent;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	TArray<TSubclassOf<UYogGameplayAbility>> GameplayAbilities;


	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;


	//Attribute change delegate
	virtual void HealthChanged(const FOnAttributeChangeData& Data);
	virtual void MaxHealthChanged(const FOnAttributeChangeData& Data);




	//UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual bool IsAlive() const;

	//UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void FinishDying();

	//UFUNCTION(BlueprintCallable, Category = "Feature")
	
	virtual void Die();
	// Friended to allow access to handle functions above
	friend UBaseAttributeSet;
	friend URuneAttributeSet;
	friend UDamageAttributeSet;
	//friend UAdditionAttributeSet;

	// ─── 命中闪白 / 攻击前闪红 ──────────────────────────────────────────────

	/** 角色闪光 Overlay 材质（需含 FlashColor、FlashAlpha、Power 三个参数） */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual")
	TObjectPtr<UMaterialInterface> CharacterFlashMaterial;

	/** 命中闪白持续时间（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual", meta = (ClampMin = "0.05"))
	float HitFlashDuration = 0.12f;

	/** 攻击前闪红脉冲频率（次/秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual", meta = (ClampMin = "0.5"))
	float PreAttackPulseFreq = 4.0f;

	/** 攻击前红光 Fresnel 指数（越小越整体发红，越大越集中在边缘）推荐 0.5–2.0 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float PreAttackFresnelPower = 1.0f;

	/** 攻击前红光最大强度（乘在 Alpha 上，推荐 0.6–1.0） */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PreAttackMaxAlpha = 0.85f;

	/** 霸体金光脉冲频率（次/秒，推荐 4–8） */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual", meta = (ClampMin = "0.5"))
	float SuperArmorPulseFreq = 6.f;

	/** 由 HealthChanged 自动调用；也可蓝图手动触发 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void StartHitFlash();

	/** 攻击前摇开始时调用（AnimNotify 或 GA 中） */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void StartPreAttackFlash();

	/** 攻击动作结束/取消时调用 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void StopPreAttackFlash();

	/** 霸体激活时调用（ASC 内部触发，也可蓝图手动调用） */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void StartSuperArmorFlash();

	/** 霸体结束时调用 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void StopSuperArmorFlash();

private:
	UPROPERTY()
	EYogCharacterState CurrentState;

	UPROPERTY()
	EYogCharacterState PreviousState;

	UPROPERTY()
	EWeaponState CurrentWeaponState;

	void InitializeComponentsWithStats(UCharacterData* characterData);

	void InitializeStats(const FYogBaseAttributeData* attributeData) const;

	void InitializeMovement(const FMovementData* movementData) const;

	void TickCharacterFlash(float DeltaTime);

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FlashDynMat;

	float HitFlashElapsed       = -1.f;
	bool  bPreAttackActive      = false;
	float PreAttackElapsed      = 0.f;
	bool  bSuperArmorFlashActive = false;
	float SuperArmorFlashElapsed = 0.f;

	FTimerHandle HitStopMovementRestoreHandle;
};



FORCEINLINE UAttributeStatComponent* AYogCharacterBase:: GetAttributeStatsComponent() const
{
	return AttributeStatsComponent;
}

FORCEINLINE UGameEffectComponent* AYogCharacterBase::GetGameEffectComponent() const
{
	return GameEffectComponent;
}

FORCEINLINE UBufferComponent* AYogCharacterBase::GetInputBufferComponent() const
{
	return InputBufferComponent;
}

FORCEINLINE UCharacterDataComponent* AYogCharacterBase::GetCharacterDataComponent() const
{
	return CharacterDataComponent;
}
