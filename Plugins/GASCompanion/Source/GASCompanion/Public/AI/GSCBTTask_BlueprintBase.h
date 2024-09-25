// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "GSCBTTask_BlueprintBase.generated.h"

/**
 * The only difference with UBTTask_BlueprintBase is that GetStaticDescription can be overriden in Blueprints.
 */
UCLASS()
class GASCOMPANION_API UGSCBTTask_BlueprintBase : public UBTTask_BlueprintBase
{
	GENERATED_BODY()


public:

	UGSCBTTask_BlueprintBase();

	virtual FString GetStaticDescription() const override;

protected:


	/**
	 * Overrides Task GetStaticDescription()
	 * Should return string containing description of this node with all setup values
	 *
	 * @return string containing description of this node with all setup values
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|AI", DisplayName="Get Static Description", meta=(ScriptName="GetStaticDescription"))
	FString K2_GetStaticDescription() const;

	bool bHasBlueprintStaticDescription;
};
