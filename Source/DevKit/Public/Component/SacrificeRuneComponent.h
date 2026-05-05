#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectKey.h"
#include "Component/CombatDeckComponent.h"
#include "SacrificeRuneComponent.generated.h"

class APlayerCharacterBase;
class ASacrificeShadowEchoActor;
class UFlowAsset;
class UYogAbilitySystemComponent;

UENUM(BlueprintType)
enum class ESacrificeRunePassiveType : uint8
{
	MoonlightShadow UMETA(DisplayName = "\u6708\u5149\u4e4b\u5f71"),
	ShadowMark      UMETA(DisplayName = "\u6697\u5f71\u5370\u8bb0"),
	GiantSwing      UMETA(DisplayName = "\u5de8\u529b\u6325\u821e"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FSacrificeRunePassiveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice")
	ESacrificeRunePassiveType PassiveType = ESacrificeRunePassiveType::MoonlightShadow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Moonlight Shadow", meta = (ClampMin = "1"))
	int32 ShadowAttackCharges = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Moonlight Shadow", meta = (ClampMin = "0.1"))
	float ShadowLifetime = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Moonlight Shadow", meta = (ClampMin = "0.0"))
	float ShadowDamageMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Moonlight Shadow", meta = (ClampMin = "1.0"))
	float ShadowAttackRange = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Moonlight Shadow", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ShadowAttackConeDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Shadow Mark", meta = (ClampMin = "0.0"))
	float DashChargeBonus = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Shadow Mark", meta = (ClampMin = "1.0"))
	float DashMarkRadius = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Shadow Mark", meta = (ClampMin = "0.1"))
	float MarkDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Shadow Mark", meta = (ClampMin = "0.0"))
	float MarkExplosionDamage = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Shadow Mark", meta = (ClampMin = "1.0"))
	float MarkExplosionRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Shadow Mark")
	FGameplayTag ShadowMarkTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Giant Swing", meta = (ClampMin = "0.0"))
	float KnockbackDistance = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Giant Swing", meta = (ClampMin = "0.0"))
	float KnockbackCollisionDamage = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Giant Swing", meta = (ClampMin = "1.0"))
	float KnockbackCollisionRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Giant Swing", meta = (ClampMin = "0.01"))
	float KnockbackCollisionDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice|Giant Swing", meta = (ClampMin = "0.01"))
	float KnockbackCollisionTickInterval = 0.05f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API USacrificeRuneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USacrificeRuneComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddSacrificePassive(FGuid SourceGuid, const FSacrificeRunePassiveConfig& Config);
	void RemoveSacrificePassive(FGuid SourceGuid);

	void NotifyDashExecuted(const FVector& StartLocation, const FVector& EndLocation, const FVector& Direction);

	bool HasPassive(ESacrificeRunePassiveType Type) const;

private:
	struct FShadowMarkState
	{
		TWeakObjectPtr<AActor> Target;
		float ExpireTime = 0.0f;
		FGameplayTag MarkTag;
	};

	struct FKnockbackCollisionTrack
	{
		TWeakObjectPtr<AActor> Target;
		float RemainingTime = 0.0f;
		float TickAccumulator = 0.0f;
		TSet<TObjectKey<AActor>> DamagedActors;
		FSacrificeRunePassiveConfig Config;
	};

	APlayerCharacterBase* GetPlayerOwner() const;
	UYogAbilitySystemComponent* GetPlayerASC() const;
	const FSacrificeRunePassiveConfig* GetConfig(ESacrificeRunePassiveType Type) const;
	void RebuildEffectiveConfigs();
	void EnsureBindings();
	void RemoveAllShadowMarks();

	void ApplyDashChargeBonus(FGuid SourceGuid, const FSacrificeRunePassiveConfig& Config);
	void RemoveDashChargeBonus(FGuid SourceGuid);

	void SpawnMoonlightShadow(const FVector& StartLocation, const FVector& Direction, const FSacrificeRunePassiveConfig& Config);
	void ReplayOffensiveCardFlowsFromShadow(const FCombatCardResolveResult& Result);
	void QueueShadowAttackDamage(float PlayerDamage);
	void PerformShadowAttack(float PlayerDamage, const FSacrificeRunePassiveConfig& Config);

	void ApplyDashShadowMarks(const FVector& StartLocation, const FVector& EndLocation, const FSacrificeRunePassiveConfig& Config);
	void AddShadowMark(AActor* Target, const FSacrificeRunePassiveConfig& Config);
	bool ConsumeShadowMark(AActor* Target);
	void DetonateShadowMark(AActor* CenterTarget, const FSacrificeRunePassiveConfig& Config);

	void TriggerGiantSwing(AActor* Target, const FSacrificeRunePassiveConfig& Config);
	void TickKnockbackCollisionTracks(float DeltaTime);

	TArray<AActor*> FindEnemyActorsInRadius(const FVector& Origin, float Radius, AActor* ExcludeActor = nullptr) const;
	bool IsValidEnemyTarget(AActor* Actor) const;
	void ApplyInstantPureDamage(AActor* Target, float Damage, FName DamageLogType, bool bSuppressFeedback);
	bool IsPlayerInAttackState() const;
	bool FlowHasOffensiveSpawnNode(const UFlowAsset* FlowAsset) const;

	UFUNCTION()
	void HandlePlayerDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage);

	UFUNCTION()
	void HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result);

	UPROPERTY()
	TMap<FGuid, FSacrificeRunePassiveConfig> ActivePassives;

	UPROPERTY()
	TMap<ESacrificeRunePassiveType, FSacrificeRunePassiveConfig> EffectiveConfigs;

	UPROPERTY()
	TMap<FGuid, FActiveGameplayEffectHandle> DashChargeEffectHandles;

	UPROPERTY()
	TObjectPtr<ASacrificeShadowEchoActor> ActiveShadow;

	TMap<TObjectKey<AActor>, FShadowMarkState> ShadowMarks;

	TArray<FKnockbackCollisionTrack> KnockbackTracks;
	int32 PendingShadowDamageApplications = 0;
	bool bBindingsInitialized = false;
	bool bApplyingSacrificeDamage = false;
};
