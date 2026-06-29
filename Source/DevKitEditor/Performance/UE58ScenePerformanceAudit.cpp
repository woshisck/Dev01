#include "Performance/UE58ScenePerformanceAudit.h"

#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Level.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

FUE58ScenePerformanceAuditResult FUE58ScenePerformanceAuditBuilder::AuditWorld(UWorld* World, const FString& MapPath)
{
	FUE58ScenePerformanceAuditResult Result;
	Result.MapPath = MapPath;
	Result.bLoaded = World != nullptr;
	if (!World)
	{
		return Result;
	}

	Result.LevelCount = World->GetLevels().Num();
	for (ULevel* Level : World->GetLevels())
	{
		if (!Level)
		{
			continue;
		}

		for (AActor* Actor : Level->Actors)
		{
			if (!Actor)
			{
				continue;
			}

			++Result.ActorCount;

			TArray<UStaticMeshComponent*> StaticMeshComponents;
			Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
			for (UStaticMeshComponent* Component : StaticMeshComponents)
			{
				if (!Component)
				{
					continue;
				}

				++Result.StaticMeshComponentCount;
				if (Component->Mobility == EComponentMobility::Movable)
				{
					++Result.MovableStaticMeshComponentCount;
				}

				if (const UStaticMesh* StaticMesh = Component->GetStaticMesh())
				{
					++Result.StaticMeshComponentWithMeshCount;
					Result.StaticMeshMaterialSlotCount += StaticMesh->GetStaticMaterials().Num();
				}
			}

			TArray<ULightComponent*> LightComponents;
			Actor->GetComponents<ULightComponent>(LightComponents);
			for (ULightComponent* LightComponent : LightComponents)
			{
				if (!LightComponent)
				{
					continue;
				}

				++Result.LightComponentCount;
				if (LightComponent->Mobility == EComponentMobility::Movable)
				{
					++Result.MovableLightCount;
				}
				if (LightComponent->CastShadows)
				{
					++Result.ShadowCastingLightCount;
				}
			}

			if (Actor->IsA<ADirectionalLight>())
			{
				++Result.DirectionalLightCount;
			}
			else if (Actor->IsA<APointLight>())
			{
				++Result.PointLightCount;
			}
			else if (Actor->IsA<ASpotLight>())
			{
				++Result.SpotLightCount;
			}
			else if (Actor->IsA<ARectLight>())
			{
				++Result.RectLightCount;
			}
		}
	}

	return Result;
}

TArray<FString> FUE58ScenePerformanceAuditBuilder::BuildMarkdownReport(const FUE58ScenePerformanceAuditResult& Result)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("# UE5.8 Scene Performance Audit"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Map: `%s`"), Result.MapPath.IsEmpty() ? TEXT("(not set)") : *Result.MapPath));
	Lines.Add(FString::Printf(TEXT("- Loaded: %s"), Result.bLoaded ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Levels: %d"), Result.LevelCount));
	Lines.Add(FString::Printf(TEXT("- Actors: %d"), Result.ActorCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Static Meshes"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- StaticMesh components: %d"), Result.StaticMeshComponentCount));
	Lines.Add(FString::Printf(TEXT("- StaticMesh components with mesh: %d"), Result.StaticMeshComponentWithMeshCount));
	Lines.Add(FString::Printf(TEXT("- StaticMesh material slot upper-bound: %d"), Result.StaticMeshMaterialSlotCount));
	Lines.Add(FString::Printf(TEXT("- Movable StaticMesh components: %d"), Result.MovableStaticMeshComponentCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Lights"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Light components: %d"), Result.LightComponentCount));
	Lines.Add(FString::Printf(TEXT("- Directional lights: %d"), Result.DirectionalLightCount));
	Lines.Add(FString::Printf(TEXT("- Point lights: %d"), Result.PointLightCount));
	Lines.Add(FString::Printf(TEXT("- Spot lights: %d"), Result.SpotLightCount));
	Lines.Add(FString::Printf(TEXT("- Rect lights: %d"), Result.RectLightCount));
	Lines.Add(FString::Printf(TEXT("- Movable lights: %d"), Result.MovableLightCount));
	Lines.Add(FString::Printf(TEXT("- Shadow-casting lights: %d"), Result.ShadowCastingLightCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Interpretation"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Material slot count is a draw-call pressure proxy before instancing, HLOD, and mesh draw command merging."));
	Lines.Add(TEXT("- Movable StaticMesh components should not enter offline geometry merge without explicit gameplay review."));
	Lines.Add(TEXT("- Movable and shadow-casting lights are the first lighting budget checks for handheld Lumen Lite tests."));
	return Lines;
}
