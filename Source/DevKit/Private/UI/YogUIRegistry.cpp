#include "UI/YogUIRegistry.h"

bool UYogUIRegistry::FindEntry(EYogUIScreenId ScreenId, FYogUIRegistryEntry& OutEntry) const
{
	for (const FYogUIRegistryEntry& Entry : Entries)
	{
		if (Entry.ScreenId == ScreenId)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

TSoftClassPtr<UUserWidget> UYogUIRegistry::GetWidgetClass(EYogUIScreenId ScreenId) const
{
	FYogUIRegistryEntry Entry;
	return FindEntry(ScreenId, Entry) ? Entry.WidgetClass : TSoftClassPtr<UUserWidget>();
}

int32 UYogUIRegistry::GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder) const
{
	FYogUIRegistryEntry Entry;
	return FindEntry(ScreenId, Entry) ? Entry.ZOrder : FallbackZOrder;
}
