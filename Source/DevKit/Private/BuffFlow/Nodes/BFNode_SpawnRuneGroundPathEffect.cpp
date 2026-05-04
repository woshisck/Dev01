#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Types/FlowDataPinResults.h"

UBFNode_SpawnRuneGroundPathEffect::UBFNode_SpawnRuneGroundPathEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Area");
#endif

	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnRuneGroundPathEffect::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);

	AActor* SourceActor = ResolveTarget(Source);
	if (!SourceActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnRuneGroundPath] Failed: source target is null Effect=%s Source=%d"),
			*GetNameSafe(Effect),
			static_cast<int32>(Source));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	if (!SourceASC)
	{
		SourceASC = GetOwnerASC();
	}

	if (!SourceASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnRuneGroundPath] Failed: source ASC is null Source=%s Effect=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(Effect));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	if (!ResolveFacing(SourceActor, Forward, Right))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnRuneGroundPath] Failed: cannot resolve facing Source=%s Effect=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(Effect));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const float SafeLength = FMath::Max(1.0f, Length);
	const float CenterOffset = bCenterOnPathLength ? SafeLength * 0.5f : 0.0f;
	FVector SpawnLocation = SourceActor->GetActorLocation()
		+ Forward * (SpawnOffset.X + CenterOffset)
		+ Right * SpawnOffset.Y
		+ FVector::UpVector * SpawnOffset.Z;

	const FFlowDataPinResult_Vector OverrideLocation = TryResolveDataPinAsVector(GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneGroundPathEffect, SpawnLocationOverride));
	const bool bUseLocationOverride = OverrideLocation.Result == EFlowDataPinResolveResult::Success;
	if (bUseLocationOverride)
	{
		SpawnLocation = OverrideLocation.Value;
	}

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpawnRuneGroundPathTrace), false, SourceActor);
	if (!bUseLocationOverride && World->LineTraceSingleByChannel(
		GroundHit,
		SpawnLocation + FVector(0.0f, 0.0f, 240.0f),
		SpawnLocation - FVector(0.0f, 0.0f, 900.0f),
		ECC_Visibility,
		QueryParams))
	{
		SpawnLocation.Z = GroundHit.Location.Z + SpawnOffset.Z;
	}

	FRotator SpawnRotation = Forward.Rotation();
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;
	SpawnRotation.Yaw += RotationYawOffset;
	SpawnRotation.Normalize();

	const FFlowDataPinResult_Rotator OverrideRotation = TryResolveDataPinAsRotator(GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneGroundPathEffect, SpawnRotationOverride));
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
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FRuneGroundPathEffectConfig Config;
	Config.Effect = Effect;
	Config.TargetPolicy = TargetPolicy;
	Config.Shape = Shape;
	Config.Duration = Duration;
	Config.TickInterval = TickInterval;
	Config.Length = Length;
	Config.Width = Width;
	Config.Height = Height;
	Config.DecalProjectionDepth = DecalProjectionDepth;
	Config.DecalPlaneRotationDegrees = DecalPlaneRotationDegrees;
	Config.SpawnOffset = SpawnOffset;
	Config.DecalMaterial = DecalMaterial;
	Config.NiagaraSystem = NiagaraSystem;
	Config.NiagaraScale = NiagaraScale;
	Config.NiagaraInstanceCount = NiagaraInstanceCount;
	Config.SetByCallerTag1 = SetByCallerTag1;
	Config.SetByCallerValue1 = SetByCallerValue1.Value;
	Config.SetByCallerTag2 = SetByCallerTag2;
	Config.SetByCallerValue2 = SetByCallerValue2.Value;
	Config.ApplicationCount = ApplicationCount;
	Config.bApplyOncePerTarget = bApplyOncePerTarget;
	PathActor->InitializeGroundPath(SourceActor, SourceASC, Config);

	UE_LOG(LogTemp, Warning, TEXT("[SpawnRuneGroundPath] Spawned Effect=%s Source=%s Policy=%d Shape=%d Facing=%d Location=%s Rotation=%s Length=%.1f Width=%.1f Duration=%.2f BurnDamage=%.2f DecalPlaneRot=%.1f OverrideLoc=%d OverrideRot=%d"),
		*GetNameSafe(Effect),
		*GetNameSafe(SourceActor),
		static_cast<int32>(TargetPolicy),
		static_cast<int32>(Shape),
		static_cast<int32>(FacingMode),
		*SpawnLocation.ToCompactString(),
		*SpawnRotation.ToCompactString(),
		Length,
		Width,
		Duration,
		SetByCallerValue1.Value,
		DecalPlaneRotationDegrees,
		bUseLocationOverride ? 1 : 0,
		OverrideRotation.Result == EFlowDataPinResolveResult::Success ? 1 : 0);

	TriggerOutput(TEXT("Out"), true);
}

bool UBFNode_SpawnRuneGroundPathEffect::ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const
{
	if (!SourceActor)
	{
		return false;
	}

	FVector Forward = SourceActor->GetActorForwardVector();
	if (FacingMode == ERuneGroundPathFacingMode::ControllerRotation)
	{
		if (const APawn* Pawn = Cast<APawn>(SourceActor))
		{
			if (const AController* Controller = Pawn->GetController())
			{
				Forward = Controller->GetControlRotation().Vector();
			}
		}
	}
	else if (FacingMode == ERuneGroundPathFacingMode::ToLastDamageTarget)
	{
		if (AActor* TargetActor = ResolveTarget(EBFTargetSelector::LastDamageTarget))
		{
			const FVector ToTarget = TargetActor->GetActorLocation() - SourceActor->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				Forward = ToTarget;
			}
		}
	}

	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = SourceActor->GetActorForwardVector();
		Forward.Z = 0.0f;
		Forward = Forward.GetSafeNormal();
	}
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
