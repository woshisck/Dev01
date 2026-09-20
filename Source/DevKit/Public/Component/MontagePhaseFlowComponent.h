#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "MontagePhaseFlowComponent.generated.h"

class UFlowAsset;
class UYogAbilitySystemComponent;

/**
 * One phase-triggered flow. Bound and revoked at runtime, never authored in an asset, so a buff
 * granted mid-run can add its own behaviour and take it away again.
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FMontagePhaseFlowBinding
{
	GENERATED_BODY()

	/** Phase that launches this flow, e.g. Character.State.Phase.Atk. */
	UPROPERTY(BlueprintReadWrite, Category = "Phase Flow")
	FGameplayTag Phase;

	UPROPERTY(BlueprintReadWrite, Category = "Phase Flow")
	TObjectPtr<UFlowAsset> Flow = nullptr;

	/** Whose BuffFlowComponent runs the flow. LastDamageTarget resolves to the most recent victim. */
	UPROPERTY(BlueprintReadWrite, Category = "Phase Flow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;
};

/**
 * Runtime container mapping attack phases to flow assets.
 *
 * Phase tags are driven by UYogTask_PlayMontageAbility from the playing montage's section, and
 * this component reacts to them through the owner ASC's OnGameplayTagChanged. Bindings live only
 * in memory, so granting or revoking one mid-run needs no asset edit.
 *
 * Mirrors UMontageVFXBindingComponent: a runtime-mutable map keyed off montage-side data, with the
 * montage supplying the key and the component resolving it.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UMontagePhaseFlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMontagePhaseFlowComponent();

	/** Binds a flow to a phase. Returns the handle needed to revoke it, or an invalid handle on failure. */
	UFUNCTION(BlueprintCallable, Category = "Phase Flow")
	FGuid BindPhaseFlow(FGameplayTag Phase, UFlowAsset* Flow, EBFTargetSelector Target = EBFTargetSelector::BuffOwner);

	UFUNCTION(BlueprintCallable, Category = "Phase Flow")
	bool UnbindPhaseFlow(FGuid Handle);

	UFUNCTION(BlueprintCallable, Category = "Phase Flow")
	void ClearPhaseFlows();

	UFUNCTION(BlueprintPure, Category = "Phase Flow")
	int32 GetPhaseFlowCount() const { return Bindings.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleGameplayTagChanged(FGameplayTag Tag, bool bAdded);

	void LaunchPhase(const FGameplayTag& Phase);

	/** Resolves a binding's target actor, or null when the selector cannot be satisfied yet. */
	AActor* ResolveTargetActor(EBFTargetSelector Target) const;

	UYogAbilitySystemComponent* GetOwnerASC() const;

	UPROPERTY()
	TMap<FGuid, FMontagePhaseFlowBinding> Bindings;

	UPROPERTY()
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
};
