#include "UI/ActiveSkillLoadoutWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"

namespace
{
	const TCHAR* DefaultShieldBurstPath = TEXT("/Game/Code/Core/ActiveSkills/DA_ActiveSkill_ShieldBurst.DA_ActiveSkill_ShieldBurst");
}

UActiveSkillLoadoutWidget::UActiveSkillLoadoutWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultSkillAsset = TSoftObjectPtr<UActiveSkillDataAsset>(FSoftObjectPath(DefaultShieldBurstPath));
}

void UActiveSkillLoadoutWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildRuntimeLayout();
	RefreshLoadout();
}

void UActiveSkillLoadoutWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
}

void UActiveSkillLoadoutWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
}

TOptional<FUIInputConfig> UActiveSkillLoadoutWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* UActiveSkillLoadoutWidget::NativeGetDesiredFocusTarget() const
{
	return Slot0Button ? Slot0Button.Get() : Super::NativeGetDesiredFocusTarget();
}

void UActiveSkillLoadoutWidget::BuildRuntimeLayout()
{
	if (RuntimeRoot || !WidgetTree)
	{
		return;
	}

	RuntimeRoot = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActiveSkillLoadoutRoot"));
	WidgetTree->RootWidget = RuntimeRoot;

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Active Skill")));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 24;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* TitleSlot = RuntimeRoot->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(16.0f, 12.0f, 16.0f, 8.0f));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetAutoWrapText(true);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.90f, 1.0f)));
	if (UVerticalBoxSlot* StatusSlot = RuntimeRoot->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(16.0f, 0.0f, 16.0f, 10.0f));
	}

	Slot0Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Slot0Button"));
	Slot0Button->IsFocusable = true;
	Slot0Button->OnClicked.AddDynamic(this, &UActiveSkillLoadoutWidget::HandleSlot0Clicked);
	Slot0Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Slot0Text"));
	Slot0Text->SetJustification(ETextJustify::Center);
	Slot0Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Slot0Button->AddChild(Slot0Text)))
	{
		ButtonSlot->SetPadding(FMargin(18.0f, 10.0f));
	}
	if (UVerticalBoxSlot* Slot0 = RuntimeRoot->AddChildToVerticalBox(Slot0Button))
	{
		Slot0->SetPadding(FMargin(24.0f, 4.0f));
	}

	Slot1Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Slot1Button"));
	Slot1Button->IsFocusable = true;
	Slot1Button->OnClicked.AddDynamic(this, &UActiveSkillLoadoutWidget::HandleSlot1Clicked);
	Slot1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Slot1Text"));
	Slot1Text->SetJustification(ETextJustify::Center);
	Slot1Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Slot1Button->AddChild(Slot1Text)))
	{
		ButtonSlot->SetPadding(FMargin(18.0f, 10.0f));
	}
	if (UVerticalBoxSlot* Slot1 = RuntimeRoot->AddChildToVerticalBox(Slot1Button))
	{
		Slot1->SetPadding(FMargin(24.0f, 4.0f));
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnClose"));
	CloseButton->IsFocusable = true;
	CloseButton->OnClicked.AddDynamic(this, &UActiveSkillLoadoutWidget::HandleCloseClicked);
	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
	CloseText->SetText(FText::FromString(TEXT("Close")));
	CloseText->SetJustification(ETextJustify::Center);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(CloseButton->AddChild(CloseText)))
	{
		ButtonSlot->SetPadding(FMargin(18.0f, 8.0f));
	}
	if (UVerticalBoxSlot* CloseSlot = RuntimeRoot->AddChildToVerticalBox(CloseButton))
	{
		CloseSlot->SetPadding(FMargin(24.0f, 12.0f, 24.0f, 16.0f));
	}
}

void UActiveSkillLoadoutWidget::RefreshLoadout()
{
	BuildRuntimeLayout();

	UPlayerActiveSkillComponent* ActiveSkillComponent = ResolveActiveSkillComponent();
	const TArray<FActiveSkillSlotView> Slots = ActiveSkillComponent
		? ActiveSkillComponent->GetSlotViews()
		: TArray<FActiveSkillSlotView>();

	if (StatusText)
	{
		StatusText->SetText(ActiveSkillComponent
			? FText::FromString(TEXT("Choose a slot to equip Shield Burst."))
			: FText::FromString(TEXT("Player active skill component not found.")));
	}
	if (Slot0Text)
	{
		Slot0Text->SetText(DescribeSlot(Slots, 0));
	}
	if (Slot1Text)
	{
		Slot1Text->SetText(DescribeSlot(Slots, 1));
	}
	if (Slot0Button)
	{
		Slot0Button->SetIsEnabled(ActiveSkillComponent && Slots.IsValidIndex(0) && !Slots[0].bLocked);
	}
	if (Slot1Button)
	{
		Slot1Button->SetIsEnabled(ActiveSkillComponent && Slots.IsValidIndex(1) && !Slots[1].bLocked);
	}
}

UPlayerActiveSkillComponent* UActiveSkillLoadoutWidget::ResolveActiveSkillComponent() const
{
	APlayerController* PC = GetOwningPlayer();
	APlayerCharacterBase* Player = PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
	if (!Player)
	{
		Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	}
	return Player ? Player->ActiveSkillComponent : nullptr;
}

UActiveSkillDataAsset* UActiveSkillLoadoutWidget::ResolveDefaultSkill() const
{
	for (UActiveSkillDataAsset* SkillAsset : AvailableSkillAssets)
	{
		if (SkillAsset)
		{
			return SkillAsset;
		}
	}

	return DefaultSkillAsset.LoadSynchronous();
}

void UActiveSkillLoadoutWidget::AssignDefaultSkillToSlot(int32 SlotIndex)
{
	UPlayerActiveSkillComponent* ActiveSkillComponent = ResolveActiveSkillComponent();
	UActiveSkillDataAsset* SkillAsset = ResolveDefaultSkill();
	if (!ActiveSkillComponent || !SkillAsset)
	{
		return;
	}

	TArray<UActiveSkillDataAsset*> Loadout = ActiveSkillComponent->GetSkillLoadout();
	const int32 RequiredNum = FMath::Max(SlotIndex + 1, ActiveSkillComponent->MaxSlotCount);
	while (Loadout.Num() < RequiredNum)
	{
		Loadout.Add(nullptr);
	}

	Loadout[SlotIndex] = SkillAsset;
	ActiveSkillComponent->SetSkillLoadout(Loadout);
	ActiveSkillComponent->SetActiveSlotIndex(SlotIndex);
	PersistLoadout(ActiveSkillComponent);
	RefreshLoadout();
}

void UActiveSkillLoadoutWidget::PersistLoadout(UPlayerActiveSkillComponent* ActiveSkillComponent)
{
	if (!ActiveSkillComponent)
	{
		return;
	}

	const TArray<UActiveSkillDataAsset*> Loadout = ActiveSkillComponent->GetSkillLoadout();

	if (UYogGameInstanceBase* GI = GetGameInstance<UYogGameInstanceBase>())
	{
		if (GI->PendingRunState.bIsValid)
		{
			GI->PendingRunState.SelectedSkillLoadout.Reset();
			for (UActiveSkillDataAsset* Skill : Loadout)
			{
				GI->PendingRunState.SelectedSkillLoadout.Add(Skill);
			}
		}

		if (UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			UYogSaveGame* SaveGame = SaveSubsystem->GetCurrentSave();
			if (SaveGame)
			{
				SaveSubsystem->SavePlayer(SaveGame);
				SaveSubsystem->DoAsyncSave();
			}
		}
	}
}

FText UActiveSkillLoadoutWidget::DescribeSlot(const TArray<FActiveSkillSlotView>& Slots, int32 SlotIndex) const
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return FText::FromString(FString::Printf(TEXT("Slot %d: unavailable"), SlotIndex + 1));
	}

	const FActiveSkillSlotView& SkillView = Slots[SlotIndex];
	if (SkillView.bLocked)
	{
		return FText::FromString(FString::Printf(TEXT("Slot %d: locked"), SlotIndex + 1));
	}

	const FString SkillName = SkillView.DisplayName.IsEmpty()
		? (SkillView.SkillId != NAME_None ? SkillView.SkillId.ToString() : TEXT("Empty"))
		: SkillView.DisplayName.ToString();
	return FText::FromString(FString::Printf(TEXT("Slot %d: %s"), SlotIndex + 1, *SkillName));
}

void UActiveSkillLoadoutWidget::HandleSlot0Clicked()
{
	AssignDefaultSkillToSlot(0);
}

void UActiveSkillLoadoutWidget::HandleSlot1Clicked()
{
	AssignDefaultSkillToSlot(1);
}

void UActiveSkillLoadoutWidget::HandleCloseClicked()
{
	DeactivateWidget();
}
