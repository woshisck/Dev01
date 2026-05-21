#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "StoryEncounterAssetFactories.generated.h"

UCLASS()
class UStoryEncounterGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UStoryEncounterGraphFactory(const FObjectInitializer& ObjectInitializer);

	virtual FText GetDisplayName() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UStoryEncounterPointFactory : public UFactory
{
	GENERATED_BODY()

public:
	UStoryEncounterPointFactory(const FObjectInitializer& ObjectInitializer);

	virtual FText GetDisplayName() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
