#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RuneCardBatchGeneratorCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API URuneCardBatchGeneratorCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URuneCardBatchGeneratorCommandlet();

	virtual int32 Main(const FString& Params) override;
};
