#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/EnemyHitImpactData.h"
#include "HitImpactVisualComponent.generated.h"

class USkeletalMeshComponent;
class AActor;
struct FBuffHitFeedbackRow;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UHitImpactVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitImpactVisualComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Plays this victim's hit sound + VFX at HitLocation. Uses the buff-row feedback from
	// UYogSettings::BuffHitFeedbackTable when the owner has a matching Buff.* tag, otherwise
	// the component defaults below. Returns the buff camera-shake level (0 = none) so the
	// caller can aggregate one shake per swing. Does not play the hit push (call that separately).
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Impact Visual")
	int32 PlayHitFeedback(const FVector& HitLocation);

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
	// Physical tier of this victim. The base hit sound/VFX come from the shared
	// UYogSettings::EnemyHitImpactData entry for this tier (a matching DT_BuffHitFeedback
	// row still overrides it). While the owner has ArmorHP > 0 the Hard tier is used instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Impact Visual|Hit React")
	EHitReactTier HitReactTier = EHitReactTier::Soft;

private:
	// Effective tier FX from the shared data: Hard while the owner's ArmorHP > 0, else HitReactTier.
	// Null if no data asset is configured.
	const FHitImpactTierFX* ResolveTierFX() const;

	// Highest-Priority DT_BuffHitFeedback row whose BuffTag the owner currently owns; null if none.
	const FBuffHitFeedbackRow* ResolveBuffHitFeedback() const;

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
