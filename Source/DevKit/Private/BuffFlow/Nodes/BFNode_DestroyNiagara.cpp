#include "BuffFlow/Nodes/BFNode_DestroyNiagara.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "NiagaraComponent.h"

UBFNode_DestroyNiagara::UBFNode_DestroyNiagara(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Visual");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_DestroyNiagara::ExecuteBuffFlowInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (BFC && EffectName != NAME_None)
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

	TriggerOutput(TEXT("Out"), true);
}
