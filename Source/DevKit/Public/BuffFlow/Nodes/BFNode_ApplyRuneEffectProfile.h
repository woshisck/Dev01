#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_ApplyRuneEffectProfile.generated.h"

class UAbilitySystemComponent;
class URuneCardEffectProfileDA;

UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Rune Effect Profile", Category = "BuffFlow|Profile"))
class DEVKIT_API UBFNode_ApplyRuneEffectProfile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Profile")
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

	UPROPERTY(EditAnywhere, Category = "Profile")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	UPROPERTY(EditAnywhere, Category = "Profile", meta = (ClampMin = "0", ToolTip = "0 uses Profile.Effect.ApplicationCount."))
	int32 ApplicationCountOverride = 0;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FActiveGameplayEffectHandle GrantedHandle;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
};
