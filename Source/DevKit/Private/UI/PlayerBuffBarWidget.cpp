#include "UI/PlayerBuffBarWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"

void UPlayerBuffBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	EnsureRuntimeWidgetTree();
	RefreshBuffIcons();
}

void UPlayerBuffBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= RefreshIntervalSeconds)
	{
		RefreshAccumulator = 0.f;
		RefreshBuffIcons();
	}
}

void UPlayerBuffBarWidget::EnsureRuntimeWidgetTree()
{
	if (BuffIconBox || !WidgetTree)
	{
		return;
	}

	BuffIconBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuffIconBox"));
	if (BuffIconBox)
	{
		BuffIconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = BuffIconBox;
	}
}

void UPlayerBuffBarWidget::RefreshBuffIcons()
{
	EnsureRuntimeWidgetTree();

	const TArray<FString> Labels = CollectVisibleBuffLabels();
	if (Labels == CachedLabels)
	{
		return;
	}

	CachedLabels = Labels;
	RebuildBuffIcons(CachedLabels);
}

void UPlayerBuffBarWidget::RebuildBuffIcons(const TArray<FString>& Labels)
{
	if (!BuffIconBox || !WidgetTree)
	{
		return;
	}

	BuffIconBox->ClearChildren();
	BuffIconBox->SetVisibility(Labels.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		const FName SlotName = *FString::Printf(TEXT("BuffIcon_%02d"), Index);
		const FName FrameName = *FString::Printf(TEXT("BuffIconFrame_%02d"), Index);
		const FName LabelName = *FString::Printf(TEXT("BuffIconLabel_%02d"), Index);

		USizeBox* IconSlot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), SlotName);
		UBorder* IconFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FrameName);
		UTextBlock* IconLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		if (!IconSlot || !IconFrame || !IconLabel)
		{
			continue;
		}

		IconSlot->SetWidthOverride(32.f);
		IconSlot->SetHeightOverride(32.f);
		IconSlot->SetMinDesiredWidth(32.f);
		IconSlot->SetMinDesiredHeight(32.f);

		const bool bIsPlaceholder = Labels[Index].IsEmpty();
		IconFrame->SetBrushColor(bIsPlaceholder
			? FLinearColor(0.025f, 0.045f, 0.052f, 0.44f)
			: FLinearColor(0.025f, 0.045f, 0.052f, 0.88f));
		IconFrame->SetPadding(FMargin(2.f));
		IconFrame->SetVisibility(ESlateVisibility::HitTestInvisible);

		IconLabel->SetText(FText::FromString(Labels[Index]));
		IconLabel->SetJustification(ETextJustify::Center);
		IconLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.90f, 1.0f, 0.96f)));
		IconLabel->SetShadowOffset(FVector2D(1.f, 1.f));
		IconLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));

		FSlateFontInfo FontInfo = IconLabel->GetFont();
		FontInfo.Size = Labels[Index].StartsWith(TEXT("+")) ? 12 : 10;
		IconLabel->SetFont(FontInfo);

		IconFrame->SetContent(IconLabel);
		IconSlot->AddChild(IconFrame);

		if (UHorizontalBoxSlot* BoxSlot = BuffIconBox->AddChildToHorizontalBox(IconSlot))
		{
			BoxSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));
			BoxSlot->SetHorizontalAlignment(HAlign_Left);
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

TArray<FString> UPlayerBuffBarWidget::CollectVisibleBuffLabels() const
{
	TArray<FString> Labels;

	APawn* OwningPawn = GetOwningPlayerPawn();
	const UAbilitySystemComponent* ASC = OwningPawn
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwningPawn)
		: nullptr;
	if (!ASC)
	{
		return MakeEmptyPlaceholderLabels();
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	TArray<FGameplayTag> TagArray;
	OwnedTags.GetGameplayTagArray(TagArray);
	TagArray.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		return Left.ToString() < Right.ToString();
	});

	TArray<FString> FilteredTags;
	for (const FGameplayTag& Tag : TagArray)
	{
		const FString TagString = Tag.ToString();
		if (ShouldShowBuffTag(TagString))
		{
			FilteredTags.Add(TagString);
		}
	}

	const int32 DisplayLimit = FMath::Max(1, MaxDisplayedBuffs);
	const bool bHasOverflow = FilteredTags.Num() > DisplayLimit;
	const int32 ExplicitLabelLimit = bHasOverflow ? DisplayLimit - 1 : DisplayLimit;
	for (int32 Index = 0; Index < FilteredTags.Num() && Labels.Num() < ExplicitLabelLimit; ++Index)
	{
		Labels.Add(MakeShortBuffLabel(FilteredTags[Index]));
	}

	if (bHasOverflow)
	{
		Labels.Add(FString::Printf(TEXT("+%d"), FilteredTags.Num() - ExplicitLabelLimit));
	}

	if (Labels.IsEmpty())
	{
		return MakeEmptyPlaceholderLabels();
	}

	return Labels;
}

TArray<FString> UPlayerBuffBarWidget::MakeEmptyPlaceholderLabels() const
{
	TArray<FString> Labels;
	if (!bShowEmptyPlaceholderSlots)
	{
		return Labels;
	}

	const int32 PlaceholderCount = FMath::Clamp(EmptyPlaceholderSlotCount, 1, MaxDisplayedBuffs);
	Labels.Reserve(PlaceholderCount);
	for (int32 Index = 0; Index < PlaceholderCount; ++Index)
	{
		Labels.Add(FString());
	}

	return Labels;
}

bool UPlayerBuffBarWidget::ShouldShowBuffTag(const FString& TagString)
{
	if (!TagString.StartsWith(TEXT("Buff.Status.")))
	{
		return false;
	}

	return !TagString.StartsWith(TEXT("Buff.Status.Dead"))
		&& !TagString.StartsWith(TEXT("Buff.Status.HitReact"))
		&& !TagString.StartsWith(TEXT("Buff.Status.HitStop"))
		&& !TagString.StartsWith(TEXT("Buff.Status.Heat.Phase"));
}

FString UPlayerBuffBarWidget::MakeShortBuffLabel(const FString& TagString)
{
	FString Leaf = TagString;
	int32 LastDotIndex = INDEX_NONE;
	if (TagString.FindLastChar(TEXT('.'), LastDotIndex))
	{
		Leaf = TagString.Mid(LastDotIndex + 1);
	}

	FString UppercaseLetters;
	for (int32 Index = 0; Index < Leaf.Len(); ++Index)
	{
		if (FChar::IsUpper(Leaf[Index]))
		{
			UppercaseLetters.AppendChar(Leaf[Index]);
		}
	}

	if (UppercaseLetters.Len() >= 2)
	{
		return UppercaseLetters.Left(3);
	}

	return Leaf.Left(2).ToUpper();
}
