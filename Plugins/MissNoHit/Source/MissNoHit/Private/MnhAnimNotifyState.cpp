// Copyright 2024 Eren Balatkan. All Rights Reserved.

#include "MnhAnimNotifyState.h"

#include "Animation/AnimSequence.h"
#include "MnhTracer.h"
#include "MnhTracerComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Launch/Resources/Version.h"


UMnhTracerComponent* UMnhAnimNotifyState::FindMnhTracerComponent(const USkeletalMeshComponent* MeshComp)
{
	if (const auto OwnerActor = MeshComp->GetOwner())
	{
		return OwnerActor->FindComponentByClass<UMnhTracerComponent>();
	}
	return nullptr;
}

UMnhTracer* UMnhAnimNotifyState::FindTracer(const USkeletalMeshComponent* MeshComp, const FGameplayTag TracerTag)
{
	if (const auto TracerComp = FindMnhTracerComponent(MeshComp))
	{
		return TracerComp->FindTracer(TracerTag);
	}
	return nullptr;
}

void UMnhActivateTracer::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                                     const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (const auto TracerComp = FindMnhTracerComponent(MeshComp))
	{
		TracerComp->StartTracers(MnhTracerTags, bResetHitCacheOnActivation);
	}
}

void UMnhActivateTracer::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UMnhActivateTracer::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (const auto TracerComp = FindMnhTracerComponent(MeshComp))
	{
		TracerComp->StopTracersDelayed(MnhTracerTags);
	}
}

void UMnhAnimNotifyTracer::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (const auto TracerComp = FindMnhTracerComponent(MeshComp))
	{
		if (const auto Tracer = TracerComp->FindTracer(TracerTag))
		{
			if (Tracer->TraceSource != EMnhTraceSource::AnimNotify)
			{
				const FString DebugMessage = FString::Printf(TEXT("MissNoHit Warning: "
														   "AnimNotifyTracer must have matching AnimNotify Tracer Setup with Tag [%s]"
														   " on Owner's [%s] MnhTracerComponent"), *TracerTag.ToString(), *MeshComp->GetOwner()->GetName());
				FMnhHelpers::Mnh_Log(DebugMessage);
				return;
			}
			
			if (!bUsePrecomputedPath)
			{
				Tracer->SocketOrBoneName = AttachedSocketOrBoneName;
				Tracer->ShapeData = ShapeData;
				Tracer->SourceComponent = MeshComp;
				Tracer->UpdateTracerData();
				TracerComp->StartTracersInternal<true>(FGameplayTagContainer{TracerTag}, bResetHitCacheOnActivation);
			}
			else
			{
				Tracer->bIsTracerActive = true;
				TickIdx = 0;
				if (bResetHitCacheOnActivation)
				{
					TracerComp->ResetHitCache();
				}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
				CurrentTimeInNotify = 0;
#endif
			}
		}
		else
		{
			const FString DebugMessage = FString::Printf(TEXT("MissNoHit Warning: "
														   "No Anim Notify Tracer with with Tag [%s] found"
														   " on Owner's [%s] MnhTracerComponent"), *TracerTag.ToString(), *MeshComp->GetOwner()->GetName());
			FMnhHelpers::Mnh_Log(DebugMessage);
		}
	}
}

FTransform GetBoneTransformFromSequence(const USkeletalMeshComponent* MeshComponent, const UAnimSequenceBase* Anim_Sequence, const FName Target_Bone_Name, const double Time)
{
	// https://forums.unrealengine.com/t/how-to-get-a-bone-location-for-the-first-frame-of-an-animmontage/18818/13
	if (!Anim_Sequence || !MeshComponent || !MeshComponent->GetSkinnedAsset())
		return FTransform::Identity;
	
	FName Bone_Name = Target_Bone_Name;
	FTransform Global_Transform = FTransform::Identity;

	const UAnimSequence* AnimSeq = Cast<UAnimSequence>(Anim_Sequence);

	if (!AnimSeq)
	{
		FMnhHelpers::Mnh_Log("Mnh Error: Failed to retrieve Animation Sequence on Precomputed Path Tracer");
		return FTransform::Identity;
	}
	
	do
	{
		const int Bone_Index = MeshComponent->GetBoneIndex(Bone_Name);
		
		FTransform Bone_Transform;
		AnimSeq->GetBoneTransform(Bone_Transform, FSkeletonPoseBoneIndex(Bone_Index), Time, false);
		
		Global_Transform *= Bone_Transform;
		Bone_Name = MeshComponent->GetParentBone(Bone_Name);
		
	} while (Bone_Name != NAME_None);

	Global_Transform *= MeshComponent->GetComponentTransform();
	return Global_Transform;
}

void UMnhAnimNotifyTracer::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (GEngine->IsEditor() && !GIsPlayInEditorWorld){
		const auto BoneTransform = MeshComp->GetBoneTransform(MeshComp->GetBoneIndex(FName(AttachedSocketOrBoneName)));
		const auto CurrentTransform = FTransform(ShapeData.Orientation, ShapeData.Offset) * BoneTransform;
		FMnhHelpers::DrawTrace(CurrentTransform.GetLocation(), CurrentTransform.GetLocation(), CurrentTransform.GetRotation(), FColor::Red, ShapeData,
		MeshComp->GetWorld(), EDrawDebugTrace::ForDuration, 2 * FrameDeltaTime);

		if (bDrawPrecomputedPath && bUsePrecomputedPath) 
		{
			auto StartTime = EventReference.GetNotify()->GetTriggerTime();
			const auto NotifyDuration = EventReference.GetNotify()->GetDuration();
			const auto NotifyFrameCount = FMath::CeilToInt(NotifyDuration * PrecomputeFps);

			for (int NotifyFrameIdx = 0; NotifyFrameIdx < NotifyFrameCount-1; NotifyFrameIdx++)
			{
				FTransform CurrentBonePose = GetBoneTransformFromSequence(MeshComp, Animation, AttachedSocketOrBoneName, StartTime + NotifyDuration * NotifyFrameIdx/NotifyFrameCount);
				FTransform NextBonePose = GetBoneTransformFromSequence(MeshComp, Animation, AttachedSocketOrBoneName, StartTime + NotifyDuration * (NotifyFrameIdx + 1)/NotifyFrameCount);;
			
				auto CurrentPoseTransform =
					FTransform(ShapeData.Orientation, ShapeData.Offset) *
					CurrentBonePose;

				auto NextPoseTransform =
					FTransform(ShapeData.Orientation, ShapeData.Offset) *
					NextBonePose;

				const auto& AverageTransform = UKismetMathLibrary::TLerp(CurrentPoseTransform, NextPoseTransform, 0.5, ELerpInterpolationMode::DualQuatInterp);
			
				FMnhHelpers::DrawTrace(CurrentPoseTransform.GetLocation(), NextPoseTransform.GetLocation(),
					AverageTransform.GetRotation(), FColor::Purple, ShapeData,
					MeshComp->GetWorld(), EDrawDebugTrace::ForDuration, 2 * FrameDeltaTime);
			}
		}
		return;
	}
	
	// - - - We are in game mode - - -
	if (!bUsePrecomputedPath)
	{
		return;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
	CurrentTimeInNotify += FrameDeltaTime;
#endif
	
	if(const auto Tracer = FindTracer(MeshComp, TracerTag))
	{
		TickIdx ++;
		
		const auto NotifyFrameCount = FMath::CeilToInt(FrameDeltaTime * PrecomputeFps);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
		const auto CurrentAnimTime = CurrentTimeInNotify + EventReference.GetNotify()->GetTriggerTime();
#else
		const auto CurrentAnimTime = EventReference.GetCurrentAnimationTime();
#endif
	
		for (int PoseIdx = 0; PoseIdx < NotifyFrameCount; PoseIdx++)
		{
			if (Tracer->bIsTracerActive == false)
			{
				break;
			}
			
			FTransform CurrentBonePose = GetBoneTransformFromSequence(MeshComp, Animation, AttachedSocketOrBoneName, (CurrentAnimTime - FrameDeltaTime) + FrameDeltaTime * PoseIdx/NotifyFrameCount);
			FTransform NextBonePose = GetBoneTransformFromSequence(MeshComp, Animation, AttachedSocketOrBoneName, (CurrentAnimTime - FrameDeltaTime) + FrameDeltaTime * (PoseIdx+1)/NotifyFrameCount);
			
			const auto CurrentPoseTransform =
				FTransform(ShapeData.Orientation, ShapeData.Offset) *
				CurrentBonePose;
	
			const auto NextPoseTransform =
				FTransform(ShapeData.Orientation, ShapeData.Offset) *
				NextBonePose;
	
			const auto& AverageTransform = UKismetMathLibrary::TLerp(CurrentPoseTransform, NextPoseTransform,
				0.5, ELerpInterpolationMode::DualQuatInterp);
	
			TArray<FHitResult> OutHits;
			FMnhHelpers::PerformTrace(
				CurrentPoseTransform, NextPoseTransform, AverageTransform,
				OutHits,
				MeshComp->GetWorld(),
				Tracer->TraceSettings,
				ShapeData,
				Tracer->CollisionParams, Tracer->ResponseParams, Tracer->ObjectQueryParams
			);
			
			if (Tracer->DrawDebugType != EDrawDebugTrace::None)
			{
				FMnhHelpers::DrawDebug(
					CurrentPoseTransform.GetLocation(),
					NextPoseTransform.GetLocation(),
					AverageTransform.GetRotation(),
					OutHits,
					ShapeData,
					MeshComp->GetWorld(),
					Tracer->DrawDebugType,
					Tracer->DebugDrawTime,
					Tracer->DebugTraceColor,
					Tracer->DebugTraceBlockColor,
					Tracer->DebugTraceHitColor
				);
			}
	
			if (const auto TracerComp = FindMnhTracerComponent(MeshComp))
			{
				TracerComp->OnTracerHitDetected(TracerTag, OutHits, FrameDeltaTime, TickIdx);
			}
		}
	}
}

void UMnhAnimNotifyTracer::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (bUsePrecomputedPath)
	{
		return;
	}
	
	if (const auto TracerComp = FindMnhTracerComponent(MeshComp))
	{
		TracerComp->StopTracersDelayed(FGameplayTagContainer(TracerTag));
	}
}
