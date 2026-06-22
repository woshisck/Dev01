#include "Actors/CelesLightCaptureBox.h"

#include "CanvasItem.h"
#include "CelesLightBlueprintLibrary.h"
#include "CelesLightEncoder.h"
#include "CelesLightSourceInterface.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
#if WITH_EDITORONLY_DATA
	UTexture2D* LoadCaptureBoxBillboardTexture()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CelesLight"));
		if (!Plugin.IsValid())
		{
			return nullptr;
		}

		const FString IconPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("CelesLightCaptureBoxBillboard.png"));
		if (!FPaths::FileExists(IconPath))
		{
			return nullptr;
		}

		return FImageUtils::ImportFileAsTexture2D(IconPath);
	}
#endif
}

ACelesLightCaptureBox::ACelesLightCaptureBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CaptureBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureBounds"));
	if (CaptureBounds)
	{
		CaptureBounds->SetupAttachment(SceneRoot);
		CaptureBounds->SetBoxExtent(FVector(500.0f));
		CaptureBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CaptureBounds->SetGenerateOverlapEvents(false);
		CaptureBounds->SetHiddenInGame(true);
	}

#if WITH_EDITORONLY_DATA
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		Billboard->SetupAttachment(SceneRoot);
		Billboard->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
		Billboard->bIsScreenSizeScaled = true;
		Billboard->ScreenSize = 0.0025f;

		if (UTexture2D* CaptureBoxSprite = LoadCaptureBoxBillboardTexture())
		{
			Billboard->SetSprite(CaptureBoxSprite);
		}
		else
		{
			static ConstructorHelpers::FObjectFinder<UTexture2D> FallbackSprite(TEXT("/Engine/EditorResources/S_Actor"));
			if (FallbackSprite.Succeeded())
			{
				Billboard->SetSprite(FallbackSprite.Object);
			}
		}
	}
#endif

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ACelesLightCaptureBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	MaxLightCount = GetClampedMaxLightCount();
	if (LightInfoRenderTarget)
	{
		UCelesLightBlueprintLibrary::ConfigureLightInfoRenderTarget(LightInfoRenderTarget, MaxLightCount);
	}
}

void ACelesLightCaptureBox::BeginPlay()
{
	Super::BeginPlay();
	RuntimeTickAccumulator = 0.0f;
	UpdateCelesLight();
	SetActorTickEnabled(bUpdateAtRuntime);
}

void ACelesLightCaptureBox::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

#if WITH_EDITOR
	if (World->WorldType == EWorldType::Editor)
	{
		if (!ShouldRunEditorUpdate())
		{
			return;
		}

		EditorTickAccumulator += DeltaSeconds;
		if (EditorTickAccumulator >= FMath::Max(0.01f, EditorUpdateInterval))
		{
			EditorTickAccumulator = 0.0f;
			UpdateCelesLight();
		}
		return;
	}
#endif

	if (bUpdateAtRuntime)
	{
		RuntimeTickAccumulator += DeltaSeconds;
		if (RuntimeTickAccumulator >= FMath::Max(0.01f, RuntimeUpdateInterval))
		{
			RuntimeTickAccumulator = 0.0f;
			UpdateCelesLight();
		}
	}
}

#if WITH_EDITOR
bool ACelesLightCaptureBox::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ACelesLightCaptureBox::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	MaxLightCount = GetClampedMaxLightCount();
	if (LightInfoRenderTarget)
	{
		UCelesLightBlueprintLibrary::ConfigureLightInfoRenderTarget(LightInfoRenderTarget, MaxLightCount);
	}
}
#endif

void ACelesLightCaptureBox::UpdateCelesLight()
{
	MaxLightCount = GetClampedMaxLightCount();

	TArray<FCelesLightSourceData> CapturedLights;
	CollectLightsInBox(CapturedLights);

	LastAvailableLightCount = CapturedLights.Num();
	LastOverflowLights.Reset();
	if (CapturedLights.Num() > MaxLightCount)
	{
		for (int32 Index = MaxLightCount; Index < CapturedLights.Num(); ++Index)
		{
			LastOverflowLights.Add(CapturedLights[Index]);
		}
		CapturedLights.SetNum(MaxLightCount, EAllowShrinking::No);
	}

	LastCapturedLights = CapturedLights;
	LastEncodedLightCount = CapturedLights.Num();
	LastOverflowLightCount = LastOverflowLights.Num();

	if (!LightInfoRenderTarget)
	{
		return;
	}

	UCelesLightBlueprintLibrary::ConfigureLightInfoRenderTarget(LightInfoRenderTarget, MaxLightCount);

	TArray<FLinearColor> Pixels;
	FCelesLightEncoder::EncodeLights(CapturedLights, MaxLightCount, Pixels);
	DrawPixelsToRenderTarget(LightInfoRenderTarget, Pixels);
}

void ACelesLightCaptureBox::CollectLightsInBox(TArray<FCelesLightSourceData>& OutLights) const
{
	OutLights.Reset();

	UWorld* World = GetWorld();
	if (!World || !CaptureBounds)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this || !ShouldCaptureActor(Actor))
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(UCelesLightSourceInterface::StaticClass()))
		{
			FCelesLightSourceData Data;
			ICelesLightSourceInterface::Execute_GetCelesLightData(Actor, Data);
			if (IsLocationInsideCaptureBox(Data.WorldPosition))
			{
				OutLights.Add(Data);
			}
			continue;
		}

		if (bIncludeNativePointLights)
		{
			TArray<UPointLightComponent*> PointLightComponents;
			Actor->GetComponents(PointLightComponents);
			for (const UPointLightComponent* PointLightComponent : PointLightComponents)
			{
				AddNativePointLightData(PointLightComponent, OutLights);
			}
		}
	}

	SortLightData(OutLights);
}

UTextureRenderTarget2D* ACelesLightCaptureBox::GetLightInfoRenderTarget() const
{
	return LightInfoRenderTarget;
}

int32 ACelesLightCaptureBox::GetLastEncodedLightCount() const
{
	return LastEncodedLightCount;
}

const TArray<FCelesLightSourceData>& ACelesLightCaptureBox::GetLastCapturedLights() const
{
	return LastCapturedLights;
}

bool ACelesLightCaptureBox::IsLocationInsideCaptureBox(const FVector& WorldLocation) const
{
	if (!CaptureBounds)
	{
		return false;
	}

	const FVector LocalLocation = CaptureBounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = CaptureBounds->GetUnscaledBoxExtent();
	return FMath::Abs(LocalLocation.X) <= Extent.X
		&& FMath::Abs(LocalLocation.Y) <= Extent.Y
		&& FMath::Abs(LocalLocation.Z) <= Extent.Z;
}

bool ACelesLightCaptureBox::ShouldCaptureActor(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (RequiredLightTags.IsEmpty())
	{
		return true;
	}

	for (const FName& RequiredTag : RequiredLightTags)
	{
		if (!RequiredTag.IsNone() && Actor->ActorHasTag(RequiredTag))
		{
			return true;
		}
	}

	return false;
}

void ACelesLightCaptureBox::AddNativePointLightData(const UPointLightComponent* PointLightComponent, TArray<FCelesLightSourceData>& OutLights) const
{
	if (!PointLightComponent || !IsLocationInsideCaptureBox(PointLightComponent->GetComponentLocation()))
	{
		return;
	}

	FCelesLightSourceData Data;
	Data.WorldPosition = PointLightComponent->GetComponentLocation();
	Data.Radius = FMath::Max(0.0f, PointLightComponent->AttenuationRadius);
	Data.Intensity = FMath::Max(0.0f, PointLightComponent->Intensity);
	Data.Color = FLinearColor(PointLightComponent->LightColor);
	OutLights.Add(Data);
}

void ACelesLightCaptureBox::SortLightData(TArray<FCelesLightSourceData>& InOutLights) const
{
	if (LightSelectionMode == ECelesLightSelectionMode::HighestIntensity)
	{
		InOutLights.Sort([](const FCelesLightSourceData& A, const FCelesLightSourceData& B)
		{
			return A.Intensity > B.Intensity;
		});
		return;
	}

	const FVector BoxCenter = CaptureBounds ? CaptureBounds->GetComponentLocation() : GetActorLocation();
	InOutLights.Sort([BoxCenter](const FCelesLightSourceData& A, const FCelesLightSourceData& B)
	{
		return FVector::DistSquared(A.WorldPosition, BoxCenter) < FVector::DistSquared(B.WorldPosition, BoxCenter);
	});
}

void ACelesLightCaptureBox::DrawPixelsToRenderTarget(UTextureRenderTarget2D* RenderTarget, const TArray<FLinearColor>& Pixels) const
{
	UWorld* World = GetWorld();
	if (!World || !RenderTarget)
	{
		return;
	}

	UCanvas* Canvas = nullptr;
	FVector2D Size = FVector2D::ZeroVector;
	FDrawToRenderTargetContext Context;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, RenderTarget, Canvas, Size, Context);
	if (Canvas)
	{
		FCanvasTileItem Tile(FVector2D::ZeroVector, FVector2D(1.0f, 1.0f), FLinearColor::Black);
		Tile.BlendMode = SE_BLEND_Opaque;

		const int32 ExpectedPixelCount = CelesLight::LightInfoTextureWidth * MaxLightCount;
		for (int32 PixelIndex = 0; PixelIndex < ExpectedPixelCount; ++PixelIndex)
		{
			const int32 X = PixelIndex % CelesLight::LightInfoTextureWidth;
			const int32 Y = PixelIndex / CelesLight::LightInfoTextureWidth;
			Tile.Position = FVector2D(static_cast<float>(X), static_cast<float>(Y));
			Tile.SetColor(Pixels.IsValidIndex(PixelIndex) ? Pixels[PixelIndex] : FLinearColor::Black);
			Canvas->DrawItem(Tile);
		}
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);
}

int32 ACelesLightCaptureBox::GetClampedMaxLightCount() const
{
	return FMath::Clamp(MaxLightCount, 1, CelesLight::MaxLightInfoCount);
}

bool ACelesLightCaptureBox::ShouldRunEditorUpdate() const
{
#if WITH_EDITOR
	return bUpdateInEditor && !IsRunningGame();
#else
	return false;
#endif
}
