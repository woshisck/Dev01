#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PlayRuneVFXProfile.generated.h"

class URuneCardEffectProfileDA;

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Rune VFX Profile", Category = "BuffFlow|Profile"))
class DEVKIT_API UBFNode_PlayRuneVFXProfile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Profile")
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

	UPROPERTY(EditAnywhere, Category = "Profile")
	EBFTargetSelector TargetOverride = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "Profile")
	bool bUseTargetOverride = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
