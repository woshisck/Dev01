#include "BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Types/FlowDataPinResults.h"

UBFNode_CalcRuneGroundPathTransform::UBFNode_CalcRuneGroundPathTransform(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Area");
#endif

	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_CalcRuneGroundPathTransform::ExecuteBuffFlowInput(const FName& PinName)
{
	Super::ExecuteBuffFlowInput(PinName);

	AActor* SourceActor = ResolveTarget(Source);
	if (!SourceActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CalcRuneGroundPathTransform] Failed: source target is null Source=%d"), static_cast<int32>(Source));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	if (!ResolveFacing(SourceActor, Forward, Right))
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FFlowDataPinResult_Float LengthResult = TryResolveDataPinAsFloat(GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, Length));
	const float ResolvedLength = FMath::Max(1.0f, LengthResult.Result == EFlowDataPinResolveResult::Success ? static_cast<float>(LengthResult.Value) : Length.Value);
	const float CenterOffset = bCenterOnPathLength ? ResolvedLength * 0.5f : 0.0f;

	FVector Location = SourceActor->GetActorLocation()
		+ Forward * (SpawnOffset.X + CenterOffset)
		+ Right * SpawnOffset.Y
		+ FVector::UpVector * SpawnOffset.Z;

	if (bProjectToGround)
	{
		if (UWorld* World = SourceActor->GetWorld())
		{
			FHitResult GroundHit;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CalcRuneGroundPathTransformTrace), false, SourceActor);
			if (World->LineTraceSingleByChannel(
				GroundHit,
				Location + FVector(0.0f, 0.0f, FMath::Max(0.0f, GroundTraceUp)),
				Location - FVector(0.0f, 0.0f, FMath::Max(0.0f, GroundTraceDown)),
				ECC_Visibility,
				QueryParams))
			{
				Location.Z = GroundHit.Location.Z + SpawnOffset.Z;
			}
		}
	}

	FRotator Rotation = Forward.Rotation();
	Rotation.Pitch = 0.0f;
	Rotation.Roll = 0.0f;
	Rotation.Yaw += RotationYawOffset;
	Rotation.Normalize();

	SpawnLocation = FFlowDataPinOutputProperty_Vector(Location);
	SpawnRotation = FFlowDataPinOutputProperty_Rotator(Rotation);
	ForwardVector = FFlowDataPinOutputProperty_Vector(Forward);

	UE_LOG(LogTemp, Warning, TEXT("[CalcRuneGroundPathTransform] Source=%s Facing=%d Length=%.1f Offset=%s Center=%d Location=%s Rotation=%s"),
		*GetNameSafe(SourceActor),
		static_cast<int32>(FacingMode),
		ResolvedLength,
		*SpawnOffset.ToCompactString(),
		bCenterOnPathLength ? 1 : 0,
		*Location.ToCompactString(),
		*Rotation.ToCompactString());

	TriggerOutput(TEXT("Out"), true);
}

bool UBFNode_CalcRuneGroundPathTransform::ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const
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
