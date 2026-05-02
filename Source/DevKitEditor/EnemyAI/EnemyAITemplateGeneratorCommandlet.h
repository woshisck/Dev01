#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EnemyAITemplateGeneratorCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UEnemyAITemplateGeneratorCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UEnemyAITemplateGeneratorCommandlet();

	virtual int32 Main(const FString& Params) override;
};
