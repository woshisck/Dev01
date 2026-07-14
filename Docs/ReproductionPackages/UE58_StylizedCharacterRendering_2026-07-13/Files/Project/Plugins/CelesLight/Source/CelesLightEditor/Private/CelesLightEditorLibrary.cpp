#include "CelesLightEditorLibrary.h"

#include "Actors/CelesLightCaptureBox.h"
#include "Actors/CelesPointLight.h"
#include "Actors/StylizedEmissiveLight.h"
#include "Actors/StylizedCharacterLookVolume.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
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
