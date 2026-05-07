#include "BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UBFNode_PlayRuneVFXProfile::UBFNode_PlayRuneVFXProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Profile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_PlayRuneVFXProfile::ExecuteInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!Profile)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, nullptr, nullptr, EBuffFlowTraceResult::Failed, TEXT("Profile is null"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const FRuneCardProfileVFXConfig& Config = Profile->VFX;
	if (!Config.NiagaraSystem)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, nullptr, EBuffFlowTraceResult::Skipped, TEXT("NiagaraSystem is null"), TEXT(""));
		}
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	const EBFTargetSelector TargetSelector = bUseTargetOverride ? TargetOverride : Config.AttachTarget;
	AActor* TargetActor = ResolveTarget(TargetSelector);
	if (!TargetActor)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, nullptr, EBuffFlowTraceResult::Failed, TEXT("Target is null"), FString::Printf(TEXT("Selector=%d"), static_cast<int32>(TargetSelector)));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UNiagaraComponent* NiagaraComp = nullptr;
	FName SpawnedSocketName = NAME_None;
	if (Config.bAttachToTarget)
	{
		USceneComponent* AttachComp = TargetActor->GetRootComponent();
		FName ResolvedSocket = Config.AttachSocketName;
		if (USkeletalMeshComponent* SkelMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			AttachComp = SkelMesh;
			auto HasSocketOrBone = [SkelMesh](const FName& Candidate)
			{
				return Candidate != NAME_None
					&& (SkelMesh->DoesSocketExist(Candidate) || SkelMesh->GetBoneIndex(Candidate) != INDEX_NONE);
			};
			if (!HasSocketOrBone(ResolvedSocket))
			{
				ResolvedSocket = NAME_None;
				for (const FName& Fallback : Config.AttachSocketFallbackNames)
				{
					if (HasSocketOrBone(Fallback))
					{
						ResolvedSocket = Fallback;
						break;
					}
				}
			}
		}

		if (!AttachComp)
		{
			if (BFC)
			{
				BFC->RecordTrace(this, Profile, TargetActor, EBuffFlowTraceResult::Failed, TEXT("No attach component"), TEXT(""));
			}
			TriggerOutput(TEXT("Failed"), true);
			return;
		}

		SpawnedSocketName = ResolvedSocket;
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Config.NiagaraSystem,
			AttachComp,
			ResolvedSocket,
			Config.LocationOffset,
			Config.RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true);
		if (NiagaraComp)
		{
			NiagaraComp->SetRelativeScale3D(Config.Scale);
		}
	}
	else if (UWorld* World = TargetActor->GetWorld())
	{
		const FTransform TargetTransform = TargetActor->GetActorTransform();
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			Config.NiagaraSystem,
			TargetTransform.TransformPosition(Config.LocationOffset),
			(TargetActor->GetActorRotation() + Config.RotationOffset).GetNormalized(),
			Config.Scale,
			true);
	}

	if (!NiagaraComp)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, TargetActor, EBuffFlowTraceResult::Failed, TEXT("SpawnSystem failed"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (BFC && Config.EffectName != NAME_None)
	{
		BFC->ActiveNiagaraEffects.Add(Config.EffectName, NiagaraComp);
	}

	if (Config.Lifetime > 0.f)
	{
		TWeakObjectPtr<UNiagaraComponent> WeakComp = NiagaraComp;
		TWeakObjectPtr<UBuffFlowComponent> WeakBFC = BFC;
		const FName CapturedEffectName = Config.EffectName;
		FTimerHandle TimerHandle;
		TargetActor->GetWorldTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateLambda([WeakComp, WeakBFC, CapturedEffectName]()
			{
				if (UBuffFlowComponent* FlowComponent = WeakBFC.Get())
				{
					if (TObjectPtr<UNiagaraComponent>* Found = FlowComponent->ActiveNiagaraEffects.Find(CapturedEffectName);
						Found && *Found == WeakComp.Get())
					{
						FlowComponent->ActiveNiagaraEffects.Remove(CapturedEffectName);
					}
				}
				if (UNiagaraComponent* Component = WeakComp.Get())
				{
					Component->Deactivate();
					Component->DestroyComponent();
				}
			}),
			Config.Lifetime,
			false);
	}

	if (BFC)
	{
		BFC->RecordTrace(
			this,
			Profile,
			TargetActor,
			EBuffFlowTraceResult::Success,
			TEXT("Spawned VFX profile"),
			FString::Printf(TEXT("System=%s Effect=%s Attach=%d Socket=%s Scale=%s Lifetime=%.2f"),
				*GetNameSafe(Config.NiagaraSystem),
				*Config.EffectName.ToString(),
				Config.bAttachToTarget ? 1 : 0,
				*SpawnedSocketName.ToString(),
				*Config.Scale.ToString(),
				Config.Lifetime));
	}

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_PlayRuneVFXProfile::Cleanup()
{
	if (Profile && Profile->VFX.bDestroyWithFlow && Profile->VFX.EffectName != NAME_None)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			if (TObjectPtr<UNiagaraComponent>* Found = BFC->ActiveNiagaraEffects.Find(Profile->VFX.EffectName))
			{
				if (*Found)
				{
					(*Found)->DeactivateImmediate();
				}
				BFC->ActiveNiagaraEffects.Remove(Profile->VFX.EffectName);
			}
		}
	}
	Super::Cleanup();
}
