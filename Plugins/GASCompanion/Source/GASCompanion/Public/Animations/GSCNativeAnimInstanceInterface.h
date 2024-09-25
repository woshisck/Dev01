// Copyright 2020 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GSCNativeAnimInstanceInterface.generated.h"

class UAbilitySystemComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGSCNativeAnimInstanceInterface : public UInterface
{
	GENERATED_BODY()
};


class GASCOMPANION_API IGSCNativeAnimInstanceInterface
{
	GENERATED_BODY()

public:

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC) { checkf(false, TEXT("IGSCNativeAnimInstanceInterface::InitializeWithAbilitySystem must be implemented in subclass")); };
};
