// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphNodeBase.h"
#include "ComboGraphNodeConduit.generated.h"

/**
 * Conduit specific node, doesn't hold any specific logic and used to easily identify them in the graph
 */
UCLASS()
class COMBOGRAPH_API UComboGraphNodeConduit : public UComboGraphNodeBase
{
	GENERATED_BODY()
};
