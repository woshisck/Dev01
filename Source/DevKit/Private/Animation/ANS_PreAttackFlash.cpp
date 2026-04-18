#include "Animation/ANS_PreAttackFlash.h"
#include "Character/YogCharacterBase.h"

void UANS_PreAttackFlash::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;
	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
	{
		Character->StartPreAttackFlash();
	}
}

void UANS_PreAttackFlash::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;
	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
	{
		Character->StopPreAttackFlash();
	}
}

FString UANS_PreAttackFlash::GetNotifyName_Implementation() const
{
	return TEXT("Pre Attack Flash");
}
