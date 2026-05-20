#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Component/YogCameraOcclusionFadeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYogCameraOcclusionFadeDefaultsTest,
	"DevKit.Camera.OcclusionFade.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogCameraOcclusionFadeDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UYogCameraOcclusionFadeComponent* Component = NewObject<UYogCameraOcclusionFadeComponent>();
	TestTrue(TEXT("Occlusion fade starts enabled"), Component->bEnableOcclusionFade);
	TestEqual(TEXT("Trace interval defaults to 0.05s"), Component->TraceInterval, 0.05f);
	TestEqual(TEXT("Trace radius defaults to 28"), Component->TraceRadius, 28.0f);
	TestEqual(TEXT("Min visible alpha defaults to 0.15"), Component->MinVisibleAlpha, 0.15f);
	TestEqual(TEXT("Fade parameter name"), Component->FadeScalarParameterName, FName(TEXT("CameraOcclusionAlpha")));
	TestTrue(TEXT("Tagged occluders are required by default"), Component->bOnlyFadeTaggedOccluders);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYogCameraOcclusionFadeTagFilterTest,
	"DevKit.Camera.OcclusionFade.TagFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogCameraOcclusionFadeTagFilterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* OwnerActor = World->SpawnActor<AActor>();
	AActor* OccluderActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Owner actor spawned"), OwnerActor);
	TestNotNull(TEXT("Occluder actor spawned"), OccluderActor);
	if (!OwnerActor || !OccluderActor)
	{
		return false;
	}

	UYogCameraOcclusionFadeComponent* FadeComponent = NewObject<UYogCameraOcclusionFadeComponent>(OwnerActor);
	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(OccluderActor);
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(MeshComponent);
	StaticMesh->GetStaticMaterials().Add(FStaticMaterial(UMaterial::GetDefaultMaterial(MD_Surface)));
	MeshComponent->SetStaticMesh(StaticMesh);
	OccluderActor->SetRootComponent(MeshComponent);

	TestFalse(TEXT("Component without CameraOccluder tag is not allowed by default"),
		FadeComponent->IsFadeAllowedForComponent(MeshComponent));

	OccluderActor->Tags.Add(FName(TEXT("CameraOccluder")));
	TestTrue(TEXT("Actor CameraOccluder tag allows fading"),
		FadeComponent->IsFadeAllowedForComponent(MeshComponent));

	OccluderActor->Tags.Reset();
	MeshComponent->ComponentTags.Add(FName(TEXT("CameraOccluder")));
	TestTrue(TEXT("Component CameraOccluder tag allows fading"),
		FadeComponent->IsFadeAllowedForComponent(MeshComponent));

	OccluderActor->Destroy();
	OwnerActor->Destroy();
	return true;
}

#endif
