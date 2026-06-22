#include "Components/CelesSDFShadowUpdateComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

UCelesSDFShadowUpdateComponent::UCelesSDFShadowUpdateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCelesSDFShadowUpdateComponent::OnRegister()
{
	Super::OnRegister();

	if (!SkeletalMesh && GetOwner())
	{
		SkeletalMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	}
}

void UCelesSDFShadowUpdateComponent::BeginPlay()
{
	Super::BeginPlay();
	InitSDF();
}

void UCelesSDFShadowUpdateComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SDFShadowTick();
}

void UCelesSDFShadowUpdateComponent::InitSDF()
{
	if (!SkeletalMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDFShadowUpdate Error : None Skeletal Mesh"));
		SetComponentTickEnabled(false);
		return;
	}

	if (FaceMaterialID < 0 || FaceMaterialID >= SkeletalMesh->GetNumMaterials())
	{
		UE_LOG(LogTemp, Warning, TEXT("SDFShadowUpdate Error : Invalid Material ID"));
		SetComponentTickEnabled(false);
		return;
	}

	SDFDynMaterial = SkeletalMesh->CreateDynamicMaterialInstance(FaceMaterialID);
	if (!SDFDynMaterial)
	{
		SetComponentTickEnabled(false);
		return;
	}

	SetComponentTickInterval(FMath::Max(0.01f, TickTimeSeconds));
	SetComponentTickEnabled(true);
	SDFShadowTick();
}

void UCelesSDFShadowUpdateComponent::SDFShadowTick()
{
	if (!SDFDynMaterial)
	{
		return;
	}

	const FRotator TraceRotation = ResolveTraceRotation();
	const FVector ForwardDirection = GetDirectionByAxis(TraceRotation, TraceBoneFwdAxis, bInvertFwd);
	const FVector RightDirection = GetDirectionByAxis(TraceRotation, TraceBoneRtAxis, bInvertRt);

	SDFDynMaterial->SetVectorParameterValue(ForwardParameterName, FLinearColor(ForwardDirection));
	SDFDynMaterial->SetVectorParameterValue(RightParameterName, FLinearColor(RightDirection));
}

FVector UCelesSDFShadowUpdateComponent::GetDirectionByAxis(const FRotator Rot, const TEnumAsByte<EAxis::Type> Axis, const bool bInvert)
{
	FVector Direction = FVector::ZeroVector;

	switch (Axis.GetValue())
	{
	case EAxis::X:
		Direction = Rot.Vector();
		break;
	case EAxis::Y:
		Direction = FRotationMatrix(Rot).GetScaledAxis(EAxis::Y);
		break;
	case EAxis::Z:
		Direction = FRotationMatrix(Rot).GetScaledAxis(EAxis::Z);
		break;
	case EAxis::None:
	default:
		Direction = FVector::ZeroVector;
		break;
	}

	return bInvert ? -Direction : Direction;
}

FRotator UCelesSDFShadowUpdateComponent::ResolveTraceRotation() const
{
	if (SkeletalMesh)
	{
		if (!TraceBoneName.IsNone() && SkeletalMesh->DoesSocketExist(TraceBoneName))
		{
			return SkeletalMesh->GetSocketRotation(TraceBoneName);
		}

		return SkeletalMesh->GetComponentRotation();
	}

	return FRotator::ZeroRotator;
}
