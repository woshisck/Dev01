// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraph.h"

#include "Graph/ComboGraphEdge.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "Graph/ComboGraphNodeBase.h"
#include "Graph/ComboGraphNodeMontage.h"
#include "Graph/ComboGraphNodeSequence.h"
#include "Utils/ComboGraphUtils.h"

UComboGraph::UComboGraph()
{
	NodeType = UComboGraphNodeAnimBase::StaticClass();
	EdgeType = UComboGraphEdge::StaticClass();
	DefaultNodeMontageType = UComboGraphNodeMontage::StaticClass();
	DefaultNodeSequenceType = UComboGraphNodeSequence::StaticClass();

#if WITH_EDITORONLY_DATA
	IconPreference = FComboGraphUtils::GetPluginProjectSettings()->IconPreference;
#endif
}

void UComboGraph::ClearGraph()
{
	for (UComboGraphNodeBase* Node : AllNodes)
	{
		if (Node)
		{
			Node->ParentNodes.Empty();
			Node->ChildrenNodes.Empty();
			Node->Edges.Empty();
		}
	}

	AllNodes.Empty();
	RootNodes.Empty();
}

int32 UComboGraph::GetLevelNum() const
{
	int32 Level = 0;
	TArray<UComboGraphNodeBase*> CurrLevelNodes = RootNodes;
	TArray<UComboGraphNodeBase*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (UComboGraphNodeBase* Node : CurrLevelNodes)
		{
			check(Node != nullptr);
			NextLevelNodes.Append(Node->ChildrenNodes);
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}

	return Level;
}

void UComboGraph::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty)
{
#if WITH_EDITORONLY_DATA
	if (bMarkAsDirty)
	{
		Modify();
	}

	PreviewSkeletalMesh = PreviewMesh;
#endif
}

USkeletalMesh* UComboGraph::GetPreviewMesh() const
{
#if WITH_EDITORONLY_DATA
	if (!PreviewSkeletalMesh.IsValid())
	{
		PreviewSkeletalMesh.LoadSynchronous();
	}

	return PreviewSkeletalMesh.Get();
#else
	return nullptr;
#endif
}
