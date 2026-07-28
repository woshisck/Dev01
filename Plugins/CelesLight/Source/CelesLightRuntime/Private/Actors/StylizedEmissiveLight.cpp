#include "Actors/StylizedEmissiveLight.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StylizedEmissiveModelLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* AutomaticBatchMaterialPath =
		TEXT("/YogArt_Material/MasterMaterial/Efect/M_Emissive_Common.M_Emissive_Common");
}

AStylizedEmissiveLight::AStylizedEmissiveLight()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> DefaultEmissiveMaterial(
		TEXT("/YogArt_Material/MasterMaterial/Efect/M_Emissive_Common.M_Emissive_Common"), LOAD_NoWarn);
	EmissiveMaterial = DefaultEmissiveMaterial.Get();

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EmissiveSource = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmissiveSource"));
	EmissiveSource->SetupAttachment(SceneRoot);

#if WITH_EDITORONLY_DATA
	EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
	if (EditorSprite)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(
			TEXT("/Engine/EditorResources/S_Emitter.S_Emitter"), LOAD_NoWarn);
		EditorSprite->SetupAttachment(SceneRoot);
		EditorSprite->SetSprite(SpriteTexture.Get());
		EditorSprite->SetHiddenInGame(true);
		EditorSprite->bIsScreenSizeScaled = true;
	}
#endif

	RefreshEmissiveSource();
}

void AStylizedEmissiveLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEmissiveSource();
}

UPointLightComponent* AStylizedEmissiveLight::GetLight_Implementation() const
{
	return nullptr;
}

AActor* AStylizedEmissiveLight::GetActor_Implementation() const
{
	return const_cast<AStylizedEmissiveLight*>(this);
}

void AStylizedEmissiveLight::GetCelesLightData_Implementation(FCelesLightSourceData& OutData) const
{
	OutData.WorldPosition = GetActorLocation();
	OutData.Radius = UsesStylizedMaterialLighting() ? FMath::Max(1.0f, AttenuationRadius) : 0.0f;
	OutData.Intensity = UsesStylizedMaterialLighting() ? FMath::Max(0.0f, Intensity) : 0.0f;
	OutData.Color = LightColor;
	OutData.bFillLight = bFillLight;
	OutData.SmoothStepMin = FMath::Clamp(SmoothStepMin, 0.0f, 1.0f);
	OutData.SmoothStepMax = FMath::Clamp(SmoothStepMax, OutData.SmoothStepMin, 1.0f);
	OutData.SpecularOffset = SpecularOffset;
	OutData.EffectType = EffectType;
}

TArray<FString> AStylizedEmissiveLight::GetLibraryModelOptions() const
{
	return ModelLibrary ? ModelLibrary->GetModelOptions() : TArray<FString>();
}

FStylizedEmissivePerInstanceData AStylizedEmissiveLight::GetPerInstanceMaterialData() const
{
	FStylizedEmissivePerInstanceData Result;
	Result.EmissiveColor = LightColor;
	Result.EmissiveIntensity = FMath::Max(0.0f, EmissiveIntensity);
	return Result;
}

bool AStylizedEmissiveLight::GetAutomaticBatchRenderData(
	UStaticMesh*& OutMesh,
	UMaterialInterface*& OutMaterial,
	FTransform& OutWorldTransform,
	bool& bOutUseLumenGI) const
{
	OutMesh = nullptr;
	OutMaterial = nullptr;
	OutWorldTransform = FTransform::Identity;
	bOutUseLumenGI = false;

	if (!bAllowAutomaticBatching
		|| VisualSource == EStylizedEmissiveVisualSource::DataOnly
		|| IsHidden())
	{
		return false;
	}

	FTransform RelativeTransform = FTransform::Identity;
	ResolveVisualModel(OutMesh, OutMaterial, RelativeTransform);
	const UMaterial* BaseMaterial = OutMaterial ? OutMaterial->GetMaterial() : nullptr;
	if (!OutMesh || !BaseMaterial || BaseMaterial->GetPathName() != AutomaticBatchMaterialPath)
	{
		OutMesh = nullptr;
		OutMaterial = nullptr;
		return false;
	}

	OutWorldTransform = RelativeTransform * GetActorTransform();
	bOutUseLumenGI = UsesLumenGI();
	return true;
}

void AStylizedEmissiveLight::SetRuntimeBatched(const bool bInRuntimeBatched)
{
	if (bRuntimeBatched == bInRuntimeBatched)
	{
		return;
	}

	bRuntimeBatched = bInRuntimeBatched;
	RefreshEmissiveSource();
}

bool AStylizedEmissiveLight::ApplyLibraryPreset(UStylizedEmissiveModelLibrary* InLibrary, const FName InModelId)
{
	if (!InLibrary)
	{
		return false;
	}

	const FStylizedEmissiveModelEntry* Entry = InLibrary->FindModel(InModelId);
	if (!Entry)
	{
		return false;
	}

	Modify();
	ModelLibrary = InLibrary;
	LibraryModel = InModelId;
	VisualSource = Entry->bUseMesh && Entry->Mesh
		? EStylizedEmissiveVisualSource::ModelLibrary
		: EStylizedEmissiveVisualSource::DataOnly;
	CustomMesh = nullptr;
	EmissiveMaterial = Entry->Material;
	LightingOutput = VisualSource == EStylizedEmissiveVisualSource::DataOnly
		? EStylizedEmissiveLightingOutput::StylizedMaterial
		: Entry->LightingOutput;
	LightColor = Entry->LightColor;
	Intensity = Entry->Intensity;
	EmissiveIntensity = Entry->EmissiveIntensity;
	AttenuationRadius = Entry->AttenuationRadius;
	bFillLight = Entry->bFillLight;
	SmoothStepMin = Entry->SmoothStepMin;
	SmoothStepMax = Entry->SmoothStepMax;
	SpecularOffset = Entry->SpecularOffset;
	EffectType = Entry->EffectType;
	RefreshEmissiveSource();
	return true;
}

void AStylizedEmissiveLight::RefreshEmissiveSource()
{
	if (!EmissiveSource)
	{
		return;
	}

	UStaticMesh* ResolvedMesh = nullptr;
	UMaterialInterface* ResolvedMaterial = EmissiveMaterial;
	FTransform RelativeTransform = FTransform::Identity;
	ResolveVisualModel(ResolvedMesh, ResolvedMaterial, RelativeTransform);

	const bool bHasVisibleModel = VisualSource != EStylizedEmissiveVisualSource::DataOnly && ResolvedMesh != nullptr;
	const bool bUseLumenGI = bHasVisibleModel && UsesLumenGI();

#if WITH_EDITORONLY_DATA
	if (EditorSprite)
	{
		EditorSprite->SetVisibility(!bHasVisibleModel);
	}
#endif

	EmissiveSource->SetStaticMesh(bHasVisibleModel ? ResolvedMesh : nullptr);
	EmissiveSource->SetRelativeTransform(RelativeTransform);
	EmissiveSource->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmissiveSource->SetGenerateOverlapEvents(false);
	EmissiveSource->SetCastShadow(false);
	EmissiveSource->SetReceivesDecals(false);

	// Lumen emissive geometry is always a genuinely visible model. A data-only
	// source has no renderable primitive, avoiding hidden geometry occlusion.
	EmissiveSource->SetVisibility(bHasVisibleModel && !bRuntimeBatched);
	EmissiveSource->SetHiddenInGame(!bHasVisibleModel || bRuntimeBatched);
	EmissiveSource->SetRenderInMainPass(true);
	EmissiveSource->SetAffectDynamicIndirectLighting(bUseLumenGI);
	EmissiveSource->SetAffectIndirectLightingWhileHidden(false);
	EmissiveSource->SetEmissiveLightSource(bUseLumenGI);
	EmissiveSource->SetVisibleInRayTracing(bUseLumenGI);
	EmissiveSource->SetAffectDistanceFieldLighting(bUseLumenGI);

	if (bHasVisibleModel && ResolvedMaterial)
	{
		if (!EmissiveMaterialInstance || EmissiveMaterialInstance->Parent != ResolvedMaterial)
		{
			EmissiveMaterialInstance = UMaterialInstanceDynamic::Create(ResolvedMaterial, this, TEXT("StylizedEmissiveMID"));
		}
		if (EmissiveMaterialInstance)
		{
			EmissiveMaterialInstance->SetVectorParameterValue(TEXT("Emissive Color"), LightColor);
			EmissiveMaterialInstance->SetScalarParameterValue(TEXT("Emissive Intensity"), FMath::Max(0.0f, EmissiveIntensity));
			EmissiveSource->SetMaterial(0, EmissiveMaterialInstance);
		}
	}
	else
	{
		EmissiveMaterialInstance = nullptr;
		EmissiveSource->SetMaterial(0, nullptr);
	}
}

bool AStylizedEmissiveLight::UsesStylizedMaterialLighting() const
{
	return VisualSource == EStylizedEmissiveVisualSource::DataOnly
		|| LightingOutput != EStylizedEmissiveLightingOutput::LumenGI;
}

bool AStylizedEmissiveLight::UsesLumenGI() const
{
	return LightingOutput != EStylizedEmissiveLightingOutput::StylizedMaterial;
}

void AStylizedEmissiveLight::ResolveVisualModel(
	UStaticMesh*& OutMesh,
	UMaterialInterface*& OutMaterial,
	FTransform& OutRelativeTransform) const
{
	OutMesh = nullptr;
	OutMaterial = EmissiveMaterial;
	OutRelativeTransform = FTransform::Identity;

	if (VisualSource == EStylizedEmissiveVisualSource::CustomMesh)
	{
		OutMesh = CustomMesh ? CustomMesh.Get() : SourceMesh.Get();
		return;
	}

	if (VisualSource == EStylizedEmissiveVisualSource::ModelLibrary && ModelLibrary)
	{
		if (const FStylizedEmissiveModelEntry* Entry = ModelLibrary->FindModel(LibraryModel))
		{
			OutMesh = Entry->Mesh;
			OutMaterial = Entry->Material ? Entry->Material.Get() : EmissiveMaterial.Get();
			OutRelativeTransform = Entry->RelativeTransform;
		}
	}
}
