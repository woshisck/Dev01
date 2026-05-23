#include "Water/InteractiveWaterVolume.h"

#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AInteractiveWaterVolume::AInteractiveWaterVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WaterSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterSurface"));
	WaterSurface->SetupAttachment(SceneRoot);
	WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurface->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		WaterSurface->SetStaticMesh(PlaneMesh.Object);
	}

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionBounds->SetBoxExtent(FVector(500.0f, 500.0f, 100.0f));

	CausticsDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("CausticsDecal"));
	CausticsDecal->SetupAttachment(SceneRoot);
	CausticsDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	CausticsDecal->DecalSize = FVector(64.0f, 1000.0f, 1000.0f);
}

void AInteractiveWaterVolume::BeginPlay()
{
	Super::BeginPlay();
	InitializeMaterials();
	InitializeRenderTargets();
	PushImpulseParametersToMaterial();
}

void AInteractiveWaterVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DecayInteractionRenderTarget(DeltaSeconds);
	TickImpulses(DeltaSeconds);
	PushImpulseParametersToMaterial();
}

bool AInteractiveWaterVolume::CanReceiveWaterImpulse_Implementation(FVector WorldLocation, float Radius) const
{
	if (!InteractionBounds)
	{
		return false;
	}

	const FBox Bounds = InteractionBounds->Bounds.GetBox();
	const FVector ClosestPoint = Bounds.GetClosestPointTo(WorldLocation);
	return FVector::DistSquared(ClosestPoint, WorldLocation) <= FMath::Square(FMath::Max(0.0f, Radius));
}

void AInteractiveWaterVolume::AddWaterImpulse_Implementation(
	FVector WorldLocation,
	float Radius,
	float Strength,
	EWaterImpulseType Type,
	FVector Direction,
	float MassScale)
{
	if (!CanReceiveWaterImpulse_Implementation(WorldLocation, Radius))
	{
		return;
	}

	FWaterImpulseRuntimeState Impulse;
	Impulse.WorldLocation = WorldLocation;
	Impulse.Radius = FMath::Max(1.0f, Radius);
	Impulse.Strength = FMath::Max(0.0f, Strength);
	Impulse.Type = Type;
	Impulse.Direction = Direction.GetSafeNormal2D();
	if (Impulse.Direction.IsNearlyZero())
	{
		Impulse.Direction = FVector::ForwardVector;
	}
	Impulse.MassScale = FMath::Max(0.0f, MassScale);
	Impulse.Lifetime = GetLifetimeForImpulse(Type);

	ActiveImpulses.Insert(Impulse, 0);
	const int32 MaxImpulses = FMath::Max(1, MaxMaterialImpulses);
	if (ActiveImpulses.Num() > MaxImpulses)
	{
		ActiveImpulses.SetNum(MaxImpulses);
	}

	DrawImpulseToRenderTarget(Impulse);
	SpawnImpulseNiagara(Impulse);
	PushImpulseParametersToMaterial();
}

void AInteractiveWaterVolume::SetPerformanceTier(EWaterPerformanceTier NewTier)
{
	if (PerformanceTier == NewTier)
	{
		return;
	}

	PerformanceTier = NewTier;
	InitializeRenderTargets();
	PushImpulseParametersToMaterial();
}

void AInteractiveWaterVolume::InitializeMaterials()
{
	if (WaterSurface)
	{
		UMaterialInterface* SourceMaterial = WaterMaterial ? WaterMaterial.Get() : WaterSurface->GetMaterial(0);
		if (SourceMaterial)
		{
			WaterMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
			WaterSurface->SetMaterial(0, WaterMID);
			ApplyVisualTuningToMaterial(WaterMID);
		}
	}

	if (CausticsMaterial)
	{
		CausticsMID = UMaterialInstanceDynamic::Create(CausticsMaterial, this);
		CausticsDecal->SetDecalMaterial(CausticsMID);
	}
	else if (CausticsDecal)
	{
		CausticsMID = CausticsDecal->CreateDynamicMaterialInstance();
	}

	if (InteractionStampMaterial)
	{
		StampMID = UMaterialInstanceDynamic::Create(InteractionStampMaterial, this);
	}
	if (InteractionDecayMaterial)
	{
		DecayMID = UMaterialInstanceDynamic::Create(InteractionDecayMaterial, this);
	}
}

void AInteractiveWaterVolume::InitializeRenderTargets()
{
	const int32 TargetSize = PerformanceTier == EWaterPerformanceTier::High ? HighTierRenderTargetSize : LowTierRenderTargetSize;
	const int32 ClampedSize = FMath::Clamp(TargetSize, 64, 2048);

	InteractionRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("WaterInteractionRT"));
	InteractionRenderTarget->RenderTargetFormat = RTF_RGBA16f;
	InteractionRenderTarget->ClearColor = FLinearColor::Transparent;
	InteractionRenderTarget->bAutoGenerateMips = false;
	InteractionRenderTarget->InitAutoFormat(ClampedSize, ClampedSize);
	InteractionRenderTarget->UpdateResourceImmediate(true);

	ScratchRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("WaterInteractionScratchRT"));
	ScratchRenderTarget->RenderTargetFormat = RTF_RGBA16f;
	ScratchRenderTarget->ClearColor = FLinearColor::Transparent;
	ScratchRenderTarget->bAutoGenerateMips = false;
	ScratchRenderTarget->InitAutoFormat(ClampedSize, ClampedSize);
	ScratchRenderTarget->UpdateResourceImmediate(true);

	if (UWorld* World = GetWorld())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(World, InteractionRenderTarget, FLinearColor::Transparent);
		UKismetRenderingLibrary::ClearRenderTarget2D(World, ScratchRenderTarget, FLinearColor::Transparent);
	}

	if (WaterMID)
	{
		WaterMID->SetTextureParameterValue(TEXT("WaterInteractionRT"), InteractionRenderTarget);
		WaterMID->SetVectorParameterValue(TEXT("WaterInteractionWorldBounds"), GetWaterBoundsVector());
		WaterMID->SetScalarParameterValue(TEXT("WaterInteractionRTSize"), static_cast<float>(ClampedSize));
	}
}

void AInteractiveWaterVolume::ApplyVisualTuningToMaterial(UMaterialInstanceDynamic* TargetMID)
{
	if (!TargetMID)
	{
		return;
	}

	TargetMID->SetVectorParameterValue(TEXT("BaseWaterColor"), VisualTuning.BaseWaterColor);
	TargetMID->SetVectorParameterValue(TEXT("DepthColor"), VisualTuning.DepthColor);
	TargetMID->SetScalarParameterValue(TEXT("Opacity"), VisualTuning.Opacity);
	TargetMID->SetScalarParameterValue(TEXT("Roughness"), VisualTuning.Roughness);
	TargetMID->SetScalarParameterValue(TEXT("SpecularIntensity"), VisualTuning.SpecularIntensity);
	TargetMID->SetScalarParameterValue(TEXT("WaveScale"), VisualTuning.WaveScale);
	TargetMID->SetVectorParameterValue(TEXT("FoamColor"), VisualTuning.FoamColor);
	TargetMID->SetScalarParameterValue(TEXT("FoamStrength"), VisualTuning.FoamStrength);
	TargetMID->SetScalarParameterValue(TEXT("FoamDecay"), VisualTuning.FoamDecay);
	TargetMID->SetVectorParameterValue(TEXT("TurbidityColor"), VisualTuning.TurbidityColor);
	TargetMID->SetScalarParameterValue(TEXT("TurbidityStrength"), VisualTuning.TurbidityStrength);
	TargetMID->SetScalarParameterValue(TEXT("TurbidityClearTime"), VisualTuning.TurbidityClearTime);
	TargetMID->SetScalarParameterValue(TEXT("GlintIntensity"), VisualTuning.GlintIntensity);
	TargetMID->SetScalarParameterValue(TEXT("GlintThreshold"), VisualTuning.GlintThreshold);
	TargetMID->SetScalarParameterValue(TEXT("GlintLifetime"), VisualTuning.GlintLifetime);

	const FWaterSurfaceAttachmentLayer* AttachmentLayers[] =
	{
		&VisualTuning.AttachmentLayer1,
		&VisualTuning.AttachmentLayer2,
		&VisualTuning.AttachmentLayer3
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AttachmentLayers); ++Index)
	{
		const int32 LayerNumber = Index + 1;
		const FWaterSurfaceAttachmentLayer& Layer = *AttachmentLayers[Index];
		const FString Prefix = FString::Printf(TEXT("AttachmentLayer%d"), LayerNumber);

		if (Layer.Texture)
		{
			TargetMID->SetTextureParameterValue(FName(*FString::Printf(TEXT("%sTexture"), *Prefix)), Layer.Texture);
		}
		TargetMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sTint"), *Prefix)), Layer.Tint);
		TargetMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("%sIntensity"), *Prefix)), Layer.Intensity);
		TargetMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sTilingSpeed"), *Prefix)), FLinearColor(Layer.Tiling.X, Layer.Tiling.Y, Layer.ScrollSpeed.X, Layer.ScrollSpeed.Y));
		TargetMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("%sRoughnessInfluence"), *Prefix)), Layer.RoughnessInfluence);
		TargetMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("%sDisplaceable"), *Prefix)), Layer.bDisplacedByInteraction ? 1.0f : 0.0f);
	}
}

void AInteractiveWaterVolume::TickImpulses(float DeltaSeconds)
{
	for (int32 Index = ActiveImpulses.Num() - 1; Index >= 0; --Index)
	{
		ActiveImpulses[Index].Age += FMath::Max(0.0f, DeltaSeconds);
		if (ActiveImpulses[Index].Age >= ActiveImpulses[Index].Lifetime)
		{
			ActiveImpulses.RemoveAtSwap(Index);
		}
	}
}

void AInteractiveWaterVolume::PushImpulseParametersToMaterial()
{
	if (!WaterMID)
	{
		return;
	}

	WaterMID->SetScalarParameterValue(TEXT("ActiveWaterImpulseCount"), static_cast<float>(ActiveImpulses.Num()));
	WaterMID->SetVectorParameterValue(TEXT("WaterInteractionWorldBounds"), GetWaterBoundsVector());
	if (InteractionRenderTarget)
	{
		WaterMID->SetTextureParameterValue(TEXT("WaterInteractionRT"), InteractionRenderTarget);
	}

	const int32 Count = FMath::Min(ActiveImpulses.Num(), FMath::Max(1, MaxMaterialImpulses));
	for (int32 Index = 0; Index < FMath::Max(1, MaxMaterialImpulses); ++Index)
	{
		const FString Prefix = FString::Printf(TEXT("WaterImpulse%d"), Index);
		if (Index < Count)
		{
			const FWaterImpulseRuntimeState& Impulse = ActiveImpulses[Index];
			const float Fade = Impulse.GetFade();
			WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sWorldLocationRadius"), *Prefix)), FLinearColor(Impulse.WorldLocation.X, Impulse.WorldLocation.Y, Impulse.WorldLocation.Z, Impulse.Radius));
			WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sDirectionStrength"), *Prefix)), FLinearColor(Impulse.Direction.X, Impulse.Direction.Y, GetFoamStrengthForImpulse(Impulse.Type, Impulse.Strength, Impulse.MassScale) * Fade, GetTurbidityStrengthForImpulse(Impulse.Type, Impulse.Strength, Impulse.MassScale) * Fade));
			WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sAgeFadeType"), *Prefix)), FLinearColor(Impulse.GetNormalizedAge(), Fade, static_cast<float>(Impulse.Type), Impulse.MassScale));
		}
		else
		{
			WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sWorldLocationRadius"), *Prefix)), FLinearColor::Transparent);
			WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sDirectionStrength"), *Prefix)), FLinearColor::Transparent);
			WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sAgeFadeType"), *Prefix)), FLinearColor::Transparent);
		}
	}
}

void AInteractiveWaterVolume::DrawImpulseToRenderTarget(const FWaterImpulseRuntimeState& Impulse)
{
	if (!StampMID || !InteractionRenderTarget || !ScratchRenderTarget || !GetWorld())
	{
		return;
	}

	StampMID->SetTextureParameterValue(TEXT("PreviousInteractionTexture"), InteractionRenderTarget);
	StampMID->SetVectorParameterValue(TEXT("WaterInteractionWorldBounds"), GetWaterBoundsVector());
	StampMID->SetVectorParameterValue(TEXT("ImpulseWorldLocationRadius"), FLinearColor(Impulse.WorldLocation.X, Impulse.WorldLocation.Y, Impulse.WorldLocation.Z, Impulse.Radius));
	StampMID->SetVectorParameterValue(TEXT("ImpulseDirectionStrength"), FLinearColor(Impulse.Direction.X, Impulse.Direction.Y, Impulse.Strength, Impulse.MassScale));
	StampMID->SetVectorParameterValue(TEXT("ImpulseChannels"), FLinearColor(
		Impulse.Strength,
		GetFoamStrengthForImpulse(Impulse.Type, Impulse.Strength, Impulse.MassScale),
		GetTurbidityStrengthForImpulse(Impulse.Type, Impulse.Strength, Impulse.MassScale),
		Impulse.Type == EWaterImpulseType::Footstep ? 0.25f : 1.0f));

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), ScratchRenderTarget, StampMID);
	SwapInteractionRenderTargets();
}

void AInteractiveWaterVolume::DecayInteractionRenderTarget(float DeltaSeconds)
{
	if (!DecayMID || !InteractionRenderTarget || !ScratchRenderTarget || !GetWorld())
	{
		return;
	}

	DecayMID->SetTextureParameterValue(TEXT("PreviousInteractionTexture"), InteractionRenderTarget);
	DecayMID->SetScalarParameterValue(TEXT("DeltaSeconds"), FMath::Max(0.0f, DeltaSeconds));
	DecayMID->SetScalarParameterValue(TEXT("FoamDecay"), VisualTuning.FoamDecay);
	DecayMID->SetScalarParameterValue(TEXT("TurbidityClearTime"), VisualTuning.TurbidityClearTime);
	DecayMID->SetScalarParameterValue(TEXT("GlintLifetime"), VisualTuning.GlintLifetime);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), ScratchRenderTarget, DecayMID);
	SwapInteractionRenderTargets();
}

void AInteractiveWaterVolume::SwapInteractionRenderTargets()
{
	Swap(InteractionRenderTarget, ScratchRenderTarget);
	if (WaterMID && InteractionRenderTarget)
	{
		WaterMID->SetTextureParameterValue(TEXT("WaterInteractionRT"), InteractionRenderTarget);
	}
}

void AInteractiveWaterVolume::SpawnImpulseNiagara(const FWaterImpulseRuntimeState& Impulse) const
{
	if (!bSpawnNiagaraOnImpulse || !GetWorld())
	{
		return;
	}

	const TObjectPtr<UNiagaraSystem>* NiagaraSystem = SplashNiagaraByType.Find(Impulse.Type);
	if (!NiagaraSystem || !NiagaraSystem->Get())
	{
		return;
	}

	const float ScaleValue = FMath::Max(0.05f, Impulse.Radius / 100.0f) * FMath::Max(0.1f, Impulse.Strength) * FMath::Max(0.25f, Impulse.MassScale);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraSystem->Get(),
		Impulse.WorldLocation,
		Impulse.Direction.Rotation(),
		FVector(ScaleValue),
		true,
		true,
		ENCPoolMethod::AutoRelease);
}

float AInteractiveWaterVolume::GetLifetimeForImpulse(EWaterImpulseType Type) const
{
	switch (Type)
	{
	case EWaterImpulseType::Footstep:
		return FMath::Max(0.2f, DefaultImpulseLifetime * 0.65f);
	case EWaterImpulseType::RollOrDash:
		return FMath::Max(0.2f, DefaultImpulseLifetime * 0.9f);
	case EWaterImpulseType::Explosion:
	case EWaterImpulseType::FallingMesh:
		return FMath::Max(0.2f, DefaultImpulseLifetime * 1.6f);
	case EWaterImpulseType::MagicOrVFX:
	default:
		return FMath::Max(0.2f, DefaultImpulseLifetime);
	}
}

float AInteractiveWaterVolume::GetFoamStrengthForImpulse(EWaterImpulseType Type, float Strength, float MassScale) const
{
	const float BaseStrength = FMath::Max(0.0f, Strength) * FMath::Max(0.1f, MassScale);
	switch (Type)
	{
	case EWaterImpulseType::Footstep:
		return BaseStrength * 0.35f;
	case EWaterImpulseType::RollOrDash:
		return BaseStrength * 0.75f;
	case EWaterImpulseType::Explosion:
		return BaseStrength * 1.5f;
	case EWaterImpulseType::FallingMesh:
		return BaseStrength * 1.2f;
	case EWaterImpulseType::MagicOrVFX:
	default:
		return BaseStrength;
	}
}

float AInteractiveWaterVolume::GetTurbidityStrengthForImpulse(EWaterImpulseType Type, float Strength, float MassScale) const
{
	const float BaseStrength = FMath::Max(0.0f, Strength) * FMath::Max(0.1f, MassScale);
	switch (Type)
	{
	case EWaterImpulseType::Footstep:
		return BaseStrength * 0.2f;
	case EWaterImpulseType::RollOrDash:
		return BaseStrength * 0.45f;
	case EWaterImpulseType::Explosion:
		return BaseStrength * 1.4f;
	case EWaterImpulseType::FallingMesh:
		return BaseStrength * 1.1f;
	case EWaterImpulseType::MagicOrVFX:
	default:
		return BaseStrength * 0.8f;
	}
}

FVector4 AInteractiveWaterVolume::GetWaterBoundsVector() const
{
	if (!InteractionBounds)
	{
		const FVector Location = GetActorLocation();
		return FVector4(Location.X, Location.Y, 100.0f, 100.0f);
	}

	const FBox Bounds = InteractionBounds->Bounds.GetBox();
	return FVector4(
		Bounds.Min.X,
		Bounds.Min.Y,
		FMath::Max(1.0f, Bounds.GetSize().X),
		FMath::Max(1.0f, Bounds.GetSize().Y));
}
