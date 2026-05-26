#include "BuffFlow/Nodes/BFNode_PlayMontage.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

UBFNode_PlayMontage::UBFNode_PlayMontage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Visual");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_PlayMontage::ExecuteBuffFlowInput(const FName& PinName)
{
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayMontage] Skip: Montage is null TargetSelector=%d"),
			static_cast<int32>(TargetSelector));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(TargetSelector);
	ACharacter* Character = Cast<ACharacter>(TargetActor);
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayMontage] Skip: target is not a character Target=%s Montage=%s TargetSelector=%d"),
			*GetNameSafe(TargetActor),
			*GetNameSafe(Montage),
			static_cast<int32>(TargetSelector));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const float Duration = Character->PlayAnimMontage(Montage, PlayRate);
	UE_LOG(LogTemp, Warning, TEXT("[PlayMontage] Played Target=%s Montage=%s TargetSelector=%d PlayRate=%.2f Duration=%.3f AnimInstance=%s"),
		*GetNameSafe(Character),
		*GetNameSafe(Montage),
		static_cast<int32>(TargetSelector),
		PlayRate,
		Duration,
		*GetNameSafe(AnimInstance));
	if (Duration <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayMontage] Failed or zero length: check montage skeleton, slot setup, and target AnimBP. Target=%s Montage=%s"),
			*GetNameSafe(Character),
			*GetNameSafe(Montage));
	}

	TriggerOutput(TEXT("Out"), true);
}
