#pragma once

#include "CoreMinimal.h"

struct FUE58ScenePerformanceAuditResult
{
	FString MapPath;
	bool bLoaded = false;
	int32 LevelCount = 0;
	int32 ActorCount = 0;
	int32 StaticMeshComponentCount = 0;
	int32 StaticMeshComponentWithMeshCount = 0;
	int32 StaticMeshMaterialSlotCount = 0;
	int32 MovableStaticMeshComponentCount = 0;
	int32 LightComponentCount = 0;
	int32 DirectionalLightCount = 0;
	int32 PointLightCount = 0;
	int32 SpotLightCount = 0;
	int32 RectLightCount = 0;
	int32 MovableLightCount = 0;
	int32 ShadowCastingLightCount = 0;
};

class FUE58ScenePerformanceAuditBuilder
{
public:
	static FUE58ScenePerformanceAuditResult AuditWorld(class UWorld* World, const FString& MapPath);
	static TArray<FString> BuildMarkdownReport(const FUE58ScenePerformanceAuditResult& Result);
};
