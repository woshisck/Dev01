#include "Data/WhiffVFXData.h"

#include "Templates/Function.h"

FWhiffVFXResolved UWhiffVFXData::Resolve(FName SlotName, const FGameplayTagContainer& ContextTags) const
{
	FWhiffVFXResolved Result;

	if (SlotName.IsNone())
	{
		return Result;
	}

	TArray<int32, TInlineAllocator<16>> SlotRows;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].SlotName == SlotName)
		{
			SlotRows.Add(Index);
		}
	}

	if (SlotRows.Num() == 0)
	{
		return Result;
	}

	const FCombatVFXCascadeResult Cascade = CombatVFXCascade::ResolveIndexed(
		SlotRows.Num(),
		[this, &SlotRows](int32 Index) -> const FCombatVFXMatchRule& { return Entries[SlotRows[Index]].Match; },
		ContextTags,
		MaxAdditiveLayers);

	if (Cascade.HasBase())
	{
		Result.Base = &Entries[SlotRows[Cascade.BaseIndex]].Config;
	}

	Result.Additive.Reserve(Cascade.AdditiveIndices.Num());
	for (const int32 AdditiveIndex : Cascade.AdditiveIndices)
	{
		Result.Additive.Add(&Entries[SlotRows[AdditiveIndex]].Config);
	}

	Result.AdditiveOverflowCount = Cascade.AdditiveOverflowCount;

	return Result;
}
