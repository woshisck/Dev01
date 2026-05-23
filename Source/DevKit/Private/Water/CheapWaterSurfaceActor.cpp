#include "Water/CheapWaterSurfaceActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACheapWaterSurfaceActor::ACheapWaterSurfaceActor()
{
	PrimaryActorTick.bCanEverTick = false;

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
}

void ACheapWaterSurfaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyWaterVisualTuning();
}

void ACheapWaterSurfaceActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyWaterVisualTuning();
}

void ACheapWaterSurfaceActor::ApplyWaterVisualTuning()
{
	EnsureMaterialInstance();
	if (!WaterMID)
	{
		return;
	}

	WaterMID->SetVectorParameterValue(TEXT("BaseWaterColor"), VisualTuning.BaseWaterColor);
	WaterMID->SetVectorParameterValue(TEXT("DepthColor"), VisualTuning.DepthColor);
	WaterMID->SetScalarParameterValue(TEXT("Opacity"), VisualTuning.Opacity);
	WaterMID->SetScalarParameterValue(TEXT("Roughness"), VisualTuning.Roughness);
	WaterMID->SetScalarParameterValue(TEXT("SpecularIntensity"), VisualTuning.SpecularIntensity);
	WaterMID->SetScalarParameterValue(TEXT("WaveScale"), VisualTuning.WaveScale);
	WaterMID->SetScalarParameterValue(TEXT("FoamStrength"), 0.0f);
	WaterMID->SetScalarParameterValue(TEXT("TurbidityStrength"), 0.0f);
	WaterMID->SetScalarParameterValue(TEXT("GlintIntensity"), 0.0f);

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
			WaterMID->SetTextureParameterValue(FName(*FString::Printf(TEXT("%sTexture"), *Prefix)), Layer.Texture);
		}
		WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sTint"), *Prefix)), Layer.Tint);
		WaterMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("%sIntensity"), *Prefix)), Layer.Intensity);
		WaterMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("%sTilingSpeed"), *Prefix)), FLinearColor(Layer.Tiling.X, Layer.Tiling.Y, Layer.ScrollSpeed.X, Layer.ScrollSpeed.Y));
		WaterMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("%sRoughnessInfluence"), *Prefix)), Layer.RoughnessInfluence);
		WaterMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("%sDisplaceable"), *Prefix)), 0.0f);
	}
}

void ACheapWaterSurfaceActor::EnsureMaterialInstance()
{
	if (WaterMID || !WaterSurface)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = WaterMaterial ? WaterMaterial.Get() : WaterSurface->GetMaterial(0);
	if (!SourceMaterial)
	{
		return;
	}

	WaterMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	WaterSurface->SetMaterial(0, WaterMID);
}
