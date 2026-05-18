#include "UI/YogMetaUpgradeTreeWidgetBase.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "MetaProgression/MetaTypes.h"
#include "Engine/DataTable.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

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

	BP_ClearNodeCards();

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

		BP_AddNodeCard(RowName, *Row, Meta->GetNodeLevel(RowName), Meta->CanPurchaseNode(RowName));
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
