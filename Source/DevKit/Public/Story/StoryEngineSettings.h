#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StoryEngineSettings.generated.h"

class UStoryRuleSetDA;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Story Engine"))
class DEVKIT_API UStoryEngineSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<TSoftObjectPtr<UStoryRuleSetDA>> RuleSets;

	virtual FName GetCategoryName() const override
	{
		return TEXT("Game");
	}
};
