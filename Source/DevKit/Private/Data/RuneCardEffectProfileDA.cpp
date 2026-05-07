#include "Data/RuneCardEffectProfileDA.h"

FName URuneCardEffectProfileDA::GetTraceName() const
{
	if (DebugName != NAME_None)
	{
		return DebugName;
	}
	if (EffectIdTag.IsValid())
	{
		return EffectIdTag.GetTagName();
	}
	return FName(*GetName());
}
