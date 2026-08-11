#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/EnemyHitImpactData.h"
#include "GameplayTagContainer.h"
#include "HitImpactVisualComponent.generated.h"

class USkeletalMeshComponent;
class AActor;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UHitImpactVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitImpactVisualComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Plays this victim's hit sound + VFX at HitLocation, resolved from UEnemyHitImpactData by
	// cascading every entry whose MatchTag this victim owns (Level picks the intensity entry).
	// Returns the resolved camera-shake level (0 = none) so the caller can aggregate one shake
	// per swing. Does not play the hit push (call that separately).
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Impact Visual")
	int32 PlayHitFeedback(const FVector& HitLocation, int32 Level);

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Impact Visual")
	void PlayHitPush(AActor* SourceActor, float Strength = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Impact Visual")
	void PlayHitPushFromLocation(FVector SourceLocation, float Strength = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Impact Visual")
	void StopHitPush();

	UFUNCTION(BlueprintPure, Category = "Combat|Hit Impact Visual")
	bool IsHitPushActive() const { return bHitPushActive; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual")
	bool bEnableHitPush = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual", meta = (ClampMin = "0.0", Units = "cm"))
	float PushDistance = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxAccumulatedPushDistance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual", meta = (ClampMin = "0.0", Units = "cm"))
	float VerticalLift = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual", meta = (ClampMin = "0.0", Units = "s"))
	float PushOutDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual", meta = (ClampMin = "0.0", Units = "s"))
	float ReturnDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual", meta = (ClampMin = "0.0", Units = "s"))
	float MinRefreshInterval = 0.03f;

	// ─── Hit React ─────────────────────────────────────────────────────────
	// Physical surface of this victim, matched against UEnemyHitImpactData::Entries to pick the
	// material layer of the hit sound. Falls back to HitReact.Material.Soft when left unset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual|Hit React",
		meta = (Categories = "HitReact.Material"))
	FGameplayTag HitReactMaterial;

private:
	// Tags the hit-impact table is matched against: this victim's material tag plus every tag its
	// ASC currently owns. Armor is an attribute rather than a tag, so Buff.Status.Armored is
	// synthesized here while ArmorHP > 0 to keep the table uniformly tag-keyed.
	FGameplayTagContainer GatherVictimTags() const;

	USkeletalMeshComponent* ResolveMesh() const;
	FVector ConvertWorldOffsetToMeshParentSpace(const USkeletalMeshComponent* Mesh, const FVector& WorldOffset) const;
	void ApplyVisualOffset(const FVector& NewOffset);
	void ClearVisualOffset();

	UPROPERTY(Transient)
	FVector AppliedOffset = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector StartOffset = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector PeakOffset = FVector::ZeroVector;

	float ElapsedTime = 0.0f;
	float LastPushStartTime = -1000000.0f;
	bool bHitPushActive = false;
};
