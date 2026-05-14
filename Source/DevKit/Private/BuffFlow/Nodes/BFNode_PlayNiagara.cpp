#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UBFNode_PlayNiagara::UBFNode_PlayNiagara(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Visual");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_PlayNiagara::ExecuteBuffFlowInput(const FName& PinName)
{
	if (!NiagaraSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayNiagara] Skip: NiagaraSystem is null Effect=%s"), *EffectName.ToString());
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayNiagara] Skip: BuffFlowComponent is null Effect=%s System=%s"),
			*EffectName.ToString(),
			*GetNameSafe(NiagaraSystem));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(AttachTarget);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayNiagara] Skip: target is null Effect=%s System=%s TargetSelector=%d"),
			*EffectName.ToString(),
			*GetNameSafe(NiagaraSystem),
			static_cast<int32>(AttachTarget));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UNiagaraComponent* NiagaraComp = nullptr;
	FName SpawnedSocketName = NAME_None;
	if (bAttachToTarget)
	{
		USceneComponent* AttachComp = TargetActor->GetRootComponent();
		FName ResolvedAttachSocketName = AttachSocketName;
		if (USkeletalMeshComponent* SkelMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			AttachComp = SkelMesh;
			auto HasSocketOrBone = [SkelMesh](const FName& CandidateName)
			{
				return CandidateName != NAME_None
					&& (SkelMesh->DoesSocketExist(CandidateName) || SkelMesh->GetBoneIndex(CandidateName) != INDEX_NONE);
			};

			if (!HasSocketOrBone(ResolvedAttachSocketName))
			{
				const FName RequestedSocketName = ResolvedAttachSocketName;
				ResolvedAttachSocketName = NAME_None;
				for (const FName& FallbackName : AttachSocketFallbackNames)
				{
					if (HasSocketOrBone(FallbackName))
					{
						ResolvedAttachSocketName = FallbackName;
						break;
					}
				}

				if (RequestedSocketName != NAME_None || ResolvedAttachSocketName != NAME_None)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[PlayNiagara] Attach socket resolved Effect=%s Target=%s Requested=%s Resolved=%s"),
						*EffectName.ToString(),
						*GetNameSafe(TargetActor),
						*RequestedSocketName.ToString(),
						*ResolvedAttachSocketName.ToString());
				}
			}
		}
		if (!AttachComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayNiagara] Skip: no attach component Target=%s Effect=%s"),
				*GetNameSafe(TargetActor),
				*EffectName.ToString());
			TriggerOutput(TEXT("Out"), true);
			return;
		}

		SpawnedSocketName = ResolvedAttachSocketName;
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			AttachComp,
			ResolvedAttachSocketName,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true);
		if (NiagaraComp)
		{
			NiagaraComp->SetRelativeScale3D(Scale);
		}
	}
	else if (UWorld* World = TargetActor->GetWorld())
	{
		const FTransform TargetTransform = TargetActor->GetActorTransform();
		const FVector SpawnLocation = TargetTransform.TransformPosition(LocationOffset);
		const FRotator SpawnRotation = (TargetActor->GetActorRotation() + RotationOffset).GetNormalized();
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			NiagaraSystem,
			SpawnLocation,
			SpawnRotation,
			Scale,
			true);
		if (NiagaraComp)
		{
			NiagaraComp->SetWorldScale3D(Scale);
		}
	}

	if (NiagaraComp && EffectName != NAME_None)
	{
		BFC->ActiveNiagaraEffects.Add(EffectName, NiagaraComp);
	}

	if (NiagaraComp && Lifetime > 0.f)
	{
		TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComp = NiagaraComp;
		TWeakObjectPtr<UBuffFlowComponent> WeakBFC = BFC;
		const FName CapturedEffectName = EffectName;
		FTimerHandle LifetimeTimerHandle;
		TargetActor->GetWorldTimerManager().SetTimer(
			LifetimeTimerHandle,
			FTimerDelegate::CreateLambda([WeakNiagaraComp, WeakBFC, CapturedEffectName]()
			{
				if (UBuffFlowComponent* FlowComponent = WeakBFC.Get())
				{
					if (TObjectPtr<UNiagaraComponent>* Found = FlowComponent->ActiveNiagaraEffects.Find(CapturedEffectName);
						Found && *Found == WeakNiagaraComp.Get())
					{
						FlowComponent->ActiveNiagaraEffects.Remove(CapturedEffectName);
					}
				}

				if (UNiagaraComponent* Component = WeakNiagaraComp.Get())
				{
					Component->Deactivate();
					Component->DestroyComponent();
				}
			}),
			Lifetime,
			false);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayNiagara] Spawned Effect=%s System=%s Target=%s Attach=%d Socket=%s Scale=%s Lifetime=%.2f"),
		*EffectName.ToString(),
		*GetNameSafe(NiagaraSystem),
		*GetNameSafe(TargetActor),
		bAttachToTarget ? 1 : 0,
		*SpawnedSocketName.ToString(),
		*Scale.ToString(),
		Lifetime);

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
