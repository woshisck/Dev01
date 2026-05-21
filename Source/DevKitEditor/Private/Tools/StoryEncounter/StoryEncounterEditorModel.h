#pragma once

#include "CoreMinimal.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterMap.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryProductionBoard.h"

enum class EStoryEncounterWorkbenchMessageSeverity : uint8
{
	Info,
	Warning,
	Error,
};

struct FStoryEncounterWorkbenchMessage
{
	EStoryEncounterWorkbenchMessageSeverity Severity = EStoryEncounterWorkbenchMessageSeverity::Info;
	FString Message;
	TWeakObjectPtr<UObject> SourceAsset;
	FName RequirementId;
	FName EncounterId;
	FName NodeId;
};

struct FStoryEncounterProductionItem
{
	TWeakObjectPtr<UStoryProductionBoardDA> Board;
	FStoryProductionRow Row;
	int32 RowIndex = INDEX_NONE;
};

struct FStoryEncounterBoardItem
{
	TWeakObjectPtr<UStoryProductionBoardDA> Board;
	int32 RowCount = 0;
};

struct FStoryEncounterMapItem
{
	TWeakObjectPtr<UStoryEncounterMap> EncounterMap;
};

struct FStoryEncounterGraphItem
{
	TWeakObjectPtr<UStoryEncounterGraph> EncounterGraph;
	int32 NodeCount = 0;
};

struct FStoryEncounterPointItem
{
	TWeakObjectPtr<UStoryEncounterPointDA> EncounterPoint;
};

struct FStoryEncounterNodeItem
{
	TWeakObjectPtr<UStoryEncounterMap> EncounterMap;
	FStoryEncounterNode Node;
	int32 NodeIndex = INDEX_NONE;
};

class FStoryEncounterEditorModel
{
public:
	static TArray<FStoryEncounterBoardItem> BuildBoardItems(const TArray<UStoryProductionBoardDA*>& Boards);
	static TArray<FStoryEncounterProductionItem> BuildProductionItems(const TArray<UStoryProductionBoardDA*>& Boards);
	static TArray<FStoryEncounterMapItem> BuildMapItems(const TArray<UStoryEncounterMap*>& EncounterMaps);
	static TArray<FStoryEncounterGraphItem> BuildGraphItems(const TArray<UStoryEncounterGraph*>& EncounterGraphs);
	static TArray<FStoryEncounterPointItem> BuildPointItems(const TArray<UStoryEncounterPointDA*>& EncounterPoints);
	static TArray<FStoryEncounterNodeItem> BuildNodeItems(UStoryEncounterMap* EncounterMap);
	static TArray<FStoryEncounterWorkbenchMessage> Validate(const TArray<UStoryProductionBoardDA*>& Boards,
		const TArray<UStoryEncounterMap*>& EncounterMaps);
	static TArray<FStoryEncounterWorkbenchMessage> Validate(const TArray<UStoryProductionBoardDA*>& Boards,
		const TArray<UStoryEncounterMap*>& EncounterMaps,
		const TArray<UStoryEncounterGraph*>& EncounterGraphs,
		const TArray<UStoryEncounterPointDA*>& EncounterPoints);
	static void InitializeNewProductionBoard(UStoryProductionBoardDA* Board);
	static void InitializeNewEncounterMap(UStoryEncounterMap* EncounterMap, FName EncounterId);
	static void InitializeNewEncounterGraph(UStoryEncounterGraph* EncounterGraph, FName EncounterId);
	static void InitializeNewEncounterPoint(UStoryEncounterPointDA* EncounterPoint, FName EncounterId, FName NodeId);

	static FString SeverityToChinese(EStoryEncounterWorkbenchMessageSeverity Severity);
	static FString StatusToChinese(EStoryProductionStatus Status);
	static FString NodeKindToChinese(EStoryEncounterNodeKind Kind);
	static FString ActionKindToChinese(EStoryEncounterActionKind Kind);
	static FString ConditionKindToChinese(EStoryEncounterConditionKind Kind);
	static FString FirePolicyToChinese(EStoryEncounterFirePolicy Policy);
	static FString DescribeAction(FName EncounterId, const FStoryEncounterAction& Action);
	static FString DescribeCondition(FName EncounterId, const FStoryEncounterCondition& Condition);
};
