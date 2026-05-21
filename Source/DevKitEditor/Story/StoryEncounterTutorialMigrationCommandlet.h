#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "StoryEncounterTutorialMigrationCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UStoryEncounterTutorialMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UStoryEncounterTutorialMigrationCommandlet();

	virtual int32 Main(const FString& Params) override;
};
