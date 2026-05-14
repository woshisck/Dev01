#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PlayRuneVFXProfile.generated.h"

class URuneCardEffectProfileDA;

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Rune VFX Profile", Category = "BuffFlow|Profile"))
class DEVKIT_API UBFNode_PlayRuneVFXProfile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// VFX 配置文件 — 包含 Niagara 系统、挂载信息和表现参数的 DA 资产
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "VFX 配置文件"))
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

	// 目标覆盖 — 覆盖 Profile 中的默认目标（仅在勾选「启用目标覆盖」时生效）
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "目标覆盖"))
	EBFTargetSelector TargetOverride = EBFTargetSelector::BuffOwner;

	// 启用目标覆盖 — 勾选后使用上方「目标覆盖」替代 Profile 的默认目标
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "启用目标覆盖"))
	bool bUseTargetOverride = false;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
