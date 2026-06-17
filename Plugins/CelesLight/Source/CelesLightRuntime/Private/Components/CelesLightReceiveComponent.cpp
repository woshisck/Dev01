#include "Components/CelesLightReceiveComponent.h"

#include "CanvasItem.h"
#include "CelesLightBlueprintLibrary.h"
#include "CelesLightEncoder.h"
#include "CelesLightSourceInterface.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetRenderingLibrary.h"

UCelesLightReceiveComponent::UCelesLightReceiveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCelesLightReceiveComponent::OnRegister()
{
	Super::OnRegister();

	AActor* Owner = GetOwner();
	if (bAutoAddReceiveActorTag && Owner && !ReceiveActorTag.IsNone() && !Owner->ActorHasTag(ReceiveActorTag))
	{
		Owner->Tags.Add(ReceiveActorTag);
	}
}

void UCelesLightReceiveComponent::BeginPlay()
{
	Super::BeginPlay();
	InitCelesLightReceive(false);
}

void UCelesLightReceiveComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bRealtimeSampling)
	{
		UpdateLightInfo(false);
	}
}

void UCelesLightReceiveComponent::InitCelesLightReceive(const bool bInEditor)
{
	CreateRT(bInEditor);
	ApplyRTToMaterial(bInEditor);
	RefreshLightSources(bInEditor);
	UpdateLightInfo(bInEditor);

	SetComponentTickEnabled(bRealtimeSampling && !bInEditor);
	if (bRealtimeSampling && RealtimeUpdateInterval > 0.0f)
	{
		SetComponentTickInterval(RealtimeUpdateInterval);
	}
}

void UCelesLightReceiveComponent::UpdateLightInfo(const bool bInEditor)
{
	LightInfoCount = FMath::Clamp(LightInfoCount, 1, CelesLight::MaxLightInfoCount);

	UTextureRenderTarget2D* RenderTarget = CreateRT(bInEditor);
	if (!RenderTarget)
	{
		return;
	}

	TArray<FCelesLightSourceData> Sources;
	CollectLightData(bInEditor, Sources);

	TArray<FLinearColor> Pixels;
	FCelesLightEncoder::EncodeLights(Sources, LightInfoCount, Pixels);
	DrawPixelsToRenderTarget(RenderTarget, Pixels);
	ApplyRTToMaterial(bInEditor);
}

void UCelesLightReceiveComponent::ApplyRTToMaterial(const bool bInEditor)
{
	// RT 现在由采集盒体统一提供；插件不再主动创建动态材质或改写 Mesh 材质。
}

UTextureRenderTarget2D* UCelesLightReceiveComponent::CreateRT(const bool bInEditor)
{
	LightInfoCount = FMath::Clamp(LightInfoCount, 1, CelesLight::MaxLightInfoCount);

	TObjectPtr<UTextureRenderTarget2D>& RenderTarget = bInEditor ? RT_LightInfo_InEditor : RT_LightInfo;
	if (!RenderTarget || RenderTarget->SizeX != CelesLight::LightInfoTextureWidth || RenderTarget->SizeY != LightInfoCount)
	{
		RenderTarget = UCelesLightBlueprintLibrary::CreateLightInfoRenderTarget(this, LightInfoCount);
	}

	return RenderTarget;
}

void UCelesLightReceiveComponent::ClearNotvalidLight(const bool bInEditor)
{
	UTextureRenderTarget2D* RenderTarget = GetActiveRenderTarget(bInEditor);
	if (RenderTarget && GetWorld())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), RenderTarget, FLinearColor::Black);
	}
}

void UCelesLightReceiveComponent::RegistLight(UObject* RegistLightInterface, const bool bInEditor)
{
	if (!RegistLightInterface || !RegistLightInterface->GetClass()->ImplementsInterface(UCelesLightSourceInterface::StaticClass()))
	{
		return;
	}

	TArray<TObjectPtr<UObject>>& LightList = GetLightList(bInEditor);
	LightList.AddUnique(RegistLightInterface);
}

void UCelesLightReceiveComponent::DeregistLight(UObject* RegistLightInterface, const bool bInEditor)
{
	if (!RegistLightInterface)
	{
		return;
	}

	GetLightList(bInEditor).Remove(RegistLightInterface);
}

void UCelesLightReceiveComponent::ClearRegisteredLights(const bool bInEditor)
{
	GetLightList(bInEditor).Reset();
}

void UCelesLightReceiveComponent::RefreshLightSources(const bool bInEditor)
{
	ClearRegisteredLights(bInEditor);

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}

	const FVector ReceiverLocation = Owner->GetActorLocation();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->GetClass()->ImplementsInterface(UCelesLightSourceInterface::StaticClass()))
		{
			FCelesLightSourceData Data;
			ICelesLightSourceInterface::Execute_GetCelesLightData(Actor, Data);
			if (Data.Radius > 0.0f && FVector::DistSquared(Data.WorldPosition, ReceiverLocation) <= FMath::Square(Data.Radius))
			{
				RegistLight(Actor, bInEditor);
			}
		}
	}
}

UTextureRenderTarget2D* UCelesLightReceiveComponent::GetActiveRenderTarget(const bool bInEditor) const
{
	return bInEditor ? RT_LightInfo_InEditor : RT_LightInfo;
}

TArray<TObjectPtr<UObject>>& UCelesLightReceiveComponent::GetLightList(const bool bInEditor)
{
	return bInEditor ? RegistLightList_InEditor : RegistLightList;
}

const TArray<TObjectPtr<UObject>>& UCelesLightReceiveComponent::GetLightList(const bool bInEditor) const
{
	return bInEditor ? RegistLightList_InEditor : RegistLightList;
}

void UCelesLightReceiveComponent::DrawPixelsToRenderTarget(UTextureRenderTarget2D* RenderTarget, const TArray<FLinearColor>& Pixels) const
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

		const int32 ExpectedPixelCount = CelesLight::LightInfoTextureWidth * FMath::Max(1, LightInfoCount);
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

void UCelesLightReceiveComponent::CollectLightData(const bool bInEditor, TArray<FCelesLightSourceData>& OutSources) const
{
	OutSources.Reset();
	const TArray<TObjectPtr<UObject>>& LightList = GetLightList(bInEditor);

	for (UObject* LightSource : LightList)
	{
		if (!LightSource || !LightSource->GetClass()->ImplementsInterface(UCelesLightSourceInterface::StaticClass()))
		{
			continue;
		}

		FCelesLightSourceData Data;
		ICelesLightSourceInterface::Execute_GetCelesLightData(LightSource, Data);
		OutSources.Add(Data);

		if (OutSources.Num() >= LightInfoCount)
		{
			break;
		}
	}
}
