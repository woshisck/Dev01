#include "Actors/CelesPointLight.h"

#include "CelesLightBlueprintLibrary.h"
#include "Components/CelesLightReceiveComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

namespace
{
#if WITH_EDITORONLY_DATA
	UTexture2D* LoadPointLightBillboardTexture()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CelesLight"));
		if (!Plugin.IsValid())
		{
			return nullptr;
		}

		const FString IconPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("CelesPointLightBillboard.png"));
		if (!FPaths::FileExists(IconPath))
		{
			return nullptr;
		}

		return FImageUtils::ImportFileAsTexture2D(IconPath);
	}
#endif
}

ACelesPointLight::ACelesPointLight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PointLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLightComponent"));
	if (PointLightComponent)
	{
		PointLightComponent->SetupAttachment(SceneRoot);
		PointLightComponent->Mobility = EComponentMobility::Movable;
		PointLightComponent->SetIntensity(5000.0f);
		PointLightComponent->SetAttenuationRadius(1000.0f);
		PointLightComponent->SetLightColor(FLinearColor::White);
	}

#if WITH_EDITORONLY_DATA
	UTexture2D* CelesLightIcon = LoadPointLightBillboardTexture();
	if (PointLightComponent && CelesLightIcon)
	{
		PointLightComponent->StaticEditorTexture = CelesLightIcon;
		PointLightComponent->DynamicEditorTexture = CelesLightIcon;
		PointLightComponent->StaticEditorTextureScale = 0.75f;
		PointLightComponent->DynamicEditorTextureScale = 0.75f;
	}
#endif

	SphereVolume = CreateDefaultSubobject<USphereComponent>(TEXT("SphereVolume"));
	if (SphereVolume)
	{
		SphereVolume->SetupAttachment(SceneRoot);
		SphereVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
		SphereVolume->SetGenerateOverlapEvents(true);
		SphereVolume->SetHiddenInGame(true);
	}

	PrimaryActorTick.bCanEverTick = false;
}

void ACelesPointLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConstructionUpdate();
}

void ACelesPointLight::BeginPlay()
{
	Super::BeginPlay();

	if (SphereVolume)
	{
		SphereVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACelesPointLight::HandleSphereBeginOverlap);
		SphereVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &ACelesPointLight::HandleSphereEndOverlap);
	}

	ConstructionUpdate();
	RegisterOverlappingReceivers(false);
}

void ACelesPointLight::ConstructionUpdate()
{
	UPointLightComponent* SourcePointLight = GetLight_Implementation();
	const float Radius = GetCelesRadius();

	if (SphereVolume)
	{
		SphereVolume->SetSphereRadius(Radius, true);
	}

	if (SourcePointLight)
	{
		SourcePointLight->SetVisibility(bEnableOriginLight);
	}
}

void ACelesPointLight::RegisterOverlappingReceivers(const bool bInEditor)
{
	TArray<AActor*> ReceiveActors;
	UCelesLightBlueprintLibrary::CelesLightReceiveTrace(this, GetActorLocation(), GetCelesRadius(), FilterActorTags, ReceiveActors);
	for (AActor* Actor : ReceiveActors)
	{
		RegisterReceiverActor(Actor, bInEditor);
	}
}

UPointLightComponent* ACelesPointLight::GetLight_Implementation() const
{
	return PointLightComponent;
}

AActor* ACelesPointLight::GetActor_Implementation() const
{
	return const_cast<ACelesPointLight*>(this);
}

void ACelesPointLight::GetCelesLightData_Implementation(FCelesLightSourceData& OutData) const
{
	const UPointLightComponent* SourcePointLight = GetLight_Implementation();
	OutData.WorldPosition = GetActorLocation();
	OutData.Radius = GetCelesRadius();
	OutData.Intensity = SourcePointLight ? SourcePointLight->Intensity * LightIntensityMultiply : 0.0f;
	OutData.Color = SourcePointLight ? FLinearColor(SourcePointLight->LightColor) : FLinearColor::White;
	OutData.bFillLight = bFillLight;
	OutData.SmoothStepMin = LightSmoothStepMin;
	OutData.SmoothStepMax = LightSmoothStepMax;
	OutData.SpecularOffset = LightSpecOffset;
	OutData.EffectType = LightEffectType;
}

void ACelesPointLight::HandleSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	RegisterReceiverActor(OtherActor, false);
}

void ACelesPointLight::HandleSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	DeregisterReceiverActor(OtherActor, false);
}

void ACelesPointLight::RegisterReceiverActor(AActor* Actor, const bool bInEditor)
{
	if (!Actor)
	{
		return;
	}

	TArray<UCelesLightReceiveComponent*> ReceiveComponents;
	Actor->GetComponents(ReceiveComponents);
	for (UCelesLightReceiveComponent* ReceiveComponent : ReceiveComponents)
	{
		if (ReceiveComponent)
		{
			ReceiveComponent->RegistLight(this, bInEditor);
			ReceiveComponent->UpdateLightInfo(bInEditor);
		}
	}
}

void ACelesPointLight::DeregisterReceiverActor(AActor* Actor, const bool bInEditor)
{
	if (!Actor)
	{
		return;
	}

	TArray<UCelesLightReceiveComponent*> ReceiveComponents;
	Actor->GetComponents(ReceiveComponents);
	for (UCelesLightReceiveComponent* ReceiveComponent : ReceiveComponents)
	{
		if (ReceiveComponent)
		{
			ReceiveComponent->DeregistLight(this, bInEditor);
			ReceiveComponent->UpdateLightInfo(bInEditor);
		}
	}
}

float ACelesPointLight::GetCelesRadius() const
{
	const UPointLightComponent* SourcePointLight = GetLight_Implementation();
	return SourcePointLight ? FMath::Max(0.0f, SourcePointLight->AttenuationRadius) : 0.0f;
}
