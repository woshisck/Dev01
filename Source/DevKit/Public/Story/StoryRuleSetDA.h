#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/StoryRuleTypes.h"
#include "StoryRuleSetDA.generated.h"

UCLASS(BlueprintType)
class DEVKIT_API UStoryRuleSetDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story", meta = (TitleProperty = "RuleId"))
	TArray<FStoryRule> Rules;

	void GetRulesForEventSorted(FGameplayTag EventTag, TArray<const FStoryRule*>& OutRules) const;
};
