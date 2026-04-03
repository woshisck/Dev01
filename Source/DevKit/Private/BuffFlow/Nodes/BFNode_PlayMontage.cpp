#include "BuffFlow/Nodes/BFNode_PlayMontage.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"

UBFNode_PlayMontage::UBFNode_PlayMontage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_PlayMontage::ExecuteInput(const FName& PinName)
{
	if (!Montage)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(TargetSelector);
	ACharacter* Character = Cast<ACharacter>(TargetActor);
	if (Character)
	{
		Character->PlayAnimMontage(Montage, PlayRate);
	}

	TriggerOutput(TEXT("Out"), true);
}
