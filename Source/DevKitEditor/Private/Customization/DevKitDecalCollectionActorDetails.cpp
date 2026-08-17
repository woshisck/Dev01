#include "Customization/DevKitDecalCollectionActorDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "Surface/DevKitDecalCollectionActor.h"
#include "Tools/DecalCollection/DevKitDecalCollectionEdMode.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitDecalCollectionActorDetails"

TSharedRef<IDetailCustomization> FDevKitDecalCollectionActorDetails::MakeInstance()
{
	return MakeShared<FDevKitDecalCollectionActorDetails>();
}

void FDevKitDecalCollectionActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if (ADevKitDecalCollectionActor* Actor = Cast<ADevKitDecalCollectionActor>(Object.Get()))
		{
			CollectionActor = Actor;
			break;
		}
	}

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Decal Collection|Edit"), LOCTEXT("EditCategory", "Decal Collection Edit"));
	Category.AddCustomRow(LOCTEXT("EditSearch", "Edit"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text_Lambda([this]
			{
				return CollectionActor.IsValid() && CollectionActor->bEditSessionActive
					? LOCTEXT("Editing", "Editing Collection")
					: LOCTEXT("EnterEdit", "Edit Collection");
			})
			.IsEnabled_Lambda([this]
			{
				return CollectionActor.IsValid() && !CollectionActor->bEditSessionActive;
			})
			.OnClicked(this, &FDevKitDecalCollectionActorDetails::EnterEditMode)
		];
}

FReply FDevKitDecalCollectionActorDetails::EnterEditMode()
{
	if (ADevKitDecalCollectionActor* Actor = CollectionActor.Get())
	{
		UE_LOG(LogTemp, Display, TEXT("DecalCollectionDetails Edit clicked actor=%s"), *Actor->GetPathName());
		UDevKitDecalCollectionEdMode::RequestCollectionForActivation(Actor);
		if (GEditor)
		{
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(Actor, true, true);
			GEditor->NoteSelectionChange();
		}
		GLevelEditorModeTools().ActivateMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection, false);
		UE_LOG(LogTemp, Display, TEXT("DecalCollectionDetails ActivateMode active=%d"),
			GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection) ? 1 : 0);
		if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			UE_LOG(LogTemp, Display, TEXT("DecalCollectionDetails mode resolved; BeginEditing result=%d"),
				Mode->BeginEditingCollection(Actor) ? 1 : 0);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DecalCollectionDetails could not resolve active scriptable mode"));
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
