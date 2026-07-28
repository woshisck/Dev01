#include "StylizedEmissiveModelLibrary.h"

const FStylizedEmissiveModelEntry* UStylizedEmissiveModelLibrary::FindModel(const FName ModelId) const
{
	return Models.FindByPredicate([ModelId](const FStylizedEmissiveModelEntry& Entry)
	{
		return !ModelId.IsNone() && Entry.ModelId == ModelId;
	});
}

TArray<FString> UStylizedEmissiveModelLibrary::GetModelOptions() const
{
	TArray<FString> Options;
	Options.Reserve(Models.Num());
	for (const FStylizedEmissiveModelEntry& Entry : Models)
	{
		if (!Entry.ModelId.IsNone())
		{
			Options.AddUnique(Entry.ModelId.ToString());
		}
	}
	return Options;
}
