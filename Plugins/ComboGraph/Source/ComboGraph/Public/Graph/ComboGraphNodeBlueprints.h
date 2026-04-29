// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ComboGraphNodeBlueprints.generated.h"

/**
 * A Combo Graph NodeMontageBlueprint is essentially a simple UObject Blueprint with default node created when first opened.
 *
 * The combo graph factory should pick this for you automatically
 */
UCLASS(BlueprintType)
class COMBOGRAPH_API UComboGraphNodeMontageBlueprint : public UBlueprint
{
	GENERATED_BODY()

#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	// End of UBlueprint interface
#endif
};

/**
 * A Combo Graph NodeSequenceBlueprint is essentially a simple UObject Blueprint with default node created when first opened.
 *
 * The combo graph factory should pick this for you automatically
 */
UCLASS(BlueprintType)
class COMBOGRAPH_API UComboGraphNodeSequenceBlueprint : public UBlueprint
{
	GENERATED_BODY()

#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const override { return false; }
	// End of UBlueprint interface
	#endif
};
