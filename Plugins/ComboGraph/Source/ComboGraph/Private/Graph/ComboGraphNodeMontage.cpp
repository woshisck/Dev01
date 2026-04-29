// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraphNodeMontage.h"

#include "Animation/AnimMontage.h"
#include "Utils/ComboGraphUtils.h"

#define LOCTEXT_NAMESPACE "ComboGraphNodeMontage"

UComboGraphNodeMontage::UComboGraphNodeMontage()
{
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("ComboGraphNodeMontage_ContextMenuName", "Montage Combo Node");
#endif
}

FName UComboGraphNodeMontage::GetMontageStartSection_Implementation() const
{
	return StartSection;
}

UAnimMontage* UComboGraphNodeMontage::GetMontage() const
{
	if (!Montage.IsValid())
	{
		Montage.LoadSynchronous();
	}

	return Montage.Get();
}

UAnimSequenceBase* UComboGraphNodeMontage::GetAnimationAsset() const
{
	if (!Montage.IsValid())
	{
		Montage.LoadSynchronous();
	}

	return Montage.Get();
}

float UComboGraphNodeMontage::GetAnimationLength() const
{
	UAnimMontage* AnimMontage = GetMontage();
	if (!AnimMontage)
	{
		return -1.f;
	}

	return AnimMontage->GetPlayLength();
}

FName UComboGraphNodeMontage::GetAnimationStartSection() const
{
	return StartSection;
}

float UComboGraphNodeMontage::GetSectionLength(const FName SectionName) const
{
	UAnimMontage* AnimMontage = GetMontage();
	if (!AnimMontage)
	{
		return -1.f;
	}

	const int32 SectionIndex = AnimMontage->GetSectionIndex(SectionName);
	if (SectionIndex == INDEX_NONE)
	{
		return -1.f;
	}

	return AnimMontage->GetSectionLength(SectionIndex);
}

float UComboGraphNodeMontage::GetAnimationLengthMinusSection(const FName SectionName) const
{
	const float AnimationLength = GetAnimationLength();
	const float SectionLength = GetSectionLength(SectionName);
	if (SectionLength == -1.f || AnimationLength == -1.f)
	{
		return -1.f;
	}

	return AnimationLength - SectionLength;
}

float UComboGraphNodeMontage::GetAnimationLengthMinusStartSection() const
{
	if (StartSection == NAME_None)
	{
		return GetAnimationLength();
	}

	return GetAnimationLengthMinusSection(StartSection);
}

void UComboGraphNodeMontage::SetAnimationAsset(UAnimationAsset* Asset)
{
	if (UAnimMontage* MontageAsset =  Cast<UAnimMontage>(Asset))
	{
		Montage = MontageAsset;
	}
}

bool UComboGraphNodeMontage::SupportsAssetClass(UClass* AssetClass)
{
	return AssetClass->IsChildOf(UAnimMontage::StaticClass());
}

FText UComboGraphNodeMontage::GetNodeTitle() const
{
	if (!NodeTitle.IsEmpty())
	{
		return NodeTitle;
	}

	UAnimationAsset* AnimationAsset = GetAnimationAsset();
	FText DefaultText = LOCTEXT("DefaultNodeTitle", "Combat Graph Node (Montage)");
	return AnimationAsset ? FText::FromString(AnimationAsset->GetName()) : DefaultText;
}

#if WITH_EDITOR
FText UComboGraphNodeMontage::GetAnimAssetLabel() const
{
	return LOCTEXT("AnimAssetLabel", "Montage");
}

FText UComboGraphNodeMontage::GetAnimAssetLabelTooltip() const
{
	return LOCTEXT("AnimAssetLabelTooltip", "Montage");
}

FText UComboGraphNodeMontage::GetAnimAssetText() const
{
	return FText::FromString(Montage ? Montage->GetName() : TEXT("NONE"));
}

FLinearColor UComboGraphNodeMontage::GetBackgroundColor() const
{
	return FComboGraphUtils::GetPluginProjectSettings()->MontageNodeColor;
}
#endif

#undef LOCTEXT_NAMESPACE
