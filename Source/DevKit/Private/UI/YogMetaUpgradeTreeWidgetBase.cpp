#include "UI/YogMetaUpgradeTreeWidgetBase.h"
#include "UI/MetaNodeCardWidgetBase.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "MetaProgression/MetaTypes.h"
#include "Engine/DataTable.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"

UYogMetaUpgradeTreeWidgetBase::UYogMetaUpgradeTreeWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeCardWidgetClass = UMetaNodeCardWidgetBase::StaticClass();
}

UYogMetaProgressionSubsystem* UYogMetaUpgradeTreeWidgetBase::GetMetaSys() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UYogMetaProgressionSubsystem>();
	}
	return nullptr;
}

void UYogMetaUpgradeTreeWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureFallbackWidgets();

	if (BtnFleshSide)  BtnFleshSide->OnClicked.AddDynamic(this,  &UYogMetaUpgradeTreeWidgetBase::HandleFleshSideClicked);
	if (BtnMysticSide) BtnMysticSide->OnClicked.AddDynamic(this, &UYogMetaUpgradeTreeWidgetBase::HandleMysticSideClicked);
	if (BtnClose)      BtnClose->OnClicked.AddDynamic(this,       &UYogMetaUpgradeTreeWidgetBase::HandleCloseClicked);

	if (UYogMetaProgressionSubsystem* Meta = GetMetaSys())
	{
		Meta->OnNodePurchased.AddDynamic(this, &UYogMetaUpgradeTreeWidgetBase::HandleNodePurchased);
		Meta->OnCurrencyChanged.AddDynamic(this, &UYogMetaUpgradeTreeWidgetBase::HandleCurrencyChanged);
	}
}

void UYogMetaUpgradeTreeWidgetBase::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshTree();
}

void UYogMetaUpgradeTreeWidgetBase::RefreshTree()
{
	UYogMetaProgressionSubsystem* Meta = GetMetaSys();
	if (!Meta) return;

	if (NodeList)
	{
		NodeList->ClearChildren();
	}
	else
	{
		BP_ClearNodeCards();
	}

	TArray<FName> AllNames;
	Meta->GetAllNodeNames(AllNames);

	for (const FName& RowName : AllNames)
	{
		const FMetaUpgradeNodeRow* Row = Meta->MetaUpgradeNodeTable
			? Meta->MetaUpgradeNodeTable->FindRow<FMetaUpgradeNodeRow>(RowName, TEXT("RefreshTree"), false)
			: nullptr;
		if (!Row) continue;

		const bool bMatchesSide = bShowFleshSide
			? (Row->Side == EMetaSide::Flesh)
			: (Row->Side == EMetaSide::Mystic);
		if (!bMatchesSide) continue;

		const int32 CurrentLevel = Meta->GetNodeLevel(RowName);
		const bool bCanPurchase = Meta->CanPurchaseNode(RowName);
		if (!NativeAddNodeCard(RowName, *Row, CurrentLevel, bCanPurchase))
		{
			BP_AddNodeCard(RowName, *Row, CurrentLevel, bCanPurchase);
		}
	}
}

bool UYogMetaUpgradeTreeWidgetBase::PurchaseNode(FName NodeRowName)
{
	UYogMetaProgressionSubsystem* Meta = GetMetaSys();
	return Meta ? Meta->TryPurchaseNode(NodeRowName) : false;
}

void UYogMetaUpgradeTreeWidgetBase::SetShowFleshSide(bool bFlesh)
{
	bShowFleshSide = bFlesh;
	RefreshTree();
}

void UYogMetaUpgradeTreeWidgetBase::HandleNodePurchased(FName NodeRowName)
{
	RefreshTree();
}

void UYogMetaUpgradeTreeWidgetBase::HandleCurrencyChanged(FGameplayTag CurrencyTag, int32 NewAmount)
{
	if (TxtCurrencyAmount)
	{
		TxtCurrencyAmount->SetText(FText::AsNumber(NewAmount));
	}
}

void UYogMetaUpgradeTreeWidgetBase::HandleFleshSideClicked()
{
	SetShowFleshSide(true);
}

void UYogMetaUpgradeTreeWidgetBase::HandleMysticSideClicked()
{
	SetShowFleshSide(false);
}

void UYogMetaUpgradeTreeWidgetBase::HandleCloseClicked()
{
	DeactivateWidget();
}

void UYogMetaUpgradeTreeWidgetBase::EnsureFallbackWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	UVerticalBox* RootBox = Cast<UVerticalBox>(WidgetTree->RootWidget);
	if (!RootBox)
	{
		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = RootBox;
	}

	if (!TxtCurrencyAmount)
	{
		TxtCurrencyAmount = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtCurrencyAmount"));
		TxtCurrencyAmount->SetText(NSLOCTEXT("MetaUpgrade", "CurrencyFallback", "0"));
		RootBox->AddChildToVerticalBox(TxtCurrencyAmount);
	}
	if (!BtnFleshSide)
	{
		BtnFleshSide = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnFleshSide"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtFleshSideLabel"));
		Label->SetText(NSLOCTEXT("MetaUpgrade", "FleshSide", "Flesh"));
		BtnFleshSide->AddChild(Label);
		RootBox->AddChildToVerticalBox(BtnFleshSide);
	}
	if (!BtnMysticSide)
	{
		BtnMysticSide = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnMysticSide"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtMysticSideLabel"));
		Label->SetText(NSLOCTEXT("MetaUpgrade", "MysticSide", "Mystic"));
		BtnMysticSide->AddChild(Label);
		RootBox->AddChildToVerticalBox(BtnMysticSide);
	}
	if (!NodeList)
	{
		NodeList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NodeList"));
		RootBox->AddChildToVerticalBox(NodeList);
	}
	if (!BtnClose)
	{
		BtnClose = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnClose"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtCloseLabel"));
		Label->SetText(NSLOCTEXT("MetaUpgrade", "Close", "Close"));
		BtnClose->AddChild(Label);
		RootBox->AddChildToVerticalBox(BtnClose);
	}
}

bool UYogMetaUpgradeTreeWidgetBase::NativeAddNodeCard(FName NodeRowName, const FMetaUpgradeNodeRow& NodeData,
                                                      int32 CurrentLevel, bool bCanPurchase)
{
	if (!NodeList || !NodeCardWidgetClass)
	{
		return false;
	}

	UMetaNodeCardWidgetBase* Card = CreateWidget<UMetaNodeCardWidgetBase>(GetOwningPlayer(), NodeCardWidgetClass);
	if (!Card)
	{
		return false;
	}

	Card->InitCard(NodeRowName, NodeData, CurrentLevel, bCanPurchase);
	Card->OnPurchaseRequested.AddDynamic(this, &UYogMetaUpgradeTreeWidgetBase::HandleCardPurchaseRequested);
	NodeList->AddChild(Card);
	return true;
}

void UYogMetaUpgradeTreeWidgetBase::HandleCardPurchaseRequested(FName NodeRowName)
{
	PurchaseNode(NodeRowName);
}
