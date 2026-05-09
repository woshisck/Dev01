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

	// 效果配置文件 — 包含 GE 类、目标选择器和施加次数的 DA 资产
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "效果配置文件"))
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

	// 施加目标 — 覆盖 Profile 中的目标设置（默认 LastDamageTarget）
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "施加目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// 施加次数覆盖 — 0 = 使用 Profile 内配置的施加次数，> 0 = 覆盖为此值
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (ClampMin = "0", ToolTip = "0 uses Profile.Effect.ApplicationCount.", DisplayName = "施加次数覆盖（0=用配置值）"))
	int32 ApplicationCountOverride = 0;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FActiveGameplayEffectHandle GrantedHandle;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
};
