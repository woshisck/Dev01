// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LootSelectionWidget.h"
#include "UI/RuneInfoCardWidget.h"
#include "UI/YogHUD.h"
#include "GameModes/YogGameMode.h"
#include "Map/RewardPickup.h"
#include "Data/RuneDataAsset.h"
#include "Character/YogPlayerControllerBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Input/CommonUIInputTypes.h"

static_assert(ULootSelectionWidget::MaxCards == 6, "Card click/hover callbacks must match MaxCards.");

// ============================================================
//  生命周期
// ============================================================

void ULootSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	SetIsFocusable(true);

	// Phase 变化保留监听（控制整体显隐 BP 事件）；OnLootGenerated 已废弃
	// 改由 RewardPickup 通过 HUD::QueueLootSelection 直接触发，确保 SourcePickup 不丢失
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->OnPhaseChanged.AddDynamic(this, &ULootSelectionWidget::HandlePhaseChanged);
	}

	if (BtnSkip)
		BtnSkip->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnBtnSkipClicked);
	if (BtnBackpackPreview)
		BtnBackpackPreview->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnBtnBackpackPreviewClicked);
}

void ULootSelectionWidget::NativeDestruct()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->OnPhaseChanged.RemoveDynamic(this, &ULootSelectionWidget::HandlePhaseChanged);
	}

	Super::NativeDestruct();
}

void ULootSelectionWidget::HandlePhaseChanged(ELevelPhase NewPhase)
{
	OnLevelPhaseChanged(NewPhase);
}

// ============================================================
//  显示
// ============================================================

void ULootSelectionWidget::ShowLootUI(const TArray<FLootOption>& Options, ARewardPickup* InSourcePickup)
{
	CurrentLootOptions = Options;
	SourcePickup       = InSourcePickup;

	// 同步到 GameMode：SelectLoot(Index) 会读 GM->CurrentLootOptions[Index] 来发放符文。
	// RewardPickup 的 AssignedLoot 直接走 HUD 队列绕开了 GM->ShowLootOptions，
	// 不在这里同步会导致选符文时取到空/陈旧的 GM 数据。
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->CurrentLootOptions = Options;
	}

	if (!RuneCardClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootSelection] ShowLootUI: RuneCardClass 未配置（Class Defaults 检查 WBP_RuneInfoCard）"));
	}
	if (!CardContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootSelection] ShowLootUI: CardContainer BindWidget 缺失"));
		return;
	}

	RebuildCards(Options);
	OnLootOptionsReady(Options);

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();
		const bool bPaused = PC->SetPause(true);
		UE_LOG(LogTemp, Log, TEXT("[LootSelection] ShowLootUI: SetPause result=%d"), bPaused ? 1 : 0);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
		// 兜底：SetPause + InputMode UIOnly 仍可能让 EnhancedInput 透传到 Pawn，
		// 显式 DisableInput 关掉 Pawn/Controller 输入，确保不能跑/攻击
		if (APawn* Pawn = PC->GetPawn())
			Pawn->DisableInput(PC);
		PC->DisableInput(PC);
	}

	SetUserFocus(GetOwningPlayer());

	bStickNavHeld = false;
	CurrentSection = ELootFocusSection::Cards;
	CurrentCardIndex = 0;
	CurrentButtonIndex = 0;
	if (SpawnedCards.Num() > 0)
		FocusCard(0);
	else
		SetSection(ELootFocusSection::Buttons);  // 没卡时直接落到按钮段

	UE_LOG(LogTemp, Log, TEXT("[LootSelection] ShowLootUI: NumOptions=%d SourcePickup=%s"),
		Options.Num(), InSourcePickup ? *InSourcePickup->GetName() : TEXT("null"));
}

// ============================================================
//  动态卡片
// ============================================================

void ULootSelectionWidget::RebuildCards(const TArray<FLootOption>& Options)
{
	// 清空现有卡片 + 按钮包装
	if (CardContainer) CardContainer->ClearChildren();
	SpawnedCards.Reset();
	SpawnedCardButtons.Reset();
	CardToOptionIndex.Reset();

	if (!RuneCardClass || !CardContainer) return;

	TArray<int32> ValidOptionIndices;
	ValidOptionIndices.Reserve(FMath::Min(Options.Num(), MaxCards));
	for (int32 i = 0; i < Options.Num() && ValidOptionIndices.Num() < MaxCards; ++i)
	{
		const FLootOption& Opt = Options[i];
		if (Opt.LootType == ELootType::Rune && Opt.RuneAsset)
		{
			ValidOptionIndices.Add(i);
		}
	}

	if (ValidOptionIndices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootSelection] RebuildCards: no valid rune options to display."));
		return;
	}

	const int32 N = ValidOptionIndices.Num();
	if (N < Options.Num())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[LootSelection] Options=%d, valid visible cards=%d, MaxCards=%d"),
			Options.Num(), N, MaxCards);
	}

	// 先统计有效卡数（非 Rune 或 RuneAsset=null 的项不算），避免无效项让 5/6 张全部缩小
	const int32 ValidCount = N;

	// ValidCount <= 4: 单行大卡 320×420，不强制换行
	// ValidCount >= 5: 缩成 240×360，WrapSize=820 强制 3 列（5 张 → 3+2，6 张 → 3+3）
	const FVector2D CardSize = (ValidCount <= 4) ? LargeCardSize : SmallCardSize;
	// WrapBox 整体水平居中（让换行后两行都居中，5 卡的 3+2 排列第二行不左对齐）
	CardContainer->SetHorizontalAlignment(HAlign_Center);
	if (ValidCount >= 5)
	{
		CardContainer->SetExplicitWrapSize(true);
		const float ThreeColumnWidth = 3.f * (SmallCardSize.X * 1.06f + CardHorizontalPadding * 2.f) + 24.f;
		CardContainer->SetWrapSize(FMath::Max(WrapBoxWrapWidthFor3Plus, ThreeColumnWidth));
	}
	else
	{
		// 不强制换行（让 WBP 默认行为生效）
		CardContainer->SetExplicitWrapSize(false);
	}

	for (int32 i = 0; i < N; ++i)
	{
		const int32 OptionIndex = ValidOptionIndices[i];
		const FLootOption& Opt = Options[OptionIndex];

		// 卡片本体
		URuneInfoCardWidget* Card = CreateWidget<URuneInfoCardWidget>(this, RuneCardClass);
		if (!Card) continue;

		Card->ShowRune(Opt.RuneAsset->RuneInfo);
		Card->SetGenericEffectsExpanded(false);  // 默认折叠，FocusCard 时按聚焦展开
		Card->SetSelected(false);                 // 重建时清场，FocusCard 再统一设当前选中

		// SizeBox 强制 Override 宽高（动态尺寸），避免 WBP_RuneInfoCard 自身 Desired Size 不一致
		USizeBox* SizeBox = WidgetTree
			? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass())
			: NewObject<USizeBox>(this);
		SizeBox->SetMinDesiredWidth(CardSize.X);
		SizeBox->SetMaxDesiredWidth(CardSize.X);
		SizeBox->SetMinDesiredHeight(CardSize.Y);
		SizeBox->SetMaxDesiredHeight(CardSize.Y);
		SizeBox->AddChild(Card);

		// 外包 UButton 处理鼠标点击
		UButton* Wrapper = WidgetTree
			? WidgetTree->ConstructWidget<UButton>(UButton::StaticClass())
			: NewObject<UButton>(this);
		Wrapper->SetVisibility(ESlateVisibility::Visible);
		Wrapper->AddChild(SizeBox);

		// 全透明 ButtonStyle，避免盖住卡片视觉（Designer 可在 WBP 里覆盖此默认）
		FButtonStyle Style = Wrapper->GetStyle();
		Style.Normal.TintColor   = FSlateColor(FLinearColor(1, 1, 1, 0));
		Style.Hovered.TintColor  = FSlateColor(FLinearColor(1, 1, 1, 0));
		Style.Pressed.TintColor  = FSlateColor(FLinearColor(1, 1, 1, 0));
		Style.Disabled.TintColor = FSlateColor(FLinearColor(1, 1, 1, 0));
		Wrapper->SetStyle(Style);

		// 静态绑定（dynamic delegate 不支持 lambda/带参，预声明 6 个回调与 MaxCards 对齐）。
		// 注意：用"可见卡片索引"（SpawnedCards.Num() 当前值）而不是原始 Options 索引 i —
		// 否则若 i=0 项无效被 continue 跳过，第一张可见卡会绑到 OnCardClicked1，
		// 而 CardToOptionIndex 只有 1 个元素，导致 ResolveCardToOption 越界失败
		const int32 VisibleIdx = SpawnedCards.Num();
		switch (VisibleIdx)
		{
		case 0:
			Wrapper->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnCardClicked0);
			Wrapper->OnHovered.AddDynamic(this, &ULootSelectionWidget::OnCardHovered0);
			break;
		case 1:
			Wrapper->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnCardClicked1);
			Wrapper->OnHovered.AddDynamic(this, &ULootSelectionWidget::OnCardHovered1);
			break;
		case 2:
			Wrapper->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnCardClicked2);
			Wrapper->OnHovered.AddDynamic(this, &ULootSelectionWidget::OnCardHovered2);
			break;
		case 3:
			Wrapper->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnCardClicked3);
			Wrapper->OnHovered.AddDynamic(this, &ULootSelectionWidget::OnCardHovered3);
			break;
		case 4:
			Wrapper->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnCardClicked4);
			Wrapper->OnHovered.AddDynamic(this, &ULootSelectionWidget::OnCardHovered4);
			break;
		case 5:
			Wrapper->OnClicked.AddDynamic(this, &ULootSelectionWidget::OnCardClicked5);
			Wrapper->OnHovered.AddDynamic(this, &ULootSelectionWidget::OnCardHovered5);
			break;
		default: break;
		}

		UPanelSlot* PSlot = CardContainer->AddChild(Wrapper);
		if (UWrapBoxSlot* WSlot = Cast<UWrapBoxSlot>(PSlot))
		{
			WSlot->SetPadding(FMargin(CardHorizontalPadding));
			WSlot->SetHorizontalAlignment(HAlign_Center);
			WSlot->SetVerticalAlignment(VAlign_Top);
		}

		SpawnedCards.Add(Card);
		SpawnedCardButtons.Add(Wrapper);
		CardToOptionIndex.Add(OptionIndex);  // 记录卡片→Options 映射，键盘选中时用
	}
}

int32 ULootSelectionWidget::ClampCardIndex(int32 Idx) const
{
	const int32 N = SpawnedCards.Num();
	if (N <= 0) return -1;
	return FMath::Clamp(Idx, 0, N - 1);
}

void ULootSelectionWidget::FocusCard(int32 Idx)
{
	const int32 N = SpawnedCards.Num();
	if (N == 0) return;

	CurrentCardIndex = ClampCardIndex(Idx);

	for (int32 i = 0; i < N; ++i)
	{
		if (URuneInfoCardWidget* C = SpawnedCards[i])
		{
			const bool bIsCurrent = (i == CurrentCardIndex);
			C->SetGenericEffectsExpanded(bIsCurrent);
			C->SetSelected(bIsCurrent);   // C++ 自管视觉高亮（缩放 + 边框）
		}
	}

	// 段切到 Cards 时按钮段不应继续高亮
	UpdateButtonHighlight(-1);

	OnCardFocused(CurrentCardIndex);
	OnSectionFocused(ELootFocusSection::Cards, CurrentCardIndex);
}

void ULootSelectionWidget::FocusButton(int32 Idx)
{
	CurrentButtonIndex = FMath::Clamp(Idx, 0, NumBottomButtons - 1);

	// 切到按钮段：清掉所有卡片选中态，只在按钮 Border 上高亮
	ClearAllCardSelection();
	UpdateButtonHighlight(CurrentButtonIndex);

	OnSectionFocused(ELootFocusSection::Buttons, CurrentButtonIndex);
}

void ULootSelectionWidget::SetSection(ELootFocusSection NewSection)
{
	CurrentSection = NewSection;
	if (NewSection == ELootFocusSection::Cards)
	{
		FocusCard(CurrentCardIndex);
	}
	else
	{
		FocusButton(CurrentButtonIndex);
	}
}

void ULootSelectionWidget::ClearAllCardSelection()
{
	// 切到按钮段时同步折叠所有卡的 GenericEffectList，
	// 避免最后选中那张卡的副窗仍然展开造成"按钮在焦点但卡详情还在"的状态残留
	for (URuneInfoCardWidget* C : SpawnedCards)
	{
		if (C)
		{
			C->SetSelected(false);
			C->SetGenericEffectsExpanded(false);
		}
	}
}

void ULootSelectionWidget::UpdateButtonHighlight(int32 SelectedIdx)
{
	// SelectedIdx == -1 表示按钮段当前不在焦点上 -> 全部隐藏
	if (SkipHighlightBorder)
		SkipHighlightBorder->SetVisibility(SelectedIdx == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (PreviewHighlightBorder)
		PreviewHighlightBorder->SetVisibility(SelectedIdx == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

// ============================================================
//  公开操作
// ============================================================

void ULootSelectionWidget::SelectRuneLoot(int32 Index)
{
	if (!CurrentLootOptions.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootSelection] SelectRuneLoot: Index=%d 越界（Num=%d）"),
			Index, CurrentLootOptions.Num());
		return;
	}

	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		GM->SelectLoot(Index);

	// 销毁源拾取物
	if (ARewardPickup* Pickup = SourcePickup.Get())
	{
		Pickup->ConsumeAndDestroy();
	}
	SourcePickup.Reset();

	// 关闭 UI
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		// 配对 ShowLootUI 的 DisableInput
		if (APawn* Pawn = PC->GetPawn())
			Pawn->EnableInput(PC);
		PC->EnableInput(PC);
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->EndPauseEffect();
		}
	}

	FinishAndNotifyHUD();
}

void ULootSelectionWidget::SkipSelection()
{
	UE_LOG(LogTemp, Log, TEXT("[LootSelection] SkipSelection"));

	APlayerCharacterBase* Player = nullptr;
	if (APlayerController* PC = GetOwningPlayer())
		Player = Cast<APlayerCharacterBase>(PC->GetPawn());

	if (ARewardPickup* Pickup = SourcePickup.Get())
	{
		Pickup->ResetForSkip(Player);
	}
	SourcePickup.Reset();

	// 关闭 UI（不开背包）
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		// 配对 ShowLootUI 的 DisableInput
		if (APawn* Pawn = PC->GetPawn())
			Pawn->EnableInput(PC);
		PC->EnableInput(PC);
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();
	}

	FinishAndNotifyHUD();
}

void ULootSelectionWidget::OpenBackpackPreview()
{
	UE_LOG(LogTemp, Log, TEXT("[LootSelection] OpenBackpackPreview"));

	if (bIsBackpackPreviewing) return;
	bIsBackpackPreviewing = true;

	// 隐藏自身（保留 SourcePickup / CurrentLootOptions / 卡片实例不重建）
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			FSimpleDelegate Cb;
			Cb.BindUObject(this, &ULootSelectionWidget::ReactivateAfterPreview);
			HUD->OpenBackpackForPreview(Cb);
		}
	}
}

void ULootSelectionWidget::ReactivateAfterPreview()
{
	UE_LOG(LogTemp, Log, TEXT("[LootSelection] ReactivateAfterPreview"));
	bIsBackpackPreviewing = false;

	// 恢复可见 + 焦点 + 输入模式
	// 注意：不再调 BeginPauseEffect — LootSelection 自身的 pause count 在 OpenBackpackPreview 时未释放，
	// Backpack 自己的 Begin/End 已在其 NativeOnActivated/NativeOnDeactivated 中配对。
	// 如果这里再 Begin 会造成 PausePopupCount 泄漏。
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}

	SetUserFocus(GetOwningPlayer());
	SetSection(CurrentSection);
}

void ULootSelectionWidget::RerollCard(int32 Index)
{
	UE_LOG(LogTemp, Warning, TEXT("[LootSelection] RerollCard(%d) — 接口未实现，留作后续 UI 动画扩展"), Index);
}

void ULootSelectionWidget::ConfirmAndTransition()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		GM->ConfirmArrangementAndTransition();
}

void ULootSelectionWidget::FinishAndNotifyHUD()
{
	if (APlayerController* PC = GetOwningPlayer())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->OnLootSelectionFinished();
}

// ============================================================
//  按钮 / 卡片回调
// ============================================================

// 鼠标点击通过 CardToOptionIndex 映射到原始 Options 索引，
// 与键盘/手柄确认路径一致 — 处理 Options 中含无效项被跳过时索引错位的情况
namespace
{
	int32 ResolveCardToOption(const TArray<int32>& Map, int32 CardIdx)
	{
		return Map.IsValidIndex(CardIdx) ? Map[CardIdx] : -1;
	}
}

void ULootSelectionWidget::OnCardClicked0()
{
	const int32 Idx = ResolveCardToOption(CardToOptionIndex, 0);
	if (Idx >= 0) SelectRuneLoot(Idx);
}
void ULootSelectionWidget::OnCardClicked1()
{
	const int32 Idx = ResolveCardToOption(CardToOptionIndex, 1);
	if (Idx >= 0) SelectRuneLoot(Idx);
}
void ULootSelectionWidget::OnCardClicked2()
{
	const int32 Idx = ResolveCardToOption(CardToOptionIndex, 2);
	if (Idx >= 0) SelectRuneLoot(Idx);
}
void ULootSelectionWidget::OnCardClicked3()
{
	const int32 Idx = ResolveCardToOption(CardToOptionIndex, 3);
	if (Idx >= 0) SelectRuneLoot(Idx);
}
void ULootSelectionWidget::OnCardClicked4()
{
	const int32 Idx = ResolveCardToOption(CardToOptionIndex, 4);
	if (Idx >= 0) SelectRuneLoot(Idx);
}
void ULootSelectionWidget::OnCardClicked5()
{
	const int32 Idx = ResolveCardToOption(CardToOptionIndex, 5);
	if (Idx >= 0) SelectRuneLoot(Idx);
}

// 鼠标 hover 卡片：与手柄/键盘等价 — 切段到 Cards + FocusCard 那张
// 鼠标移开后不主动复位，最后一次 hover/键盘选择保持高亮
void ULootSelectionWidget::HandleCardHover(int32 VisibleIdx)
{
	if (!SpawnedCards.IsValidIndex(VisibleIdx)) return;
	// 已是当前选中卡且已在 Cards 段则跳过，避免重复刷新
	if (CurrentSection == ELootFocusSection::Cards && CurrentCardIndex == VisibleIdx) return;

	CurrentSection = ELootFocusSection::Cards;
	FocusCard(VisibleIdx);
}

void ULootSelectionWidget::OnCardHovered0() { HandleCardHover(0); }
void ULootSelectionWidget::OnCardHovered1() { HandleCardHover(1); }
void ULootSelectionWidget::OnCardHovered2() { HandleCardHover(2); }
void ULootSelectionWidget::OnCardHovered3() { HandleCardHover(3); }
void ULootSelectionWidget::OnCardHovered4() { HandleCardHover(4); }
void ULootSelectionWidget::OnCardHovered5() { HandleCardHover(5); }

void ULootSelectionWidget::OnBtnSkipClicked() { SkipSelection(); }
void ULootSelectionWidget::OnBtnBackpackPreviewClicked() { OpenBackpackPreview(); }

// ============================================================
//  CommonUI 生命周期（仅供 Stack 场景保留，手动放置路径不走这里）
// ============================================================

TOptional<FUIInputConfig> ULootSelectionWidget::GetDesiredInputConfig() const
{
	// 不让 CommonUI 管理此 Widget 的输入
	return TOptional<FUIInputConfig>();
}

void ULootSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
		// 兜底禁用 Pawn 输入，避免 EnhancedInput 透传
		if (APawn* Pawn = PC->GetPawn())
			Pawn->DisableInput(PC);
		PC->DisableInput(PC);
	}

	SetUserFocus(GetOwningPlayer());
}

void ULootSelectionWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	// 不调 Super：避免 BP_OnFocusLost → RemoveFromParent 销毁本控件
}

void ULootSelectionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		// 配对 NativeOnActivated 的 DisableInput
		if (APawn* Pawn = PC->GetPawn())
			Pawn->EnableInput(PC);
		PC->EnableInput(PC);
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();
	}

	// 不调 Super：避免 CommonUI bDestroyOnDeactivation 造成本控件销毁
}

// ============================================================
//  输入
// ============================================================

FReply ULootSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// ── 全局快捷键 ─────────────────────────────────────────
	// B / Circle / Esc → 跳过
	if (Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Escape)
	{
		SkipSelection();
		return FReply::Handled();
	}
	// Y / Triangle / Tab → 预览背包
	if (Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Tab)
	{
		OpenBackpackPreview();
		return FReply::Handled();
	}

	// ── 段切换 ─────────────────────────────────────────────
	if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Up)
	{
		SetSection(ELootFocusSection::Cards);
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Down)
	{
		SetSection(ELootFocusSection::Buttons);
		return FReply::Handled();
	}

	// ── 段内左右导航 + 确认 ─────────────────────────────────
	const int32 MaxCardIdx = SpawnedCards.Num() - 1;
	const int32 MaxButtonIdx = NumBottomButtons - 1;

	if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Left)
	{
		if (CurrentSection == ELootFocusSection::Cards && MaxCardIdx >= 0)
		{
			CurrentCardIndex = FMath::Max(0, CurrentCardIndex - 1);
			OnNavigateSelection(-1);
			FocusCard(CurrentCardIndex);
		}
		else if (CurrentSection == ELootFocusSection::Buttons)
		{
			CurrentButtonIndex = FMath::Max(0, CurrentButtonIndex - 1);
			FocusButton(CurrentButtonIndex);
		}
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Right)
	{
		if (CurrentSection == ELootFocusSection::Cards && MaxCardIdx >= 0)
		{
			CurrentCardIndex = FMath::Min(MaxCardIdx, CurrentCardIndex + 1);
			OnNavigateSelection(1);
			FocusCard(CurrentCardIndex);
		}
		else if (CurrentSection == ELootFocusSection::Buttons)
		{
			CurrentButtonIndex = FMath::Min(MaxButtonIdx, CurrentButtonIndex + 1);
			FocusButton(CurrentButtonIndex);
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter)
	{
		if (CurrentSection == ELootFocusSection::Cards && MaxCardIdx >= 0)
		{
			// 通过映射拿原始 Options 索引（处理无效项被跳过的情况）
			const int32 OptionIdx = CardToOptionIndex.IsValidIndex(CurrentCardIndex)
				? CardToOptionIndex[CurrentCardIndex] : -1;
			if (OptionIdx >= 0) SelectRuneLoot(OptionIdx);
		}
		else if (CurrentSection == ELootFocusSection::Buttons)
		{
			if (CurrentButtonIndex == 0) SkipSelection();
			else if (CurrentButtonIndex == 1) OpenBackpackPreview();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply ULootSelectionWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	if (GetVisibility() == ESlateVisibility::Collapsed)
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);

	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();

	if (Key == EKeys::Gamepad_LeftX)
	{
		const int32 MaxCardIdx = SpawnedCards.Num() - 1;
		const int32 MaxButtonIdx = NumBottomButtons - 1;
		if (FMath::Abs(Value) > 0.5f && !bStickNavHeld)
		{
			bStickNavHeld = true;
			const int32 Delta = (Value < 0.f) ? -1 : 1;

			if (CurrentSection == ELootFocusSection::Cards && MaxCardIdx >= 0)
			{
				CurrentCardIndex = FMath::Clamp(CurrentCardIndex + Delta, 0, MaxCardIdx);
				OnNavigateSelection(Delta);
				FocusCard(CurrentCardIndex);
			}
			else if (CurrentSection == ELootFocusSection::Buttons)
			{
				CurrentButtonIndex = FMath::Clamp(CurrentButtonIndex + Delta, 0, MaxButtonIdx);
				FocusButton(CurrentButtonIndex);
			}
			return FReply::Handled();
		}
		if (FMath::Abs(Value) < 0.3f)
			bStickNavHeld = false;
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
}
