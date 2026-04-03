#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"

UBFNode_PlayNiagara::UBFNode_PlayNiagara(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_PlayNiagara::ExecuteInput(const FName& PinName)
{
	if (!NiagaraSystem)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(AttachTarget);
	if (!TargetActor)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	// 找到 SkeletalMeshComponent 以支持挂点
	USceneComponent* AttachComp = TargetActor->GetRootComponent();
	if (USkeletalMeshComponent* SkelMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		AttachComp = SkelMesh;
	}

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachComp,
		AttachSocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true  // bAutoDestroy — 若不跟随 Flow 生命周期时自动销毁
	);

	// 存入 BFC 供 DestroyNiagara 或 Cleanup 使用
	if (NiagaraComp && EffectName != NAME_None)
	{
		BFC->ActiveNiagaraEffects.Add(EffectName, NiagaraComp);
	}

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_PlayNiagara::Cleanup()
{
	if (bDestroyWithFlow && EffectName != NAME_None)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			if (TObjectPtr<UNiagaraComponent>* Found = BFC->ActiveNiagaraEffects.Find(EffectName))
			{
				if (*Found)
				{
					(*Found)->DeactivateImmediate();
				}
				BFC->ActiveNiagaraEffects.Remove(EffectName);
			}
		}
	}

	Super::Cleanup();
}
