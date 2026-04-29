// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraphNodeSequence.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Utils/ComboGraphUtils.h"

#define LOCTEXT_NAMESPACE "ComboGraphNodeSequence"

UComboGraphNodeSequence::UComboGraphNodeSequence()
{
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("ComboGraphNodeSequence_ContextMenuName", "Sequence Combo Node");
#endif
}

UAnimMontage* UComboGraphNodeSequence::GetMontage() const
{
	if (!Sequence.IsValid())
	{
		Sequence.LoadSynchronous();
	}

	UAnimSequence* AnimSequence = Sequence.Get();
	if (!AnimSequence)
	{
		return nullptr;
	}

	// create asset using the information
	UAnimMontage* NewMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(AnimSequence, FComboGraphUtils::GetPluginProjectSettings()->DynamicMontageSlotName);
	return NewMontage;
}

UAnimSequenceBase* UComboGraphNodeSequence::GetAnimationAsset() const
{
	if (!Sequence.IsValid())
	{
		Sequence.LoadSynchronous();
	}

	return Sequence.Get();
}

float UComboGraphNodeSequence::GetAnimationLength() const
{
	if (!Sequence.IsValid())
	{
		Sequence.LoadSynchronous();
	}


	UAnimSequence* AnimSequence = Sequence.Get();
	if (!AnimSequence)
	{
		return -1.f;
	}

	return Sequence->GetPlayLength();
}

float UComboGraphNodeSequence::GetAnimationLengthMinusStartSection() const
{
	// Sequences don't have Start Section, return whole anim length by default
	return GetAnimationLength();
}

void UComboGraphNodeSequence::SetAnimationAsset(UAnimationAsset* Asset)
{
	if (const UAnimSequence* SequenceAsset =  Cast<UAnimSequence>(Asset))
	{
		Sequence = SequenceAsset;
	}
}

bool UComboGraphNodeSequence::SupportsAssetClass(UClass* AssetClass)
{
	return AssetClass->IsChildOf(UAnimSequence::StaticClass());
}

FText UComboGraphNodeSequence::GetNodeTitle() const
{
	if (!NodeTitle.IsEmpty())
	{
		return NodeTitle;
	}

	UAnimationAsset* AnimationAsset = GetAnimationAsset();
	FText DefaultText = LOCTEXT("DefaultNodeTitle", "Combat Graph Node (Sequence)");
	return AnimationAsset ? FText::FromString(AnimationAsset->GetName()) : DefaultText;
}

#if WITH_EDITOR
FText UComboGraphNodeSequence::GetAnimAssetLabel() const
{
	return LOCTEXT("AnimAssetLabel", "Sequence");
}

FText UComboGraphNodeSequence::GetAnimAssetLabelTooltip() const
{
	return LOCTEXT("AnimAssetLabelTooltip", "Sequence");
}

FText UComboGraphNodeSequence::GetAnimAssetText() const
{
	return FText::FromString(Sequence ? Sequence->GetName() : TEXT("NONE"));
}

FLinearColor UComboGraphNodeSequence::GetBackgroundColor() const
{
	return FComboGraphUtils::GetPluginProjectSettings()->SequenceNodeColor;
}
#endif

#undef LOCTEXT_NAMESPACE
