#include "RuneEditor/RuneEditorValidation.h"

#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#define LOCTEXT_NAMESPACE "RuneEditorValidation"

namespace
{
	FString SeverityPrefix(ERuneEditorValidationSeverity Severity)
	{
		switch (Severity)
		{
		case ERuneEditorValidationSeverity::Error:
			return TEXT("[错误]");
		case ERuneEditorValidationSeverity::Warning:
			return TEXT("[警告]");
		case ERuneEditorValidationSeverity::Info:
		default:
			return TEXT("[信息]");
		}
	}

	void AddIssue(FRuneEditorValidationResult& Result, ERuneEditorValidationSeverity Severity, const FText& Message)
	{
		Result.Issues.Emplace(Severity, Message);
	}
}

bool FRuneEditorValidationResult::HasErrors() const
{
	return CountSeverity(ERuneEditorValidationSeverity::Error) > 0;
}

int32 FRuneEditorValidationResult::CountSeverity(ERuneEditorValidationSeverity Severity) const
{
	int32 Count = 0;
	for (const FRuneEditorValidationIssue& Issue : Issues)
	{
		if (Issue.Severity == Severity)
		{
			++Count;
		}
	}
	return Count;
}

FText FRuneEditorValidationResult::BuildSummaryText() const
{
	const int32 ErrorCount = CountSeverity(ERuneEditorValidationSeverity::Error);
	const int32 WarningCount = CountSeverity(ERuneEditorValidationSeverity::Warning);
	if (ErrorCount > 0)
	{
		return FText::Format(LOCTEXT("ValidationSummaryErrors", "阻塞：{0} 个错误，{1} 个警告。"), ErrorCount, WarningCount);
	}
	if (WarningCount > 0)
	{
		return FText::Format(LOCTEXT("ValidationSummaryWarnings", "可运行，但有 {0} 个警告。"), WarningCount);
	}
	return LOCTEXT("ValidationSummaryReady", "Yog 符文流程可运行：没有阻塞问题。");
}

FText FRuneEditorValidationResult::BuildDetailsText() const
{
	if (Issues.Num() == 0)
	{
		return LOCTEXT("ValidationNoIssues", "[信息] 没有校验问题。");
	}

	TArray<FString> Lines;
	Lines.Reserve(Issues.Num());
	for (const FRuneEditorValidationIssue& Issue : Issues)
	{
		Lines.Add(FString::Printf(TEXT("%s %s"), *SeverityPrefix(Issue.Severity), *Issue.Message.ToString()));
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FRuneEditorValidationResult FRuneEditorValidation::ValidateRuneGraph(URuneDataAsset* Rune)
{
	FRuneEditorValidationResult Result;
	if (!Rune)
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("NoRune", "未选择符文。"));
		return Result;
	}

	if (Rune->GetRuneName().IsNone())
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("MissingRuneName", "符文名称为空。"));
	}

	if (!Rune->GetRuneIdTag().IsValid())
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("MissingRuneIdTag", "符文标签缺失或未加载。"));
	}

	TSet<FName> TuningKeys;
	for (const FRuneTuningScalar& Scalar : Rune->GetTuningScalars())
	{
		if (Scalar.Key.IsNone())
		{
			AddIssue(Result, ERuneEditorValidationSeverity::Warning, LOCTEXT("EmptyTuningKey", "调参标量包含空 Key。"));
			continue;
		}

		if (TuningKeys.Contains(Scalar.Key))
		{
			AddIssue(Result, ERuneEditorValidationSeverity::Warning, FText::Format(
				LOCTEXT("DuplicateTuningKey", "调参标量包含重复 Key：{0}。"),
				FText::FromName(Scalar.Key)));
		}
		TuningKeys.Add(Scalar.Key);

		if (Scalar.MaxValue < Scalar.MinValue)
		{
			AddIssue(Result, ERuneEditorValidationSeverity::Warning, FText::Format(
				LOCTEXT("InvalidTuningRange", "调参标量 {0} 的最大值小于最小值。"),
				FText::FromName(Scalar.Key)));
		}
	}

	UFlowAsset* FlowAsset = Rune->GetFlowAsset();
	if (!FlowAsset)
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("MissingFlowAsset", "缺少流程资产。"));
		return Result;
	}

	if (!FlowAsset->GetGraph())
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("MissingFlowGraph", "流程资产没有编辑器图表。"));
		return Result;
	}

	FlowAsset->HarvestNodeConnections();

	const TMap<FGuid, UFlowNode*>& Nodes = FlowAsset->GetNodes();
	int32 ValidNodeCount = 0;
	for (const TPair<FGuid, UFlowNode*>& Pair : Nodes)
	{
		if (Pair.Value)
		{
			++ValidNodeCount;
		}
	}

	if (ValidNodeCount == 0)
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("NoFlowNodes", "流程资产没有流程节点。"));
		return Result;
	}

	UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
	if (!EntryNode)
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Error, LOCTEXT("MissingEntryNode", "缺少入口节点。"));
		return Result;
	}

	TArray<UFlowNode*> ReachableNodes;
	FlowAsset->GetNodesInExecutionOrder<UFlowNode>(EntryNode, ReachableNodes);
	TSet<FGuid> ReachableGuids;
	for (UFlowNode* Node : ReachableNodes)
	{
		if (Node)
		{
			ReachableGuids.Add(Node->GetGuid());
		}
	}

	int32 ReachableRuntimeNodeCount = 0;
	for (UFlowNode* Node : ReachableNodes)
	{
		if (Node && Node != EntryNode)
		{
			++ReachableRuntimeNodeCount;
		}
	}

	if (ReachableRuntimeNodeCount == 0)
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Warning, LOCTEXT("EntryOnly", "入口节点之后没有可抵达的运行节点。"));
	}

	int32 UnreachableNodeCount = 0;
	for (const TPair<FGuid, UFlowNode*>& Pair : Nodes)
	{
		if (Pair.Value && !ReachableGuids.Contains(Pair.Key))
		{
			++UnreachableNodeCount;
		}
	}
	if (UnreachableNodeCount > 0)
	{
		AddIssue(Result, ERuneEditorValidationSeverity::Warning, FText::Format(
			LOCTEXT("UnreachableNodes", "{0} 个流程节点无法从入口节点抵达。"),
			UnreachableNodeCount));
	}

	AddIssue(Result, ERuneEditorValidationSeverity::Info, FText::Format(
		LOCTEXT("FlowNodeStats", "流程节点：共 {0} 个，其中 {1} 个可从入口节点抵达。"),
		ValidNodeCount,
		ReachableGuids.Num()));
	return Result;
}

#undef LOCTEXT_NAMESPACE
