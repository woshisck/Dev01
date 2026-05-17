#include "Story/StoryEventRegistryDA.h"

const FStoryEventEntry* UStoryEventRegistryDA::FindEntry(FGameplayTag EventTag) const
{
	if (!EventTag.IsValid())
	{
		return nullptr;
	}

	return Entries.FindByPredicate([EventTag](const FStoryEventEntry& Entry)
	{
		return Entry.EventTag.MatchesTagExact(EventTag);
	});
}
