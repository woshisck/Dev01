#include "Tools/DecalCollection/DevKitDecalCollectionEdMode.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorModeManager.h"
#include "CollisionQueryParams.h"
#include "Components/DecalComponent.h"
#include "Elements/Framework/TypedElementSelectionSet.h"
#include "Elements/SMInstance/SMInstanceElementData.h"
#include "Elements/SMInstance/SMInstanceElementId.h"
#include "Elements/Interfaces/TypedElementWorldInterface.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "ScopedTransaction.h"
#include "SceneView.h"
#include "Subsystems/EditorElementSubsystem.h"
#include "Surface/DevKitDecalCollectionActor.h"
#include "Tools/DecalCollection/SDevKitDecalCollectionWidget.h"
#include "Toolkits/BaseToolkit.h"
#include "Selection.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitDecalCollectionEdMode"

const FEditorModeID UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection(TEXT("DevKit.DecalCollection"));

namespace
{
	TWeakObjectPtr<ADevKitDecalCollectionActor> GRequestedCollection;

	class FDevKitDecalCollectionModeToolkit final : public FModeToolkit
	{
	public:
		virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override
		{
			FModeToolkit::Init(InitToolkitHost, InOwningMode);
			OwningMode = InOwningMode;
			SAssignNew(ToolkitWidget, SBorder)
				.Padding(4.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SDevKitDecalCollectionWidget)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4, 6, 4, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("Apply", "应用并退出"))
						.OnClicked_Lambda([this]
						{
							if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(OwningMode.Get())) Mode->ApplyAndExit();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("Cancel", "取消"))
						.OnClicked_Lambda([this]
						{
							if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(OwningMode.Get())) Mode->CancelAndExit();
							return FReply::Handled();
						})
					]
				]
			];
		}

		virtual FName GetToolkitFName() const override { return FName(TEXT("DevKitDecalCollectionMode")); }
		virtual FText GetBaseToolkitName() const override { return LOCTEXT("ToolkitName", "Decal & Surface"); }
		virtual TSharedPtr<SWidget> GetInlineContent() const override { return ToolkitWidget; }

	private:
		TWeakObjectPtr<UEdMode> OwningMode;
	};
}

UDevKitDecalCollectionEdMode::UDevKitDecalCollectionEdMode()
{
	Info = FEditorModeInfo(
		EM_DevKitDecalCollection,
		LOCTEXT("ModeName", "贴花与地表物件"),
		FSlateIcon(),
		true,
		120);
}

void UDevKitDecalCollectionEdMode::CreateToolkit()
{
	Toolkit = MakeShared<FDevKitDecalCollectionModeToolkit>();
}

void UDevKitDecalCollectionEdMode::Enter()
{
	UEdMode::Enter();
	ADevKitDecalCollectionActor* RequestedCollection = GRequestedCollection.Get();
	GRequestedCollection.Reset();
	const bool bRequestedStarted = BeginEditingCollection(RequestedCollection);
	UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode Enter requested=%s started=%d"),
		RequestedCollection ? *RequestedCollection->GetPathName() : TEXT("None"), bRequestedStarted ? 1 : 0);
	if (!bRequestedStarted)
	{
		ADevKitDecalCollectionActor* FallbackCollection = GetActiveCollection();
		const bool bFallbackStarted = BeginEditingCollection(FallbackCollection);
		UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode Enter fallback=%s started=%d"),
			FallbackCollection ? *FallbackCollection->GetPathName() : TEXT("None"), bFallbackStarted ? 1 : 0);
	}
}

void UDevKitDecalCollectionEdMode::ModeTick(float DeltaTime)
{
	UEdMode::ModeTick(DeltaTime);

	// Selecting the Collection after the mode was already active does not call
	// Enter() again.  Keep the session target self-healing so both the Outliner
	// selection and the inline Details button reliably enter the same session.
	if (!SessionCollection.IsValid())
	{
		ADevKitDecalCollectionActor* RequestedCollection = GRequestedCollection.Get();
		if (RequestedCollection)
		{
			GRequestedCollection.Reset();
		}
		const bool bRequestedStarted = BeginEditingCollection(RequestedCollection);
		UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode Tick requested=%s started=%d"),
			RequestedCollection ? *RequestedCollection->GetPathName() : TEXT("None"), bRequestedStarted ? 1 : 0);
		if (!bRequestedStarted)
		{
			ADevKitDecalCollectionActor* FallbackCollection = GetActiveCollection();
			const bool bFallbackStarted = BeginEditingCollection(FallbackCollection);
			UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode Tick fallback=%s started=%d"),
				FallbackCollection ? *FallbackCollection->GetPathName() : TEXT("None"), bFallbackStarted ? 1 : 0);
		}
	}
}

void UDevKitDecalCollectionEdMode::RequestCollectionForActivation(ADevKitDecalCollectionActor* Collection)
{
	GRequestedCollection = Collection;
	UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode request collection=%s"),
		Collection ? *Collection->GetPathName() : TEXT("None"));
	// ActivateMode is allowed to be a no-op when this Mode is already active.
	// In that case Enter() will not run, so complete the request synchronously
	// instead of relying solely on the next ModeTick (which can be suppressed
	// while Slate is processing the button click).
	if (Collection && GLevelEditorModeTools().IsModeActive(EM_DevKitDecalCollection))
	{
		if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(EM_DevKitDecalCollection)))
		{
			const bool bStarted = Mode->BeginEditingCollection(Collection);
			if (bStarted)
			{
				GRequestedCollection.Reset();
			}
			UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode request active-mode begin=%d"), bStarted ? 1 : 0);
		}
	}
}

bool UDevKitDecalCollectionEdMode::BeginEditingCollection(ADevKitDecalCollectionActor* Collection)
{
	if (!Collection || !Collection->Collection)
	{
		UE_LOG(LogTemp, Warning, TEXT("DecalCollectionMode BeginEditing rejected: null collection/component"));
		return false;
	}

	// A Mode can already be active when the user selects a Collection in the
	// Outliner or presses Edit in the inline toolkit.  In that case UE does not
	// call Enter() again, so the target must be installed explicitly here.
	if (SessionCollection.IsValid() && SessionCollection.Get() != Collection)
	{
		UE_LOG(LogTemp, Warning, TEXT("DecalCollectionMode BeginEditing rejected: session=%s requested=%s"),
			*SessionCollection->GetPathName(), *Collection->GetPathName());
		return false;
	}

	if (!SessionCollection.IsValid())
	{
		SessionCollection = Collection;
		SessionRecords = Collection->Collection->Records;
		bSessionSnapshotValid = true;
		bAcceptOnExit = true;
		SelectedDeferredRecordGuid.Invalidate();
		SelectedDeferredComponent.Reset();
	}

	Collection->BeginEditSession();
	// UE5.8 only routes an ISM hit proxy to an instance Typed Element when
	// both switches below are enabled. If either is disabled the viewport
	// intentionally falls back to the component/actor hit proxy, so all
	// records outline together and W/E/R cannot edit one placement.
	int32 SMInstanceElementsCVar = -1;
	int32 ViewportSMSelectionCVar = -1;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("TypedElements.EnableSMInstanceElements")))
	{
		CVar->Set(1, ECVF_SetByCode);
		SMInstanceElementsCVar = CVar->GetInt();
	}
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("TypedElements.EnableViewportSMInstanceSelection")))
	{
		CVar->Set(1, ECVF_SetByCode);
		ViewportSMSelectionCVar = CVar->GetInt();
	}
	Collection->SetDerivedInstanceEditingEnabled(true);
	// Modern UEdMode instances do not opt into the legacy viewport widget
	// contract by themselves.  The mode derives from UBaseLegacyWidgetEdMode,
	// and the explicit settings below make the first selected ISM show a normal
	// translate gizmo immediately instead of requiring the user to cycle a
	// hidden/unsupported widget mode.
	GLevelEditorModeTools().SetShowWidget(true);
	GLevelEditorModeTools().SetWidgetMode(UE::Widget::WM_Translate);
	UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode BeginEditing started collection=%s records=%d typed_instance_elements=%d typed_viewport_selection=%d derived_components=%d"),
		*Collection->GetPathName(), Collection->Collection->GetRecordCount(), SMInstanceElementsCVar,
		ViewportSMSelectionCVar, Collection->DerivedRVTComponents.Num());

	// Do not keep the whole Collection actor selected: UE would outline every
	// derived ISM instance. Typed-element hit proxies remain active and the
	// active collection is kept by SessionCollection instead.
	if (GEditor)
	{
		GEditor->SelectNone(false, true);
		GEditor->NoteSelectionChange();
		// The viewport hit-proxy map can still contain the Collection actor proxy
		// from before the mode was entered.  Reregistering the ISMs creates the
		// per-instance proxies, but they are not picked until every level viewport
		// discards its cached map.
		for (FEditorViewportClient* ViewportClient : GEditor->GetAllViewportClients())
		{
			if (ViewportClient && ViewportClient->Viewport)
			{
				ViewportClient->Viewport->InvalidateHitProxy();
			}
		}
		GEditor->RedrawLevelEditingViewports();
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::PlaceAssetAtViewportCenter(UDevKitDecalAsset* Asset)
{
	FLevelEditorViewportClient* ViewportClient = GCurrentLevelEditingViewportClient;
	if (!ViewportClient || !ViewportClient->Viewport)
	{
		if (GEditor)
		{
			for (FLevelEditorViewportClient* Candidate : GEditor->GetLevelViewportClients())
			{
				if (Candidate && Candidate->Viewport && Candidate->IsPerspective())
				{
					ViewportClient = Candidate;
					break;
				}
			}
		}
	}
	if (!ViewportClient || !ViewportClient->Viewport)
	{
		return false;
	}

	const FIntPoint ViewportSize = ViewportClient->Viewport->GetSizeXY();
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return false;
	}
	return PlaceAssetAtViewportCoordinates(Asset, ViewportClient, ViewportSize.X / 2, ViewportSize.Y / 2, false);
}

bool UDevKitDecalCollectionEdMode::PlaceAssetAtViewportCursor(UDevKitDecalAsset* Asset, FLevelEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY)
{
	return PlaceAssetAtViewportCoordinates(Asset, ViewportClient, ViewportX, ViewportY, true);
}

bool UDevKitDecalCollectionEdMode::PlaceAssetAtViewportCoordinates(
	UDevKitDecalAsset* Asset,
	FLevelEditorViewportClient* ViewportClient,
	int32 ViewportX,
	int32 ViewportY,
	bool bRequireSurfaceHit)
{
	FTransform PlacementTransform;
	if (!ResolvePlacementTransform(Asset, ViewportClient, ViewportX, ViewportY, bRequireSurfaceHit, PlacementTransform))
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("PlaceCollectionPaletteAsset", "放置贴花与地表物件"));
	return AddPlacementRecord(Asset, PlacementTransform);
}

bool UDevKitDecalCollectionEdMode::ResolvePlacementTransform(
	UDevKitDecalAsset* Asset,
	FEditorViewportClient* ViewportClient,
	int32 ViewportX,
	int32 ViewportY,
	bool bRequireSurfaceHit,
	FTransform& OutTransform) const
{
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !Collection->Collection || !Asset || !Asset->IsValidDefinition()
		|| !ViewportClient || !ViewportClient->Viewport)
	{
		UE_LOG(LogTemp, Warning, TEXT("DecalCollectionMode place rejected: collection, asset or viewport is invalid"));
		return false;
	}

	UWorld* World = Collection->GetWorld();
	if (!World || World->IsGameWorld())
	{
		return false;
	}

	const FIntPoint ViewportSize = ViewportClient->Viewport->GetSizeXY();
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0
		|| ViewportX < 0 || ViewportY < 0 || ViewportX >= ViewportSize.X || ViewportY >= ViewportSize.Y)
	{
		return false;
	}

	FVector PlacementLocation = Asset->DefaultTransform.GetLocation();
	FVector PlacementNormal = FVector::UpVector;
	bool bHitSurface = false;
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));
	if (FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily))
	{
		const FViewportCursorLocation Cursor(SceneView, ViewportClient, ViewportX, ViewportY);
		const FVector TraceStart = Cursor.GetOrigin();
		const FVector TraceEnd = TraceStart + Cursor.GetDirection().GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector) * HALF_WORLD_MAX;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DevKitDecalCollectionCenterPlacement), false);
		QueryParams.bReturnPhysicalMaterial = false;
		QueryParams.AddIgnoredActor(Collection);

		TArray<FHitResult> Hits;
		World->LineTraceMultiByObjectType(
			Hits,
			TraceStart,
			TraceEnd,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
			QueryParams);
		for (const FHitResult& Hit : Hits)
		{
			if (Hit.Component.IsValid() && Hit.GetActor() != Collection)
			{
				PlacementNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				PlacementLocation = Hit.ImpactPoint + PlacementNormal * 0.5f;
				bHitSurface = true;
				break;
			}
		}
	}
	if (bRequireSurfaceHit && !bHitSurface)
	{
		return false;
	}

	OutTransform = Asset->DefaultTransform;
	OutTransform.SetLocation(PlacementLocation);
	if (bHitSurface && Asset->Backend == EDevKitDecalBackend::DeferredProjection)
	{
		// Deferred Decal projects along its local +X axis.  A freshly authored
		// deferred asset therefore follows a ground/wall normal when it is dragged
		// or painted, instead of retaining a sideways identity rotation.
		const FQuat SurfaceAlignment = FQuat::FindBetweenNormals(FVector::ForwardVector, -PlacementNormal);
		OutTransform.SetRotation(SurfaceAlignment * Asset->DefaultTransform.GetRotation());
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::AddPlacementRecord(UDevKitDecalAsset* Asset, const FTransform& PlacementTransform)
{
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !Collection->Collection || !Asset || !Asset->IsValidDefinition())
	{
		return false;
	}

	Collection->Modify();
	if (!Collection->Collection->AddPaletteAsset(Asset)
		|| !Collection->Collection->AddRecord(Asset, PlacementTransform).IsValid())
	{
		return false;
	}
	Collection->RebuildDerivedRendering();
	Collection->MarkPackageDirty();
	if (Collection->GetLevel())
	{
		Collection->GetLevel()->MarkPackageDirty();
	}
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	UE_LOG(LogTemp, Display, TEXT("DecalCollectionMode placed palette asset=%s collection=%s location=(%.1f,%.1f,%.1f)"),
		*Asset->GetPathName(), *Collection->GetPathName(), PlacementTransform.GetLocation().X, PlacementTransform.GetLocation().Y, PlacementTransform.GetLocation().Z);
	return true;
}

bool UDevKitDecalCollectionEdMode::ToggleBrushPlacement(UDevKitDecalAsset* Asset)
{
	if (!Asset || !Asset->IsValidDefinition() || !SessionCollection.IsValid())
	{
		return false;
	}

	const bool bDisable = bBrushPlacementEnabled && BrushPlacementAsset.Get() == Asset;
	bBrushPlacementEnabled = !bDisable;
	BrushPlacementAsset = bDisable ? nullptr : Asset;
	bBrushStrokeActive = false;
	bHasBrushLastPlacement = false;
	BrushPlacementTransaction.Reset();
	return bBrushPlacementEnabled;
}

bool UDevKitDecalCollectionEdMode::IsBrushPlacementActiveFor(const UDevKitDecalAsset* Asset) const
{
	return bBrushPlacementEnabled && BrushPlacementAsset.Get() == Asset;
}

void UDevKitDecalCollectionEdMode::SetBrushSpacing(float InSpacing)
{
	BrushSpacing = FMath::Clamp(InSpacing, 25.f, 1000.f);
}

bool UDevKitDecalCollectionEdMode::PaintBrushAtViewportCoordinates(FEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY)
{
	UDevKitDecalAsset* Asset = BrushPlacementAsset.Get();
	FTransform PlacementTransform;
	if (!ResolvePlacementTransform(Asset, ViewportClient, ViewportX, ViewportY, true, PlacementTransform))
	{
		return false;
	}

	const FVector Location = PlacementTransform.GetLocation();
	if (bHasBrushLastPlacement && FVector::DistSquared(Location, BrushLastPlacement) < FMath::Square(BrushSpacing))
	{
		return false;
	}
	if (!AddPlacementRecord(Asset, PlacementTransform))
	{
		return false;
	}
	BrushLastPlacement = Location;
	bHasBrushLastPlacement = true;
	return true;
}

void UDevKitDecalCollectionEdMode::Exit()
{
	DeferredTransformTransaction.Reset();
	BrushPlacementTransaction.Reset();
	bBrushStrokeActive = false;
	bHasBrushLastPlacement = false;
	if (!bAcceptOnExit && bSessionSnapshotValid)
	{
		if (ADevKitDecalCollectionActor* Collection = SessionCollection.Get())
		{
			if (Collection->Collection)
			{
				RestoreSessionAdoptionSourceVisibility(Collection);
				Collection->Modify();
				Collection->Collection->Modify();
				Collection->Collection->Records = SessionRecords;
				Collection->RebuildDerivedRendering();
			}
		}
	}
	if (ADevKitDecalCollectionActor* Collection = SessionCollection.Get())
	{
		Collection->SetDerivedInstanceEditingEnabled(false);
		Collection->EndEditSession(bAcceptOnExit);
	}
	SessionCollection.Reset();
	SessionRecords.Reset();
	SelectedDeferredRecordGuid.Invalidate();
	SelectedDeferredComponent.Reset();
	bSessionSnapshotValid = false;
	UEdMode::Exit();
}

void UDevKitDecalCollectionEdMode::RestoreSessionAdoptionSourceVisibility(ADevKitDecalCollectionActor* Collection) const
{
	if (!Collection || !Collection->Collection)
	{
		return;
	}

	// Adopt intentionally hides the source instead of deleting it.  The normal
	// record snapshot is therefore not sufficient for Cancel: source component
	// visibility must be restored to the state at Edit entry as well.
	TMap<FString, bool> DesiredHiddenBySource;
	TMap<FString, TPair<FString, FName>> SourceDescriptorByKey;
	auto RegisterRecord = [&DesiredHiddenBySource, &SourceDescriptorByKey](const FDevKitDecalPlacementRecord& Record, bool bUseRecordState)
	{
		if (Record.SourceActorPath.IsEmpty() || Record.SourceComponentName.IsNone())
		{
			return;
		}
		const FString SourceKey = Record.SourceActorPath + TEXT("|") + Record.SourceComponentName.ToString();
		if (bUseRecordState)
		{
			DesiredHiddenBySource.FindOrAdd(SourceKey) = Record.bSourceHiddenForAdoption;
		}
		else
		{
			DesiredHiddenBySource.FindOrAdd(SourceKey, false);
		}
		SourceDescriptorByKey.FindOrAdd(SourceKey) = TPair<FString, FName>(Record.SourceActorPath, Record.SourceComponentName);
	};

	for (const FDevKitDecalPlacementRecord& Record : SessionRecords)
	{
		RegisterRecord(Record, true);
	}
	for (const FDevKitDecalPlacementRecord& Record : Collection->Collection->Records)
	{
		RegisterRecord(Record, false);
	}

	UWorld* World = Collection->GetWorld();
	if (!World)
	{
		return;
	}
	for (const TPair<FString, TPair<FString, FName>>& Pair : SourceDescriptorByKey)
	{
		AActor* SourceActor = nullptr;
		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			if (*ActorIt && (*ActorIt)->GetPathName() == Pair.Value.Key)
			{
				SourceActor = *ActorIt;
				break;
			}
		}
		if (!SourceActor)
		{
			continue;
		}

		TInlineComponentArray<USceneComponent*> Components(SourceActor);
		if (USceneComponent* const* SourceComponent = Components.FindByPredicate([&Pair](const USceneComponent* Component)
		{
			return Component && Component->GetFName() == Pair.Value.Value;
		}))
		{
			const bool bShouldBeHidden = DesiredHiddenBySource.FindRef(Pair.Key);
			SourceActor->Modify();
			(*SourceComponent)->Modify();
			(*SourceComponent)->SetVisibility(!bShouldBeHidden, true);
			(*SourceComponent)->SetHiddenInGame(bShouldBeHidden, true);
		}
	}
}

ADevKitDecalCollectionActor* UDevKitDecalCollectionEdMode::GetActiveCollection() const
{
	if (SessionCollection.IsValid())
	{
		return SessionCollection.Get();
	}
	if (GEditor)
	{
		if (AActor* SelectedActor = GEditor->GetSelectedActors()->GetTop<AActor>())
		{
			return Cast<ADevKitDecalCollectionActor>(SelectedActor);
		}

		// Mode activation can happen before the user has selected the Collection
		// actor (for example directly from the Selection Mode dropdown).  If the
		// current editor world has exactly one Collection, it is unambiguous and
		// should become the active edit target instead of leaving the toolkit in
		// the seemingly inert "please select one" state.
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		ADevKitDecalCollectionActor* OnlyCollection = nullptr;
		int32 CollectionCount = 0;
		if (EditorWorld)
		{
			for (TActorIterator<ADevKitDecalCollectionActor> It(EditorWorld); It; ++It)
			{
				ADevKitDecalCollectionActor* Candidate = *It;
				if (!Candidate || Candidate->IsTemplate() || Candidate->IsActorBeingDestroyed())
				{
					continue;
				}
				OnlyCollection = Candidate;
				++CollectionCount;
				if (CollectionCount > 1)
				{
					break;
				}
			}
		}
		if (CollectionCount == 1)
		{
			return OnlyCollection;
		}
	}
	return nullptr;
}

bool UDevKitDecalCollectionEdMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	return InActor == nullptr || InActor->IsA<ADevKitDecalCollectionActor>();
}

bool UDevKitDecalCollectionEdMode::IsEditingDisallowed(AActor* InActor) const
{
	return InActor && !InActor->IsA<ADevKitDecalCollectionActor>();
}

bool UDevKitDecalCollectionEdMode::ProcessEditDelete()
{
	FGuid RecordGuid;
	FDevKitDecalPlacementRecord Record;
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (Collection && Collection->Collection && GetSelectedInstanceRecord(RecordGuid, Record))
	{
		const FScopedTransaction Transaction(LOCTEXT("RemoveCollectionRecord", "删除贴花与地表物件实例"));
		Collection->Modify();
		if (Collection->Collection->RemoveRecord(RecordGuid))
		{
			SelectedDeferredRecordGuid.Invalidate();
			SelectedDeferredComponent.Reset();
			Collection->RebuildDerivedRendering();
			Collection->MarkPackageDirty();
			if (Collection->GetLevel())
			{
				Collection->GetLevel()->MarkPackageDirty();
			}
			if (GEditor)
			{
				GEditor->SelectNone(false, true);
				GEditor->NoteSelectionChange();
				GEditor->RedrawLevelEditingViewports();
			}
			return true;
		}
	}
	// In the active mode an unowned actor must never be deleted through a decal
	// hotkey. Leaving it untouched makes the ownership boundary explicit.
	return SessionCollection.IsValid() ? true : UEdMode::ProcessEditDelete();
}

bool UDevKitDecalCollectionEdMode::ProcessEditDuplicate()
{
	FGuid RecordGuid;
	FDevKitDecalPlacementRecord SourceRecord;
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !Collection->Collection || !GetSelectedInstanceRecord(RecordGuid, SourceRecord) || !SourceRecord.Asset)
	{
		return SessionCollection.IsValid();
	}

	const FScopedTransaction Transaction(LOCTEXT("DuplicateCollectionRecord", "复制贴花与地表物件实例"));
	Collection->Modify();
	const FGuid NewGuid = Collection->Collection->AddRecord(SourceRecord.Asset, SourceRecord.Transform);
	FDevKitDecalPlacementRecord* NewRecord = Collection->Collection->FindRecord(NewGuid);
	if (!NewRecord)
	{
		return true;
	}
	NewRecord->DecalSize = SourceRecord.DecalSize;
	NewRecord->CustomData = SourceRecord.CustomData;
	NewRecord->MaterialOverride = SourceRecord.MaterialOverride;
	NewRecord->Tags = SourceRecord.Tags;
	NewRecord->bEnabled = SourceRecord.bEnabled;
	Collection->RebuildDerivedRendering();
	Collection->MarkPackageDirty();
	if (Collection->GetLevel())
	{
		Collection->GetLevel()->MarkPackageDirty();
	}
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::HandleClick(
	FEditorViewportClient* InViewportClient,
	HHitProxy* HitProxy,
	const FViewportClick& Click)
{
	// UDecalComponent has no SM-instance typed element or per-instance hit proxy.
	// Resolve the authoring record from its projected bounds before falling back
	// to UE's ordinary actor/ISM click handling.
	if (Click.GetKey() == EKeys::LeftMouseButton && Click.GetEvent() == IE_Pressed && TrySelectDeferredRecord(Click))
	{
		if (GEditor)
		{
			GEditor->SelectNone(false, true);
			GEditor->NoteSelectionChange();
			GEditor->RedrawLevelEditingViewports();
		}
		return true;
	}
	if (Click.GetKey() == EKeys::LeftMouseButton && Click.GetEvent() == IE_Pressed)
	{
		// A normal scene/ISM click must relinquish the deferred selection; otherwise
		// the widget would keep editing the last projected decal after the artist
		// visibly selected a different record.
		SelectedDeferredRecordGuid.Invalidate();
		SelectedDeferredComponent.Reset();
	}

	return Super::HandleClick(InViewportClient, HitProxy, Click);
}

bool UDevKitDecalCollectionEdMode::InputKey(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FKey Key,
	EInputEvent Event)
{
	if (bBrushPlacementEnabled && BrushPlacementAsset.IsValid() && InViewportClient && InViewport && Key == EKeys::LeftMouseButton)
	{
		if (Event == IE_Pressed)
		{
			bBrushStrokeActive = true;
			bHasBrushLastPlacement = false;
			BrushPlacementTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("PaintCollectionDecals", "画笔放置贴花与地表物件"));
			PaintBrushAtViewportCoordinates(InViewportClient, InViewport->GetMouseX(), InViewport->GetMouseY());
			return true;
		}
		if (Event == IE_Released)
		{
			bBrushStrokeActive = false;
			bHasBrushLastPlacement = false;
			BrushPlacementTransaction.Reset();
			return true;
		}
	}
	return Super::InputKey(InViewportClient, InViewport, Key, Event);
}

bool UDevKitDecalCollectionEdMode::InputAxis(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	int32 ControllerId,
	FKey Key,
	float Delta,
	float DeltaTime)
{
	if (bBrushStrokeActive && InViewportClient && InViewport
		&& (Key == EKeys::MouseX || Key == EKeys::MouseY)
		&& !FMath::IsNearlyZero(Delta))
	{
		PaintBrushAtViewportCoordinates(InViewportClient, InViewport->GetMouseX(), InViewport->GetMouseY());
		return true;
	}
	return Super::InputAxis(InViewportClient, InViewport, ControllerId, Key, Delta, DeltaTime);
}

bool UDevKitDecalCollectionEdMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	FGuid RecordGuid;
	FDevKitDecalPlacementRecord Record;
	if (!GetSelectedDeferredRecord(RecordGuid, Record))
	{
		return Super::StartTracking(InViewportClient, InViewport);
	}

	if (!DeferredTransformTransaction)
	{
		ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
		if (!Collection || !Collection->Collection)
		{
			return false;
		}
		DeferredTransformTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("TransformDeferredRecord", "调整延迟贴花实例"));
		Collection->Modify();
		Collection->Collection->Modify();
		if (UDecalComponent* Component = SelectedDeferredComponent.Get())
		{
			Component->Modify();
		}
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (DeferredTransformTransaction)
	{
		DeferredTransformTransaction.Reset();
		return true;
	}
	return Super::EndTracking(InViewportClient, InViewport);
}

bool UDevKitDecalCollectionEdMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	FGuid RecordGuid;
	FDevKitDecalPlacementRecord Record;
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	UDecalComponent* Component = SelectedDeferredComponent.Get();
	if (!Collection || !Component || !GetSelectedDeferredRecord(RecordGuid, Record))
	{
		return Super::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
	}

	if (!DeferredTransformTransaction && !StartTracking(InViewportClient, InViewport))
	{
		return false;
	}

	FTransform UpdatedTransform = Record.Transform;
	UpdatedTransform.AddToTranslation(InDrag);
	UpdatedTransform.ConcatenateRotation(InRot.Quaternion());
	UpdatedTransform.NormalizeRotation();
	const FVector CurrentScale = UpdatedTransform.GetScale3D();
	const FVector UpdatedScale = CurrentScale + CurrentScale * InScale;
	UpdatedTransform.SetScale3D(FVector(
		FMath::Max(KINDA_SMALL_NUMBER, UpdatedScale.X),
		FMath::Max(KINDA_SMALL_NUMBER, UpdatedScale.Y),
		FMath::Max(KINDA_SMALL_NUMBER, UpdatedScale.Z)));

	if (!Collection->UpdateDerivedDeferredTransform(Component, UpdatedTransform))
	{
		return false;
	}
	Collection->MarkPackageDirty();
	if (Collection->GetLevel())
	{
		Collection->GetLevel()->MarkPackageDirty();
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::GetSelectedInstanceTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	FGuid DeferredGuid;
	FDevKitDecalPlacementRecord DeferredRecord;
	if (GetSelectedDeferredRecord(DeferredGuid, DeferredRecord))
	{
		OutTransform = DeferredRecord.Transform;
		return true;
	}

	// The Level Editor owns a separate typed-element selection set from the
	// legacy editor actor selection set.  The viewport already normalizes that
	// set and filters it through CanMoveSMInstance before drawing the widget.
	// Prefer the viewport cache so an ISM instance selected by its hit proxy is
	// the exact element used by the native W/E/R interaction; falling back to
	// the mode set keeps the toolkit usable when no level viewport is active.
	if (GCurrentLevelEditingViewportClient)
	{
		const FTypedElementListConstRef ElementsToManipulate = GCurrentLevelEditingViewportClient->GetElementsToManipulate();
		if (const TTypedElement<ITypedElementWorldInterface> SelectedElement =
			ElementsToManipulate->GetBottomElement<ITypedElementWorldInterface>())
		{
			if (SelectedElement.GetWorldTransform(OutTransform))
			{
				return true;
			}
		}
	}

	FEditorModeTools* ModeTools = GetModeManager();
	if (!ModeTools)
	{
		return false;
	}

	UTypedElementSelectionSet* SelectionSet = ModeTools->GetEditorSelectionSet();
	if (!SelectionSet)
	{
		return false;
	}

	const FTypedElementListRef NormalizedSelection = UEditorElementSubsystem::GetEditorNormalizedSelectionSet(*SelectionSet);
	const TTypedElement<ITypedElementWorldInterface> SelectedElement =
		UEditorElementSubsystem::GetLastSelectedEditorManipulableElement(
			NormalizedSelection,
			GLevelEditorModeTools().GetWidgetMode(),
			GetWorld());

	return SelectedElement && SelectedElement.GetWorldTransform(OutTransform);
}

bool UDevKitDecalCollectionEdMode::GetSelectedInstanceRecord(
	FGuid& OutRecordGuid,
	FDevKitDecalPlacementRecord& OutRecord) const
{
	OutRecordGuid.Invalidate();
	OutRecord = FDevKitDecalPlacementRecord();
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !Collection->Collection)
	{
		return false;
	}
	if (GetSelectedDeferredRecord(OutRecordGuid, OutRecord))
	{
		return true;
	}

	auto ResolveHandle = [&](const FTypedElementHandle& Handle) -> bool
	{
		const FSMInstanceElementData* ElementData = Handle.GetData<FSMInstanceElementData>(true);
		if (!ElementData)
		{
			return false;
		}

		const FSMInstanceId InstanceId = FSMInstanceElementIdMap::Get().GetSMInstanceIdFromSMInstanceElementId(ElementData->InstanceElementId);
		if (!InstanceId || InstanceId.ISMComponent->GetOwner() != Collection)
		{
			return false;
		}

		if (!Collection->FindRecordForDerivedInstance(InstanceId.ISMComponent, InstanceId.InstanceIndex, OutRecordGuid))
		{
			return false;
		}
		if (const FDevKitDecalPlacementRecord* Record = Collection->Collection->FindRecord(OutRecordGuid))
		{
			OutRecord = *Record;
			return true;
		}
		OutRecordGuid.Invalidate();
		return false;
	};

	if (GCurrentLevelEditingViewportClient)
	{
		const FTypedElementListConstRef ElementsToManipulate = GCurrentLevelEditingViewportClient->GetElementsToManipulate();
		if (const TTypedElement<ITypedElementWorldInterface> SelectedElement =
			ElementsToManipulate->GetBottomElement<ITypedElementWorldInterface>())
		{
			if (ResolveHandle(SelectedElement))
			{
				return true;
			}
		}
	}

	if (FEditorModeTools* ModeTools = GetModeManager())
	{
		if (UTypedElementSelectionSet* SelectionSet = ModeTools->GetEditorSelectionSet())
		{
			const FTypedElementListRef NormalizedSelection = UEditorElementSubsystem::GetEditorNormalizedSelectionSet(*SelectionSet);
			const TTypedElement<ITypedElementWorldInterface> SelectedElement =
				UEditorElementSubsystem::GetLastSelectedEditorManipulableElement(
					NormalizedSelection,
					GLevelEditorModeTools().GetWidgetMode(),
					GetWorld());
			if (SelectedElement && ResolveHandle(SelectedElement))
			{
				return true;
			}
		}
	}

	return false;
}

bool UDevKitDecalCollectionEdMode::GetSelectedDeferredRecord(
	FGuid& OutRecordGuid,
	FDevKitDecalPlacementRecord& OutRecord) const
{
	OutRecordGuid.Invalidate();
	OutRecord = FDevKitDecalPlacementRecord();
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	UDecalComponent* Component = SelectedDeferredComponent.Get();
	if (!Collection || !Collection->Collection || !Component || !SelectedDeferredRecordGuid.IsValid())
	{
		return false;
	}

	FGuid ComponentGuid;
	if (!Collection->FindRecordForDerivedDeferred(Component, ComponentGuid)
		|| ComponentGuid != SelectedDeferredRecordGuid)
	{
		return false;
	}
	if (const FDevKitDecalPlacementRecord* Record = Collection->Collection->FindRecord(ComponentGuid))
	{
		OutRecordGuid = ComponentGuid;
		OutRecord = *Record;
		return true;
	}
	return false;
}

bool UDevKitDecalCollectionEdMode::TrySelectDeferredRecord(const FViewportClick& Click)
{
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !Collection->Collection)
	{
		return false;
	}

	const FVector RayOrigin = Click.GetOrigin();
	const FVector RayDirection = Click.GetDirection().GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	float BestDistanceSquared = TNumericLimits<float>::Max();
	int32 BestComponentIndex = INDEX_NONE;
	for (int32 ComponentIndex = 0; ComponentIndex < Collection->DerivedDeferredComponents.Num(); ++ComponentIndex)
	{
		UDecalComponent* Component = Collection->DerivedDeferredComponents[ComponentIndex];
		if (!Component || !Collection->DerivedDeferredGuids.IsValidIndex(ComponentIndex))
		{
			continue;
		}
		const FDevKitDecalPlacementRecord* Record = Collection->Collection->FindRecord(Collection->DerivedDeferredGuids[ComponentIndex]);
		if (!Record || !Record->bEnabled || !Record->Asset || Record->Asset->Backend != EDevKitDecalBackend::DeferredProjection)
		{
			continue;
		}

		const FVector ToDecal = Record->Transform.GetLocation() - RayOrigin;
		const float RayDistance = FVector::DotProduct(ToDecal, RayDirection);
		if (RayDistance < 0.f)
		{
			continue;
		}
		const FVector ClosestPoint = RayOrigin + RayDirection * RayDistance;
		const float SelectionRadius = FMath::Max(32.f, Record->DecalSize.GetAbsMax() * 0.5f);
		const float DistanceSquared = FVector::DistSquared(ClosestPoint, Record->Transform.GetLocation());
		if (DistanceSquared <= FMath::Square(SelectionRadius) && DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestComponentIndex = ComponentIndex;
		}
	}

	if (BestComponentIndex == INDEX_NONE)
	{
		return false;
	}

	SelectedDeferredComponent = Collection->DerivedDeferredComponents[BestComponentIndex];
	SelectedDeferredRecordGuid = Collection->DerivedDeferredGuids[BestComponentIndex];
	GLevelEditorModeTools().SetShowWidget(true);
	GLevelEditorModeTools().SetWidgetMode(UE::Widget::WM_Translate);
	return true;
}

bool UDevKitDecalCollectionEdMode::GetSelectedInstanceDetails(FDevKitDecalPlacementRecord& OutRecord) const
{
	FGuid RecordGuid;
	return GetSelectedInstanceRecord(RecordGuid, OutRecord);
}

bool UDevKitDecalCollectionEdMode::SetSelectedInstanceMaterialOverride(UMaterialInterface* Material)
{
	FGuid RecordGuid;
	FDevKitDecalPlacementRecord Record;
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !GetSelectedInstanceRecord(RecordGuid, Record))
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("PreviewInstanceMaterial", "预览单个贴花实例材质"));
	Collection->Modify();
	if (!Collection->SetRecordMaterialOverride(RecordGuid, Material))
	{
		return false;
	}
	Collection->RebuildDerivedRendering();
	Collection->MarkPackageDirty();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::BakeSelectedInstanceAsset(UDevKitDecalAsset* NewAsset)
{
	FGuid RecordGuid;
	FDevKitDecalPlacementRecord Record;
	ADevKitDecalCollectionActor* Collection = SessionCollection.Get();
	if (!Collection || !NewAsset || !GetSelectedInstanceRecord(RecordGuid, Record))
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("BakeInstanceMaterial", "烘焙单个贴花实例材质批次"));
	Collection->Modify();
	if (!Collection->RebindRecordAsset(RecordGuid, NewAsset))
	{
		return false;
	}
	Collection->RebuildDerivedRendering();
	Collection->MarkPackageDirty();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	return true;
}

bool UDevKitDecalCollectionEdMode::AllowWidgetMove()
{
	return SessionCollection.IsValid() && ShouldDrawWidget();
}

EAxisList::Type UDevKitDecalCollectionEdMode::GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const
{
	switch (InWidgetMode)
	{
	case UE::Widget::WM_Translate:
	case UE::Widget::WM_Rotate:
	case UE::Widget::WM_Scale:
		return EAxisList::XYZ;
	default:
		return EAxisList::None;
	}
}

FVector UDevKitDecalCollectionEdMode::GetWidgetLocation() const
{
	FTransform SelectedTransform;
	if (GetSelectedInstanceTransform(SelectedTransform))
	{
		// Keep the editor pivot in sync as well.  This is useful for the 2D and
		// translate/rotate-Z widgets, which read the mode manager pivot while
		// the regular gizmo uses this location directly.
		if (FEditorModeTools* ModeTools = GetModeManager())
		{
			ModeTools->SetPivotLocation(SelectedTransform.GetLocation(), false);
		}
		return SelectedTransform.GetLocation();
	}

	return GetModeManager() ? GetModeManager()->PivotLocation : FVector::ZeroVector;
}

bool UDevKitDecalCollectionEdMode::ShouldDrawWidget() const
{
	FTransform SelectedTransform;
	return SessionCollection.IsValid() && GetSelectedInstanceTransform(SelectedTransform);
}

bool UDevKitDecalCollectionEdMode::UsesTransformWidget() const
{
	return ShouldDrawWidget();
}

bool UDevKitDecalCollectionEdMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	return ShouldDrawWidget()
		&& (CheckMode == UE::Widget::WM_Translate
			|| CheckMode == UE::Widget::WM_Rotate
			|| CheckMode == UE::Widget::WM_Scale
			|| CheckMode == UE::Widget::WM_TranslateRotateZ);
}

void UDevKitDecalCollectionEdMode::ApplyAndExit()
{
	bAcceptOnExit = true;
	if (ADevKitDecalCollectionActor* Collection = GetActiveCollection())
	{
		Collection->ValidateCollection();
		Collection->RebuildDerivedRendering();
	}
	if (GLevelEditorModeTools().IsModeActive(EM_DevKitDecalCollection))
	{
		GLevelEditorModeTools().DeactivateMode(EM_DevKitDecalCollection);
	}
}

void UDevKitDecalCollectionEdMode::CancelAndExit()
{
	// Snapshot restore is supplied by the editor session subsystem in M2. We still exit explicitly;
	// no global transaction reset is performed here.
	bAcceptOnExit = false;
	if (GLevelEditorModeTools().IsModeActive(EM_DevKitDecalCollection))
	{
		GLevelEditorModeTools().DeactivateMode(EM_DevKitDecalCollection);
	}
}

#undef LOCTEXT_NAMESPACE
