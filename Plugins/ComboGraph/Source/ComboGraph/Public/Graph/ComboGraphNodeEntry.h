// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "ComboGraphNodeEntry.generated.h"

/** Runtime node for combo graph entry. */
UCLASS()
class COMBOGRAPH_API UComboGraphNodeEntry : public UComboGraphNodeAnimBase
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle() const override;
};
