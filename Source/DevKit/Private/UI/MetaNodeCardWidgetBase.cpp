#include "UI/MetaNodeCardWidgetBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UMetaNodeCardWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree)
	{
		UVerticalBox* RootBox = Cast<UVerticalBox>(WidgetTree->RootWidget);
		if (!RootBox)
		{
			RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
			WidgetTree->RootWidget = RootBox;
		}
		if (!TxtNodeName)
		{
			TxtNodeName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtNodeName"));
			RootBox->AddChildToVerticalBox(TxtNodeName);
		}
		if (!TxtLevelProgress)
		{
			TxtLevelProgress = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtLevelProgress"));
			RootBox->AddChildToVerticalBox(TxtLevelProgress);
		}
		if (!TxtCost)
		{
			TxtCost = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtCost"));
			RootBox->AddChildToVerticalBox(TxtCost);
		}
		if (!ProgressLevel)
		{
			ProgressLevel = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressLevel"));
			RootBox->AddChildToVerticalBox(ProgressLevel);
		}
		if (!BtnPurchase)
		{
			BtnPurchase = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnPurchase"));
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtPurchaseLabel"));
			Label->SetText(NSLOCTEXT("MetaNodeCard", "Purchase", "Purchase"));
			BtnPurchase->AddChild(Label);
			RootBox->AddChildToVerticalBox(BtnPurchase);
		}
	}

	if (BtnPurchase)
	{
		BtnPurchase->OnClicked.AddDynamic(this, &UMetaNodeCardWidgetBase::HandlePurchaseClicked);
	}
}

void UMetaNodeCardWidgetBase::InitCard(FName InRowName, const FMetaUpgradeNodeRow& InData,
                                       int32 InCurrentLevel, bool bInCanPurchase)
{
	RowName  = InRowName;
	NodeData = InData;
	UpdateVisuals(InCurrentLevel, bInCanPurchase);
}

void UMetaNodeCardWidgetBase::RefreshState(int32 InCurrentLevel, bool bInCanPurchase)
{
	UpdateVisuals(InCurrentLevel, bInCanPurchase);
}

void UMetaNodeCardWidgetBase::UpdateVisuals(int32 CurrentLevel, bool bCanPurchase)
{
	const bool bMaxed = (CurrentLevel >= NodeData.MaxLevel);

	CardState = bMaxed ? EMetaNodeCardState::Purchased
	          : bCanPurchase ? EMetaNodeCardState::Available
	          : EMetaNodeCardState::Locked;

	if (TxtNodeName)
	{
		TxtNodeName->SetText(NodeData.DisplayName);
	}

	if (TxtLevelProgress)
	{
		TxtLevelProgress->SetText(
			FText::Format(NSLOCTEXT("MetaNode", "LevelFmt", "{0}/{1}"),
			              FText::AsNumber(CurrentLevel),
			              FText::AsNumber(NodeData.MaxLevel)));
	}

	if (ProgressLevel && NodeData.MaxLevel > 0)
	{
		ProgressLevel->SetPercent(static_cast<float>(CurrentLevel) / NodeData.MaxLevel);
	}

	if (TxtCost && !NodeData.CostsPerLevel.IsEmpty())
	{
		TxtCost->SetText(FText::AsNumber(NodeData.CostsPerLevel[0].Amount));
	}

	if (BtnPurchase)
	{
		BtnPurchase->SetIsEnabled(CardState == EMetaNodeCardState::Available);
	}

	switch (CardState)
	{
	case EMetaNodeCardState::Locked:
		SetRenderOpacity(0.45f);
		break;
	case EMetaNodeCardState::Available:
		SetRenderOpacity(1.0f);
		break;
	case EMetaNodeCardState::Purchased:
		SetRenderOpacity(0.75f);
		break;
	default:
		break;
	}

	BP_OnStateChanged(CardState);
}

void UMetaNodeCardWidgetBase::HandlePurchaseClicked()
{
	OnPurchaseRequested.Broadcast(RowName);
}
