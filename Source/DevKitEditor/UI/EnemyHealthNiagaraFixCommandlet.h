#pragma once

#include "Commandlets/Commandlet.h"
#include "EnemyHealthNiagaraFixCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UEnemyHealthNiagaraFixCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UEnemyHealthNiagaraFixCommandlet();

	virtual int32 Main(const FString& Params) override;
};
