#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "StoryFlowAssetFactory.generated.h"

UCLASS()
class UStoryFlowAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UStoryFlowAssetFactory(const FObjectInitializer& ObjectInitializer);

	virtual FText GetDisplayName() const override;

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
