#include "CelesLightEditorLibrary.h"

#include "Actors/CelesLightCaptureBox.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "Selection.h"
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

	const FScopedTransaction Transaction(NSLOCTEXT("CelesLightEditor", "CreateCaptureBoxTransaction", "创建 Celes Light 采集盒体"));
	TargetWorld->Modify();

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	if (GEditor)
	{
		if (const FViewport* Viewport = GEditor->GetActiveViewport())
		{
			if (const FViewportClient* ViewportClient = Viewport->GetClient())
			{
				if (const FEditorViewportClient* EditorViewportClient = static_cast<const FEditorViewportClient*>(ViewportClient))
				{
					SpawnLocation = EditorViewportClient->GetViewLocation() + EditorViewportClient->GetViewRotation().Vector() * 500.0f;
					SpawnRotation = FRotator::ZeroRotator;
				}
			}
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = MakeUniqueObjectName(TargetWorld->PersistentLevel, ACelesLightCaptureBox::StaticClass(), TEXT("CelesLightCaptureBox"));
	SpawnParameters.OverrideLevel = TargetWorld->PersistentLevel;
	SpawnParameters.ObjectFlags = RF_Transactional;

	ACelesLightCaptureBox* CaptureBox = TargetWorld->SpawnActor<ACelesLightCaptureBox>(SpawnLocation, SpawnRotation, SpawnParameters);
	if (CaptureBox && GEditor)
	{
		CaptureBox->Modify();
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(CaptureBox, true, true);
		GEditor->NoteSelectionChange();
	}

	return CaptureBox;
}
