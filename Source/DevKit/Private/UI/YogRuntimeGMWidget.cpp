#include "UI/YogRuntimeGMWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/EnemyData.h"
#include "Input/CommonUIInputTypes.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "System/YogRuntimeGMSubsystem.h"

namespace
{
UTextBlock* MakeGMText(UWidgetTree* WidgetTree, const FName Name, const FText& Text, float FontSize = 18.f)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = static_cast<int32>(FontSize);
	TextBlock->SetFont(FontInfo);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}

UButton* MakeGMButton(UWidgetTree* WidgetTree, const FName Name, const FText& Text)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	UTextBlock* Label = MakeGMText(WidgetTree, FName(*(Name.ToString() + TEXT("_Label"))), Text, 18.f);
	Label->SetJustification(ETextJustify::Center);
	Button->AddChild(Label);
	return Button;
}

void AddVerticalChild(UVerticalBox* Root, UWidget* Child, const FMargin Padding = FMargin(0.f, 0.f, 0.f, 8.f))
{
	if (UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Child))
	{
		Slot->SetPadding(Padding);
	}
}
}

void UYogRuntimeGMWidget::InitializeRuntimeGM(UYogRuntimeGMSubsystem* InSubsystem)
{
	RuntimeGMSubsystem = InSubsystem;
	RefreshFromSubsystem();
}

void UYogRuntimeGMWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	if (!WidgetTree->RootWidget)
	{
		BuildFallbackWidget();
	}

	if (GiveWeaponButton)
	{
		GiveWeaponButton->IsFocusable = true;
		GiveWeaponButton->OnClicked.AddUniqueDynamic(this, &UYogRuntimeGMWidget::HandleGiveWeaponClicked);
	}
	if (SpawnEnemyButton)
	{
		SpawnEnemyButton->IsFocusable = true;
		SpawnEnemyButton->OnClicked.AddUniqueDynamic(this, &UYogRuntimeGMWidget::HandleSpawnEnemyClicked);
	}
	if (ResetButton)
	{
		ResetButton->IsFocusable = true;
		ResetButton->OnClicked.AddUniqueDynamic(this, &UYogRuntimeGMWidget::HandleResetClicked);
	}
	if (CloseButton)
	{
		CloseButton->IsFocusable = true;
		CloseButton->OnClicked.AddUniqueDynamic(this, &UYogRuntimeGMWidget::HandleCloseClicked);
	}
	if (SpawnCountSpinBox)
	{
		SpawnCountSpinBox->OnValueChanged.AddUniqueDynamic(this, &UYogRuntimeGMWidget::HandleSpawnCountChanged);
	}
	if (SpawnRadiusSpinBox)
	{
		SpawnRadiusSpinBox->OnValueChanged.AddUniqueDynamic(this, &UYogRuntimeGMWidget::HandleSpawnRadiusChanged);
	}

	RefreshFromSubsystem();
}

void UYogRuntimeGMWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	if (GiveWeaponButton)
	{
		GiveWeaponButton->SetKeyboardFocus();
	}
}

void UYogRuntimeGMWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeOnDeactivated();
}

UWidget* UYogRuntimeGMWidget::NativeGetDesiredFocusTarget() const
{
	if (GiveWeaponButton)
	{
		return GiveWeaponButton.Get();
	}
	if (SpawnEnemyButton)
	{
		return SpawnEnemyButton.Get();
	}
	if (ResetButton)
	{
		return ResetButton.Get();
	}
	return CloseButton.Get();
}

TOptional<FUIInputConfig> UYogRuntimeGMWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UYogRuntimeGMWidget::BuildFallbackWidget()
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeGMPanel"));
	WidgetTree->RootWidget = Panel;
	Panel->SetPadding(FMargin(20.f));
	Panel->SetBrushColor(FLinearColor(0.03f, 0.10f, 0.24f, 0.98f));

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RuntimeGMRoot"));
	Panel->AddChild(Root);

	AddVerticalChild(Root, MakeGMText(WidgetTree, TEXT("TitleText"), NSLOCTEXT("YogRuntimeGM", "Title", "运行时 GM 面板"), 28.f));

	SummaryText = MakeGMText(WidgetTree, TEXT("SummaryText"), FText::GetEmpty(), 16.f);
	AddVerticalChild(Root, SummaryText);

	UHorizontalBox* SpawnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SpawnRow"));
	AddVerticalChild(Root, SpawnRow);

	UTextBlock* CountLabel = MakeGMText(WidgetTree, TEXT("SpawnCountLabel"), NSLOCTEXT("YogRuntimeGM", "SpawnCount", "刷敌数量"), 16.f);
	if (UHorizontalBoxSlot* CountSlot = SpawnRow->AddChildToHorizontalBox(CountLabel))
	{
		CountSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	SpawnCountSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("SpawnCountSpinBox"));
	SpawnCountSpinBox->SetMinValue(1.f);
	SpawnCountSpinBox->SetMaxValue(50.f);
	SpawnCountSpinBox->SetDelta(1.f);
	if (UHorizontalBoxSlot* CountInputSlot = SpawnRow->AddChildToHorizontalBox(SpawnCountSpinBox))
	{
		CountInputSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	UTextBlock* RadiusLabel = MakeGMText(WidgetTree, TEXT("SpawnRadiusLabel"), NSLOCTEXT("YogRuntimeGM", "SpawnRadius", "刷敌半径"), 16.f);
	if (UHorizontalBoxSlot* RadiusSlot = SpawnRow->AddChildToHorizontalBox(RadiusLabel))
	{
		RadiusSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	SpawnRadiusSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("SpawnRadiusSpinBox"));
	SpawnRadiusSpinBox->SetMinValue(100.f);
	SpawnRadiusSpinBox->SetMaxValue(5000.f);
	SpawnRadiusSpinBox->SetDelta(100.f);
	if (UHorizontalBoxSlot* RadiusInputSlot = SpawnRow->AddChildToHorizontalBox(SpawnRadiusSpinBox))
	{
		RadiusInputSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
	}

	GiveWeaponButton = MakeGMButton(WidgetTree, TEXT("GiveWeaponButton"), NSLOCTEXT("YogRuntimeGM", "GiveWeapon", "给予配置武器"));
	SpawnEnemyButton = MakeGMButton(WidgetTree, TEXT("SpawnEnemyButton"), NSLOCTEXT("YogRuntimeGM", "SpawnEnemies", "生成配置敌人"));
	ResetButton = MakeGMButton(WidgetTree, TEXT("ResetButton"), NSLOCTEXT("YogRuntimeGM", "Reset", "重置玩家 / 敌人"));
	CloseButton = MakeGMButton(WidgetTree, TEXT("CloseButton"), NSLOCTEXT("YogRuntimeGM", "Close", "关闭"));

	AddVerticalChild(Root, GiveWeaponButton);
	AddVerticalChild(Root, SpawnEnemyButton);
	AddVerticalChild(Root, ResetButton);
	AddVerticalChild(Root, CloseButton);

	StatusText = MakeGMText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), 15.f);
	AddVerticalChild(Root, StatusText, FMargin(0.f, 8.f, 0.f, 0.f));
}

void UYogRuntimeGMWidget::RefreshFromSubsystem()
{
	if (!RuntimeGMSubsystem)
	{
		return;
	}

	if (SpawnCountSpinBox)
	{
		SpawnCountSpinBox->SetValue(RuntimeGMSubsystem->GetRuntimeSpawnCount());
	}
	if (SpawnRadiusSpinBox)
	{
		SpawnRadiusSpinBox->SetValue(RuntimeGMSubsystem->GetRuntimeSpawnRadius());
	}
	if (SummaryText)
	{
		const UWeaponDefinition* Weapon = RuntimeGMSubsystem->LoadConfiguredWeapon();
		const UEnemyData* EnemyData = RuntimeGMSubsystem->LoadConfiguredEnemyData();
		SummaryText->SetText(FText::Format(
			NSLOCTEXT("YogRuntimeGM", "Summary", "武器：{0}\n敌人：{1}\n按 F12 打开/关闭此面板。资产在 Project Settings > Game > Yog Runtime GM 中配置。"),
			FText::FromString(GetNameSafe(Weapon)),
			FText::FromString(GetNameSafe(EnemyData))));
	}
	if (StatusText)
	{
		StatusText->SetText(RuntimeGMSubsystem->GetLastStatusText());
	}
}

void UYogRuntimeGMWidget::HandleGiveWeaponClicked()
{
	if (RuntimeGMSubsystem)
	{
		RuntimeGMSubsystem->GiveConfiguredWeapon(GetOwningPlayer());
	}
}

void UYogRuntimeGMWidget::HandleSpawnEnemyClicked()
{
	if (RuntimeGMSubsystem)
	{
		RuntimeGMSubsystem->SpawnConfiguredEnemies(GetOwningPlayer());
	}
}

void UYogRuntimeGMWidget::HandleResetClicked()
{
	if (RuntimeGMSubsystem)
	{
		RuntimeGMSubsystem->ResetPlayerAndEnemies(GetOwningPlayer());
	}
}

void UYogRuntimeGMWidget::HandleCloseClicked()
{
	if (RuntimeGMSubsystem)
	{
		RuntimeGMSubsystem->CloseGMPanel(GetOwningPlayer());
	}
}

void UYogRuntimeGMWidget::HandleSpawnCountChanged(float Value)
{
	if (RuntimeGMSubsystem)
	{
		RuntimeGMSubsystem->SetRuntimeSpawnCount(FMath::RoundToInt(Value));
	}
}

void UYogRuntimeGMWidget::HandleSpawnRadiusChanged(float Value)
{
	if (RuntimeGMSubsystem)
	{
		RuntimeGMSubsystem->SetRuntimeSpawnRadius(Value);
	}
}
