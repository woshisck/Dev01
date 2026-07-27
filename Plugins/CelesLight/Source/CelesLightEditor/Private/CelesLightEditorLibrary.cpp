#include "CelesLightEditorLibrary.h"

#include "Actors/CelesLightCaptureBox.h"
#include "Actors/CelesPointLight.h"
#include "Actors/StylizedEmissiveLight.h"
#include "Actors/StylizedCharacterLookVolume.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Factories/DataAssetFactory.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "StylizedEmissiveModelLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UnrealClient.h"

namespace
{
	UWorld* ResolveEditorWorld(UWorld* World)
	{
		if (World)
		{
			return World;
		}

		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	FTransform GetViewportSpawnTransform()
	{
		FVector SpawnLocation = FVector::ZeroVector;
		if (GEditor)
		{
			if (const FViewport* Viewport = GEditor->GetActiveViewport())
			{
				if (const FViewportClient* ViewportClient = Viewport->GetClient())
				{
					if (const FEditorViewportClient* EditorViewportClient = static_cast<const FEditorViewportClient*>(ViewportClient))
					{
						SpawnLocation = EditorViewportClient->GetViewLocation() + EditorViewportClient->GetViewRotation().Vector() * 500.0f;
					}
				}
			}
		}

		return FTransform(FRotator::ZeroRotator, SpawnLocation);
	}

	template <typename ActorT>
	ActorT* SpawnCelesActor(UWorld* TargetWorld, const TCHAR* BaseName)
	{
		if (!TargetWorld)
		{
			return nullptr;
		}

		TargetWorld->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(TargetWorld->PersistentLevel, ActorT::StaticClass(), BaseName);
		SpawnParameters.OverrideLevel = TargetWorld->PersistentLevel;
		SpawnParameters.ObjectFlags = RF_Transactional;

		ActorT* Actor = TargetWorld->SpawnActor<ActorT>(ActorT::StaticClass(), GetViewportSpawnTransform(), SpawnParameters);
		if (Actor && GEditor)
		{
			Actor->Modify();
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(Actor, true, true);
			GEditor->NoteSelectionChange();
		}

		return Actor;
	}

	void AddDefaultEmissiveModels(UStylizedEmissiveModelLibrary& Library)
	{
		bool bLibraryChanged = false;
		UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/YogArt_Material/MasterMaterial/Efect/M_Emissive_Common.M_Emissive_Common"));

		auto AddModel = [&Library, &bLibraryChanged, DefaultMaterial](
			const TCHAR* ModelId,
			const FText& DisplayName,
			const FText& Description,
			const TCHAR* MeshPath,
			const FVector& Scale)
		{
			const FName EntryId(ModelId);
			FStylizedEmissiveModelEntry* ExistingEntry = Library.Models.FindByPredicate([EntryId](const FStylizedEmissiveModelEntry& Candidate)
			{
				return Candidate.ModelId == EntryId;
			});
			FStylizedEmissiveModelEntry& Entry = ExistingEntry ? *ExistingEntry : Library.Models.AddDefaulted_GetRef();
			if (!ExistingEntry)
			{
				Entry.ModelId = EntryId;
				Entry.bUseMesh = true;
				Entry.Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
				Entry.Material = DefaultMaterial;
				Entry.RelativeTransform.SetScale3D(Scale);
				bLibraryChanged = true;
			}
			if (Entry.DisplayName.IsEmpty())
			{
				Entry.DisplayName = DisplayName;
				bLibraryChanged = true;
			}
			if (Entry.Description.IsEmpty())
			{
				Entry.Description = Description;
				bLibraryChanged = true;
			}
		};

		AddModel(TEXT("Sphere"), NSLOCTEXT("CelesLightEditor", "DefaultEmissiveSphere", "球形发光源"),
			NSLOCTEXT("CelesLightEditor", "DefaultEmissiveSphereDescription", "适合灯泡、魔法球和全向发光物体。"),
			TEXT("/Engine/BasicShapes/Sphere.Sphere"), FVector(0.25, 0.25, 0.25));
		AddModel(TEXT("Cube"), NSLOCTEXT("CelesLightEditor", "DefaultEmissiveCube", "方形发光源"),
			NSLOCTEXT("CelesLightEditor", "DefaultEmissiveCubeDescription", "适合方灯、灯箱和规则体积发光物体。"),
			TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0.25, 0.25, 0.25));
		AddModel(TEXT("Cylinder"), NSLOCTEXT("CelesLightEditor", "DefaultEmissiveCylinder", "柱形发光源"),
			NSLOCTEXT("CelesLightEditor", "DefaultEmissiveCylinderDescription", "适合灯管、立柱和圆柱形发光物体。"),
			TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0.25, 0.25, 0.25));
		AddModel(TEXT("Plane"), NSLOCTEXT("CelesLightEditor", "DefaultEmissivePlane", "面形发光源"),
			NSLOCTEXT("CelesLightEditor", "DefaultEmissivePlaneDescription", "适合窗户、面板和单面发光区域。"),
			TEXT("/Engine/BasicShapes/Plane.Plane"), FVector(0.5, 0.5, 0.5));

		const FName DataOnlyId(TEXT("DataOnly"));
		FStylizedEmissiveModelEntry* DataOnly = Library.Models.FindByPredicate([DataOnlyId](const FStylizedEmissiveModelEntry& Candidate)
		{
			return Candidate.ModelId == DataOnlyId;
		});
		if (!DataOnly)
		{
			DataOnly = &Library.Models.AddDefaulted_GetRef();
			DataOnly->ModelId = DataOnlyId;
			DataOnly->DisplayName = NSLOCTEXT("CelesLightEditor", "DefaultEmissiveDataOnly", "无模型数据光源");
			DataOnly->Description = NSLOCTEXT("CelesLightEditor", "DefaultEmissiveDataOnlyDescription", "场景中不显示模型，只向风格化光照系统提供位置、颜色、强度和半径数据。");
			DataOnly->bUseMesh = false;
			DataOnly->Material = DefaultMaterial;
			DataOnly->LightingOutput = EStylizedEmissiveLightingOutput::StylizedMaterial;
			bLibraryChanged = true;
		}
		if (bLibraryChanged)
		{
			Library.MarkPackageDirty();
		}
	}
}

int32 UCelesLightEditorLibrary::ManualUpdateCelesLights(UWorld* World)
{
	UWorld* TargetWorld = ResolveEditorWorld(World);
	if (!TargetWorld)
	{
		return 0;
	}

	int32 UpdatedCount = 0;
	for (TActorIterator<ACelesLightCaptureBox> It(TargetWorld); It; ++It)
	{
		ACelesLightCaptureBox* CaptureBox = *It;
		if (!CaptureBox)
		{
			continue;
		}

		CaptureBox->UpdateCelesLight();
		++UpdatedCount;
	}

	return UpdatedCount;
}

ACelesLightCaptureBox* UCelesLightEditorLibrary::CreateCelesLightCaptureBox(UWorld* World)
{
	UWorld* TargetWorld = ResolveEditorWorld(World);
	if (!TargetWorld)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("CelesLightEditor", "CreateCaptureBoxTransaction", "Create Celes Light Capture Box"));
	return SpawnCelesActor<ACelesLightCaptureBox>(TargetWorld, TEXT("CelesLightCaptureBox"));
}

ACelesPointLight* UCelesLightEditorLibrary::CreateCelesPointLight(UWorld* World)
{
	UWorld* TargetWorld = ResolveEditorWorld(World);
	if (!TargetWorld)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("CelesLightEditor", "CreatePointLightTransaction", "Create Celes Light"));
	return SpawnCelesActor<ACelesPointLight>(TargetWorld, TEXT("CelesPointLight"));
}

AStylizedEmissiveLight* UCelesLightEditorLibrary::CreateStylizedEmissiveLight(UWorld* World)
{
	UWorld* TargetWorld = ResolveEditorWorld(World);
	if (!TargetWorld)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("CelesLightEditor", "CreateStylizedEmissiveLightTransaction", "Create Stylized Emissive Light"));
	return SpawnCelesActor<AStylizedEmissiveLight>(TargetWorld, TEXT("StylizedEmissiveLight"));
}

UStylizedEmissiveModelLibrary* UCelesLightEditorLibrary::GetOrCreateStylizedEmissiveModelLibrary()
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> ExistingLibraries;
	AssetRegistryModule.Get().GetAssetsByClass(
		UStylizedEmissiveModelLibrary::StaticClass()->GetClassPathName(),
		ExistingLibraries,
		true);

	UStylizedEmissiveModelLibrary* Library = ExistingLibraries.IsEmpty()
		? nullptr
		: Cast<UStylizedEmissiveModelLibrary>(ExistingLibraries[0].GetAsset());

	if (!Library)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetTools.CreateUniqueAssetName(
			TEXT("/Game/Art/Lighting/DA_StylizedEmissiveModelLibrary"),
			TEXT(""),
			UniquePackageName,
			UniqueAssetName);

		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = UStylizedEmissiveModelLibrary::StaticClass();
		Library = Cast<UStylizedEmissiveModelLibrary>(AssetTools.CreateAsset(
			UniqueAssetName,
			FPackageName::GetLongPackagePath(UniquePackageName),
			UStylizedEmissiveModelLibrary::StaticClass(),
			Factory));
	}

	if (Library)
	{
		// Also upgrades libraries created by the earlier raw Data Asset workflow.
		AddDefaultEmissiveModels(*Library);
	}

	return Library;
}

UStylizedEmissiveModelLibrary* UCelesLightEditorLibrary::OpenOrCreateStylizedEmissiveModelLibrary()
{
	UStylizedEmissiveModelLibrary* Library = GetOrCreateStylizedEmissiveModelLibrary();
	if (!Library)
	{
		return nullptr;
	}

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<UObject*> AssetsToSync;
	AssetsToSync.Add(Library);
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Library);
	}
	return Library;
}

AStylizedCharacterLookVolume* UCelesLightEditorLibrary::CreateStylizedCharacterLookVolume(UWorld* World)
{
	UWorld* TargetWorld = ResolveEditorWorld(World);
	if (!TargetWorld)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("CelesLightEditor", "CreateStylizedCharacterLookVolumeTransaction", "Create Stylized Character Look Volume"));
	return SpawnCelesActor<AStylizedCharacterLookVolume>(TargetWorld, TEXT("StylizedCharacterLookVolume"));
}
