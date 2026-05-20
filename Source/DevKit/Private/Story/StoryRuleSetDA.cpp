#include "Story/StoryRuleSetDA.h"

void UStoryRuleSetDA::GetRulesForEventSorted(FGameplayTag EventTag, TArray<const FStoryRule*>& OutRules) const
{
	OutRules.Reset();

	if (!bEnabled || !EventTag.IsValid())
	{
		return;
	}

	for (const FStoryRule& Rule : Rules)
	{
		if (!Rule.bEnabled || !Rule.TriggerEventTag.MatchesTagExact(EventTag))
		{
			continue;
		}

		OutRules.Add(&Rule);
	}

	OutRules.Sort([](const FStoryRule& Left, const FStoryRule& Right)
	{
		if (Left.Priority != Right.Priority)
		{
			return Left.Priority > Right.Priority;
		}
		return Left.RuleId.LexicalLess(Right.RuleId);
	});
}
