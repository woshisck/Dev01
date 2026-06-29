#pragma once

#include "Commandlets/Commandlet.h"
#include "UE58RuntimeProfilingPlanCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UUE58RuntimeProfilingPlanCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUE58RuntimeProfilingPlanCommandlet();

	virtual int32 Main(const FString& Params) override;
};
