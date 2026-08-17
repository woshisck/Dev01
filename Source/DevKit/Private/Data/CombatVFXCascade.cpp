#include "Data/CombatVFXCascade.h"

#include "Templates/Function.h"

namespace CombatVFXCascade
{
	FCombatVFXCascadeResult ResolveIndexed(
		int32 NumRows,
		TFunctionRef<const FCombatVFXMatchRule& (int32)> GetRule,
		const FGameplayTagContainer& ContextTags,
		int32 MaxAdditiveLayers)
	{
		FCombatVFXCascadeResult Result;

		const int32 AdditiveBudget = FMath::Max(MaxAdditiveLayers, 0);

		int32 BasePriority = 0;
		TArray<int32, TInlineAllocator<8>> AdditiveMatches;

		for (int32 Index = 0; Index < NumRows; ++Index)
		{
			const FCombatVFXMatchRule& Rule = GetRule(Index);

			// HasAll returns true for an empty container, which is how a universal default row matches.
			if (!ContextTags.HasAll(Rule.MatchTags))
			{
				continue;
			}

			if (Rule.bIsAdditive)
			{
				AdditiveMatches.Add(Index);
				continue;
			}

			// Strict greater-than keeps the first authored row on a priority tie.
			if (!Result.HasBase() || Rule.Priority > BasePriority)
			{
				Result.BaseIndex = Index;
				BasePriority = Rule.Priority;
			}
		}

		if (AdditiveMatches.Num() == 0)
		{
			return Result;
		}

		// Stable so equal priorities fall back to authoring order rather than an arbitrary one.
		AdditiveMatches.StableSort([&GetRule](int32 A, int32 B)
			{
				return GetRule(A).Priority > GetRule(B).Priority;
			});

		const int32 KeptCount = FMath::Min(AdditiveMatches.Num(), AdditiveBudget);
		Result.AdditiveOverflowCount = AdditiveMatches.Num() - KeptCount;

		Result.AdditiveIndices.Reserve(KeptCount);
		for (int32 i = 0; i < KeptCount; ++i)
		{
			Result.AdditiveIndices.Add(AdditiveMatches[i]);
		}

		return Result;
	}
}
