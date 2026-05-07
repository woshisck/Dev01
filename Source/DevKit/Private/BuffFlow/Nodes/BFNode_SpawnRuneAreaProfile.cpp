#include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "NiagaraSystem.h"
#include "Types/FlowDataPinResults.h"

UBFNode_SpawnRuneAreaProfile::UBFNode_SpawnRuneAreaProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Profile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnRuneAreaProfile::ExecuteInput(const FName& PinName)
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

	const FRuneCardProfileAreaConfig& Area = Profile->Area;
	AActor* SourceActor = ResolveTarget(Area.Source);
	if (!SourceActor)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, nullptr, EBuffFlowTraceResult::Failed, TEXT("Source is null"), FString::Printf(TEXT("Selector=%d"), static_cast<int32>(Area.Source)));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	if (!SourceASC)
	{
		SourceASC = GetOwnerASC();
	}
	if (!SourceASC || !SourceActor->GetWorld() || !Area.Effect)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, SourceActor, EBuffFlowTraceResult::Failed, TEXT("Missing SourceASC, world, or area effect"), FString::Printf(TEXT("SourceASC=%s Effect=%s"), *GetNameSafe(SourceASC), *GetNameSafe(Area.Effect.Get())));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	if (!ResolveFacing(SourceActor, Forward, Right))
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, SourceActor, EBuffFlowTraceResult::Failed, TEXT("Cannot resolve facing"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FTransform SourceTransformOverride;
	FVector SourceLocation = SourceActor->GetActorLocation();
	if (BFC && BFC->GetActiveSourceTransformOverride(SourceTransformOverride))
	{
		SourceLocation = SourceTransformOverride.GetLocation();
		Forward = SourceTransformOverride.GetRotation().GetForwardVector();
		Forward.Z = 0.0f;
		Forward = Forward.GetSafeNormal();
		Right = SourceTransformOverride.GetRotation().GetRightVector();
		Right.Z = 0.0f;
		Right = Right.GetSafeNormal();
	}
	if (Forward.IsNearlyZero())
	{
		Forward = SourceActor->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
	}
	if (Right.IsNearlyZero())
	{
		Right = SourceActor->GetActorRightVector();
		Right.Z = 0.f;
		Right.Normalize();
	}

	const float SafeLength = FMath::Max(1.0f, Area.Length);
	const float CenterOffset = Area.bCenterOnPathLength ? SafeLength * 0.5f : 0.0f;
	FVector SpawnLocation = SourceLocation
		+ Forward * (Area.SpawnOffset.X + CenterOffset)
		+ Right * Area.SpawnOffset.Y
		+ FVector::UpVector * Area.SpawnOffset.Z;

	const FFlowDataPinResult_Vector OverrideLocation = TryResolveDataPinAsVector(GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneAreaProfile, SpawnLocationOverride));
	const bool bUseLocationOverride = OverrideLocation.Result == EFlowDataPinResolveResult::Success;
	if (bUseLocationOverride)
	{
		SpawnLocation = OverrideLocation.Value;
	}

	UWorld* World = SourceActor->GetWorld();
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpawnRuneAreaProfileTrace), false, SourceActor);
	if (!bUseLocationOverride && World->LineTraceSingleByChannel(
		GroundHit,
		SpawnLocation + FVector(0.0f, 0.0f, 240.0f),
		SpawnLocation - FVector(0.0f, 0.0f, 900.0f),
		ECC_Visibility,
		QueryParams))
	{
		SpawnLocation.Z = GroundHit.Location.Z + Area.SpawnOffset.Z;
	}

	FRotator SpawnRotation = Forward.Rotation();
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;
	SpawnRotation.Yaw += Area.RotationYawOffset;
	SpawnRotation.Normalize();

	const FFlowDataPinResult_Rotator OverrideRotation = TryResolveDataPinAsRotator(GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneAreaProfile, SpawnRotationOverride));
	if (OverrideRotation.Result == EFlowDataPinResolveResult::Success)
	{
		SpawnRotation = OverrideRotation.Value;
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;
		SpawnRotation.Normalize();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = Cast<APawn>(SourceActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARuneGroundPathEffectActor* PathActor = World->SpawnActor<ARuneGroundPathEffectActor>(
		ARuneGroundPathEffectActor::StaticClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParams);
	if (!PathActor)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, SourceActor, EBuffFlowTraceResult::Failed, TEXT("SpawnActor failed"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FRuneGroundPathEffectConfig Config;
	Config.Effect = Area.Effect;
	Config.TargetPolicy = Area.TargetPolicy;
	Config.Shape = Area.Shape;
	Config.Duration = Area.Duration;
	Config.TickInterval = Area.TickInterval;
	Config.Length = Area.Length;
	Config.Width = Area.Width;
	Config.Height = Area.Height;
	Config.DecalProjectionDepth = Area.DecalProjectionDepth;
	Config.DecalPlaneRotationDegrees = Area.DecalPlaneRotationDegrees;
	Config.SpawnOffset = Area.SpawnOffset;
	Config.DecalMaterial = Area.DecalMaterial;
	Config.NiagaraSystem = Area.NiagaraSystem;
	Config.NiagaraScale = Area.NiagaraScale;
	Config.NiagaraInstanceCount = Area.NiagaraInstanceCount;
	Config.SetByCallerTag1 = Area.SetByCallerTag1;
	Config.SetByCallerValue1 = Area.SetByCallerValue1;
	Config.SetByCallerTag2 = Area.SetByCallerTag2;
	Config.SetByCallerValue2 = Area.SetByCallerValue2;
	Config.ApplicationCount = Area.ApplicationCount;
	Config.bApplyOncePerTarget = Area.bApplyOncePerTarget;
	Config.bPlayTargetVFXOnApply = Profile->VFX.NiagaraSystem != nullptr;
	Config.TargetNiagaraSystem = Profile->VFX.NiagaraSystem;
	Config.TargetAttachSocketName = Profile->VFX.AttachSocketName;
	Config.TargetAttachSocketFallbackNames = Profile->VFX.AttachSocketFallbackNames;
	Config.bAttachTargetVFXToTarget = Profile->VFX.bAttachToTarget;
	Config.TargetVFXLocationOffset = Profile->VFX.LocationOffset;
	Config.TargetVFXRotationOffset = Profile->VFX.RotationOffset;
	Config.TargetVFXScale = Profile->VFX.Scale;
	Config.TargetVFXLifetime = Profile->VFX.Lifetime;
	Config.bDestroyTargetVFXWithArea = Profile->VFX.bDestroyWithFlow;
	PathActor->InitializeGroundPath(SourceActor, SourceASC, Config);

	if (BFC)
	{
		BFC->RecordTrace(
			this,
			Profile,
			SourceActor,
			EBuffFlowTraceResult::Success,
			TEXT("Spawned area profile"),
			FString::Printf(TEXT("Effect=%s Shape=%d Policy=%d Loc=%s Rot=%s Length=%.1f Width=%.1f Duration=%.2f Tick=%.2f Value1=%.2f TargetVFX=%s"),
				*GetNameSafe(Area.Effect.Get()),
				static_cast<int32>(Area.Shape),
				static_cast<int32>(Area.TargetPolicy),
				*SpawnLocation.ToCompactString(),
				*SpawnRotation.ToCompactString(),
				Area.Length,
				Area.Width,
				Area.Duration,
				Area.TickInterval,
				Area.SetByCallerValue1,
				*GetNameSafe(Profile->VFX.NiagaraSystem.Get())));
	}

	TriggerOutput(TEXT("Out"), true);
}

bool UBFNode_SpawnRuneAreaProfile::ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const
{
	if (!SourceActor || !Profile)
	{
		return false;
	}

	FVector Forward = SourceActor->GetActorForwardVector();
	switch (Profile->Area.FacingMode)
	{
	case ERuneGroundPathFacingMode::ControllerRotation:
		if (const APawn* Pawn = Cast<APawn>(SourceActor))
		{
			if (const AController* Controller = Pawn->GetController())
			{
				Forward = Controller->GetControlRotation().Vector();
			}
		}
		break;
	case ERuneGroundPathFacingMode::ToLastDamageTarget:
		if (AActor* TargetActor = ResolveTarget(EBFTargetSelector::LastDamageTarget))
		{
			const FVector ToTarget = TargetActor->GetActorLocation() - SourceActor->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				Forward = ToTarget;
			}
		}
		break;
	default:
		break;
	}

	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		return false;
	}

	OutForward = Forward;
	OutRight = FVector::CrossProduct(FVector::UpVector, OutForward).GetSafeNormal();
	if (OutRight.IsNearlyZero())
	{
		OutRight = SourceActor->GetActorRightVector();
		OutRight.Z = 0.0f;
		OutRight = OutRight.GetSafeNormal();
	}
	return !OutRight.IsNearlyZero();
}
