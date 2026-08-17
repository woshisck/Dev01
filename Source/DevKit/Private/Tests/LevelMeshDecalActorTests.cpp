#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "RVT/DevKitLevelMeshDecalActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitLevelMeshDecalActorDefaultsTest,
	"DevKit.RVT.LevelMeshDecalActor.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelMeshDecalActorDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const ADevKitLevelMeshDecalActor* ActorDefault = GetDefault<ADevKitLevelMeshDecalActor>();
	TestNotNull(TEXT("Actor class default exists"), ActorDefault);
	if (!ActorDefault)
	{
		return false;
	}

	TestNull(TEXT("Base class deliberately creates no native scene root"), ActorDefault->GetRootComponent());
	TestEqual(TEXT("Default proxy component name"), ActorDefault->BoundsProxyName, FName(TEXT("BoundsProxy")));
	TestEqual(
		TEXT("Default proxy component tag"),
		ActorDefault->BoundsProxyTag,
		FName(TEXT("LevelMeshDecalBounds")));
	TestEqual(
		TEXT("Default shared bounds extent"),
		ActorDefault->DesiredBoundsExtent,
		FVector(6500.0, 6500.0, 1500.0));
	TestFalse(TEXT("BoundsScale fallback is opt-in"), ActorDefault->bUseBoundsScaleFallback);
	TestFalse(TEXT("Actor is excluded from automatic HLOD generation"), ActorDefault->bEnableAutoLODGeneration);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitLevelMeshDecalActorCullingPolicyTest,
	"DevKit.RVT.LevelMeshDecalActor.CullingPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelMeshDecalActorCullingPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transient;
	AddExpectedError(
		TEXT("has no UBoxComponent named"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	ADevKitLevelMeshDecalActor* Actor =
		World->SpawnActor<ADevKitLevelMeshDecalActor>(SpawnParameters);
	TestNotNull(TEXT("Transient level mesh decal actor spawned"), Actor);
	if (!Actor)
	{
		return false;
	}

	USceneComponent* BlueprintRoot = NewObject<USceneComponent>(Actor, TEXT("DefaultSceneRoot"), RF_Transient);
	Actor->AddInstanceComponent(BlueprintRoot);
	Actor->SetRootComponent(BlueprintRoot);
	BlueprintRoot->RegisterComponent();

	UBoxComponent* BoundsProxy =
		NewObject<UBoxComponent>(Actor, TEXT("BoundsProxy_GEN_VARIABLE"), RF_Transient);
	Actor->AddInstanceComponent(BoundsProxy);
	BoundsProxy->SetupAttachment(BlueprintRoot);
	BoundsProxy->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoundsProxy->SetVisibility(true);
	BoundsProxy->RegisterComponent();

	USceneComponent* IntermediateOuter =
		NewObject<USceneComponent>(Actor, TEXT("IntermediateOuter"), RF_Transient);
	Actor->AddInstanceComponent(IntermediateOuter);
	IntermediateOuter->SetupAttachment(BoundsProxy);
	IntermediateOuter->RegisterComponent();

	USceneComponent* IntermediateInner =
		NewObject<USceneComponent>(Actor, TEXT("IntermediateInner"), RF_Transient);
	Actor->AddInstanceComponent(IntermediateInner);
	IntermediateInner->SetupAttachment(IntermediateOuter);
	IntermediateInner->RegisterComponent();

	UInstancedStaticMeshComponent* ISM =
		NewObject<UInstancedStaticMeshComponent>(Actor, TEXT("TestISM"), RF_Transient);
	Actor->AddInstanceComponent(ISM);
	ISM->SetupAttachment(IntermediateInner);
	ISM->RegisterComponent();
	ISM->SetCullDistances(100, 200);
	ISM->InstanceMinDrawDistance = 50;
	ISM->MinDrawDistance = 25.0f;
	ISM->LDMaxDrawDistance = 500.0f;
	ISM->bNeverDistanceCull = false;
	ISM->bAllowCullDistanceVolume = true;
	ISM->bEnableAutoLODGeneration = true;
	ISM->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;
	const int32 InstanceIndex = ISM->AddInstance(FTransform(FVector(123.0, 456.0, 7.0)));

	UHierarchicalInstancedStaticMeshComponent* HISM =
		NewObject<UHierarchicalInstancedStaticMeshComponent>(Actor, TEXT("TestHISM"), RF_Transient);
	Actor->AddInstanceComponent(HISM);
	HISM->SetupAttachment(BoundsProxy);
	HISM->RegisterComponent();
	HISM->SetCullDistances(300, 400);
	HISM->MinDrawDistance = 35.0f;
	HISM->LDMaxDrawDistance = 600.0f;
	HISM->bNeverDistanceCull = false;
	HISM->bAllowCullDistanceVolume = true;
	HISM->bEnableAutoLODGeneration = true;

	FTransform InstanceTransformBefore;
	ISM->GetInstanceTransform(InstanceIndex, InstanceTransformBefore, false);
	const FTransform ComponentTransformBefore = ISM->GetComponentTransform();
	Actor->ApplyCullingPolicy();

	TestTrue(TEXT("Blueprint scene root is preserved"), Actor->GetRootComponent() == BlueprintRoot);
	TestTrue(TEXT("BoundsProxy remains attached to the Blueprint root"), BoundsProxy->GetAttachParent() == BlueprintRoot);
	TestEqual(
		TEXT("BoundsProxy receives the desired extent"),
		BoundsProxy->GetUnscaledBoxExtent(),
		FVector(6500.0, 6500.0, 1500.0));
	TestFalse(TEXT("BoundsProxy is not visible"), BoundsProxy->IsVisible());
	TestTrue(TEXT("BoundsProxy is hidden in game"), BoundsProxy->bHiddenInGame);
	TestFalse(TEXT("BoundsProxy has no collision"), BoundsProxy->IsCollisionEnabled());
	TestFalse(TEXT("BoundsProxy does not affect navigation"), BoundsProxy->CanEverAffectNavigation());

	int32 StartCullDistance = INDEX_NONE;
	int32 EndCullDistance = INDEX_NONE;
	ISM->GetCullDistances(StartCullDistance, EndCullDistance);
	TestEqual(TEXT("ISM start cull distance is disabled"), StartCullDistance, 0);
	TestEqual(TEXT("ISM end cull distance is disabled"), EndCullDistance, 0);
	TestEqual(TEXT("ISM minimum draw distance is disabled"), ISM->InstanceMinDrawDistance, 0);
	TestEqual(TEXT("ISM primitive minimum draw distance is disabled"), ISM->MinDrawDistance, 0.0f);
	TestEqual(TEXT("ISM desired max draw distance is disabled"), ISM->LDMaxDrawDistance, 0.0f);
	TestTrue(TEXT("ISM ignores distance culling"), ISM->bNeverDistanceCull);
	TestFalse(TEXT("ISM ignores Cull Distance Volumes"), ISM->bAllowCullDistanceVolume);
	TestFalse(TEXT("ISM is excluded from automatic HLOD generation"), ISM->bEnableAutoLODGeneration);
	TestEqual(TEXT("ISM cached max draw distance is cleared"), ISM->CachedMaxDrawDistance, 0.0f);
	TestTrue(TEXT("ISM uses its attach-parent bound"), ISM->bUseAttachParentBound);
	TestTrue(TEXT("Outer intermediate forwards the proxy bound"), IntermediateOuter->bUseAttachParentBound);
	TestTrue(TEXT("Inner intermediate forwards the proxy bound"), IntermediateInner->bUseAttachParentBound);
	TestTrue(TEXT("Existing component hierarchy is preserved"), ISM->GetAttachParent() == IntermediateInner);
	const FBox ProxyBounds = BoundsProxy->Bounds.GetBox();
	TestTrue(
		TEXT("Outer intermediate receives the current proxy bounds in one application"),
		ProxyBounds.Equals(IntermediateOuter->Bounds.GetBox(), KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("Inner intermediate receives the current proxy bounds in one application"),
		ProxyBounds.Equals(IntermediateInner->Bounds.GetBox(), KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("ISM receives the current proxy bounds in one application"),
		ProxyBounds.Equals(ISM->Bounds.GetBox(), KINDA_SMALL_NUMBER));

	HISM->GetCullDistances(StartCullDistance, EndCullDistance);
	TestEqual(TEXT("HISM start cull distance is disabled"), StartCullDistance, 0);
	TestEqual(TEXT("HISM end cull distance is disabled"), EndCullDistance, 0);
	TestEqual(TEXT("HISM primitive minimum draw distance is disabled"), HISM->MinDrawDistance, 0.0f);
	TestEqual(TEXT("HISM desired max draw distance is disabled"), HISM->LDMaxDrawDistance, 0.0f);
	TestTrue(TEXT("HISM ignores distance culling"), HISM->bNeverDistanceCull);
	TestFalse(TEXT("HISM ignores Cull Distance Volumes"), HISM->bAllowCullDistanceVolume);
	TestFalse(TEXT("HISM is excluded from automatic HLOD generation"), HISM->bEnableAutoLODGeneration);
	TestTrue(TEXT("HISM uses the Blueprint BoundsProxy"), HISM->bUseAttachParentBound);

	FTransform InstanceTransformAfter;
	ISM->GetInstanceTransform(InstanceIndex, InstanceTransformAfter, false);
	TestTrue(
		TEXT("Applying the policy preserves the ISM component transform"),
		ComponentTransformBefore.Equals(ISM->GetComponentTransform()));
	TestTrue(
		TEXT("Applying the policy preserves per-instance transforms"),
		InstanceTransformBefore.Equals(InstanceTransformAfter));
	TestEqual(
		TEXT("Culling policy does not change RVT/main-pass behavior"),
		ISM->VirtualTextureRenderPassType,
		ERuntimeVirtualTextureMainPassType::Always);

	Actor->BoundsProxyName = TEXT("DifferentProxyName");
	BoundsProxy->ComponentTags.AddUnique(Actor->BoundsProxyTag);
	BoundsProxy->SetVisibility(true);
	Actor->ApplyCullingPolicy();
	TestFalse(TEXT("Component tag can identify a differently named proxy"), BoundsProxy->IsVisible());

	Actor->Destroy();
	return true;
}

#endif
