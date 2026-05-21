#include "AssetUtility/StoryEncounterAssetFactories.h"

#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"

#define LOCTEXT_NAMESPACE "StoryEncounterAssetFactories"

UStoryEncounterGraphFactory::UStoryEncounterGraphFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStoryEncounterGraph::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

FText UStoryEncounterGraphFactory::GetDisplayName() const
{
	return LOCTEXT("GraphDisplayName", "剧情教学流程图");
}

UObject* UStoryEncounterGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UStoryEncounterGraph* Graph = NewObject<UStoryEncounterGraph>(
		InParent, UStoryEncounterGraph::StaticClass(), Name, Flags | RF_Transactional);
	Graph->EncounterId = Name;
	Graph->DisplayName = FText::FromName(Name);
	Graph->Name = Name.ToString();
	return Graph;
}

UStoryEncounterPointFactory::UStoryEncounterPointFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStoryEncounterPointDA::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

FText UStoryEncounterPointFactory::GetDisplayName() const
{
	return LOCTEXT("PointDisplayName", "剧情点DA");
}

UObject* UStoryEncounterPointFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>(
		InParent, UStoryEncounterPointDA::StaticClass(), Name, Flags | RF_Transactional);
	Point->EncounterId = TEXT("EM_NewEncounter");
	Point->NodeId = Name;
	Point->DisplayName = FText::FromName(Name);
	Point->Kind = EStoryEncounterNodeKind::System;
	Point->PlayerFacingEvent = LOCTEXT("DefaultPlayerFacingEvent", "填写玩家在这里看见或做了什么。");

	FStoryEncounterAction HintAction;
	HintAction.Kind = EStoryEncounterActionKind::WeakHint;
	HintAction.Title = LOCTEXT("DefaultHintTitle", "细微提示");
	HintAction.Body = LOCTEXT("DefaultHintBody", "填写玩家触发时看到的一句弱提示。");
	Point->Actions.Add(HintAction);
	return Point;
}

#undef LOCTEXT_NAMESPACE
