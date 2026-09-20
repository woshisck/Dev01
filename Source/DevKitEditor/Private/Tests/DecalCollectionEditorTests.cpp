#if WITH_DEV_AUTOMATION_TESTS

#include "Components/DecalComponent.h"
#include "Editor/TransBuffer.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Surface/DevKitDecalCollectionActor.h"
#include "Tools/DecalCollection/DevKitDecalCollectionEdMode.h"
#include "Tools/DecalCollection/DevKitDecalSelectionGeometry.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitDecalProjectionBoxTest,
	"DevKitEditor.DecalCollection.ProjectorBoxSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalProjectionBoxTest::RunTest(const FString& Parameters)
{
	double Distance = 0.0;
	const FVector Extent(10.0, 20.0, 30.0);
	using DevKit::DecalSelection::IntersectProjectionBox;
	TestTrue(TEXT("Ray hits the projector box"), IntersectProjectionBox(FTransform::Identity, Extent, FVector(-100, 0, 0), FVector(1, 0, 0), Distance));
	TestEqual(TEXT("DecalSize is a half extent, not half of a half extent"), Distance, 90.0);
	TestFalse(TEXT("Parallel ray outside the projected rectangle is rejected"), IntersectProjectionBox(FTransform::Identity, Extent, FVector(-100, 21, 0), FVector(1, 0, 0), Distance));
	TestFalse(TEXT("Projector behind the ray origin is rejected"), IntersectProjectionBox(FTransform::Identity, Extent, FVector(100, 0, 0), FVector(1, 0, 0), Distance));
	TestTrue(TEXT("Camera inside the volume remains selectable"), IntersectProjectionBox(FTransform::Identity, Extent, FVector::ZeroVector, FVector(1, 0, 0), Distance));
	TestEqual(TEXT("Inside-volume entry distance is zero"), Distance, 0.0);
	const FTransform Rotated(FRotator(0, 90, 0), FVector(400, -200, 30), FVector(2, 3, 0.5));
	TestTrue(TEXT("Rotated, translated and non-uniformly scaled projector is selectable"),
		IntersectProjectionBox(Rotated, Extent, Rotated.TransformPosition(FVector(-50, 0, 0)), Rotated.TransformVector(FVector(1, 0, 0)), Distance));
	TestTrue(TEXT("Intersection distance stays in world units"), FMath::IsNearlyEqual(Distance, 80.0, 0.001));
	const FTransform Mirrored(FRotator(0, 35, 0), FVector(10, 20, 30), FVector(-2, 3, 1));
	TestTrue(TEXT("Mirrored projector also uses its full world-space volume"),
		IntersectProjectionBox(Mirrored, Extent, Mirrored.TransformPosition(FVector(-50, 0, 0)), Mirrored.TransformVector(FVector(1, 0, 0)), Distance));
	TestTrue(TEXT("Mirrored intersection distance is correct"), FMath::IsNearlyEqual(Distance, 80.0, 0.001));
	TestFalse(TEXT("Degenerate scale is rejected"), IntersectProjectionBox(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0, 1, 1)), Extent, FVector(-100, 0, 0), FVector(1, 0, 0), Distance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitDecalDeleteUndoIsolationTest,
	"DevKitEditor.DecalCollection.DeleteUndoRecordIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalDeleteUndoIsolationTest::RunTest(const FString& Parameters)
{
	if (GUndo || GIsTransacting)
	{
		AddError(TEXT("Run this test outside an active editor transaction; no user transaction was changed."));
		return false;
	}
	TStrongObjectPtr<UDevKitDecalAsset> Asset(NewObject<UDevKitDecalAsset>());
	Asset->Backend = EDevKitDecalBackend::DeferredProjection;
	Asset->Material = UMaterial::GetDefaultMaterial(MD_DeferredDecal);
	TStrongObjectPtr<UDevKitDecalCollectionComponent> Records(NewObject<UDevKitDecalCollectionComponent>());
	const FGuid First = Records->AddRecord(Asset.Get(), FTransform(FVector(1, 2, 3)));
	const FGuid Target = Records->AddRecord(Asset.Get(), FTransform(FVector(4, 5, 6)));
	const FGuid Last = Records->AddRecord(Asset.Get(), FTransform(FVector(7, 8, 9)));
	TestTrue(TEXT("All isolated test records are valid"), First.IsValid() && Target.IsValid() && Last.IsValid());
	// This private buffer never replaces or clears GEditor->Trans / the user's undo history.
	TStrongObjectPtr<UTransBuffer> Buffer(NewObject<UTransBuffer>());
	Buffer->Initialize(1024 * 1024);
	Buffer->Begin(TEXT("DevKitDecalTest"), FText::FromString(TEXT("Delete one test record")));
	TestTrue(TEXT("Delete removes the target record"), Records->RemoveRecord(Target));
	Buffer->End();
	TestEqual(TEXT("Other two records remain"), Records->Records.Num(), 2);
	TestTrue(TEXT("Undo succeeds in the isolated transaction buffer"), Buffer->Undo());
	TestEqual(TEXT("Undo restores all three records"), Records->Records.Num(), 3);
	TestNotNull(TEXT("Undo restores the same stable GUID"), Records->FindRecord(Target));
	TestTrue(TEXT("Redo succeeds"), Buffer->Redo());
	TestNull(TEXT("Redo removes only the target again"), Records->FindRecord(Target));
	const FDevKitDecalPlacementRecord* FirstRecord = Records->FindRecord(First);
	const FDevKitDecalPlacementRecord* LastRecord = Records->FindRecord(Last);
	TestTrue(TEXT("First record and transform remain untouched"), FirstRecord && FirstRecord->Transform.GetLocation().Equals(FVector(1, 2, 3)));
	TestTrue(TEXT("Last record and transform remain untouched"), LastRecord && LastRecord->Transform.GetLocation().Equals(FVector(7, 8, 9)));
	TestEqual(TEXT("Shared asset definition is not removed"), Records->PaletteAssets.Num(), 1);
	TestFalse(TEXT("Deleting the same GUID twice is a no-op"), Records->RemoveRecord(Target));
	Buffer->Reset(FText::FromString(TEXT("Test complete")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitDecalSelectionRebuildTest,
	"DevKitEditor.DecalCollection.GuidSelectionSurvivesRebuild", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitDecalSelectionRebuildTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, false));
	if (!TestNotNull(TEXT("Isolated preview world exists"), World.Get())) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transient;
	ADevKitDecalCollectionActor* Actor = World->SpawnActor<ADevKitDecalCollectionActor>(SpawnParameters);
	if (!TestNotNull(TEXT("Isolated collection spawned"), Actor)) return false;
	TStrongObjectPtr<UDevKitDecalAsset> Asset(NewObject<UDevKitDecalAsset>());
	Asset->Backend = EDevKitDecalBackend::DeferredProjection;
	Asset->Material = UMaterial::GetDefaultMaterial(MD_DeferredDecal);
	const FGuid Target = Actor->Collection->AddRecord(Asset.Get(), FTransform(FVector(100, 200, 30)));
	Actor->RebuildDerivedRendering();
	if (!TestEqual(TEXT("One deferred proxy was created"), Actor->DerivedDeferredComponents.Num(), 1)) return false;
	UDecalComponent* OldProxy = Actor->DerivedDeferredComponents[0];
	TStrongObjectPtr<UDevKitDecalCollectionEdMode> Mode(NewObject<UDevKitDecalCollectionEdMode>());
	// Supply an isolated session without Enter/SelectRecord: do not touch the user's
	// active mode, actor selection, viewport camera, or unsaved level.
	Mode->SessionCollection = Actor;
	Mode->SelectedRecordGuid = Target;
	FDevKitDecalPlacementRecord Selected;
	TestTrue(TEXT("GUID resolves before rebuild"), Mode->GetSelectedInstanceDetails(Selected));
	Actor->RebuildDerivedRendering();
	TestTrue(TEXT("Rebuild replaces the disposable proxy"), Actor->DerivedDeferredComponents.Num() == 1 && Actor->DerivedDeferredComponents[0] != OldProxy);
	TestTrue(TEXT("Selection still resolves after proxy replacement"), Mode->GetSelectedInstanceDetails(Selected));
	TestEqual(TEXT("Selection stays on the exact authored record"), Selected.InstanceGuid, Target);
	FGuid ComponentGuid;
	TestTrue(TEXT("New Details component maps back to authored GUID"), Actor->FindRecordForDerivedDeferred(Actor->DerivedDeferredComponents[0], ComponentGuid));
	TestEqual(TEXT("Details component and stable selection agree"), ComponentGuid, Target);
	TestTrue(TEXT("Selection transform remains unchanged"), Selected.Transform.GetLocation().Equals(FVector(100, 200, 30)));
	Mode->SessionCollection.Reset();
	return true;
}

#endif
