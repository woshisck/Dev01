// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "AssetEditor/ComboGraphDebugger.h"

#include "ComboGraphDelegates.h"
#include "ComboGraphEditorLog.h"
#include "UnrealEdGlobals.h"
#include "Components/ComboGraphSystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_StartGraph.h"
#include "AssetEditor/ComboGraphAssetEditor.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Selection.h"
#include "Graph/ComboGraph.h"

static void ForEachGameWorld(const TFunction<void(UWorld*)>& Func)
{
	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			Func(PlayWorld);
		}
	}
}

static bool AreAllGameWorldPaused()
{
	bool bPaused = true;
	ForEachGameWorld([&](const UWorld* World)
	{
		bPaused = bPaused && World->bDebugPauseExecution;
	});
	return bPaused;
}

FComboGraphDebugger::FComboGraphDebugger()
{
	ComboGraphAsset = nullptr;
	bIsPIEActive = false;

	FEditorDelegates::BeginPIE.AddRaw(this, &FComboGraphDebugger::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FComboGraphDebugger::OnEndPIE);
	FEditorDelegates::PausePIE.AddRaw(this, &FComboGraphDebugger::OnPausePIE);
}

FComboGraphDebugger::~FComboGraphDebugger()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
	FEditorDelegates::PausePIE.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
	FComboGraphDelegates::OnComboGraphStarted.RemoveAll(this);
	FComboGraphDelegates::OnComboGraphEnded.RemoveAll(this);
}

void FComboGraphDebugger::Tick(float DeltaTime)
{
	// CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger Tick"))
}

bool FComboGraphDebugger::IsTickable() const
{
	return IsDebuggerReady();
}

void FComboGraphDebugger::Setup(UComboGraph* InComboGraphAsset, TSharedRef<FComboGraphAssetEditor> InEditorOwner)
{
	EditorOwner = InEditorOwner;
	ComboGraphAsset = InComboGraphAsset;
	KnownInstances.Reset();
	KnownActors.Reset();

#if WITH_EDITORONLY_DATA
	if (IsPIESimulating())
	{
		OnBeginPIE(GEditor->bIsSimulatingInEditor);
	}
#endif
}

bool FComboGraphDebugger::IsDebuggerReady() const
{
	return bIsPIEActive;
}

bool FComboGraphDebugger::IsDebuggerRunning() const
{
	return ActorInstance.IsValid();
}

UComboGraphAbilityTask_StartGraph* FComboGraphDebugger::GetDebuggedTaskForSelectedActor() const
{
	AActor* SelectedActor = GetSelectedActor();
	if (!SelectedActor)
	{
		return nullptr;
	}

	UComboGraphAbilityTask_StartGraph* Result = nullptr;
	for (TWeakObjectPtr<UComboGraphAbilityTask_StartGraph> Task : KnownInstances)
	{
		if (!Task.IsValid())
		{
			continue;
		}

		AActor* Avatar = Task->GetAvatarActorFromActorInfo();
		if (Avatar == SelectedActor)
		{
			Result = Task.Get();
		}
	}

	return Result;
}

AActor* FComboGraphDebugger::GetSelectedActor() const
{
	return ActorInstance.IsValid() ? ActorInstance.Get() : nullptr;
}

void FComboGraphDebugger::OnBeginPIE(const bool bIsSimulating)
{
	bIsPIEActive = true;
	if(EditorOwner.IsValid())
	{
		const TSharedPtr<FComboGraphAssetEditor> EditorOwnerPtr = EditorOwner.Pin();
		EditorOwnerPtr->RegenerateMenusAndToolbars();
		EditorOwnerPtr->DebuggerUpdateGraph(true);
	}

	// remove these delegates first as we can get multiple calls to OnBeginPIE()
	USelection::SelectObjectEvent.RemoveAll(this);
	FComboGraphDelegates::OnComboGraphStarted.RemoveAll(this);
	FComboGraphDelegates::OnComboGraphEnded.RemoveAll(this);

	USelection::SelectObjectEvent.AddRaw(this, &FComboGraphDebugger::OnObjectSelected);
	FComboGraphDelegates::OnComboGraphStarted.AddRaw(this, &FComboGraphDebugger::OnComboGraphStarted);
	FComboGraphDelegates::OnComboGraphEnded.AddRaw(this, &FComboGraphDebugger::OnComboGraphEnded);
}

void FComboGraphDebugger::OnEndPIE(const bool bIsSimulating)
{
	bIsPIEActive = false;
	if (EditorOwner.IsValid())
	{
		const TSharedPtr<FComboGraphAssetEditor> EditorOwnerPtr = EditorOwner.Pin();
		EditorOwnerPtr->RegenerateMenusAndToolbars();
		EditorOwnerPtr->DebuggerUpdateGraph(false);
	}

	USelection::SelectObjectEvent.RemoveAll(this);
	FComboGraphDelegates::OnComboGraphStarted.RemoveAll(this);
	FComboGraphDelegates::OnComboGraphEnded.RemoveAll(this);
}

void FComboGraphDebugger::OnPausePIE(const bool bIsSimulating)
{
#if WITH_EDITORONLY_DATA
	// // We might have paused while executing a sub-tree, so make sure that the editor is showing the correct tree
#endif
}

void FComboGraphDebugger::OnObjectSelected(UObject* Object)
{
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger TestDebugger OnObjectSelected %s"), *GetNameSafe(Object))

	if (Object && Object->IsSelected())
	{
		AActor* Actor = Cast<AActor>(Object);
		if (Actor)
		{
			CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger TestDebugger OnObjectSelected Update actor instance %s"), *GetNameSafe(Actor))
			ActorInstance = Actor;
		}
	}
}

void FComboGraphDebugger::OnComboGraphStarted(const UComboGraphAbilityTask_StartGraph& InOwnerTask, const UComboGraph& InComboGraphAsset)
{
	const bool bAssetMatches = ComboGraphAsset && ComboGraphAsset == &InComboGraphAsset;
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger TestDebugger OnComboGraphStarted %s - %d (Ability: %s)"), *GetNameSafe(&InComboGraphAsset), InComboGraphAsset.GetUniqueID(), *GetNameSafe(&InOwnerTask))
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger TestDebugger OnComboGraphStarted bAssetMatches %s)"), bAssetMatches ? TEXT("true") : TEXT("false"))

	// start debugging if combo graph asset matches, and no other actor was selected
	if (!ActorInstance.IsValid() && bAssetMatches)
	{
		AActor* Avatar = InOwnerTask.GetAvatarActorFromActorInfo();

		CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger TestDebugger OnComboGraphStarted SetObjeSetActorBeingDebuggedctBeingDebugged %s"), *GetNameSafe(Avatar))
		ActorInstance = MakeWeakObjectPtr(Avatar);
	}

	// Update known instances if combo graph asset matches
	if (bAssetMatches)
	{
		const TWeakObjectPtr<UComboGraphAbilityTask_StartGraph> OwnerTaskInstance = const_cast<UComboGraphAbilityTask_StartGraph*>(&InOwnerTask);
		if (OwnerTaskInstance.IsValid())
		{
			KnownInstances.AddUnique(OwnerTaskInstance);
		}

		const TWeakObjectPtr<AActor> OwnerActor = InOwnerTask.GetAvatarActorFromActorInfo();
		if (OwnerActor.IsValid())
		{
			KnownActors.AddUnique(OwnerActor);
		}
	}
}

void FComboGraphDebugger::OnComboGraphEnded(const UComboGraphAbilityTask_StartGraph& InOwnerTask, const UComboGraph& InComboGraphAsset)
{
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger TestDebugger OnComboGraphEnded %s (Ability: %s)"), *GetNameSafe(&InComboGraphAsset), *GetNameSafe(&InOwnerTask))

	const bool bAssetMatches = ComboGraphAsset && ComboGraphAsset == &InComboGraphAsset;

	// Update known instances if combo graph asset matches
	if (!bAssetMatches)
	{
		return;
	}

	for (int32 i = KnownInstances.Num() - 1; i >= 0; i--)
	{
		UComboGraphAbilityTask_StartGraph* Task = KnownInstances[i].Get();
		if (Task == nullptr)
		{
			KnownInstances.RemoveAt(i);
			continue;
		}

		if (Task == &InOwnerTask)
		{
			KnownInstances.RemoveAt(i);
		}
	}
}

bool FComboGraphDebugger::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}

bool FComboGraphDebugger::IsPlaySessionRunning()
{
	return !AreAllGameWorldPaused();
}

bool FComboGraphDebugger::IsPIESimulating()
{
	return GEditor->bIsSimulatingInEditor || GEditor->PlayWorld;
}

bool FComboGraphDebugger::IsPIENotSimulating()
{
	return !GEditor->bIsSimulatingInEditor && (GEditor->PlayWorld == nullptr);
}

FString FComboGraphDebugger::GetDebuggedInstanceDesc() const
{
	AActor* Actor = ActorInstance.Get();
	if (Actor)
	{
		return DescribeInstance(*Actor);
	}

	return NSLOCTEXT("ComboGraphAssetEditor", "DebugActorNothingSelected", "No debug object selected").ToString();
}

FString FComboGraphDebugger::GetActorLabel(const AActor* InActor, const bool bIncludeNetModeSuffix, const bool bIncludeSpawnedContext) const
{
	if (!InActor)
	{
		return TEXT_NULL;
	}

	FString Context;
	FString Label = InActor->GetActorLabel();

	if (bIncludeNetModeSuffix)
	{
		// ReSharper disable once CppIncompleteSwitchStatement
		// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
		switch (InActor->GetNetMode())
		{
		case ENetMode::NM_Client:
			{
				Context = NSLOCTEXT("BlueprintEditor", "DebugWorldClient", "Client").ToString();

				FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(InActor->GetWorld());
				if (WorldContext != nullptr && WorldContext->PIEInstance > 1)
				{
					Context += TEXT(" ");
					Context += FText::AsNumber(WorldContext->PIEInstance - 1).ToString();
				}
			}
			break;

		case ENetMode::NM_ListenServer:
		case ENetMode::NM_DedicatedServer:
			Context = NSLOCTEXT("BlueprintEditor", "DebugWorldServer", "Server").ToString();
			break;
		}
	}

	if (bIncludeSpawnedContext)
	{
		if (!Context.IsEmpty())
		{
			Context += TEXT(", ");
		}

		Context += NSLOCTEXT("BlueprintEditor", "DebugObjectSpawned", "spawned").ToString();
	}

	if (!Context.IsEmpty())
	{
		Label = FString::Printf(TEXT("%s (%s)"), *Label, *Context);
	}

	return Label;
}

FString FComboGraphDebugger::DescribeInstance(const AActor& ActorToDescribe) const
{
	return FString::Printf(TEXT("%s"), *GetActorLabel(&ActorToDescribe));
}

void FComboGraphDebugger::OnInstanceSelectedInDropdown(AActor* SelectedActor)
{
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger OnInstanceSelectedInDropdown Actor: %s"), *GetNameSafe(SelectedActor))

	if (SelectedActor)
	{
		USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
		if (SelectedActors)
		{
			SelectedActors->DeselectAll();
		}

		ActorInstance = SelectedActor;

		if (SelectedActors)
		{
			SelectedActors->Select(SelectedActor);
		}
	}
}

void FComboGraphDebugger::GetMatchingInstances(TArray<UComboGraphAbilityTask_StartGraph*>& MatchingInstances, TArray<AActor*>& MatchingActors)
{
	for (int32 i = KnownInstances.Num() - 1; i >= 0; i--)
	{
		UComboGraphAbilityTask_StartGraph* Task = KnownInstances[i].Get();
		if (Task == nullptr)
		{
			KnownInstances.RemoveAt(i);
			continue;
		}

		MatchingInstances.Add(Task);
	}

	for (int32 i = KnownActors.Num() - 1; i >= 0; i--)
	{
		AActor* Actor = KnownActors[i].Get();
		if (Actor == nullptr)
		{
			KnownActors.RemoveAt(i);
			continue;
		}

		MatchingActors.Add(Actor);
	}
}
