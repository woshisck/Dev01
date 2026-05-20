#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "StoryRuleSetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UStoryRuleSetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UStoryRuleSetSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
