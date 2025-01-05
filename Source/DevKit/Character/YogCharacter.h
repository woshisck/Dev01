#pragma once

#include "ModularCharacter.h"
#include "YogCharacter.generated.h"


class DEVKIT_API AYogCharacter : public AModularCharacter{

    GENERATED_BODY()
public:
    AYogCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
//~End of AActor interface

    
}