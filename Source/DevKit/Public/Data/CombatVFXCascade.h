#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/FunctionFwd.h"
#include "CombatVFXCascade.generated.h"

/**
 * Match metadata for one row of a combat VFX table.
 *
 * Deliberately carries no payload so the cascade can resolve any table shape
 * (whiff configs, hit FX, future parry/block sets) without the resolver knowing
 * what it is selecting. Tables pair this with their own config struct and use
 * the resolved indices to look the payload back up.
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FCombatVFXMatchRule
{
	GENERATED_BODY()

	/**
	 * Every tag here must be owned by the context for the row to match, so a row can
	 * key on a combination such as Weapon.TwoHandedSword + Buff.Fire. Tags match
	 * hierarchically, so a parent tag row covers all its children. Leaving this empty
	 * matches any context, which is how a table declares a universal default row.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	FGameplayTagContainer MatchTags;

	// Higher wins the base slot. Ties resolve in authoring order.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	int32 Priority = 0;

	/**
	 * Additive rows stack on top of the base instead of competing for it. Use for
	 * extras that should read alongside another element rather than replace it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	bool bIsAdditive = false;
};

/** Indices into the caller's row array. Not reflected — plain by-value result of a Resolve call. */
struct FCombatVFXCascadeResult
{
	// Highest-priority matching non-additive row, or INDEX_NONE when none matched.
	int32 BaseIndex = INDEX_NONE;

	// Matching additive rows, highest priority first, truncated to MaxAdditiveLayers.
	TArray<int32, TInlineAllocator<4>> AdditiveIndices;

	// Additive rows dropped by the cap. Callers warn on this; the resolver stays silent.
	int32 AdditiveOverflowCount = 0;

	bool HasBase() const { return BaseIndex != INDEX_NONE; }
	bool IsEmpty() const { return BaseIndex == INDEX_NONE && AdditiveIndices.Num() == 0; }
};

namespace CombatVFXCascade
{
	/** Rows beyond this are dropped and reported via AdditiveOverflowCount. Each layer is a real component. */
	constexpr int32 DefaultMaxAdditiveLayers = 3;

	/**
	 * Selects one base row plus any additive rows whose MatchTags the context owns.
	 *
	 * Unlike UEnemyHitImpactData::Resolve, which cascades field-by-field across rows, the
	 * base here is taken whole from a single winning row. A whiff config is one coherent
	 * look (one system, one weapon material) and blending fields across rows would make
	 * authoring unpredictable; stacking is expressed with additive rows instead.
	 */
	DEVKIT_API FCombatVFXCascadeResult ResolveIndexed(
		int32 NumRows,
		TFunctionRef<const FCombatVFXMatchRule& (int32)> GetRule,
		const FGameplayTagContainer& ContextTags,
		int32 MaxAdditiveLayers = DefaultMaxAdditiveLayers);

	/** Convenience wrapper for a table whose rows embed a rule reachable by projection. */
	template <typename RowType, typename RuleProjection>
	FCombatVFXCascadeResult Resolve(
		const TArray<RowType>& Rows,
		RuleProjection GetRuleFromRow,
		const FGameplayTagContainer& ContextTags,
		int32 MaxAdditiveLayers = DefaultMaxAdditiveLayers)
	{
		return ResolveIndexed(
			Rows.Num(),
			[&Rows, &GetRuleFromRow](int32 Index) -> const FCombatVFXMatchRule& { return GetRuleFromRow(Rows[Index]); },
			ContextTags,
			MaxAdditiveLayers);
	}
}
