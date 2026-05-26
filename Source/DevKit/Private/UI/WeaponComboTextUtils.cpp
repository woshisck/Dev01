#include "UI/WeaponComboTextUtils.h"

#include "Data/GameplayAbilityComboGraph.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	FString ComboInputActionToMoveToken(EYogComboGraphInputAction Action)
	{
		switch (Action)
		{
		case EYogComboGraphInputAction::Light:
			return TEXT("L");
		case EYogComboGraphInputAction::Heavy:
			return TEXT("H");
		case EYogComboGraphInputAction::Dash:
		case EYogComboGraphInputAction::Any:
		default:
			return FString();
		}
	}

	FString MoveTokenToInputActionMarkup(const FString& Token)
	{
		if (Token == TEXT("L"))
		{
			return TEXT("<input action=\"LightAttack\"/>");
		}

		if (Token == TEXT("H"))
		{
			return TEXT("<input action=\"HeavyAttack\"/>");
		}

		return TEXT("[Input]");
	}

	FString FormatComboForCommonUI(const FString& Sequence)
	{
		TArray<FString> Tokens;
		Sequence.ParseIntoArray(Tokens, TEXT(" - "), true);

		TArray<FString> DisplayTokens;
		DisplayTokens.Reserve(Tokens.Num());
		for (const FString& Token : Tokens)
		{
			DisplayTokens.Add(MoveTokenToInputActionMarkup(Token));
		}

		return FString::Join(DisplayTokens, TEXT(" -> "));
	}

	void GatherComboMoveListsFromNode(
		const UGameplayAbilityComboGraphNode* Node,
		TArray<FString> CurrentTokens,
		TSet<const UGameplayAbilityComboGraphNode*>& Visiting,
		TArray<FString>& OutSequences)
	{
		if (!Node || CurrentTokens.IsEmpty() || Visiting.Contains(Node))
		{
			return;
		}

		Visiting.Add(Node);

		bool bHasDisplayableChild = false;
		for (UGenericGraphNode* ChildGenericNode : Node->ChildrenNodes)
		{
			const UGameplayAbilityComboGraphNode* ChildNode = Cast<UGameplayAbilityComboGraphNode>(ChildGenericNode);
			UGenericGraphEdge* const* EdgePtr = Node->Edges.Find(ChildGenericNode);
			const UGameplayAbilityComboGraphEdge* Edge = EdgePtr ? Cast<UGameplayAbilityComboGraphEdge>(*EdgePtr) : nullptr;
			const FString Token = Edge ? ComboInputActionToMoveToken(Edge->InputAction) : FString();
			if (!ChildNode || Token.IsEmpty())
			{
				continue;
			}

			bHasDisplayableChild = true;
			TArray<FString> NextTokens = CurrentTokens;
			NextTokens.Add(Token);
			GatherComboMoveListsFromNode(ChildNode, MoveTemp(NextTokens), Visiting, OutSequences);
		}

		if (Node->bIsComboFinisher || !bHasDisplayableChild)
		{
			OutSequences.Add(FString::Join(CurrentTokens, TEXT(" - ")));
		}

		Visiting.Remove(Node);
	}

	void GatherComboMoveLists(const UGameplayAbilityComboGraph* ComboGraph, TArray<FString>& OutSequences)
	{
		if (!ComboGraph)
		{
			return;
		}

		TArray<const UGameplayAbilityComboGraphNode*> RootComboNodes;
		for (const UGenericGraphNode* RootNode : ComboGraph->RootNodes)
		{
			if (const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(RootNode))
			{
				RootComboNodes.Add(ComboNode);
			}
		}

		if (RootComboNodes.IsEmpty())
		{
			for (const UGenericGraphNode* Node : ComboGraph->AllNodes)
			{
				const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
				if (ComboNode && ComboNode->ParentNodes.IsEmpty())
				{
					RootComboNodes.Add(ComboNode);
				}
			}
		}

		for (const UGameplayAbilityComboGraphNode* RootNode : RootComboNodes)
		{
			const FString RootToken = RootNode ? ComboInputActionToMoveToken(RootNode->RootInputAction) : FString();
			if (RootToken.IsEmpty())
			{
				continue;
			}

			TArray<FString> Tokens;
			Tokens.Add(RootToken);
			TSet<const UGameplayAbilityComboGraphNode*> Visiting;
			GatherComboMoveListsFromNode(RootNode, MoveTemp(Tokens), Visiting, OutSequences);
		}
	}
}

FText WeaponComboTextUtils::BuildComboHintText(
	const UWeaponDefinition* WeaponDefinition,
	int32 MaxLines,
	bool bCompactSpacing)
{
	if (!WeaponDefinition)
	{
		return FText::FromString(TEXT("\u62fe\u53d6\u6b66\u5668\u540e\u663e\u793a\u51fa\u62db\u8868\u3002"));
	}

	TArray<FString> Sequences;
	GatherComboMoveLists(WeaponDefinition->GameplayAbilityComboGraph, Sequences);

	TSet<FString> UniqueSequences;
	TArray<FString> MoveListLines;
	const int32 ClampedMaxLines = FMath::Max(1, MaxLines);
	for (const FString& Sequence : Sequences)
	{
		if (Sequence.IsEmpty() || UniqueSequences.Contains(Sequence))
		{
			continue;
		}

		UniqueSequences.Add(Sequence);
		MoveListLines.Add(FString::Printf(TEXT("\u8fde\u6bb5 %02d   %s"),
			MoveListLines.Num() + 1,
			*FormatComboForCommonUI(Sequence)));
		if (MoveListLines.Num() >= ClampedMaxLines)
		{
			break;
		}
	}

	if (MoveListLines.IsEmpty())
	{
		MoveListLines.Add(TEXT("\u8fde\u6bb5 01   <input action=\"LightAttack\"/> -> <input action=\"LightAttack\"/> -> <input action=\"HeavyAttack\"/>"));
		if (ClampedMaxLines > 1)
		{
			MoveListLines.Add(TEXT("\u8fde\u6bb5 02   <input action=\"LightAttack\"/> -> <input action=\"HeavyAttack\"/>"));
		}
	}

	return FText::FromString(FString::Join(MoveListLines, bCompactSpacing ? TEXT("\n") : TEXT("\n\n")));
}
