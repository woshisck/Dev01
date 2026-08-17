#if WITH_DEV_AUTOMATION_TESTS

#include "Components/InstancedStaticMeshComponent.h"
#include "Elements/Interfaces/TypedElementWorldInterface.h"
#include "Elements/SMInstance/SMInstanceElementId.h"
#include "Elements/SMInstance/SMInstanceManager.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureOutput.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/AutomationTest.h"
#include "RVT/DevKitRVTSurfaceAsset.h"
#include "RVT/DevKitRVTSurfaceInstanceActor.h"
#include "Tools/RVTMeshDecal/DevKitRVTMeshDecalService.h"
#include "Tools/RVTMeshDecal/SDevKitRVTMeshDecalWidget.h"
#include "VT/RuntimeVirtualTexture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTMeshDecalInfersDefaultFolderTest,
	"DevKitEditor.RVTSurfaceLibrary.InfersDefaultFolder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMeshDecalInfersDefaultFolderTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Final surface assets sit under the sibling BakeInfo/RVTSurfaceLibrary folder"),
		FDevKitRVTMeshDecalService::InferDefaultSurfaceAssetFolderFromWorldPackage(
			TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Art")),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo/RVTSurfaceLibrary"));
	TestEqual(
		TEXT("Legacy foliage compatibility assets remain under the sibling BakeInfo folder"),
		FDevKitRVTMeshDecalService::InferDefaultFoliageTypeFolderFromWorldPackage(
			TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Art")),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo/RVTDecalFoliage"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTMeshDecalBuildsAssetNameTest,
	"DevKitEditor.RVTSurfaceLibrary.BuildsAssetName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMeshDecalBuildsAssetNameTest::RunTest(const FString& Parameters)
{
	const FString SurfaceDecalName = FDevKitRVTMeshDecalService::BuildDefaultSurfaceAssetName(
		TEXT("/Game/Art/EnvironmentAsset/Decal/Mesh/SM_DecalPlane_01.SM_DecalPlane_01"),
		TEXT("/Game/Art/Map/LevelMaterial/MI_Dirt_Crack.MI_Dirt_Crack"),
		EDevKitRVTSurfaceAssetType::PlaneDecal,
		20);
	const FString SurfaceDecalPrefix = TEXT("DA_RVTDecal_MI_Dirt_Crack_P20_H");
	TestTrue(
		TEXT("Final plane-decal asset name has its own prefix and collision-safe hash"),
		SurfaceDecalName.StartsWith(SurfaceDecalPrefix)
			&& SurfaceDecalName.Len() == SurfaceDecalPrefix.Len() + 8);

	const FString SurfaceObjectName = FDevKitRVTMeshDecalService::BuildDefaultSurfaceAssetName(
		TEXT("/Game/Art/EnvironmentAsset/Ground/SM_Metal_Spoke.SM_Metal_Spoke"),
		TEXT("/Game/Art/Map/LevelMaterial/MI_Metal_Spoke.MI_Metal_Spoke"),
		EDevKitRVTSurfaceAssetType::VisibleObject,
		-5);
	const FString SurfaceObjectPrefix = TEXT("DA_RVTObject_SM_Metal_Spoke_MI_Metal_Spoke_PM5_H");
	TestTrue(
		TEXT("Final visible-object asset name includes mesh, material, priority, and hash"),
		SurfaceObjectName.StartsWith(SurfaceObjectPrefix)
			&& SurfaceObjectName.Len() == SurfaceObjectPrefix.Len() + 8);

	const FString DecalName = FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(
		TEXT("/Game/Art/Map/LevelMaterial/MI_Dirt_Crack.MI_Dirt_Crack"),
		20,
		EDevKitRVTSurfaceItemMode::Decal,
		TEXT("/Game/Art/EnvironmentAsset/Decal/Mesh/SM_DecalPlane_01.SM_DecalPlane_01"));
	const FString DecalPrefix = TEXT("FT_RVTDecal_MI_Dirt_Crack_P20_H");
	TestTrue(
		TEXT("Decal name includes material, priority, and an eight-digit source hash"),
		DecalName.StartsWith(DecalPrefix) && DecalName.Len() == DecalPrefix.Len() + 8);
	TestNotEqual(
		TEXT("Same leaf material name in another folder gets a collision-safe asset name"),
		DecalName,
		FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(
			TEXT("/Game/Other/MI_Dirt_Crack.MI_Dirt_Crack"),
			20,
			EDevKitRVTSurfaceItemMode::Decal,
			TEXT("/Game/Art/EnvironmentAsset/Decal/Mesh/SM_DecalPlane_01.SM_DecalPlane_01")));

	const FString VisibleName = FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(
			TEXT("/Game/Art/Map/LevelMaterial/MI_Metal_Spoke.MI_Metal_Spoke"),
			-5,
			EDevKitRVTSurfaceItemMode::VisibleGroundObject,
			TEXT("/Game/Art/EnvironmentAsset/Ground/SM_Metal_Spoke.SM_Metal_Spoke"));
	const FString VisiblePrefix = TEXT("FT_RVTObject_SM_Metal_Spoke_MI_Metal_Spoke_PM5_H");
	TestTrue(
		TEXT("Visible ground object uses a distinct prefix and source hash"),
		VisibleName.StartsWith(VisiblePrefix) && VisibleName.Len() == VisiblePrefix.Len() + 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTMeshDecalBuildsPathsTest,
	"DevKitEditor.RVTSurfaceLibrary.BuildsPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMeshDecalBuildsPathsTest::RunTest(const FString& Parameters)
{
	FDevKitRVTMeshDecalRequest Request;
	Request.FoliageTypeFolder = TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Test_01/BakeInfo/RVTDecalFoliage");
	Request.MeshObjectPath = TEXT("/Game/Art/EnvironmentAsset/Decal/Mesh/SM_DecalPlane_01.SM_DecalPlane_01");
	Request.MaterialObjectPath = TEXT("/Game/Art/Map/LevelMaterial/MI_Dirt.MI_Dirt");
	Request.SurfaceRuntimeVirtualTextureObjectPath =
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Test_01/BakeInfo/RVT_L1_CommonLevel_Test_01_Ground_BaseColorNormalSpecular.RVT_L1_CommonLevel_Test_01_Ground_BaseColorNormalSpecular");
	Request.ItemMode = EDevKitRVTSurfaceItemMode::Decal;
	Request.TranslucencySortPriority = 10;
	Request.MinScale = 0.25f;
	Request.MaxScale = 2.0f;

	FText Error;
	const TOptional<FDevKitRVTMeshDecalPaths> Paths =
		FDevKitRVTMeshDecalService::BuildPaths(Request, Error);
	TestTrue(TEXT("Paths are valid"), Paths.IsSet());
	if (!Paths.IsSet())
	{
		return false;
	}

	const FString ExpectedAssetName = FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(
		Request.MaterialObjectPath,
		Request.TranslucencySortPriority,
		Request.ItemMode,
		Request.MeshObjectPath);
	TestEqual(TEXT("Asset name"), Paths->FoliageTypeName, ExpectedAssetName);
	TestEqual(
		TEXT("Package path"),
		Paths->FoliageTypePackage,
		Request.FoliageTypeFolder / ExpectedAssetName);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTVisibleObjectRequiresHeightTest,
	"DevKitEditor.RVTSurfaceLibrary.VisibleObjectRequiresHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTVisibleObjectRequiresHeightTest::RunTest(const FString& Parameters)
{
	FDevKitRVTMeshDecalRequest Request;
	Request.FoliageTypeFolder = TEXT("/Game/Test/RVTDecalFoliage");
	Request.MeshObjectPath = TEXT("/Game/Test/SM_Test.SM_Test");
	Request.MaterialObjectPath = TEXT("/Game/Test/MI_Test.MI_Test");
	Request.SurfaceRuntimeVirtualTextureObjectPath = TEXT("/Game/Test/RVT_Surface.RVT_Surface");
	Request.ItemMode = EDevKitRVTSurfaceItemMode::VisibleGroundObject;
	Request.bBindWorldHeight = true;

	FText Error;
	TestFalse(
		TEXT("Visible ground object with WorldHeight enabled rejects a missing height RVT"),
		FDevKitRVTMeshDecalService::BuildPaths(Request, Error).IsSet());
	TestFalse(TEXT("Validation provides a useful error"), Error.IsEmpty());

	Request.bBindWorldHeight = false;
	TestTrue(
		TEXT("Visible ground object may opt out of WorldHeight"),
		FDevKitRVTMeshDecalService::BuildPaths(Request, Error).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTMaterialWriterCapabilitiesTest,
	"DevKitEditor.RVTSurfaceLibrary.MaterialWriterCapabilities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMaterialWriterCapabilitiesTest::RunTest(const FString& Parameters)
{
	UMaterial* SurfaceAndHeightWriter = NewObject<UMaterial>(GetTransientPackage());
	UMaterialExpressionRuntimeVirtualTextureOutput* Output =
		NewObject<UMaterialExpressionRuntimeVirtualTextureOutput>(SurfaceAndHeightWriter);
	UMaterialExpressionConstant3Vector* SurfaceValue =
		NewObject<UMaterialExpressionConstant3Vector>(SurfaceAndHeightWriter);
	UMaterialExpressionConstant* HeightValue =
		NewObject<UMaterialExpressionConstant>(SurfaceAndHeightWriter);
	SurfaceAndHeightWriter->GetExpressionCollection().AddExpression(Output);
	SurfaceAndHeightWriter->GetExpressionCollection().AddExpression(SurfaceValue);
	SurfaceAndHeightWriter->GetExpressionCollection().AddExpression(HeightValue);
	Output->BaseColor.Expression = SurfaceValue;

	UMaterialInstanceConstant* WriterInstance = NewObject<UMaterialInstanceConstant>(GetTransientPackage());
	WriterInstance->SetParentEditorOnly(SurfaceAndHeightWriter, false);
	TestTrue(
		TEXT("Material instance inherits Surface RVT output from its parent"),
		FDevKitRVTMeshDecalService::MaterialWritesSurfaceToRuntimeVirtualTexture(WriterInstance));
	TestFalse(
		TEXT("Surface-only writer is not mistaken for a WorldHeight writer"),
		FDevKitRVTMeshDecalService::MaterialWritesWorldHeightToRuntimeVirtualTexture(WriterInstance));

	Output->WorldHeight.Expression = HeightValue;
	TestTrue(
		TEXT("Material instance inherits WorldHeight RVT output from its parent"),
		FDevKitRVTMeshDecalService::MaterialWritesWorldHeightToRuntimeVirtualTexture(WriterInstance));

	UMaterial* Mask4OnlyWriter = NewObject<UMaterial>(GetTransientPackage());
	UMaterialExpressionRuntimeVirtualTextureOutput* Mask4Output =
		NewObject<UMaterialExpressionRuntimeVirtualTextureOutput>(Mask4OnlyWriter);
	UMaterialExpressionConstant3Vector* Mask4Value =
		NewObject<UMaterialExpressionConstant3Vector>(Mask4OnlyWriter);
	Mask4OnlyWriter->GetExpressionCollection().AddExpression(Mask4Output);
	Mask4OnlyWriter->GetExpressionCollection().AddExpression(Mask4Value);
	Mask4Output->Mask4.Expression = Mask4Value;
	TestFalse(
		TEXT("Mask4-only output is not accepted as a Surface writer"),
		FDevKitRVTMeshDecalService::MaterialWritesSurfaceToRuntimeVirtualTexture(Mask4OnlyWriter));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTSurfaceInstanceEditingContractTest,
	"DevKitEditor.RVTSurfaceLibrary.InstanceEditingContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTSurfaceInstanceEditingContractTest::RunTest(const FString& Parameters)
{
	const ADevKitRVTSurfaceInstanceActor* ActorDefault =
		GetDefault<ADevKitRVTSurfaceInstanceActor>();
	const UInstancedStaticMeshComponent* InstanceComponent =
		ActorDefault ? ActorDefault->InstanceComponent.Get() : nullptr;
	TestNotNull(TEXT("Surface controller owns a plain ISM component"), InstanceComponent);
	if (!InstanceComponent)
	{
		return false;
	}

	TestTrue(TEXT("ISM component remains selectable"), InstanceComponent->bSelectable);
	TestTrue(
		TEXT("ISM component creates one hit proxy per instance"),
		InstanceComponent->bHasPerInstanceHitProxies);
	TestTrue(
		TEXT("Inherited native component remains editable"),
		InstanceComponent->IsEditableWhenInherited());
	TestEqual(
		TEXT("Surface controller root matches its static ISM mobility"),
		ActorDefault->SceneRoot->Mobility,
		EComponentMobility::Static);

	FSMInstanceId InstanceId;
	InstanceId.ISMComponent = const_cast<UInstancedStaticMeshComponent*>(InstanceComponent);
	InstanceId.InstanceIndex = 0;
	const ISMInstanceManager* InstanceManager = InstanceComponent;
	TestTrue(
		TEXT("Typed Elements can edit one surface instance"),
		InstanceManager->CanEditSMInstance(InstanceId));
	TestTrue(
		TEXT("Typed Elements can move one surface instance in the editor"),
		InstanceManager->CanMoveSMInstance(InstanceId, ETypedElementWorldType::Editor));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTSurfaceQualityScaledGeometryPolicyTest,
	"DevKitEditor.RVTSurfaceLibrary.QualityScaledGeometryPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTSurfaceQualityScaledGeometryPolicyTest::RunTest(const FString& Parameters)
{
	// Keep the platform override orthogonal to the user-selected quality tier.
	TestFalse(
		TEXT("Plane decals remain RVT-only at every quality"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::PlaneDecal,
			EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible,
			false,
			false));
	TestTrue(
		TEXT("Legacy visible objects preserve their source mesh"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault,
			true,
			true));
	TestTrue(
		TEXT("Quality Scaled keeps the source mesh for PC High and Epic"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			false,
			false));
	TestFalse(
		TEXT("Quality Scaled uses RVT projection for PC Mid and Low"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			true,
			false));
	TestFalse(
		TEXT("The handheld platform override forces Quality Scaled projection"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			false,
			true));
	TestTrue(
		TEXT("Always Visible remains an explicit gameplay-geometry escape hatch"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible,
			true,
			true));
	TestTrue(
		TEXT("Gameplay collision keeps Quality Scaled source geometry on every tier and platform"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::QualityScaled,
			true,
			true,
			true));
	TestFalse(
		TEXT("RVT Only visible objects never render their source mesh"),
		ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			EDevKitRVTSurfaceAssetType::VisibleObject,
			EDevKitRVTSurfaceGeometryPolicy::RVTOnly,
			false,
			false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTSurfaceCollisionPolicyValidationTest,
	"DevKitEditor.RVTSurfaceLibrary.CollisionRequiresAlwaysVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTSurfaceCollisionPolicyValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FDevKitRVTSurfaceAssetRequest Request;
	Request.AssetFolder = TEXT("/Game/Developers/Codex/Automation/RVTSurfaceLibrary");
	Request.AssetName = TEXT("DA_InvalidQualityScaledCollision_TestOnly");
	Request.AssetType = EDevKitRVTSurfaceAssetType::VisibleObject;
	Request.GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::QualityScaled;
	Request.bEnableCollision = true;

	const FDevKitRVTSurfaceAssetResult Result =
		FDevKitRVTMeshDecalService::CreateOrUpdateSurfaceAsset(Request);
	TestFalse(TEXT("QualityScaled gameplay collision is rejected before asset mutation"), Result.bSuccess);
	TestTrue(
		TEXT("Validation explains that collision requires AlwaysVisible"),
		Result.Message.ToString().Contains(TEXT("始终保留模型")));
	UWorld* ControllerWorld = GWorld;
	TestNotNull(TEXT("Automation world exists for controller validation"), ControllerWorld);
	if (!ControllerWorld)
	{
		return false;
	}

	UDevKitRVTSurfaceAsset* InvalidAsset =
		NewObject<UDevKitRVTSurfaceAsset>(GetTransientPackage());
	InvalidAsset->AssetType = EDevKitRVTSurfaceAssetType::VisibleObject;
	InvalidAsset->GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::QualityScaled;
	InvalidAsset->bEnableCollision = true;
	const FDevKitRVTSurfaceControllerResult ControllerResult =
		FDevKitRVTMeshDecalService::FindOrCreateSurfaceInstanceActor(ControllerWorld, InvalidAsset, true);
	TestFalse(TEXT("An externally authored invalid collision asset cannot create a controller"), ControllerResult.bSuccess);
	TestTrue(
		TEXT("Controller validation reports the collision policy before RVT or asset completeness"),
		ControllerResult.Message.ToString().Contains(TEXT("始终保留模型")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTSurfaceActorRenderingStateTest,
	"DevKitEditor.RVTSurfaceLibrary.RenderingStateTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTSurfaceActorRenderingStateTest::RunTest(const FString& Parameters)
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
	ADevKitRVTSurfaceInstanceActor* Actor =
		World->SpawnActor<ADevKitRVTSurfaceInstanceActor>(SpawnParameters);
	TestNotNull(TEXT("Transient RVT surface actor spawned"), Actor);
	if (!Actor || !Actor->InstanceComponent)
	{
		return false;
	}

	UDevKitRVTSurfaceAsset* Asset = NewObject<UDevKitRVTSurfaceAsset>(GetTransientPackage());
	URuntimeVirtualTexture* SurfaceRVT = NewObject<URuntimeVirtualTexture>(GetTransientPackage());
	TestTrue(TEXT("An empty controller accepts its initial SurfaceAsset"), Actor->InitializeSurfaceAsset(Asset));
	Actor->SurfaceRVT = SurfaceRVT;
	Asset->AssetType = EDevKitRVTSurfaceAssetType::VisibleObject;
	Asset->GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::RVTOnly;
	Asset->bEnableCollision = false;
	Asset->bCastShadow = true;

	const int32 InstanceIndex = Actor->AddSurfaceInstance(FTransform(FVector(10.0, 20.0, 1.0)));
	TestTrue(TEXT("A surface instance can be retained through policy changes"), InstanceIndex != INDEX_NONE);
	const int32 InstanceCountBefore = Actor->GetSurfaceInstanceCount();
	UDevKitRVTSurfaceAsset* DifferentAsset = NewObject<UDevKitRVTSurfaceAsset>(GetTransientPackage());
	TestFalse(
		TEXT("A populated controller rejects rebinding to a different SurfaceAsset"),
		Actor->InitializeSurfaceAsset(DifferentAsset));
	TestEqual(TEXT("Rejected rebinding preserves the original SurfaceAsset"), Actor->SurfaceAsset.Get(), Asset);
	TestTrue(
		TEXT("A populated controller accepts reapplying the same SurfaceAsset"),
		Actor->InitializeSurfaceAsset(Asset));
	Actor->ApplySurfaceAsset();

	TestEqual(
		TEXT("RVTOnly selects the Exclusive main-pass policy"),
		Actor->InstanceComponent->VirtualTextureRenderPassType,
		ERuntimeVirtualTextureMainPassType::Exclusive);
	TestFalse(TEXT("Projected geometry has no collision"), Actor->InstanceComponent->IsCollisionEnabled());
	TestFalse(TEXT("Projected geometry casts no shadow"), Actor->InstanceComponent->CastShadow);
	TestTrue(
		TEXT("Projected geometry keeps its Surface RVT binding"),
		Actor->InstanceComponent->RuntimeVirtualTextures.Contains(SurfaceRVT));

	Asset->GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible;
	Asset->bEnableCollision = true;
	Actor->ApplySurfaceAsset();
	TestEqual(
		TEXT("AlwaysVisible selects the Always main-pass policy"),
		Actor->InstanceComponent->VirtualTextureRenderPassType,
		ERuntimeVirtualTextureMainPassType::Always);
	TestTrue(TEXT("AlwaysVisible restores configured collision"), Actor->InstanceComponent->IsCollisionEnabled());
	TestTrue(TEXT("Collision-enabled geometry remains navigation-relevant"), Actor->InstanceComponent->CanEverAffectNavigation());
	TestTrue(TEXT("AlwaysVisible restores configured shadows"), Actor->InstanceComponent->CastShadow);

	Asset->GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::RVTOnly;
	Asset->bEnableCollision = false;
	Actor->SurfaceRVT = nullptr;
	Actor->ApplySurfaceAsset();
	TestEqual(
		TEXT("A visible object without Surface RVT safely falls back to source geometry"),
		Actor->InstanceComponent->VirtualTextureRenderPassType,
		ERuntimeVirtualTextureMainPassType::Always);
	TestEqual(
		TEXT("Rendering policy transitions preserve every ISM instance"),
		Actor->GetSurfaceInstanceCount(),
		InstanceCountBefore);

	Actor->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDevKitRVTSurfaceLibraryWidgetDefaultViewTest,
	"DevKitEditor.RVTSurfaceLibrary.WidgetDefaultUseView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTSurfaceLibraryWidgetDefaultViewTest::RunTest(const FString& Parameters)
{
	const TSharedRef<SDevKitRVTMeshDecalWidget> Widget = SNew(SDevKitRVTMeshDecalWidget);
	TestTrue(
		TEXT("RVT surface library opens on the two-column use view"),
		Widget->IsUseViewActiveForAutomation());
	return true;
}

#endif
