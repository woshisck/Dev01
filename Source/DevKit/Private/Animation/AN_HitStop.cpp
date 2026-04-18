#include "Animation/AN_HitStop.h"
#include "Animation/HitStopManager.h"
#include "Engine/World.h"

void UAN_HitStop::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;
	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	if (UHitStopManager* Manager = World->GetSubsystem<UHitStopManager>())
	{
		Manager->RequestHitStop(FrozenDuration, SlowDuration, SlowTimeDilation);
	}
}

FString UAN_HitStop::GetNotifyName_Implementation() const
{
	if (SlowDuration > 0.0f)
	{
		return FString::Printf(TEXT("HitStop F=%.0fms S=%.0fms@%.0f%%"),
			FrozenDuration * 1000.f, SlowDuration * 1000.f, SlowTimeDilation * 100.f);
	}
	return FString::Printf(TEXT("HitStop %.0fms"), FrozenDuration * 1000.f);
}
