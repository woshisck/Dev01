#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Component/MontageVFXBindingComponent.h"
#include "Data/CombatVFXCascade.h"
#include "WhiffVFXData.generated.h"

/** One row: "for this montage slot, while the context owns these tags, contribute this look". */
USTRUCT(BlueprintType)
struct FWhiffVFXEntry
{
	GENERATED_BODY()

	// Montage VFX slot this row can supply, matched exactly against ANS_MontageVFXBinding::SlotName.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whiff")
	FName SlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whiff")
	FCombatVFXMatchRule Match;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whiff")
	FMontageVFXBindingConfig Config;
};

/**
 * Flattened layer set. Not reflected — plain by-value result of a Resolve call.
 * Pointers alias entries on the resolved asset, so they stay valid only while it is loaded.
 */
struct FWhiffVFXResolved
{
	// Highest-priority matching non-additive row. Null when only additive rows matched.
	const FMontageVFXBindingConfig* Base = nullptr;

	// Stacking layers, highest priority first.
	TArray<const FMontageVFXBindingConfig*, TInlineAllocator<4>> Additive;

	// Rows dropped by MaxAdditiveLayers. Callers warn on this.
	int32 AdditiveOverflowCount = 0;

	int32 TotalLayers() const { return (Base ? 1 : 0) + Additive.Num(); }
	bool IsEmpty() const { return Base == nullptr && Additive.Num() == 0; }
};

/**
 * Project-wide attacker-side whiff table, keyed by montage slot plus GameplayTags describing
 * the swing's context (equipped weapon type + the attacker's owned buff tags). Referenced from
 * UYogSettings::WhiffVFXData and read by UMontageVFXBindingComponent when a slot activates.
 *
 * This resolves the swing's OWN presentation, which fires whether or not the swing connects.
 * Contact feedback is the victim's business and lives in UEnemyHitImpactData; the two never
 * resolve from the same data.
 *
 * A BuffFlow BFNode_SetMontageVFXBinding registration takes precedence over this table for the
 * slot it targets, so one-off rune behaviour stays authorable in a graph.
 */
UCLASS(BlueprintType)
class DEVKIT_API UWhiffVFXData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Whiff")
	TArray<FWhiffVFXEntry> Entries;

	/** Each additive layer is a real component, so cap what one swing can spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Whiff", meta = (ClampMin = "0"))
	int32 MaxAdditiveLayers = CombatVFXCascade::DefaultMaxAdditiveLayers;

	/**
	 * Selects one base config plus any additive configs for SlotName whose match tags the
	 * context owns. Returns an empty result when the slot has no rows at all.
	 */
	FWhiffVFXResolved Resolve(FName SlotName, const FGameplayTagContainer& ContextTags) const;
};
