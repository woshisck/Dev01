#include "Tools/StoryEncounter/StoryEncounterEditorModel.h"

#include "LevelFlow/LevelFlowAsset.h"
#include "Story/Encounter/StoryEncounterGraphEdge.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

namespace
{
void AddWorkbenchMessage(TArray<FStoryEncounterWorkbenchMessage>& OutMessages,
	EStoryEncounterWorkbenchMessageSeverity Severity,
	const FString& Message,
	UObject* SourceAsset,
	FName RequirementId = NAME_None,
	FName EncounterId = NAME_None,
	FName NodeId = NAME_None)
{
	FStoryEncounterWorkbenchMessage Entry;
	Entry.Severity = Severity;
	Entry.Message = Message;
	Entry.SourceAsset = SourceAsset;
	Entry.RequirementId = RequirementId;
	Entry.EncounterId = EncounterId;
	Entry.NodeId = NodeId;
	OutMessages.Add(Entry);
}

bool IsStatusAtLeast(EStoryProductionStatus Actual, EStoryProductionStatus Expected)
{
	return static_cast<uint8>(Actual) >= static_cast<uint8>(Expected);
}

FString TagOrDash(const FGameplayTag& Tag)
{
	return Tag.IsValid() ? Tag.ToString() : TEXT("-");
}
}

TArray<FStoryEncounterBoardItem> FStoryEncounterEditorModel::BuildBoardItems(
	const TArray<UStoryProductionBoardDA*>& Boards)
{
	TArray<FStoryEncounterBoardItem> Items;
	for (UStoryProductionBoardDA* Board : Boards)
	{
		if (!Board)
		{
			continue;
		}

		FStoryEncounterBoardItem Item;
		Item.Board = Board;
		Item.RowCount = Board->Rows.Num();
		Items.Add(Item);
	}

	Items.Sort([](const FStoryEncounterBoardItem& Left, const FStoryEncounterBoardItem& Right)
	{
		const UStoryProductionBoardDA* LeftBoard = Left.Board.Get();
		const UStoryProductionBoardDA* RightBoard = Right.Board.Get();
		return LeftBoard && RightBoard ? LeftBoard->GetName() < RightBoard->GetName() : LeftBoard != nullptr;
	});
	return Items;
}

TArray<FStoryEncounterProductionItem> FStoryEncounterEditorModel::BuildProductionItems(
	const TArray<UStoryProductionBoardDA*>& Boards)
{
	TArray<FStoryEncounterProductionItem> Items;
	for (UStoryProductionBoardDA* Board : Boards)
	{
		if (!Board)
		{
			continue;
		}

		for (int32 RowIndex = 0; RowIndex < Board->Rows.Num(); ++RowIndex)
		{
			FStoryEncounterProductionItem Item;
			Item.Board = Board;
			Item.Row = Board->Rows[RowIndex];
			Item.RowIndex = RowIndex;
			Items.Add(Item);
		}
	}

	Items.Sort([](const FStoryEncounterProductionItem& Left, const FStoryEncounterProductionItem& Right)
	{
		if (Left.Row.FlowId == Right.Row.FlowId)
		{
			return Left.Row.RequirementId.LexicalLess(Right.Row.RequirementId);
		}
		return Left.Row.FlowId.LexicalLess(Right.Row.FlowId);
	});
	return Items;
}

TArray<FStoryEncounterMapItem> FStoryEncounterEditorModel::BuildMapItems(const TArray<UStoryEncounterMap*>& EncounterMaps)
{
	TArray<FStoryEncounterMapItem> Items;
	for (UStoryEncounterMap* EncounterMap : EncounterMaps)
	{
		if (!EncounterMap)
		{
			continue;
		}

		FStoryEncounterMapItem Item;
		Item.EncounterMap = EncounterMap;
		Items.Add(Item);
	}

	Items.Sort([](const FStoryEncounterMapItem& Left, const FStoryEncounterMapItem& Right)
	{
		const UStoryEncounterMap* LeftMap = Left.EncounterMap.Get();
		const UStoryEncounterMap* RightMap = Right.EncounterMap.Get();
		return LeftMap && RightMap ? LeftMap->EncounterId.LexicalLess(RightMap->EncounterId) : LeftMap != nullptr;
	});
	return Items;
}

TArray<FStoryEncounterGraphItem> FStoryEncounterEditorModel::BuildGraphItems(
	const TArray<UStoryEncounterGraph*>& EncounterGraphs)
{
	TArray<FStoryEncounterGraphItem> Items;
	for (UStoryEncounterGraph* EncounterGraph : EncounterGraphs)
	{
		if (!EncounterGraph)
		{
			continue;
		}

		FStoryEncounterGraphItem Item;
		Item.EncounterGraph = EncounterGraph;
		Item.NodeCount = EncounterGraph->AllNodes.Num();
		Items.Add(Item);
	}

	Items.Sort([](const FStoryEncounterGraphItem& Left, const FStoryEncounterGraphItem& Right)
	{
		const UStoryEncounterGraph* LeftGraph = Left.EncounterGraph.Get();
		const UStoryEncounterGraph* RightGraph = Right.EncounterGraph.Get();
		if (LeftGraph && RightGraph)
		{
			return LeftGraph->EncounterId.LexicalLess(RightGraph->EncounterId);
		}
		return LeftGraph != nullptr;
	});
	return Items;
}

TArray<FStoryEncounterPointItem> FStoryEncounterEditorModel::BuildPointItems(
	const TArray<UStoryEncounterPointDA*>& EncounterPoints)
{
	TArray<FStoryEncounterPointItem> Items;
	for (UStoryEncounterPointDA* EncounterPoint : EncounterPoints)
	{
		if (!EncounterPoint)
		{
			continue;
		}

		FStoryEncounterPointItem Item;
		Item.EncounterPoint = EncounterPoint;
		Items.Add(Item);
	}

	Items.Sort([](const FStoryEncounterPointItem& Left, const FStoryEncounterPointItem& Right)
	{
		const UStoryEncounterPointDA* LeftPoint = Left.EncounterPoint.Get();
		const UStoryEncounterPointDA* RightPoint = Right.EncounterPoint.Get();
		if (!LeftPoint || !RightPoint)
		{
			return LeftPoint != nullptr;
		}
		if (LeftPoint->EncounterId == RightPoint->EncounterId)
		{
			return LeftPoint->GetStableNodeId().LexicalLess(RightPoint->GetStableNodeId());
		}
		return LeftPoint->EncounterId.LexicalLess(RightPoint->EncounterId);
	});
	return Items;
}

TArray<FStoryEncounterNodeItem> FStoryEncounterEditorModel::BuildNodeItems(UStoryEncounterMap* EncounterMap)
{
	TArray<FStoryEncounterNodeItem> Items;
	if (!EncounterMap)
	{
		return Items;
	}

	for (int32 NodeIndex = 0; NodeIndex < EncounterMap->Nodes.Num(); ++NodeIndex)
	{
		FStoryEncounterNodeItem Item;
		Item.EncounterMap = EncounterMap;
		Item.Node = EncounterMap->Nodes[NodeIndex];
		Item.NodeIndex = NodeIndex;
		Items.Add(Item);
	}
	return Items;
}

TArray<FStoryEncounterWorkbenchMessage> FStoryEncounterEditorModel::Validate(
	const TArray<UStoryProductionBoardDA*>& Boards,
	const TArray<UStoryEncounterMap*>& EncounterMaps)
{
	TArray<FStoryEncounterWorkbenchMessage> Messages;

	TMap<FName, UStoryEncounterMap*> MapsById;
	TSet<FName> DuplicateEncounterIds;
	for (UStoryEncounterMap* EncounterMap : EncounterMaps)
	{
		if (!EncounterMap)
		{
			continue;
		}

		if (EncounterMap->EncounterId.IsNone())
		{
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
				FString::Printf(TEXT("流程资产 %s 缺少 EncounterId。"), *EncounterMap->GetName()),
				EncounterMap);
			continue;
		}

		if (MapsById.Contains(EncounterMap->EncounterId))
		{
			DuplicateEncounterIds.Add(EncounterMap->EncounterId);
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
				FString::Printf(TEXT("流程ID重复：%s。每张 StoryEncounterMap 必须有唯一 EncounterId。"),
					*EncounterMap->EncounterId.ToString()),
				EncounterMap, NAME_None, EncounterMap->EncounterId);
			continue;
		}

		MapsById.Add(EncounterMap->EncounterId, EncounterMap);
	}

	for (UStoryEncounterMap* EncounterMap : EncounterMaps)
	{
		if (!EncounterMap || EncounterMap->EncounterId.IsNone() || DuplicateEncounterIds.Contains(EncounterMap->EncounterId))
		{
			continue;
		}

		TSet<FName> NodeIds;
		for (int32 NodeIndex = 0; NodeIndex < EncounterMap->Nodes.Num(); ++NodeIndex)
		{
			const FStoryEncounterNode& Node = EncounterMap->Nodes[NodeIndex];
			const FString NodeLabel = Node.DisplayName.IsEmpty()
				? FString::Printf(TEXT("第 %d 个节点"), NodeIndex + 1)
				: Node.DisplayName.ToString();

			if (Node.NodeId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("%s：%s 缺少 NodeId。"), *EncounterMap->EncounterId.ToString(), *NodeLabel),
					EncounterMap, NAME_None, EncounterMap->EncounterId);
				continue;
			}

			if (NodeIds.Contains(Node.NodeId))
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("%s：节点ID重复：%s。"), *EncounterMap->EncounterId.ToString(), *Node.NodeId.ToString()),
					EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
			}
			NodeIds.Add(Node.NodeId);

			if (!Node.NextNodeId.IsNone() && !EncounterMap->FindNode(Node.NextNodeId))
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("%s/%s 的下一个节点 %s 不存在。"),
						*EncounterMap->EncounterId.ToString(),
						*Node.NodeId.ToString(),
						*Node.NextNodeId.ToString()),
					EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
			}

			if (Node.Condition.Kind == EStoryEncounterConditionKind::ProgressMissing
				|| Node.Condition.Kind == EStoryEncounterConditionKind::ProgressCompleted)
			{
				if (Node.Condition.ProgressKey.IsNone())
				{
					AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
						FString::Printf(TEXT("%s/%s 的触发条件需要填写进度键。"),
							*EncounterMap->EncounterId.ToString(),
							*Node.NodeId.ToString()),
						EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
				}
			}
			else if (Node.Condition.Kind == EStoryEncounterConditionKind::FeatureUnlocked
				&& !Node.Condition.FeatureTag.IsValid())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("%s/%s 的触发条件需要填写功能Tag。"),
						*EncounterMap->EncounterId.ToString(),
						*Node.NodeId.ToString()),
					EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
			}

			if (Node.Actions.Num() == 0)
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
					FString::Printf(TEXT("%s/%s 没有动作；玩家触发后不会看到反馈。"),
						*EncounterMap->EncounterId.ToString(),
						*Node.NodeId.ToString()),
					EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
			}

			for (int32 ActionIndex = 0; ActionIndex < Node.Actions.Num(); ++ActionIndex)
			{
				const FStoryEncounterAction& Action = Node.Actions[ActionIndex];
				const FString Prefix = FString::Printf(TEXT("%s/%s 动作%d"),
					*EncounterMap->EncounterId.ToString(),
					*Node.NodeId.ToString(),
					ActionIndex + 1);

				switch (Action.Kind)
				{
				case EStoryEncounterActionKind::WeakHint:
				case EStoryEncounterActionKind::Dialogue:
					if (Action.Body.IsEmpty())
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
							FString::Printf(TEXT("%s 缺少显示文本。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::TutorialPopup:
					if (Action.TutorialEventId.IsNone())
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 是教程弹窗，但没有填写教程事件ID。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::RecordProgress:
					if (Action.ProgressKey.IsNone())
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 是记录进度，但没有填写进度键。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::UnlockFeature:
					if (!Action.FeatureTag.IsValid())
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 是解锁功能，但没有填写功能Tag。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::SetQuestObjective:
					if (!Action.QuestTaskTag.IsValid())
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 是设置目标，但没有填写任务Tag。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::TeleportToNode:
					if (Action.TargetNodeId.IsNone() || !EncounterMap->FindNode(Action.TargetNodeId))
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 要跳到节点，但目标节点不存在。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::PlayLevelFlow:
					if (!Action.LevelFlow)
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 是播放流程，但没有指定 LevelFlow。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				case EStoryEncounterActionKind::SetActorEnabled:
					if (Action.TargetActorName.IsNone() && Action.TargetActorTag.IsNone())
					{
						AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
							FString::Printf(TEXT("%s 是关卡对象控制，但没有填写 TargetActorName 或 TargetActorTag。"), *Prefix),
							EncounterMap, NAME_None, EncounterMap->EncounterId, Node.NodeId);
					}
					break;

				default:
					break;
				}
			}
		}
	}

	TSet<FName> RequirementIds;
	for (UStoryProductionBoardDA* Board : Boards)
	{
		if (!Board)
		{
			continue;
		}

		for (int32 RowIndex = 0; RowIndex < Board->Rows.Num(); ++RowIndex)
		{
			const FStoryProductionRow& Row = Board->Rows[RowIndex];
			const FString RowName = Row.RequirementId.IsNone()
				? FString::Printf(TEXT("%s 第 %d 行"), *Board->GetName(), RowIndex + 1)
				: Row.RequirementId.ToString();

			if (Row.RequirementId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("%s 缺少需求ID。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
			else if (RequirementIds.Contains(Row.RequirementId))
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求ID重复：%s。"), *Row.RequirementId.ToString()),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
			RequirementIds.Add(Row.RequirementId);

			if (Row.EncounterId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 缺少流程ID。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
				continue;
			}

			UStoryEncounterMap* const* MatchingMap = MapsById.Find(Row.EncounterId);
			if (!MatchingMap)
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 指向的流程 %s 不存在。"),
						*RowName,
						*Row.EncounterId.ToString()),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
				continue;
			}

			if (Row.NodeId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 缺少节点ID。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
			else if (!(*MatchingMap)->FindNode(Row.NodeId))
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 指向的节点 %s 不存在。"),
						*RowName,
						*Row.NodeId.ToString()),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}

			if (IsStatusAtLeast(Row.Status, EStoryProductionStatus::PlacedInLevel) && Row.LevelName.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
					FString::Printf(TEXT("需求 %s 已标记为关卡放置，但没有填写关卡名。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}

			if (IsStatusAtLeast(Row.Status, EStoryProductionStatus::Connected) && Row.PlacementName.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
					FString::Printf(TEXT("需求 %s 已标记为接入触发，但没有填写放置名。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
		}
	}

	Messages.Sort([](const FStoryEncounterWorkbenchMessage& Left, const FStoryEncounterWorkbenchMessage& Right)
	{
		if (Left.Severity != Right.Severity)
		{
			return static_cast<uint8>(Left.Severity) > static_cast<uint8>(Right.Severity);
		}
		return Left.Message < Right.Message;
	});
	return Messages;
}

TArray<FStoryEncounterWorkbenchMessage> FStoryEncounterEditorModel::Validate(
	const TArray<UStoryProductionBoardDA*>& Boards,
	const TArray<UStoryEncounterMap*>& EncounterMaps,
	const TArray<UStoryEncounterGraph*>& EncounterGraphs,
	const TArray<UStoryEncounterPointDA*>& EncounterPoints)
{
	TArray<FStoryEncounterWorkbenchMessage> Messages;

	TSet<FString> ValidStoryPointKeys;
	auto MakePointKey = [](FName EncounterId, FName NodeId)
	{
		return FString::Printf(TEXT("%s/%s"), *EncounterId.ToString(), *NodeId.ToString());
	};

	for (UStoryEncounterMap* EncounterMap : EncounterMaps)
	{
		if (!EncounterMap || EncounterMap->EncounterId.IsNone())
		{
			continue;
		}

		for (const FStoryEncounterNode& Node : EncounterMap->Nodes)
		{
			if (!Node.NodeId.IsNone())
			{
				ValidStoryPointKeys.Add(MakePointKey(EncounterMap->EncounterId, Node.NodeId));
			}
		}
	}

	TSet<FString> SeenPointKeys;
	for (UStoryEncounterPointDA* EncounterPoint : EncounterPoints)
	{
		if (!EncounterPoint)
		{
			continue;
		}

		const FName NodeId = EncounterPoint->GetStableNodeId();
		if (EncounterPoint->EncounterId.IsNone())
		{
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
				FString::Printf(TEXT("剧情点资产 %s 缺少流程ID。"), *EncounterPoint->GetName()),
				EncounterPoint);
			continue;
		}
		if (NodeId.IsNone())
		{
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
				FString::Printf(TEXT("剧情点资产 %s 缺少节点ID。"), *EncounterPoint->GetName()),
				EncounterPoint, NAME_None, EncounterPoint->EncounterId);
			continue;
		}

		const FString PointKey = MakePointKey(EncounterPoint->EncounterId, NodeId);
		if (SeenPointKeys.Contains(PointKey))
		{
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
				FString::Printf(TEXT("剧情点重复：%s。每个流程/节点只能有一个剧情点DA。"), *PointKey),
				EncounterPoint, NAME_None, EncounterPoint->EncounterId, NodeId);
		}
		SeenPointKeys.Add(PointKey);
		ValidStoryPointKeys.Add(PointKey);

		if (EncounterPoint->Actions.Num() == 0)
		{
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
				FString::Printf(TEXT("%s 没有触发结果，玩家触发后不会看到反馈。"), *PointKey),
				EncounterPoint, NAME_None, EncounterPoint->EncounterId, NodeId);
		}
	}

	for (UStoryEncounterGraph* EncounterGraph : EncounterGraphs)
	{
		if (!EncounterGraph)
		{
			continue;
		}

		if (EncounterGraph->EncounterId.IsNone())
		{
			AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
				FString::Printf(TEXT("流程图资产 %s 缺少流程ID。"), *EncounterGraph->GetName()),
				EncounterGraph);
		}

		for (UGenericGraphNode* RawNode : EncounterGraph->AllNodes)
		{
			const UStoryEncounterGraphNode* GraphNode = Cast<UStoryEncounterGraphNode>(RawNode);
			if (!GraphNode)
			{
				continue;
			}

			if (!GraphNode->Point)
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
					FString::Printf(TEXT("流程图 %s 有未绑定剧情点DA的节点：%s。"),
						*EncounterGraph->GetName(),
						*GraphNode->GetStoryNodeId().ToString()),
					EncounterGraph, NAME_None, EncounterGraph->EncounterId, GraphNode->GetStoryNodeId());
			}
		}
	}

	TSet<FName> RequirementIds;
	for (UStoryProductionBoardDA* Board : Boards)
	{
		if (!Board)
		{
			continue;
		}

		for (int32 RowIndex = 0; RowIndex < Board->Rows.Num(); ++RowIndex)
		{
			const FStoryProductionRow& Row = Board->Rows[RowIndex];
			const FString RowName = Row.RequirementId.IsNone()
				? FString::Printf(TEXT("%s 第%d行"), *Board->GetName(), RowIndex + 1)
				: Row.RequirementId.ToString();

			if (Row.RequirementId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("%s 缺少需求ID。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
			else if (RequirementIds.Contains(Row.RequirementId))
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求ID重复：%s。"), *Row.RequirementId.ToString()),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
			RequirementIds.Add(Row.RequirementId);

			if (Row.EncounterId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 缺少流程ID。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
				continue;
			}
			if (Row.NodeId.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 缺少剧情点节点ID。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
				continue;
			}

			const FString PointKey = MakePointKey(Row.EncounterId, Row.NodeId);
			if (!ValidStoryPointKeys.Contains(PointKey))
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Error,
					FString::Printf(TEXT("需求 %s 指向的剧情点 %s 不存在。"), *RowName, *PointKey),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}

			if (IsStatusAtLeast(Row.Status, EStoryProductionStatus::PlacedInLevel) && Row.LevelName.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
					FString::Printf(TEXT("需求 %s 已标记为关卡放置，但没有填写关卡名。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}

			if (IsStatusAtLeast(Row.Status, EStoryProductionStatus::Connected) && Row.PlacementName.IsNone())
			{
				AddWorkbenchMessage(Messages, EStoryEncounterWorkbenchMessageSeverity::Warning,
					FString::Printf(TEXT("需求 %s 已标记为接入触发，但没有填写放置名。"), *RowName),
					Board, Row.RequirementId, Row.EncounterId, Row.NodeId);
			}
		}
	}

	Messages.Sort([](const FStoryEncounterWorkbenchMessage& Left, const FStoryEncounterWorkbenchMessage& Right)
	{
		if (Left.Severity != Right.Severity)
		{
			return static_cast<uint8>(Left.Severity) > static_cast<uint8>(Right.Severity);
		}
		return Left.Message < Right.Message;
	});
	return Messages;
}

void FStoryEncounterEditorModel::InitializeNewProductionBoard(UStoryProductionBoardDA* Board)
{
	if (!Board)
	{
		return;
	}

	Board->Rows.Reset();
}

void FStoryEncounterEditorModel::InitializeNewEncounterMap(UStoryEncounterMap* EncounterMap, FName EncounterId)
{
	if (!EncounterMap)
	{
		return;
	}

	EncounterMap->EncounterId = EncounterId;
	EncounterMap->DisplayName = FText::FromName(EncounterId);
	EncounterMap->Nodes.Reset();

	FStoryEncounterNode EntryNode;
	EntryNode.NodeId = TEXT("entry");
	EntryNode.DisplayName = FText::FromString(TEXT("入口"));
	EntryNode.Kind = EStoryEncounterNodeKind::System;
	EntryNode.PlayerFacingEvent = FText::FromString(TEXT("填写玩家进入这个剧情/教程流程时看见或做了什么。"));
	EntryNode.FirePolicy = EStoryEncounterFirePolicy::Once;

	FStoryEncounterAction HintAction;
	HintAction.Kind = EStoryEncounterActionKind::WeakHint;
	HintAction.Title = FText::FromString(TEXT("底部操作提示条"));
	HintAction.Body = FText::FromString(TEXT("填写玩家第一次触发时在底部看到的操作提示。"));
	EntryNode.Actions.Add(HintAction);

	EncounterMap->Nodes.Add(EntryNode);
}

void FStoryEncounterEditorModel::InitializeNewEncounterGraph(UStoryEncounterGraph* EncounterGraph, FName EncounterId)
{
	if (!EncounterGraph)
	{
		return;
	}

	EncounterGraph->EncounterId = EncounterId;
	EncounterGraph->DisplayName = FText::FromName(EncounterId);
	EncounterGraph->Description = FText::FromString(TEXT("在流程图中右键创建剧情点节点，并把节点绑定到剧情点DA。"));
	EncounterGraph->Name = EncounterId.ToString();
	EncounterGraph->NodeType = UStoryEncounterGraphNode::StaticClass();
	EncounterGraph->EdgeType = UStoryEncounterGraphEdge::StaticClass();
	EncounterGraph->bEdgeEnabled = true;

#if WITH_EDITORONLY_DATA
	EncounterGraph->bCanRenameNode = true;
	EncounterGraph->bCanBeCyclical = false;
#endif
}

void FStoryEncounterEditorModel::InitializeNewEncounterPoint(
	UStoryEncounterPointDA* EncounterPoint, FName EncounterId, FName NodeId)
{
	if (!EncounterPoint)
	{
		return;
	}

	EncounterPoint->EncounterId = EncounterId;
	EncounterPoint->NodeId = NodeId.IsNone() ? TEXT("entry") : NodeId;
	EncounterPoint->DisplayName = FText::FromName(EncounterPoint->NodeId);
	EncounterPoint->Kind = EStoryEncounterNodeKind::System;
	EncounterPoint->PlayerFacingEvent = FText::FromString(TEXT("填写玩家在这里看见或做了什么。"));
	EncounterPoint->FirePolicy = EStoryEncounterFirePolicy::Once;
	EncounterPoint->Actions.Reset();

	FStoryEncounterAction HintAction;
	HintAction.Kind = EStoryEncounterActionKind::WeakHint;
	HintAction.Title = FText::FromString(TEXT("底部操作提示条"));
	HintAction.Body = FText::FromString(TEXT("填写玩家触发时在底部看到的一句操作提示。"));
	EncounterPoint->Actions.Add(HintAction);
}

FString FStoryEncounterEditorModel::SeverityToChinese(EStoryEncounterWorkbenchMessageSeverity Severity)
{
	switch (Severity)
	{
	case EStoryEncounterWorkbenchMessageSeverity::Error:
		return TEXT("错误");
	case EStoryEncounterWorkbenchMessageSeverity::Warning:
		return TEXT("提醒");
	case EStoryEncounterWorkbenchMessageSeverity::Info:
	default:
		return TEXT("信息");
	}
}

FString FStoryEncounterEditorModel::StatusToChinese(EStoryProductionStatus Status)
{
	switch (Status)
	{
	case EStoryProductionStatus::Designed:
		return TEXT("设计中");
	case EStoryProductionStatus::InSimulator:
		return TEXT("已进模拟器");
	case EStoryProductionStatus::InEncounterMap:
		return TEXT("已进流程图");
	case EStoryProductionStatus::PlacedInLevel:
		return TEXT("已在关卡放置");
	case EStoryProductionStatus::Connected:
		return TEXT("已接入触发");
	case EStoryProductionStatus::PIEVerified:
		return TEXT("已PIE验证");
	case EStoryProductionStatus::Done:
		return TEXT("完成");
	default:
		return TEXT("未知");
	}
}

FString FStoryEncounterEditorModel::NodeKindToChinese(EStoryEncounterNodeKind Kind)
{
	switch (Kind)
	{
	case EStoryEncounterNodeKind::Area:
		return TEXT("区域");
	case EStoryEncounterNodeKind::Object:
		return TEXT("物件");
	case EStoryEncounterNodeKind::NPC:
		return TEXT("NPC");
	case EStoryEncounterNodeKind::System:
		return TEXT("系统");
	case EStoryEncounterNodeKind::Death:
		return TEXT("死亡");
	case EStoryEncounterNodeKind::Feature:
		return TEXT("功能");
	default:
		return TEXT("未知");
	}
}

FString FStoryEncounterEditorModel::ActionKindToChinese(EStoryEncounterActionKind Kind)
{
	switch (Kind)
	{
	case EStoryEncounterActionKind::WeakHint:
		return TEXT("底部操作提示条");
	case EStoryEncounterActionKind::Dialogue:
		return TEXT("对话");
	case EStoryEncounterActionKind::TutorialPopup:
		return TEXT("教程弹窗");
	case EStoryEncounterActionKind::RecordProgress:
		return TEXT("记录进度");
	case EStoryEncounterActionKind::UnlockFeature:
		return TEXT("解锁功能");
	case EStoryEncounterActionKind::SetQuestObjective:
		return TEXT("设置目标");
	case EStoryEncounterActionKind::TeleportToNode:
		return TEXT("跳到节点");
	case EStoryEncounterActionKind::PlayLevelFlow:
		return TEXT("播放流程");
	case EStoryEncounterActionKind::SetActorEnabled:
		return TEXT("关卡对象控制");
	default:
		return TEXT("未知");
	}
}

FString FStoryEncounterEditorModel::ConditionKindToChinese(EStoryEncounterConditionKind Kind)
{
	switch (Kind)
	{
	case EStoryEncounterConditionKind::None:
		return TEXT("无条件");
	case EStoryEncounterConditionKind::ProgressMissing:
		return TEXT("还没有发生过");
	case EStoryEncounterConditionKind::ProgressCompleted:
		return TEXT("已经发生过");
	case EStoryEncounterConditionKind::RunCountAtLeast:
		return TEXT("第N局之后");
	case EStoryEncounterConditionKind::FeatureUnlocked:
		return TEXT("功能已解锁");
	default:
		return TEXT("未知");
	}
}

FString FStoryEncounterEditorModel::FirePolicyToChinese(EStoryEncounterFirePolicy Policy)
{
	switch (Policy)
	{
	case EStoryEncounterFirePolicy::Once:
		return TEXT("只触发一次");
	case EStoryEncounterFirePolicy::Repeat:
		return TEXT("可重复触发");
	case EStoryEncounterFirePolicy::OncePerRun:
		return TEXT("每局一次");
	default:
		return TEXT("未知");
	}
}

FString FStoryEncounterEditorModel::DescribeAction(FName EncounterId, const FStoryEncounterAction& Action)
{
	const FString KindText = ActionKindToChinese(Action.Kind);
	switch (Action.Kind)
	{
	case EStoryEncounterActionKind::WeakHint:
	case EStoryEncounterActionKind::Dialogue:
		return FString::Printf(TEXT("%s：%s - %s"),
			*KindText,
			*Action.Title.ToString(),
			*Action.Body.ToString());
	case EStoryEncounterActionKind::TutorialPopup:
		return FString::Printf(TEXT("%s：%s%s"),
			*KindText,
			*Action.TutorialEventId.ToString(),
			Action.bPauseGame ? TEXT("（暂停）") : TEXT("（不暂停）"));
	case EStoryEncounterActionKind::RecordProgress:
		return FString::Printf(TEXT("%s：%s（隐藏Tag：%s）"),
			*KindText,
			*Action.ProgressLabel.ToString(),
			*UStoryEncounterRuntimeSubsystem::MakeProgressTagName(EncounterId, Action.ProgressKey));
	case EStoryEncounterActionKind::UnlockFeature:
		return FString::Printf(TEXT("%s：%s"), *KindText, *TagOrDash(Action.FeatureTag));
	case EStoryEncounterActionKind::SetQuestObjective:
		return FString::Printf(TEXT("%s：%s - %s"), *KindText, *TagOrDash(Action.QuestTaskTag), *Action.Body.ToString());
	case EStoryEncounterActionKind::TeleportToNode:
		return FString::Printf(TEXT("%s：%s"), *KindText, *Action.TargetNodeId.ToString());
	case EStoryEncounterActionKind::PlayLevelFlow:
		return FString::Printf(TEXT("%s：%s"), *KindText, Action.LevelFlow ? *Action.LevelFlow->GetName() : TEXT("-"));
	case EStoryEncounterActionKind::SetActorEnabled:
	{
		const FString NamePart = Action.TargetActorName.IsNone()
			? FString()
			: FString::Printf(TEXT("Name=%s "), *Action.TargetActorName.ToString());
		const FString TagPart = Action.TargetActorTag.IsNone()
			? FString()
			: FString::Printf(TEXT("Tag=%s"), *Action.TargetActorTag.ToString());
		return FString::Printf(TEXT("%s：%s%s -> %s"),
			*KindText,
			*NamePart,
			*TagPart,
			Action.bActorEnabled ? TEXT("启用") : TEXT("隐藏/禁用"));
	}
	default:
		return KindText;
	}
}

FString FStoryEncounterEditorModel::DescribeCondition(FName EncounterId, const FStoryEncounterCondition& Condition)
{
	const FString KindText = ConditionKindToChinese(Condition.Kind);
	switch (Condition.Kind)
	{
	case EStoryEncounterConditionKind::ProgressMissing:
	case EStoryEncounterConditionKind::ProgressCompleted:
		return FString::Printf(TEXT("%s：%s（隐藏Tag：%s）"),
			*KindText,
			*Condition.ProgressLabel.ToString(),
			*UStoryEncounterRuntimeSubsystem::MakeProgressTagName(EncounterId, Condition.ProgressKey));
	case EStoryEncounterConditionKind::RunCountAtLeast:
		return FString::Printf(TEXT("%s：%d"), *KindText, Condition.RunCount);
	case EStoryEncounterConditionKind::FeatureUnlocked:
		return FString::Printf(TEXT("%s：%s"), *KindText, *TagOrDash(Condition.FeatureTag));
	case EStoryEncounterConditionKind::None:
	default:
		return KindText;
	}
}
