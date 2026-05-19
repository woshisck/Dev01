#pragma once

#include "Commandlets/Commandlet.h"
#include "ActiveSkillSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UActiveSkillSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UActiveSkillSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
