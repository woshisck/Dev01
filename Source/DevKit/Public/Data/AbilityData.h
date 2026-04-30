// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;
class UMontageConfigDA;


UENUM(BlueprintType)
enum class EYogEffectTarget : uint8
{
	ToSelf,
	ToTarget
};


UENUM(BlueprintType)
enum class EHitBoxType : uint8
{
	Annulus,
	Triangle,
	Square,
	Circle
};

USTRUCT(Blueprintable)
struct FHitboxAnnulus
{
	GENERATED_BODY()

	/** 内圆剔除半径（圆心到目标距离小于此值则不命中）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float inner_radius = 0;

	/** 扇形圆弧角度（度）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float degree = 0;

	/**
	 * 自动偏移（默认开）：
	 *   开 — 圆心沿角色朝向后移 inner_radius，使扇形内沿恰好贴合玩家位置；OffsetCore 被忽略。
	 *   关 — 圆心沿角色朝向偏移 OffsetCore（正值=向前，负值=向后），完全手动控制。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoOffset = true;

	/**
	 * 手动模式下圆心的前后位移（世界单位）。
	 * 正值沿角色朝向向前，负值向后。bAutoOffset = true 时此值被忽略。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bAutoOffset", EditConditionHides))
	float OffsetCore = 0;

};

USTRUCT(Blueprintable)
struct FHitboxTriangle
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Degree = 0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Radius = 0;
};


USTRUCT(Blueprintable)
struct FHitboxSquare
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Length = 0;
};


USTRUCT(Blueprintable)
struct FHitboxCircle
{
	GENERATED_BODY()

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Radius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Offset = FVector(0,0,0);
};

USTRUCT(Blueprintable)
struct FYogHitboxType
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitBoxType hitboxType = EHitBoxType::Annulus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus", EditConditionHides))
	FHitboxAnnulus AnnulusHitbox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Circle", EditConditionHides))
	TArray<FHitboxCircle> HitboxCircles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Square", EditConditionHides))
	TArray<FHitboxSquare> HitboxSquares;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle", EditConditionHides))
	TArray<FHitboxTriangle> HitboxTriangles;



};



USTRUCT(Blueprintable)
struct FYogTagContainerWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer TagContainer;

	// Equality operator
	bool operator==(const FYogTagContainerWrapper& Other) const
	{
		// This uses the FGameplayTagContainer's own == operator, which checks for exact matching tags.
		return TagContainer == Other.TagContainer;
	}

	// Hash function
	friend uint32 GetTypeHash(const FYogTagContainerWrapper& Wrapper)
	{
		// A simple but potentially inefficient hash: combine hashes of all individual tags.
		uint32 Hash = 0;
		for (const FGameplayTag& Tag : Wrapper.TagContainer)
		{
			Hash = HashCombine(Hash, GetTypeHash(Tag));
		}
		return Hash;
	}
};



USTRUCT(BlueprintType)
struct FYogApplyEffect 
{
	GENERATED_BODY()

public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//EYogEffectTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> GameplayEffect;

	// Add equality operator to define "sameness"
	bool operator==(const FYogApplyEffect& Other) const
	{
		//return Target == Other.Target && TriggerTag == Other.TriggerTag && GameplayEffect == Other.GameplayEffect;

		return TriggerTag == Other.TriggerTag && GameplayEffect == Other.GameplayEffect;
	}

	// Also need != operator
	bool operator!=(const FYogApplyEffect& Other) const
	{
		return !(*this == Other);
	}

	// For TMap key, add GetTypeHash
	//friend uint32 GetTypeHash(const FYogApplyEffect& Effect)
	//{
	//	//uint32 Hash = GetTypeHash(Effect.Target);
	//	uint32 Hash = HashCombine(Hash, GetTypeHash(Effect.TriggerTag));
	//	Hash = HashCombine(Hash, GetTypeHash(Effect.GameplayEffect));
	//	return Hash;
	//}
	friend uint32 GetTypeHash(const FYogApplyEffect& Effect)
	{
		uint32 Hash = GetTypeHash(Effect.GameplayEffect);
		Hash = HashCombine(Hash, GetTypeHash(Effect.level));
		return Hash;
	}


};


USTRUCT(BlueprintType)
struct FHitBoxData 
{
	GENERATED_BODY()

public:
	//FHitBoxData()
	//	: Point(FVector(0, 0, 0)), HasTriggered(false), Index(0), FrameAt(0)
	//{
	//}


	UPROPERTY(BlueprintReadOnly)
	//FVector Point = FVector(Radius * cos(Degree), Radius * sin(Degree), 0.0);
	FVector Point = FVector(0,0,0);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FVector Location_Start;


	/*
	* 	Annulus,
		Triangle,
		Square,
		Circle
	*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus"))
	float inner_radius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus"))
	float outer_radius = 0;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle"))
	float Degree = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle"))
	float Radius = 0;


};

USTRUCT(BlueprintType)
struct FPassiveActionData
{
	GENERATED_BODY()

public:
	FPassiveActionData() {}


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYogApplyEffect> UniqueEffects;

	/**
	 * 死亡消解特效的 GameplayCue Tag（留空则不触发消解）
	 * 无论是否有死亡蒙太奇，都在死亡动画（或 2s 等待）结束后触发
	 * 在对应的 GameplayCue BP 里配置 Niagara/材质消解/音效等效果
	 * ⚠️ GC 内的粒子需要在世界坐标生成（非附加模式），否则 Actor 销毁后粒子也会消失
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dissolve")
	FGameplayTag DissolveGameplayCueTag;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<UYogGameplayAbility> Ability_Template;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
	//TArray<FYogHitboxType> hitboxTypes;




};



USTRUCT(BlueprintType)
struct FActionData
{
    GENERATED_BODY()

public:
	FActionData(){}
    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActDamage = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActRange = 400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActResilience = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActDmgReduce = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYogApplyEffect> UniqueEffects;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<UYogGameplayAbility> Ability_Template;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
	TArray<FYogHitboxType> hitboxTypes;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FTaggedMontageConfig
{
	GENERATED_BODY()

	/** Tags that must be present on the ASC/current combo context. Empty means always allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage Config")
	FGameplayTagContainer RequiredTags;

	/** Tags that reject this candidate when present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage Config")
	FGameplayTagContainer BlockedTags;

	/** Larger priority wins when more than one candidate matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage Config")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage Config")
	TObjectPtr<UMontageConfigDA> MontageConfig;

	bool Matches(const FGameplayTagContainer& ContextTags) const
	{
		const bool bRequiredOk = RequiredTags.IsEmpty() || ContextTags.HasAll(RequiredTags);
		const bool bBlockedOk = BlockedTags.IsEmpty() || !ContextTags.HasAny(BlockedTags);
		return bRequiredOk && bBlockedOk && MontageConfig != nullptr;
	}
};

USTRUCT(BlueprintType)
struct DEVKIT_API FAbilityMontageConfigList
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage Config")
	TArray<FTaggedMontageConfig> Configs;
};




USTRUCT(BlueprintType)
struct DEVKIT_API FCharacterAnimationsConfig : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (Categories = "Animation"))
	TMap<FGameplayTag, UAnimMontage*> MontagesList;
};



UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	/**
	 * 技能 Tag → 蒙太奇映射。
	 * 攻击参数（伤害、范围、命中框等）已迁移至蒙太奇内的 AN_MeleeDamage Notify 上配置。
	 * ForceInlineRow：每条记录 Key/Value 平铺在同一行，不展开。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ForceInlineRow))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> MontageMap;

	/**
	 * Ability Tag -> selectable montage configs.
	 *
	 * This is the configurable combo path. Multiple configs can share one
	 * montage but hold different Entries / AttackData lists, selected by tags.
	 * MontageMap remains as a fallback for existing content.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Montage Config")
	TMap<FGameplayTag, FAbilityMontageConfigList> MontageConfigMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow), Category = "Action|General")
	TMap<FGameplayTag, FPassiveActionData> PassiveMap;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	UAnimMontage* GetMontage(const FGameplayTag& Key) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	UMontageConfigDA* GetMontageConfig(const FGameplayTag& Key, const FGameplayTagContainer& ContextTags) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool HasAbility(const FGameplayTag& Key) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FPassiveActionData GetPassiveAbility(const FGameplayTag& Key) const
	{
		return PassiveMap.FindRef(Key);
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool HasPassiveAbility(const FGameplayTag& Key) const
	{
		return PassiveMap.Contains(Key);
	}

};

// ---------------------------------------------------------------
// 敌人技能数据 — 在内容浏览器创建时自动填入标准 Enemy Tag
// ---------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable, DisplayName = "Enemy Ability Montage Data")
class DEVKIT_API UEnemyAbilityMontageData : public UAbilityData
{
	GENERATED_BODY()

	virtual void PostInitProperties() override;
};

// ---------------------------------------------------------------
// 玩家技能数据 — 在内容浏览器创建时自动填入标准 Player Tag
// ---------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable, DisplayName = "Player Ability Montage Data")
class DEVKIT_API UPlayerAbilityMontageData : public UAbilityData
{
	GENERATED_BODY()

	virtual void PostInitProperties() override;
};
