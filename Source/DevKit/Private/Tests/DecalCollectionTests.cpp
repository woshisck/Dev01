#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Surface/DevKitDecalCollectionActor.h"
#include "RVT/DevKitRVTSurfaceInstanceActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitDecalCollectionRecordContractTest,
	"DevKit.Surface.DecalCollection.RecordContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalCollectionRecordContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UDevKitDecalAsset* Asset = NewObject<UDevKitDecalAsset>(GetTransientPackage());
	Asset->LegacyRVTAsset = NewObject<UDevKitRVTSurfaceAsset>(Asset);
	TestTrue(TEXT("New unified asset receives a stable definition GUID"), Asset->DefinitionGuid.IsValid());

	UDevKitDecalCollectionComponent* Records = NewObject<UDevKitDecalCollectionComponent>(GetTransientPackage());
	const FGuid InstanceGuid = Records->AddRecord(Asset, FTransform(FVector(100.0, 200.0, 3.0)));
	TestTrue(TEXT("Valid unified asset creates a placement record"), InstanceGuid.IsValid());
	TestEqual(TEXT("Collection has one record"), Records->GetRecordCount(), 1);

	const FTransform NewTransform(FRotator(0.0, 45.0, 0.0), FVector(400.0, 500.0, 7.0), FVector(2.0));
	TestTrue(TEXT("Placement transform updates by stable GUID"), Records->UpdateRecordTransform(InstanceGuid, NewTransform));
	FDevKitDecalPlacementRecord* Record = Records->FindRecord(InstanceGuid);
	TestNotNull(TEXT("Updated record remains addressable"), Record);
	if (Record)
	{
		TestTrue(TEXT("Updated transform is preserved"), Record->Transform.Equals(NewTransform));
		Record->SourceActorPath = TEXT("/Game/TestMap.TestMap:PersistentLevel.LegacyDecalActor");
		Record->SourceComponentName = TEXT("LegacyDecalISM");
		Record->bSourceHiddenForAdoption = true;
		TestTrue(TEXT("Adopted records retain their source provenance"), !Record->SourceActorPath.IsEmpty() && !Record->SourceComponentName.IsNone() && Record->bSourceHiddenForAdoption);
	}

	TestTrue(TEXT("Placement record removes by stable GUID"), Records->RemoveRecord(InstanceGuid));
	TestEqual(TEXT("Collection is empty after removal"), Records->GetRecordCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitDecalCollectionBackendContractTest,
	"DevKit.Surface.DecalCollection.BackendContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalCollectionBackendContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UDevKitDecalAsset* Deferred = NewObject<UDevKitDecalAsset>(GetTransientPackage());
	Deferred->Backend = EDevKitDecalBackend::DeferredProjection;
	Deferred->Material = NewObject<UMaterial>(Deferred);
	TestTrue(TEXT("Deferred projection is valid with a decal material and no mesh"), Deferred->IsValidDefinition());
	TestEqual(TEXT("Deferred default projection size stays artist-friendly"), Deferred->DefaultDecalSize, FVector(64.f, 64.f, 64.f));

	UDevKitDecalAsset* MeshDecal = NewObject<UDevKitDecalAsset>(GetTransientPackage());
	MeshDecal->Backend = EDevKitDecalBackend::MeshDecal;
	MeshDecal->Material = Deferred->Material;
	TestFalse(TEXT("Mesh decal rejects a material-only definition"), MeshDecal->IsValidDefinition());

	UDevKitDecalCollectionComponent* Records = NewObject<UDevKitDecalCollectionComponent>(GetTransientPackage());
	const FGuid DeferredGuid = Records->AddRecord(Deferred, FTransform(FVector(2.f, 3.f, 4.f)));
	const FDevKitDecalPlacementRecord* DeferredRecord = Records->FindRecord(DeferredGuid);
	TestNotNull(TEXT("Deferred placement creates a record"), DeferredRecord);
	if (DeferredRecord)
	{
		TestEqual(TEXT("Deferred placement inherits its default projector size"), DeferredRecord->DecalSize, Deferred->DefaultDecalSize);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitDecalCollectionQualityPolicyContractTest,
	"DevKit.Surface.DecalCollection.QualityPolicyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalCollectionQualityPolicyContractTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("RVT plane decals never retain main-pass source geometry"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::PlaneDecal,
			EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible,
			false,
			false));

	TestTrue(TEXT("Quality-scaled visible meshes retain source geometry at PC High/Epic"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			false,
			false));
	TestFalse(TEXT("Quality-scaled visible meshes project at PC Mid/Low"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			true,
			false));
	TestFalse(TEXT("Handheld force-projection overrides PC quality preference"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			false,
			true));
	TestTrue(TEXT("Gameplay collision keeps a visible source mesh on every quality tier"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			true,
			true,
			true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitDecalCollectionActorDefaultsTest,
	"DevKit.Surface.DecalCollection.ActorDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalCollectionActorDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ADevKitDecalCollectionActor* DefaultActor = GetDefault<ADevKitDecalCollectionActor>();
	TestNotNull(TEXT("Collection actor CDO exists"), DefaultActor);
	if (!DefaultActor)
	{
		return false;
	}
	TestNotNull(TEXT("Collection actor owns a records component"), DefaultActor->Collection.Get());
	TestTrue(TEXT("Collection actor has a stable collection GUID"), DefaultActor->CollectionGuid.IsValid());
	TestFalse(TEXT("Collection edit session is closed by default"), DefaultActor->bEditSessionActive);
	return true;
}

#endif
