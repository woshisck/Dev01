#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Component/SacrificeRuneComponent.h"
#include "BFNode_GrantSacrificePassive.generated.h"

/**
 * Grants a run-level sacrifice passive to the owning player.
 * This node is intended for hidden passive runes that do not enter the combat deck.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Grant Sacrifice Passive", Category = "BuffFlow|Sacrifice"))
class DEVKIT_API UBFNode_GrantSacrificePassive : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice")
	FSacrificeRunePassiveConfig Config;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FGuid RuntimeGrantGuid;
	bool bGranted = false;
};
