#include "CelesLightBlueprintLibrary.h"

#include "CelesLightTypes.h"
#include "Components/CelesLightReceiveComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/OverlapResult.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

namespace
{
	bool ActorMatchesAnyTag(const AActor* Actor, const TArray<FName>& FilterActorTags)
	{
		if (!Actor)
		{
			return false;
		}

		if (FilterActorTags.IsEmpty())
		{
			return true;
		}

		for (const FName& Tag : FilterActorTags)
		{
			if (!Tag.IsNone() && Actor->ActorHasTag(Tag))
			{
				return true;
			}
		}

		return false;
	}
}

void UCelesLightBlueprintLibrary::CelesLightReceiveTrace(
	const UObject* WorldContextObject,
	const FVector Origin,
	const float Radius,
	const TArray<FName>& FilterActorTags,
	TArray<AActor*>& ReceiveActors)
{
	ReceiveActors.Reset();

	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || Radius <= 0.0f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CelesLightReceiveTrace), false);
	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	TSet<TWeakObjectPtr<AActor>> UniqueActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || UniqueActors.Contains(Actor))
		{
			continue;
		}

		if (!Actor->FindComponentByClass<UCelesLightReceiveComponent>())
		{
			continue;
		}

		if (!ActorMatchesAnyTag(Actor, FilterActorTags))
		{
			continue;
		}

		UniqueActors.Add(Actor);
		ReceiveActors.Add(Actor);
	}
}

UTextureRenderTarget2D* UCelesLightBlueprintLibrary::CreateLightInfoRenderTarget(UObject* Outer, const int32 LightInfoCount)
{
	UObject* SafeOuter = Outer ? Outer : GetTransientPackage();
	UTextureRenderTarget2D* RenderTarget = NewObject<UCanvasRenderTarget2D>(SafeOuter);
	ConfigureLightInfoRenderTarget(RenderTarget, LightInfoCount);
	return RenderTarget;
}

bool UCelesLightBlueprintLibrary::ConfigureLightInfoRenderTarget(UTextureRenderTarget2D* RenderTarget, const int32 LightInfoCount)
{
	if (!RenderTarget)
	{
		return false;
	}

	const int32 SafeLightInfoCount = FMath::Clamp(LightInfoCount, 1, CelesLight::MaxLightInfoCount);
	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->AddressX = TA_Clamp;
	RenderTarget->AddressY = TA_Clamp;
	RenderTarget->SRGB = false;

	if (RenderTarget->SizeX != CelesLight::LightInfoTextureWidth || RenderTarget->SizeY != SafeLightInfoCount)
	{
		RenderTarget->InitAutoFormat(CelesLight::LightInfoTextureWidth, SafeLightInfoCount);
	}

	RenderTarget->UpdateResourceImmediate(true);
	return true;
}
