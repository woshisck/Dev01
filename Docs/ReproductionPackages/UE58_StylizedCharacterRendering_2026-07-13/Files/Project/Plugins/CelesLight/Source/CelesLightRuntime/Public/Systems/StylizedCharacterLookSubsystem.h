#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StylizedCharacterLookSubsystem.generated.h"

/** Resolves character look volumes against the primary player camera. */
UCLASS()
class CELESLIGHTRUNTIME_API UStylizedCharacterLookSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
};
