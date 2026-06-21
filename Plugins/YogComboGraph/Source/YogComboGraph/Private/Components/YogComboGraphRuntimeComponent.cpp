#include "Components/YogComboGraphRuntimeComponent.h"

namespace
{
	FName GetRuntimeComponentNodeId(const UGameplayAbilityComboGraphNode* Node)
	{
		return Node && !Node->NodeId.IsNone()
			? Node->NodeId
			: FName(*GetNameSafe(Node));
	}
}

UYogComboGraphRuntimeComponent::UYogComboGraphRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UYogComboGraphRuntimeComponent::LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	ComboGraph = InComboGraph;
	ResetCombo();

	if (!ComboGraph)
	{
		return;
	}

	TArray<FText> Warnings;
	ComboGraph->ValidateComboGraph(Warnings);
	for (const FText& Warning : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameplayAbilityComboGraph] %s: %s"), *GetNameSafe(ComboGraph), *Warning.ToString());
	}
}

bool UYogComboGraphRuntimeComponent::TryActivateComboGraphNode(EYogComboGraphInputAction InputAction, const FGameplayTagContainer& OwnedTags)
{
	FYogComboGraphNodeSelection Selection;
	if (!FindNextComboGraphNode(InputAction, &OwnedTags, Selection))
	{
		MarkComboActivationMiss();
		return false;
	}

	PrepareComboGraphNodeActivation(Selection);
	CommitPreparedComboActivation();
	return true;
}

bool UYogComboGraphRuntimeComponent::FindNextComboGraphNode(EYogComboGraphInputAction InputAction, const FGameplayTagContainer* OwnedTags, FYogComboGraphNodeSelection& OutSelection) const
{
	OutSelection = FYogComboGraphNodeSelection();
	if (!ComboGraph)
	{
		return false;
	}

	const UGameplayAbilityComboGraphNode* NextGraphNode = ComboGraph->FindChildComboNode(CurrentNodeId, InputAction, OwnedTags);
	OutSelection.bFoundChildNode = NextGraphNode != nullptr;
	if (!NextGraphNode)
	{
		NextGraphNode = ComboGraph->FindRootComboNode(InputAction);
	}

	if (!NextGraphNode)
	{
		return false;
	}

	OutSelection.Node = NextGraphNode;
	return true;
}

void UYogComboGraphRuntimeComponent::PrepareComboGraphNodeActivation(const FYogComboGraphNodeSelection& Selection)
{
	ActiveGraphNode = const_cast<UGameplayAbilityComboGraphNode*>(Selection.Node);
	PrepareComboNodeActivation(
		GetRuntimeComponentNodeId(Selection.Node),
		Selection.bFoundChildNode);
}

void UYogComboGraphRuntimeComponent::PrepareComboNodeActivation(FName NodeId, bool bFoundChildNode)
{
	ActiveNodeId = NodeId;
	bActiveNodeValid = !NodeId.IsNone();
	bComboContinued = bFoundChildNode;
	bExitedComboState = !CurrentNodeId.IsNone() && !bFoundChildNode;
	ActiveAttackGuid = FGuid::NewGuid();
	bPreparedFoundChildNode = bFoundChildNode;
}

void UYogComboGraphRuntimeComponent::CommitPreparedComboActivation()
{
	if (!bActiveNodeValid)
	{
		return;
	}

	ComboIndex = bPreparedFoundChildNode
		? FMath::Max(1, ComboIndex + 1)
		: 1;
	ComboTags.Reset();

	CurrentNodeId = ActiveNodeId;
}

void UYogComboGraphRuntimeComponent::ClearPreparedComboActivation()
{
	ActiveGraphNode = nullptr;
	ActiveNodeId = NAME_None;
	ActiveAttackGuid.Invalidate();
	bActiveNodeValid = false;
	bComboContinued = false;
	bPreparedFoundChildNode = false;
}

void UYogComboGraphRuntimeComponent::MarkComboActivationMiss()
{
	bComboContinued = false;
	bExitedComboState = !CurrentNodeId.IsNone();
	CurrentNodeId = NAME_None;
	ComboIndex = 0;
	ComboTags.Reset();
	ClearPreparedComboActivation();
}

void UYogComboGraphRuntimeComponent::ResetCombo()
{
	CurrentNodeId = NAME_None;
	ComboIndex = 0;
	ComboTags.Reset();
	bExitedComboState = true;
	ClearPreparedComboActivation();
}
