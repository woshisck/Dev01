#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "TimerManager.h"
#include "UObject/ObjectKey.h"
#include "ANS_MontageVFXBinding.generated.h"

class UAnimInstance;
class UAnimMontage;

UCLASS(meta = (DisplayName = "ANS Montage VFX Binding"))
class DEVKIT_API UANS_MontageVFXBinding : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding")
	FName SlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding|Annulus Plane",
		meta = (ClampMin = "0.0", ToolTip = "Annulus plane lifetime in seconds. Values <= 0 keep the notify state's begin/end lifetime."))
	float RemainTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding|Charge Gate",
		meta = (ToolTip = "When enabled, the notify uses RemainTime as a charge duration, updates annulus fill progress, then releases the montage to the next section."))
	bool bGateMontageSectionWithRemainTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding|Charge Gate",
		meta = (EditCondition = "bGateMontageSectionWithRemainTime", EditConditionHides, ToolTip = "Loop the current montage section while the charge fills. Only applies when ReleaseSectionName is set."))
	bool bLoopCurrentSectionUntilCharged = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding|Charge Gate",
		meta = (EditCondition = "bGateMontageSectionWithRemainTime", EditConditionHides, ToolTip = "Optional section to jump to when charge completes. Leave None to resume the current montage section naturally."))
	FName ReleaseSectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding|Charge Gate",
		meta = (EditCondition = "bGateMontageSectionWithRemainTime", EditConditionHides, ToolTip = "Jump immediately to ReleaseSectionName when charged. If false, the current section finishes this loop, then flows into ReleaseSectionName."))
	bool bJumpToReleaseSectionWhenCharged = true;

private:
	struct FChargeGateRuntime
	{
		TWeakObjectPtr<UAnimInstance> AnimInstance;
		TWeakObjectPtr<UAnimMontage> Montage;
		FTimerHandle ChargeTimerHandle;
		FName ChargeSectionName;
		float ElapsedTime = 0.f;
		float StartWorldTime = 0.f;
		float ChargeDuration = 0.f;
	};

	TMap<TObjectKey<USkeletalMeshComponent>, FChargeGateRuntime> ChargeGateRuntimeByMesh;

	void ReleaseChargeGate(USkeletalMeshComponent* MeshComp, bool bJumpToReleaseSection);
	void UpdateChargeGate(USkeletalMeshComponent* MeshComp);
};
