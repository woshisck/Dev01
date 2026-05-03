#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_PlayNiagara.generated.h"

class UNiagaraSystem;

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Niagara", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_PlayNiagara : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FName EffectName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	TArray<FName> AttachSocketFallbackNames;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	EBFTargetSelector AttachTarget = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	bool bAttachToTarget = true;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Niagara")
	bool bDestroyWithFlow = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
