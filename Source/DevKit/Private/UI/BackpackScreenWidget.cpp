#include "UI/BackpackScreenWidget.h"
#include "UI/BackpackGridWidget.h"
#include "UI/PendingGridWidget.h"
#include "UI/RuneDragDropOperation.h"
#include "UI/RuneTooltipWidget.h"
#include "UI/RuneInfoCardWidget.h"
#include "UI/CombatDeckEditWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Component/BackpackGridComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogPlayerControllerBase.h"
#include "CommonInputSubsystem.h"
#include "Input/CommonUIInputTypes.h"
#include "GameFramework/Pawn.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InputCoreTypes.h"
#include "UI/WeaponComboTextUtils.h"
#include "UI/YogHUD.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "GameModes/YogGameMode.h"
#include "Story/StoryEngineSubsystem.h"
#include "TimerManager.h"
#include "SaveGame/YogSaveSubsystem.h"

// ============================================================
//  鍐呴儴杈呭姪
// ============================================================

UBackpackGridComponent* UBackpackScreenWidget::GetBackpack() const
{
    if (CachedBackpack.IsValid())
        return CachedBackpack.Get();

    APawn* Pawn = GetOwningPlayerPawn();
    if (!Pawn) return nullptr;

    return Pawn->FindComponentByClass<UBackpackGridComponent>();
}

void UBackpackScreenWidget::SetTextIfSupported(UWidget* Widget, const FText& Text) const
{
    if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        TextBlock->SetText(Text);
        return;
    }

    if (URichTextBlock* RichTextBlock = Cast<URichTextBlock>(Widget))
    {
        RichTextBlock->SetText(Text);
    }
}

FText UBackpackScreenWidget::BuildOperationHintText() const
{
    if (bIsPreviewMode)
    {
        return FText::FromString(TEXT("Preview mode: backpack editing is locked."));
    }

    if (bIsGamepadInputMode)
    {
        return FText::FromString(TEXT("Use <input action=\"Interact\"/> to confirm, <input action=\"ReverseCard\"/> to rotate/link."));
    }

    return FText::FromString(TEXT("<input action=\"MouseClick\"/> select/place  <input action=\"ReverseCard\"/> rotate/link"));
}

FText UBackpackScreenWidget::BuildConfirmButtonText() const
{
    return bIsGamepadInputMode
        ? FText::FromString(TEXT("<input action=\"Interact\"/> Confirm"))
        : FText::FromString(TEXT("<input action=\"MouseClick\"/> Confirm"));
}

FText UBackpackScreenWidget::BuildCancelButtonText() const
{
    // Gamepad close uses B (Gamepad_FaceButton_Right). The "Back" decorator action resolves
    // to IA_WeaponSkill on gamepad (B icon); "Esc" resolves to IA_Esc which is mapped to the Start
    // button glyph on gamepad 鈥?the wrong icon for this button.
    return bIsGamepadInputMode
        ? FText::FromString(TEXT("<input action=\"Back\"/> Cancel"))
        : FText::FromString(TEXT("<input action=\"Esc\"/> Cancel"));
}

FText UBackpackScreenWidget::BuildEndPreviewButtonText() const
{
    return bIsGamepadInputMode
        ? FText::FromString(TEXT("<input action=\"Back\"/> 缁撴潫棰勮"))
        : FText::FromString(TEXT("<input action=\"Esc\"/> 缁撴潫棰勮"));
}

void UBackpackScreenWidget::SetButtonContentText(UButton* Button, const FText& Text) const
{
    if (!Button)
    {
        return;
    }

    SetTextIfSupported(Button->GetContent(), Text);
}

void UBackpackScreenWidget::RefreshActionButtonHints()
{
    const bool bCurrentGamepad = bIsGamepadInputMode;
    if (bActionButtonHintsInitialized &&
        bLastActionButtonHintsGamepad == bCurrentGamepad &&
        bLastActionButtonHintsPreviewMode == bIsPreviewMode)
    {
        return;
    }

    bActionButtonHintsInitialized = true;
    bLastActionButtonHintsGamepad = bCurrentGamepad;
    bLastActionButtonHintsPreviewMode = bIsPreviewMode;

    SetButtonContentText(ConfirmButton, BuildConfirmButtonText());
    SetButtonContentText(CloseButton, BuildCancelButtonText());
    SetButtonContentText(EndPreviewButton, BuildEndPreviewButtonText());
}

FText UBackpackScreenWidget::BuildComboHintText(const UWeaponDefinition* WeaponDefinition) const
{
    return WeaponComboTextUtils::BuildComboHintText(WeaponDefinition, 4, false);
}

void UBackpackScreenWidget::RefreshWeaponAndComboInfo()
{
    const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    const UWeaponDefinition* WeaponDefinition = Player ? Player->EquippedWeaponDef.Get() : nullptr;
    const UWeaponInfoDA* WeaponInfo = WeaponDefinition ? WeaponDefinition->WeaponInfo.Get() : nullptr;

    FText WeaponName = FText::FromString(TEXT("No Weapon"));
    FText WeaponDesc = FText::FromString(TEXT("Equip a weapon to view its description and combo hints."));
    UTexture2D* Thumbnail = nullptr;

    if (WeaponDefinition)
    {
        WeaponName = WeaponInfo && !WeaponInfo->WeaponName.IsEmpty()
            ? WeaponInfo->WeaponName
            : FText::FromString(WeaponDefinition->GetName());
        if (WeaponInfo)
        {
            if (!WeaponInfo->WeaponDescription.IsEmpty())
            {
                WeaponDesc = WeaponInfo->WeaponDescription;
            }
            if (!WeaponInfo->WeaponSubDescription.IsEmpty())
            {
                const FString MainDesc = WeaponDesc.ToString();
                WeaponDesc = FText::FromString(MainDesc.IsEmpty()
                    ? WeaponInfo->WeaponSubDescription.ToString()
                    : FString::Printf(TEXT("%s\n%s"), *MainDesc, *WeaponInfo->WeaponSubDescription.ToString()));
            }
            Thumbnail = WeaponInfo->Thumbnail.Get();
        }
    }

    if (WeaponIcon)
    {
        if (!Thumbnail && BackpackGridWidget && BackpackGridWidget->StyleDA)
        {
            Thumbnail = BackpackGridWidget->StyleDA->DefaultWeaponIconTexture.Get();
        }

        if (Thumbnail)
        {
            WeaponIcon->SetBrushFromTexture(Thumbnail, true);
            WeaponIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            WeaponIcon->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    SetTextIfSupported(WeaponNameText, WeaponName);
    SetTextIfSupported(WeaponDescText, WeaponDesc);
    SetTextIfSupported(ComboHintText, BuildComboHintText(WeaponDefinition));
}

// ============================================================
//  鐢熷懡鍛ㄦ湡
// ============================================================

UBackpackScreenWidget::UBackpackScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsFocusable = true;
}

void UBackpackScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Log, TEXT("[BackpackScreen] NativeConstruct outer=%s"), *GetNameSafe(GetOuter()));

    SetVisibility(ESlateVisibility::Collapsed);

    if (APawn* Pawn = GetOwningPlayerPawn())
        CachedBackpack = Pawn->FindComponentByClass<UBackpackGridComponent>();

    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.AddDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.AddDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.AddDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }

    // 瀛?Widget 鍒濆鍖栵紙NativeConstruct 鏃跺瓙 Widget 宸叉瀯寤哄畬姣曪級
    if (BackpackGridWidget)
        BackpackGridWidget->BuildGrid(GetBackpack());

    if (PendingGridWidget)
    {
        PendingGridWidget->BuildSlots();
        PendingCols = PendingGridWidget->PendingGridCols;
        PendingRows = PendingGridWidget->PendingGridRows;
    }

    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->SetExternalDetailInfoCard(RuneInfoCard);
        CombatDeckEditWidget->BindToOwningPlayerCombatDeck();
        CombatDeckEditWidget->SetInteractionLocked(IsInCombatPhase() || bIsPreviewMode);
    }

    RefreshPendingRuneSlots();

    if (SellButton)
        SellButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnSellButtonClicked);

    if (CloseButton)
        CloseButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnCloseButtonClicked);

    if (ConfirmButton)
        ConfirmButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnConfirmButtonClicked);

    if (EndPreviewButton)
    {
        EndPreviewButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnEndPreviewClicked);
        EndPreviewButton->SetVisibility(ESlateVisibility::Collapsed);  // 榛樿闅愯棌锛岃繘鍏ラ瑙堟ā寮忔椂鐢?SetPreviewMode 鍒囧埌 Visible
    }
    // CloseButton 榛樿璁?WBP Designer 璁剧殑 Visibility 鐢熸晥锛堥€氬父鏄?Visible锛夛紝
    // SetPreviewMode 浼氬湪鍒囨崲鏃跺己鍒惰鐩栦负 Visible/Collapsed

    if (HintText)
        HintText->SetVisibility(ESlateVisibility::Collapsed);

    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UCommonInputSubsystem* CommonInput = ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(LocalPlayer))
        {
            CommonInput->OnInputMethodChangedNative.AddUObject(
                this, &UBackpackScreenWidget::HandleCommonInputMethodChanged);
        }
    }

    RefreshWeaponAndComboInfo();
    RefreshActionButtonHints();
    UpdateOperationHintVisibility();
}

void UBackpackScreenWidget::NativeDestruct()
{
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.RemoveDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.RemoveDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.RemoveDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }

    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UCommonInputSubsystem* CommonInput = ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(LocalPlayer))
        {
            CommonInput->OnInputMethodChangedNative.RemoveAll(this);
        }
    }

    Super::NativeDestruct();
}

void UBackpackScreenWidget::HandleCommonInputMethodChanged(ECommonInputType NewInputType)
{
    bIsGamepadInputMode = NewInputType == ECommonInputType::Gamepad;
    RefreshWeaponAndComboInfo();
    RefreshActionButtonHints();
    UpdateOperationHintVisibility();
}

// ============================================================
//  濮旀墭澶勭悊
// ============================================================

void UBackpackScreenWidget::HandleRunePlaced(const FRuneInstance& Rune)
{
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::HandleRuneRemoved(FGuid RuneGuid)
{
    if (SelectedCell != FIntPoint(-1, -1))
    {
        if (UBackpackGridComponent* Backpack = GetBackpack())
        {
            if (Backpack->GetRuneIndexAtCell(SelectedCell) == -1)
            {
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
            }
        }
    }
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated)
{
    OnGridNeedsRefresh();
}

// ============================================================
//  鏍煎瓙鍒锋柊锛堝鎵樼粰 BackpackGridWidget锛?
// ============================================================

void UBackpackScreenWidget::OnGridNeedsRefresh_Implementation()
{
    if (BackpackGridWidget)
    {
        // 鎵嬫焺妯″紡涓嬬洿鎺ョ敤 GamepadCursorCell 浣滀负鎮诞鏍硷紝
        // 涓嶄緷璧?HoverCol/Row锛岄槻姝㈠悎鎴愰紶鏍囦簨浠跺皢鍏舵竻闆跺悗涓€甯у唴楂樹寒娑堝け銆?
        const FIntPoint EffectiveHover = (bIsGamepadInputMode && !bGrabbingRune)
            ? GamepadCursorCell
            : FIntPoint(HoverCol, HoverRow);

        BackpackGridWidget->RefreshCells(
            GetBackpack(),
            SelectedCell,
            EffectiveHover,
            GrabbedFromCell,
            bGrabbingRune,
            PreviewPhase);
    }
}

// ============================================================
//  璇︽儏闈㈡澘鍒锋柊
// ============================================================

void UBackpackScreenWidget::OnSelectionChanged_Implementation()
{
    FRuneInstance Info = GetFocusedRuneInfo();
    const bool bHasSelection = Info.RuneGuid.IsValid();

    if (DetailPanel)
        DetailPanel->SetVisibility(bHasSelection ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

    if (bHasSelection)
    {
        if (DetailName)
            DetailName->SetText(FText::FromName(Info.RuneConfig.RuneName));

        if (DetailDesc)
            DetailDesc->SetText(Info.RuneConfig.RuneDescription);

        if (DetailIcon)
        {
            if (Info.RuneConfig.RuneIcon)
            {
                DetailIcon->SetBrushFromTexture(Info.RuneConfig.RuneIcon, false);
                DetailIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            }
            else
            {
                DetailIcon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    if (HintText)
    {
        if (SelectedCell != FIntPoint(-1, -1))
        {
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintMove", "Move {0}\nClick a destination cell to place it."),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else if (SelectedRuneIndex >= 0)
        {
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintPlace", "Place {0}\nChoose an empty grid cell."),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else
        {
            HintText->SetText(NSLOCTEXT("Backpack", "HintIdle", "Select a rune or grid cell to edit your backpack."));
        }
    }

    if (RuneInfoCard)
    {
        const bool bDeckOwnsDetailCard = CombatDeckEditWidget && CombatDeckEditWidget->IsUsingExternalDetailInfoCard(RuneInfoCard);
        if (bHasSelection)
        {
            RuneInfoCard->ShowRune(Info);
        }
        else if (bDeckOwnsDetailCard)
        {
            CombatDeckEditWidget->RefreshSelectedCardInfo();
        }
        else
        {
            RuneInfoCard->HideCard();
        }
    }

    OnGridNeedsRefresh();
}

// ============================================================
//  鐘舵€佹煡璇?
// ============================================================

bool UBackpackScreenWidget::IsCellInActivationZone(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;
    return Backpack->GetActivationZoneCells().Contains(FIntPoint(Col, Row));
}

bool UBackpackScreenWidget::IsCellOccupied(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;
    return Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row)) >= 0;
}

FPlacedRune UBackpackScreenWidget::GetRuneAtCell(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return FPlacedRune();

    int32 Idx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    if (Idx < 0) return FPlacedRune();

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    return Placed.IsValidIndex(Idx) ? Placed[Idx] : FPlacedRune();
}

FRuneInstance UBackpackScreenWidget::GetFocusedRuneInfo() const
{
    if (SelectedCell != FIntPoint(-1, -1))
    {
        FPlacedRune PR = GetRuneAtCell(SelectedCell.X, SelectedCell.Y);
        if (PR.Rune.RuneGuid.IsValid())
            return PR.Rune;
    }
    return GetSelectedRuneInfo();
}

UTexture2D* UBackpackScreenWidget::GetRuneIconAtCell(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return nullptr;

    int32 Idx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    if (Idx < 0) return nullptr;

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(Idx)) return nullptr;

    return Placed[Idx].Rune.RuneConfig.RuneIcon;
}

EBackpackCellState UBackpackScreenWidget::GetCellVisualState(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return EBackpackCellState::Empty;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    bool bInZone  = IsCellInActivationZone(Col, Row);

    if (RuneIdx < 0)
        return bInZone ? EBackpackCellState::EmptyActive : EBackpackCellState::Empty;

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (Placed.IsValidIndex(RuneIdx) && Placed[RuneIdx].bIsActivated)
        return EBackpackCellState::OccupiedActive;

    return EBackpackCellState::OccupiedInactive;
}

TArray<FRuneInstance> UBackpackScreenWidget::GetRuneList() const
{
    TArray<FRuneInstance> Result;
    if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
        Result.Append(Player->PendingRunes);

    for (const TObjectPtr<URuneDataAsset>& DA : AvailableRunes)
        if (DA) Result.Add(DA->RuneInfo);

    return Result;
}

int32 UBackpackScreenWidget::GetPendingRuneCount() const
{
    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    return Player ? Player->PendingRunes.Num() : 0;
}

FRuneInstance UBackpackScreenWidget::GetSelectedRuneInfo() const
{
    TArray<FRuneInstance> RuneList = GetRuneList();
    if (!RuneList.IsValidIndex(SelectedRuneIndex)) return FRuneInstance();
    return RuneList[SelectedRuneIndex];
}

const TArray<FPlacedRune>& UBackpackScreenWidget::GetAllPlacedRunes() const
{
    static TArray<FPlacedRune> Empty;
    UBackpackGridComponent* Backpack = GetBackpack();
    return Backpack ? Backpack->GetAllPlacedRunes() : Empty;
}

// ============================================================
//  鎿嶄綔
// ============================================================

void UBackpackScreenWidget::SelectRuneFromList(int32 Index)
{
    SelectedRuneIndex = (SelectedRuneIndex == Index) ? -1 : Index;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}

void UBackpackScreenWidget::ClickCell(int32 Col, int32 Row)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    FIntPoint Cell(Col, Row);
    int32 RuneIdx = Backpack->GetRuneIndexAtCell(Cell);

    if (RuneIdx >= 0)
    {
        // 棰勮妯″紡 / 鎴樻枟闃舵锛氫笌鎴樻枟涓€鑷?鈥?shake + 闂孩鍏夛紝涓嶅厑璁搁€変腑
        if (IsInCombatPhase() || bIsPreviewMode)
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
            OnStatusMessage(bIsPreviewMode
                ? NSLOCTEXT("Backpack", "PreviewLock", "Preview mode: backpack editing is locked.")
                : NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat."));
            return;
        }
        SelectedCell = Cell;
        SelectedRuneIndex = -1;
        // 鐒︾偣褰诲簳鍒囧埌涓昏儗鍖咃細娓?pending 榛勬 + PendingSelectedIdx锛岄伩鍏?R 閿粛鏃嬭浆 pending
        ClearPendingFocus(true);
        OnSelectionChanged();
    }
    else if (bIsPreviewMode)
    {
        // 棰勮妯″紡锛氱┖鏍肩偣鍑绘槸"鏀剧疆"鎿嶄綔锛堜粠 pending 鏀剧疆 / 浠庡垪琛ㄦ斁缃級锛岀姝?
        return;
    }
    else if (PendingSelectedIdx >= 0
        && PendingGrid.IsValidIndex(PendingSelectedIdx)
        && PendingGrid[PendingSelectedIdx].RuneGuid.IsValid())
    {
        // 鐐瑰嚮涓绘牸瀛愮┖鏍?鈫?浠庡緟鏀剧疆鍖烘斁缃€変腑鐨勭鏂?
        const FRuneInstance Instance = PendingGrid[PendingSelectedIdx];
        if (Backpack->TryPlaceRune(Instance, Cell))
        {
            PendingGrid[PendingSelectedIdx] = FRuneInstance();
            SyncPendingToPlayer();
            PendingSelectedIdx   = -1;
            bCursorInPendingArea = false;
            SelectedCell         = FIntPoint(-1, -1);
            RefreshPendingGrid();
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "PlaceOK", "宸叉斁缃細{0}"),
                FText::FromName(Instance.RuneConfig.RuneName)));
        }
        else
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
            OnStatusMessage(NSLOCTEXT("Backpack", "PlaceFail", "Could not place rune here."));
        }
    }
    else if (SelectedRuneIndex >= 0)
    {
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        const int32 PendingCount = Player ? Player->PendingRunes.Num() : 0;
        const bool bFromPending  = SelectedRuneIndex < PendingCount;

        FRuneInstance Instance;
        if (bFromPending)
        {
            Instance = Player->PendingRunes[SelectedRuneIndex];
        }
        else
        {
            const int32 AvIdx = SelectedRuneIndex - PendingCount;
            if (!AvailableRunes.IsValidIndex(AvIdx) || !AvailableRunes[AvIdx]) return;
            Instance = AvailableRunes[AvIdx]->CreateInstance();
        }

        if (Backpack->TryPlaceRune(Instance, Cell))
        {
            if (bFromPending && Player)
            {
                Player->PendingRunes.RemoveAt(SelectedRuneIndex);
                OnRuneListChanged();
            }
            SelectedRuneIndex = -1;
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","PlaceOK","宸叉斁缃細{0}"), FText::FromName(Instance.RuneConfig.RuneName)));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack","PlaceFail","Could not place rune here."));
        }
    }
}

void UBackpackScreenWidget::RemoveRuneAtSelectedCell()
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈠垹闄ょ鏂?
    if (SelectedCell == FIntPoint(-1, -1)) return;

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(SelectedCell);
    if (RuneIdx < 0) { OnStatusMessage(NSLOCTEXT("Backpack","RemoveEmpty","No rune in this cell.")); return; }

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(RuneIdx)) return;

    FGuid RuneGuid = Placed[RuneIdx].Rune.RuneGuid;
    FName RuneName = Placed[RuneIdx].Rune.RuneConfig.RuneName;

    if (Backpack->RemoveRune(RuneGuid))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","RemoveOK","宸茬Щ闄わ細{0}"), FText::FromName(RuneName)));
    }
}

void UBackpackScreenWidget::ClearSelection()
{
    SelectedRuneIndex = -1;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}

// ============================================================
//  寰呮斁缃尯鍒锋柊锛堝鎵樼粰 PendingGridWidget锛?
// ============================================================

void UBackpackScreenWidget::RefreshPendingRuneSlots()
{
    RefreshPendingGrid();
}

// 鈹€鈹€ 寰呮斁缃尯绋€鐤忔牸瀛愯緟鍔?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

void UBackpackScreenWidget::SyncPendingFromPlayer()
{
    PendingCols = PendingGridWidget ? FMath::Max(1, PendingGridWidget->PendingGridCols) : 2;
    PendingRows = PendingGridWidget ? FMath::Max(1, PendingGridWidget->PendingGridRows) : 4;
    PendingGrid.SetNum(PendingCols * PendingRows);
    for (FRuneInstance& R : PendingGrid) R = FRuneInstance();

    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    if (!Player) return;
    for (int32 i = 0; i < Player->PendingRunes.Num() && i < PendingGrid.Num(); i++)
        PendingGrid[i] = Player->PendingRunes[i];
}

void UBackpackScreenWidget::SyncPendingToPlayer()
{
    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    if (!Player) return;
    Player->PendingRunes.Empty();
    for (const FRuneInstance& R : PendingGrid)
        if (R.RuneGuid.IsValid())
            Player->PendingRunes.Add(R);
}

void UBackpackScreenWidget::RefreshPendingGrid()
{
    if (!PendingGridWidget) return;
    const int32 CursorIdx  = bCursorInPendingArea ? PendingCursorIdx : -1;
    const int32 GrabbedIdx = bGrabbingFromPending ? PendingGrabbedIdx : -1;
    PendingGridWidget->RefreshSlots(PendingGrid, CursorIdx, GrabbedIdx);
}

void UBackpackScreenWidget::ClearPendingFocus(bool bClearSelection)
{
    bool bDirty = false;
    if (bCursorInPendingArea)
    {
        bCursorInPendingArea = false;
        bDirty = true;
    }
    if (bClearSelection && PendingSelectedIdx >= 0)
    {
        PendingSelectedIdx = -1;
        bDirty = true;
    }
    if (bDirty) RefreshPendingGrid();
}

void UBackpackScreenWidget::UpdateOperationHintVisibility()
{
    if (!OperationHintWidget) return;

    const bool bShow = CombatDeckEditWidget && (bIsPreviewMode || CombatDeckEditWidget->CanHandleDeckInput());
    SetTextIfSupported(OperationHintText, BuildOperationHintText());
    if (bShow == bOperationHintVisible) return;  // 缂撳瓨锛氶伩鍏嶆瘡甯ч噸澶?SetVisibility
    bOperationHintVisible = bShow;
    OperationHintWidget->SetVisibility(bShow
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Collapsed);
}

// ============================================================
//  CommonUI 鐢熷懡鍛ㄦ湡
// ============================================================

TOptional<FUIInputConfig> UBackpackScreenWidget::GetDesiredInputConfig() const
{
    return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

void UBackpackScreenWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    SetVisibility(ESlateVisibility::Visible);

    if (APlayerController* PC = GetOwningPlayer())
        if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
        {
        }

    if (APlayerController* PC = GetOwningPlayer())
    {
        // 璁板綍褰撳墠鍏夋爣浣嶇疆锛岄槻姝㈡縺娲诲悗鍚堟垚榧犳爣浜嬩欢琚鍒や负鐪熷疄绉诲姩鑰屾竻闄ゆ墜鏌勯珮浜?
        float MouseX = 0.f, MouseY = 0.f;
        PC->GetMousePosition(MouseX, MouseY);
        LastMouseAbsPos = FVector2D(MouseX, MouseY);
    }

    // SetUserFocus(player) was targeting the wrong root 鈥?focus is routed via GetDesiredFocusTarget
    // and the explicit ReleaseMouseCapture().SetUserFocus(TakeWidget()) calls in NativeOnKeyDown.

    // 姣忔鎵撳紑鏃堕噸寤烘牸瀛愶紝纭繚姝﹀櫒鍒囨崲鍚庣殑 GridWidth/GridHeight 鐢熸晥
    if (BackpackGridWidget)
    {
        UBackpackGridComponent* BG = GetBackpack();
        UE_LOG(LogTemp, Warning, TEXT("[BackpackScreenWidget] NativeOnActivated BuildGrid: Backpack=%s, W=%d H=%d"),
            BG ? *BG->GetName() : TEXT("null"),
            BG ? BG->GridWidth  : -1,
            BG ? BG->GridHeight : -1);
        BackpackGridWidget->BuildGrid(BG);
    }

    SyncPendingFromPlayer();
    RefreshPendingGrid();

    if (UBackpackGridComponent* BG = GetBackpack())
        PreviewPhase = FMath::Clamp(BG->GetCurrentPhase(), 0, 2);

    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->SetExternalDetailInfoCard(RuneInfoCard);
        CombatDeckEditWidget->BindToOwningPlayerCombatDeck();
        CombatDeckEditWidget->SetInteractionLocked(IsInCombatPhase() || bIsPreviewMode);
    }

    RefreshWeaponAndComboInfo();
    RefreshActionButtonHints();
    UpdateOperationHintVisibility();

    OnGridNeedsRefresh();
    OnSelectionChanged();

    // Tutorial 鈶細绗竴娆℃墦寮€鑳屽寘鏃跺脊绐楋紙state guard 鍐呴儴鍘婚噸锛?
    // 涓嬩竴甯у箍鎾?鈥斺€?閬垮厤鍦?backpack 鑷韩 NativeOnActivated 璋冪敤鏍堥噷鍚屾 push TutorialPopup锛?
    // 瑙﹀彂 CommonUI activatable stack 鎺掗槦锛屽鑷?popup 绛夊埌 backpack 鍏抽棴鍚庢墠鏄剧ず銆?
    if (UWorld* World = GetWorld())
    {
        TWeakObjectPtr<UBackpackScreenWidget> WeakSelf(this);
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateLambda([WeakSelf]()
            {
                if (!WeakSelf.IsValid() || !WeakSelf->IsActivated())
                {
                    return;
                }
                APlayerController* PC = WeakSelf->GetOwningPlayer();
                if (!PC)
                {
                    return;
                }
                UGameInstance* GI = WeakSelf->GetGameInstance();
                if (!GI)
                {
                    return;
                }
                if (UStoryEngineSubsystem* StoryEngine = GI->GetSubsystem<UStoryEngineSubsystem>())
                {
                    StoryEngine->BroadcastStoryEvent(
                        FGameplayTag::RequestGameplayTag(TEXT("Story.Event.FirstRun.FirstBackpackOpened"), false),
                        PC);
                }
            }));
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UBackpackScreenWidget::LogLayoutDiagnostics));
    }
}

void UBackpackScreenWidget::LogLayoutDiagnostics()
{
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

    UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] Viewport=%.0fx%.0f Scale=%.2f Pending=%dx%d PreviewPhase=%d SelectedCell=(%d,%d) PendingSelected=%d Grabbing=%d PreviewMode=%d"),
        ViewportSize.X, ViewportSize.Y, ViewportScale,
        PendingCols, PendingRows, PreviewPhase,
        SelectedCell.X, SelectedCell.Y, PendingSelectedIdx,
        bGrabbingRune ? 1 : 0,
        bIsPreviewMode ? 1 : 0);

    auto LogWidget = [](const TCHAR* Name, const UWidget* Widget)
    {
        if (!Widget)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] %s=null"), Name);
            return;
        }

        const FGeometry& Geo = Widget->GetCachedGeometry();
        const FVector2D LocalSize = Geo.GetLocalSize();
        const FVector2D AbsPos = Geo.GetAbsolutePosition();
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] %s Vis=%d Local=%.1fx%.1f Abs=(%.1f,%.1f)"),
            Name,
            static_cast<int32>(Widget->GetVisibility()),
            LocalSize.X, LocalSize.Y,
            AbsPos.X, AbsPos.Y);
    };

    LogWidget(TEXT("Screen"), this);
    LogWidget(TEXT("BackpackGridWidget"), BackpackGridWidget);
    LogWidget(TEXT("BackpackGridPanel"), BackpackGridWidget ? BackpackGridWidget->BackpackGrid.Get() : nullptr);
    LogWidget(TEXT("BackpackGridSizeBox"), BackpackGridWidget ? BackpackGridWidget->GridSizeBox.Get() : nullptr);
    LogWidget(TEXT("PendingGridWidget"), PendingGridWidget);
    LogWidget(TEXT("PendingGridPanel"), PendingGridWidget ? PendingGridWidget->PendingRuneGrid.Get() : nullptr);
    LogWidget(TEXT("PendingGridSizeBox"), PendingGridWidget ? PendingGridWidget->PendingGridSizeBox.Get() : nullptr);
    LogWidget(TEXT("OperationHintWidget"), OperationHintWidget);
    LogWidget(TEXT("RuneInfoCard"), RuneInfoCard);

    const UBackpackStyleDataAsset* GridStyle = BackpackGridWidget ? BackpackGridWidget->StyleDA.Get() : nullptr;
    const UBackpackStyleDataAsset* PendingStyle = PendingGridWidget ? PendingGridWidget->StyleDA.Get() : nullptr;
    UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] GridStyle=%s PendingStyle=%s GridCell=%.1f GridPadding=%.1f PendingCell=%.1f PendingPadding=%.1f"),
        *GetNameSafe(GridStyle),
        *GetNameSafe(PendingStyle),
        GridStyle ? GridStyle->CellSize : -1.f,
        GridStyle ? GridStyle->CellPadding : -1.f,
        PendingStyle ? PendingStyle->CellSize : -1.f,
        PendingStyle ? PendingStyle->CellPadding : -1.f);
}

void UBackpackScreenWidget::NativeOnDeactivated()
{
    SetVisibility(ESlateVisibility::Collapsed);

    // 棰勮妯″紡锛氳烦杩?SyncPendingToPlayer锛堣儗鍖呭彧璇伙紝鏃犻渶鍐欏洖鏁版嵁锛?
    // 浣?Pause/Input/PauseEffect 鎷嗚В蹇呴』鎵ц锛?
    //   - EndPauseEffect 蹇呴』閰嶅 NativeOnActivated 閲岀殑 BeginPauseEffect锛屽惁鍒?PausePopupCount 娉勬紡
    //   - SetPause(false) / SetInputMode(GameOnly) 鐢辫皟鐢ㄦ柟锛圠ootSelection.ReactivateAfterPreview锛夐殢鍚庡啀鎭㈠
    const bool bSkipDataSync = bIsPreviewMode;

    if (!bSkipDataSync)
    {
        SyncPendingToPlayer();

        if (UGameInstance* GI = GetGameInstance())
        {
            if (UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>())
            {
                SaveSys->QuickSave();
            }
        }
    }

    bCursorInPendingArea = false;
    bGrabbingFromPending = false;
    PendingGrabbedIdx    = -1;
    PendingSelectedIdx   = -1;

    bGrabbingRune   = false;
    GrabbedFromCell = FIntPoint(-1, -1);
    PreviewPhase    = -1;
    ClearSelection();

    if (APlayerController* PC = GetOwningPlayer())
    {
        // Mouse cursor + InputMode are owned by UYogUIManagerSubsystem::ApplyInputModeForLayer.
        // UBackpackScreenWidget is a UCommonActivatableWidget opened via PushScreen, so the
        // Subsystem hears OnDeactivated and recomputes the top layer 鈫?restores GameOnly itself.
        // Letting it run unimpeded also keeps focus correct when closing Backpack returns to
        // an active LootSelection (preview flow).
    }

    if (APlayerController* PC = GetOwningPlayer())
        if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
        {
        }

    Super::NativeOnDeactivated();

    // 缁熶竴璧?SetPreviewMode(false) 瑙﹀彂 BP 浜嬩欢 + 鍒囨崲鎸夐挳鏄鹃殣 + 娓呯悊鐘舵€?
    // 涓嶅啀瑁稿啓 bIsPreviewMode = false锛堥伩鍏嶆紡瑙﹀彂 OnPreviewModeChanged 鍜屾寜閽浣嶏級
    SetPreviewMode(false);
}

void UBackpackScreenWidget::SetPreviewMode(bool bReadOnly)
{
    if (bIsPreviewMode == bReadOnly && !bReadOnly)
    {
        // 鍚岀姸鎬佸垏 false 鏃朵篃鍏佽鎵ц锛圢ativeOnDeactivated 鍏滃簳鍦烘櫙锛?
    }
    bIsPreviewMode = bReadOnly;
    UE_LOG(LogTemp, Log, TEXT("[Backpack] SetPreviewMode(%s)"), bReadOnly ? TEXT("true") : TEXT("false"));

    // 杩涘叆棰勮妯″紡鏃舵竻鐞嗘墍鏈夊彲鑳藉鑷?鍗″湪鎶撳彇/鎷栨嫿涓?鐨勪腑闂寸姸鎬?
    if (bIsPreviewMode)
    {
        bGrabbingRune        = false;
        bGrabbingFromPending = false;
        bMouseDragging       = false;
        GrabbedFromCell      = FIntPoint(-1, -1);
        PendingGrabbedIdx    = -1;
        PendingDragIndex     = -1;
        PendingDragCol       = PendingDragRow = -1;
        HoverCol = HoverRow  = -1;
        MouseDragTex         = nullptr;
        HideShapePreview();
        ClearSelection();
        OnGridNeedsRefresh();
        RefreshPendingGrid();
    }

    // EndPreviewButton 涓?CloseButton 浜掓枼鏄鹃殣 鈥?蹇呴』鐢?Visible 鑰岄潪 SelfHitTestInvisible锛?
    // 鍚﹀垯鎸夐挳鏈綋涓嶅弬涓庡懡涓祴璇曪紝OnClicked 涓嶈Е鍙?
    if (EndPreviewButton)
        EndPreviewButton->SetVisibility(bIsPreviewMode
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
    if (CloseButton)
        CloseButton->SetVisibility(bIsPreviewMode
            ? ESlateVisibility::Collapsed
            : ESlateVisibility::Visible);

    bActionButtonHintsInitialized = false;
    RefreshActionButtonHints();

    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->SetInteractionLocked(IsInCombatPhase() || bIsPreviewMode);
    }

}

void UBackpackScreenWidget::OnEndPreviewClicked()
{
    if (!bIsPreviewMode) return;
    UE_LOG(LogTemp, Log, TEXT("[Backpack] OnEndPreviewClicked 鈫?DeactivateWidget"));
    // DeactivateWidget 鈫?NativeOnDeactivated 鈫?HUD 鐩戝惉鍣ㄥ洖璋?LootSelection.ReactivateAfterPreview
    DeactivateWidget();
}

// ============================================================
//  Shape 鎷栨嫿棰勮锛圥hase 2锛?
// ============================================================

void UBackpackScreenWidget::ShowShapePreview(const FRuneInstance& Rune, FIntPoint AnchorCellInRotatedLocal,
                                              UTexture2D* IconTex, float CellPx)
{
    if (!ShapePreviewCanvas) return;  // BindWidgetOptional锛氭湭缁戝畾灏卞洖閫€鍒?GrabbedRuneIcon

    HideShapePreview();

    // 璁＄畻 N 娆℃棆杞悗鐨?Shape锛堝悓 BackpackGridWidget 鐨勬覆鏌撹绾︼級
    FRuneShape RotShape = Rune.Shape;
    const int32 N = ((Rune.Rotation % 4) + 4) % 4;
    for (int32 i = 0; i < N; i++)
        RotShape = RotShape.Rotate90();

    // 涓烘瘡涓?cell 鍒涘缓 UImage
    for (const FIntPoint& Cell : RotShape.Cells)
    {
        UImage* CellImg = NewObject<UImage>(this);
        if (IconTex)
            CellImg->SetBrushFromTexture(IconTex, false);
        // pivot/anchor cell 涓嶉€忔槑锛屽叾瀹?cell 鍗婇€忔槑鍋氳瑙夊尯鍒?
        const float Alpha = (Cell == AnchorCellInRotatedLocal) ? 1.0f : 0.65f;
        CellImg->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Alpha));

        UPanelSlot* PSlot = ShapePreviewCanvas->AddChild(CellImg);
        if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(PSlot))
        {
            CSlot->SetAutoSize(false);
            CSlot->SetPosition(FVector2D(Cell.X * CellPx, Cell.Y * CellPx));
            CSlot->SetSize(FVector2D(CellPx, CellPx));
        }
        ShapePreviewCells.Add(CellImg);
    }

    ShapePreviewAnchorCell = AnchorCellInRotatedLocal;
    ShapePreviewCellPx     = CellPx;
    bShapePreviewActive    = true;
    ShapePreviewCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBackpackScreenWidget::UpdateShapePreviewPosition(const FGeometry& MyGeometry, FVector2D ScreenAbsPos)
{
    if (!bShapePreviewActive || !ShapePreviewCanvas) return;
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenAbsPos);
    // 璁?anchor cell 鐨勪腑蹇冿紙涓嶆槸宸︿笂瑙掞級瀵归綈榧犳爣浣嶇疆 鈥?瑙嗚涓?Pivot 璺熸墜鎰熻鏇磋嚜鐒?
    const FVector2D AnchorOffset(
        (ShapePreviewAnchorCell.X + 0.5f) * ShapePreviewCellPx,
        (ShapePreviewAnchorCell.Y + 0.5f) * ShapePreviewCellPx);
    FWidgetTransform T;
    T.Translation = LocalPos - AnchorOffset;
    ShapePreviewCanvas->SetRenderTransform(T);
}

void UBackpackScreenWidget::HideShapePreview()
{
    if (!ShapePreviewCanvas) return;
    for (UImage* Img : ShapePreviewCells)
    {
        if (Img) ShapePreviewCanvas->RemoveChild(Img);
    }
    ShapePreviewCells.Reset();
    ShapePreviewCanvas->SetVisibility(ESlateVisibility::Collapsed);
    bShapePreviewActive = false;
}

// ============================================================
//  鍑哄敭鎸夐挳
// ============================================================

void UBackpackScreenWidget::OnSellButtonClicked()
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈠嚭鍞?
    if (SelectedCell == FIntPoint(-1, -1)) return;
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        FPlacedRune PR = GetRuneAtCell(SelectedCell.X, SelectedCell.Y);
        if (PR.Rune.RuneGuid.IsValid())
            Backpack->SellRune(PR.Rune.RuneGuid);
    }
}

void UBackpackScreenWidget::OnCloseButtonClicked()
{
    DeactivateWidget();
}

void UBackpackScreenWidget::OnConfirmButtonClicked()
{
    if (bCursorInPendingArea)
    {
        PendingGamepadConfirm();
        return;
    }

    if (bGrabbingRune || bGrabbingFromPending)
    {
        GamepadConfirm();
        return;
    }

    DeactivateWidget();
}

bool UBackpackScreenWidget::IsInCombatPhase() const
{
    if (AYogGameMode* GM = GetWorld()->GetAuthGameMode<AYogGameMode>())
        return GM->CurrentPhase == ELevelPhase::Combat;
    return false;
}

// ============================================================
//  鍧愭爣杈呭姪锛堝鎵樼粰瀛?Widget锛?
// ============================================================

bool UBackpackScreenWidget::GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const
{
    if (!BackpackGridWidget) return false;
    return BackpackGridWidget->GetCellAtScreenPos(AbsolutePos, GetBackpack(), OutCol, OutRow);
}

bool UBackpackScreenWidget::GetPendingSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const
{
    if (!PendingGridWidget) return false;
    return PendingGridWidget->GetSlotAtScreenPos(AbsPos, OutIndex);
}

// ============================================================
//  鎷栨嫿浜嬩欢瀹炵幇
// ============================================================

FReply UBackpackScreenWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[BackpackScreen] MouseButtonDown button=%s pos=(%f,%f)"),
        *InMouseEvent.GetEffectingButton().ToString(),
        InMouseEvent.GetScreenSpacePosition().X, InMouseEvent.GetScreenSpacePosition().Y);

    // 鍙抽敭 鈫?鍙栨秷鎶撳彇
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (bGrabbingRune || bGrabbingFromPending)
        {
            GamepadCancel();
            return FReply::Handled();
        }
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    bool bCommonInputGamepad = false;
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
        {
            bCommonInputGamepad = CommonInput->GetCurrentInputType() == ECommonInputType::Gamepad;
        }
    }

    const bool bGamepadAIsDown = GetOwningPlayer()
        && GetOwningPlayer()->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
    const bool bTreatAsGamepadSelect =
        bIsGamepadInputMode || bCommonInputGamepad || bGamepadAIsDown || bDeckSelectButtonWasDown;

    if (bTreatAsGamepadSelect && CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        if (!bGamepadAIsDown && CurrentTime < SuppressDeckVirtualMouseSelectUntilTime)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] SuppressVirtualMouseAsA Until=%.3f Now=%.3f"),
                SuppressDeckVirtualMouseSelectUntilTime,
                CurrentTime);
            bIsGamepadInputMode = true;
            bDeckSelectFromVirtualMouse = false;
            CombatDeckEditWidget->NotifyGamepadNavigationInput();
            if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
            {
                if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
                {
                    CommonInput->SetCurrentInputType(ECommonInputType::Gamepad);
                }
            }
            return FReply::Handled().SetUserFocus(TakeWidget());
        }

        // A is physically held AND NativeOnPreviewKeyDown already started the drag.
        // The LMB down here is the virtual click CommonUI generates from that same A press.
        // Calling HandleDeckSelectPressed again would immediately commit the drag (no-op at same
        // position) before the user can move the card 鈥?skip this duplicate event entirely.
        if (bGamepadAIsDown && bDeckSelectButtonWasDown)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] SkipVirtualMouseDuplicate ADown=1 WasDown=1"));
            bIsGamepadInputMode = true;
            bDeckSelectFromVirtualMouse = false;
            return FReply::Handled().SetUserFocus(TakeWidget());
        }

        bIsGamepadInputMode = true;
        bDeckSelectFromVirtualMouse = true;
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] VirtualMouseAsA Mode=%d Common=%d ADown=%d WasDown=%d"),
            bIsGamepadInputMode ? 1 : 0,
            bCommonInputGamepad ? 1 : 0,
            bGamepadAIsDown ? 1 : 0,
            bDeckSelectButtonWasDown ? 1 : 0);
        return HandleCombatDeckSelectButtonState(true, TEXT("VirtualMouseDown")).CaptureMouse(TakeWidget());
    }

    if (bIsGamepadInputMode)
    {
        bIsGamepadInputMode = false;
    }
    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->NotifyPointerNavigationInput();
    }

    // 宸︿晶寰呮斁缃Ы锛氫紭鍏堜簬涓绘牸瀛?
    {
        int32 PendingIdx;
        if (GetPendingSlotAtScreenPos(InMouseEvent.GetScreenSpacePosition(), PendingIdx))
        {
            bCursorInPendingArea = true;
            PendingCursorIdx     = PendingIdx;

            const bool bHasRune = PendingGrid.IsValidIndex(PendingIdx)
                && PendingGrid[PendingIdx].RuneGuid.IsValid();

            if (bHasRune)
            {
                PendingSelectedIdx = PendingIdx;
                SelectedCell       = FIntPoint(-1, -1);
                SelectedRuneIndex  = -1;
                RefreshPendingGrid();
                OnSelectionChanged();

                // 棰勮妯″紡 / 鎴樻枟闃舵锛氬脊娑堟伅 + 绂佹 DetectDrag锛坧ending 鏃?shake锛屼富鏍煎瓙鏈夛級
                if (IsInCombatPhase() || bIsPreviewMode)
                {
                    OnStatusMessage(bIsPreviewMode
                        ? NSLOCTEXT("Backpack", "PreviewLock", "Preview mode: backpack editing is locked.")
                        : NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat."));
                    return FReply::Handled();
                }
                PendingDragIndex = PendingIdx;
                return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
            }
            else if (PendingSelectedIdx >= 0
                && PendingGrid.IsValidIndex(PendingSelectedIdx)
                && PendingGrid[PendingSelectedIdx].RuneGuid.IsValid())
            {
                if (bIsPreviewMode) return FReply::Handled();  // 棰勮妯″紡锛氱姝?pending 鍐?swap
                PendingGrid[PendingIdx]          = PendingGrid[PendingSelectedIdx];
                PendingGrid[PendingSelectedIdx]  = FRuneInstance();
                SyncPendingToPlayer();
                PendingSelectedIdx = PendingIdx;
                PendingCursorIdx   = PendingIdx;
                RefreshPendingGrid();
                return FReply::Handled();
            }
            else
            {
                PendingSelectedIdx = -1;
                RefreshPendingGrid();
                return FReply::Handled();
            }
        }
    }

    int32 Col, Row;
    if (!GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    const FIntPoint ClickCell(Col, Row);

    // 鐒︾偣鍒囧埌涓昏儗鍖咃細娓?pending 榛勬 + 寰呮斁缃€夋嫨锛堥紶鏍囪矾寰勪笉鏀寔 click-to-place from pending锛?
    ClearPendingFocus(true);

    // 鈹€鈹€ 宸叉姄鍙栫姸鎬佷笅鐐瑰嚮鍙︿竴鏍硷細绉诲姩鎴栦簰鎹?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
    if (bGrabbingRune && ClickCell != GrabbedFromCell)
    {
        if (IsInCombatPhase())
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GrabbedFromCell.X, GrabbedFromCell.Y);
            OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat."));
            return FReply::Handled();
        }

        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack) return FReply::Handled();

        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        const int32 SrcIdx = Backpack->GetRuneIndexAtCell(GrabbedFromCell);
        if (SrcIdx < 0)
        {
            bGrabbingRune   = false;
            GrabbedFromCell = FIntPoint(-1,-1);
            SelectedCell    = FIntPoint(-1,-1);
            OnSelectionChanged();
            return FReply::Handled();
        }

        const FRuneInstance RuneA  = Placed[SrcIdx].Rune;
        const FIntPoint     PivotA = Placed[SrcIdx].Pivot;
        const int32 DstIdx = Backpack->GetRuneIndexAtCell(ClickCell);

        if (DstIdx >= 0)
        {
            // 浜掓崲 鈫?鑷姩鎶撳彇琚浛鎹㈢殑绗︽枃
            const FRuneInstance RuneB  = Placed[DstIdx].Rune;
            const FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneA.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);
            Backpack->TryPlaceRune(RuneA, PivotB);
            Backpack->TryPlaceRune(RuneB, PivotA);

            GrabbedFromCell = PivotA;   // RuneB 鐜板湪鍦?PivotA锛岃嚜鍔ㄦ姄鍙?
            SelectedCell    = PivotA;
            // bGrabbingRune 淇濇寔 true
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "SwapOK", "宸蹭簰鎹細{0} 鈫?{1}"),
                FText::FromName(RuneA.RuneConfig.RuneName),
                FText::FromName(RuneB.RuneConfig.RuneName)));
        }
        else
        {
            // 绉诲姩鍒扮┖鏍?鈫?缁撴潫鎶撳彇
            const FIntPoint Offset   = GrabbedFromCell - PivotA;
            const FIntPoint NewPivot = ClickCell - Offset;

            if (Backpack->MoveRune(RuneA.RuneGuid, NewPivot))
            {
                bGrabbingRune   = false;
                GrabbedFromCell = FIntPoint(-1,-1);
                SelectedCell    = FIntPoint(-1,-1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "MoveOK", "宸茬Щ鍔細{0}"),
                    FText::FromName(RuneA.RuneConfig.RuneName)));
            }
            else
            {
                if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
                OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "鏃犳硶鏀剧疆锛氱洰鏍囦綅缃鍗犵敤"));
            }
        }
        return FReply::Handled();
    }

    // 鈹€鈹€ 鐐瑰嚮宸叉姄鍙栫鏂囪嚜韬?鈫?鍙栨秷鎶撳彇 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
    if (bGrabbingRune && ClickCell == GrabbedFromCell)
    {
        bGrabbingRune     = false;
        GrabbedFromCell   = FIntPoint(-1,-1);
        SelectedCell      = FIntPoint(-1,-1);
        OnSelectionChanged();
        return FReply::Handled();
    }

    // 鈹€鈹€ 闈炴姄鍙栵細鐐瑰嚮鍗犵敤鏍?鈫?杩涘叆鎶撳彇/鎮诞 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
    if (IsCellOccupied(Col, Row))
    {
        SelectedCell      = ClickCell;
        SelectedRuneIndex = -1;
        GamepadCursorCell = ClickCell;
        OnSelectionChanged();

        // 棰勮妯″紡 / 鎴樻枟闃舵锛氫笌鎴樻枟涓€鑷?鈥?shake + 闂孩鍏?+ 鐘舵€佹彁绀猴紝绂佹鎶撳彇
        if (IsInCombatPhase() || bIsPreviewMode)
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
            OnStatusMessage(bIsPreviewMode
                ? NSLOCTEXT("Backpack", "PreviewLock", "Preview mode: backpack editing is locked.")
                : NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat."));
            return FReply::Handled();
        }

        bGrabbingRune     = true;
        GrabbedFromCell   = ClickCell;
        HoverCol = HoverRow = -1;

        PendingDragCol = Col;
        PendingDragRow = Row;
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    else
    {
        PendingDragCol    = PendingDragRow = -1;
        if (SelectedCell != FIntPoint(-1, -1) || SelectedRuneIndex >= 0)
        {
            SelectedCell      = FIntPoint(-1, -1);
            SelectedRuneIndex = -1;
            OnSelectionChanged();
        }
        return FReply::Handled();
    }
}

FReply UBackpackScreenWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[BackpackScreen] MouseButtonUp button=%s pos=(%f,%f) VirtualDeck=%d"),
        *InMouseEvent.GetEffectingButton().ToString(),
        InMouseEvent.GetScreenSpacePosition().X,
        InMouseEvent.GetScreenSpacePosition().Y,
        bDeckSelectFromVirtualMouse ? 1 : 0);

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDeckSelectFromVirtualMouse)
    {
        bDeckSelectFromVirtualMouse = false;
        return HandleCombatDeckSelectButtonState(false, TEXT("VirtualMouseUp")).ReleaseMouseCapture();
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
        bIsGamepadInputMode &&
        (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) < SuppressDeckVirtualMouseSelectUntilTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] SuppressVirtualMouseUpAsA"));
        if (CombatDeckEditWidget)
        {
            CombatDeckEditWidget->NotifyGamepadNavigationInput();
        }
        if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
        {
            if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
            {
                CommonInput->SetCurrentInputType(ECommonInputType::Gamepad);
            }
        }
        return FReply::Handled().ReleaseMouseCapture().SetUserFocus(TakeWidget());
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
        bIsGamepadInputMode &&
        bDeckSelectButtonWasDown &&
        CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] VirtualMouseUpAsARelease"));
        return HandleCombatDeckSelectButtonState(false, TEXT("VirtualMouseUpFallback")).ReleaseMouseCapture();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UBackpackScreenWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈡嫋鎷?

    // 鎷栨嫿鎺ョ锛氭竻闄ょ偣鍑绘姄鍙栫姸鎬?
    bGrabbingRune     = false;
    GrabbedFromCell   = FIntPoint(-1,-1);

    if (PendingDragIndex >= 0)
    {
        const bool bValid = PendingGrid.IsValidIndex(PendingDragIndex)
            && PendingGrid[PendingDragIndex].RuneGuid.IsValid();
        if (bValid)
        {
            if (IsInCombatPhase())
            {
                PendingDragIndex = -1;
                return;
            }

            const FRuneInstance PendingRune = PendingGrid[PendingDragIndex];
            URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
            DragOp->SrcCol             = -1;
            DragOp->SrcRow             = -1;
            DragOp->SrcPivot           = FIntPoint(-1, -1);
            DragOp->DraggedRune        = PendingRune;
            DragOp->PendingSourceIndex = PendingDragIndex;

            bMouseDragging  = true;
            MouseDragTex    = PendingRune.RuneConfig.RuneIcon;
            LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

            // Phase 2: 鏄剧ず瀹屾暣 Shape 璺熼殢榧犳爣锛孉nchorCell = Pivot 鍦ㄦ棆杞悗 Shape 鐨勪綅缃?
            if (ShapePreviewCanvas && BackpackGridWidget)
            {
                const FIntPoint AnchorCell = PendingRune.Shape.GetPivotOffset(PendingRune.Rotation);
                const FVector2D GridSize   = BackpackGridWidget->GetGridGeometry().GetLocalSize();
                const int32 GW = (CachedBackpack.IsValid() && CachedBackpack->GridWidth > 0) ? CachedBackpack->GridWidth : 5;
                const float CellPx = (GridSize.X > 0.f) ? (GridSize.X / GW) : 64.f;
                ShowShapePreview(PendingRune, AnchorCell, MouseDragTex, CellPx);
                // 棣栧抚绔嬪嵆瀵逛綅锛岄伩鍏嶅嚭鐜板湪 (0,0) 涓€甯?
                UpdateShapePreviewPosition(InGeometry, InMouseEvent.GetScreenSpacePosition());
            }

            PendingDragIndex = -1;
            OutOperation = DragOp;
            return;
        }
        PendingDragIndex = -1;
        return;
    }

    if (PendingDragCol < 0 || PendingDragRow < 0)
        return;

    FPlacedRune PR = GetRuneAtCell(PendingDragCol, PendingDragRow);
    if (!PR.Rune.RuneGuid.IsValid())
    {
        PendingDragCol = PendingDragRow = -1;
        return;
    }

    if (IsInCombatPhase())
    {
        PendingDragCol = PendingDragRow = -1;
        return;
    }

    URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
    DragOp->SrcCol      = PendingDragCol;
    DragOp->SrcRow      = PendingDragRow;
    DragOp->SrcPivot    = PR.Pivot;
    DragOp->DraggedRune = PR.Rune;

    bMouseDragging  = true;
    MouseDragTex    = PR.Rune.RuneConfig.RuneIcon;
    LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

    // Phase 2: 涓绘牸瀛愭嫋鎷戒篃鏄惧畬鏁?Shape锛孉nchorCell = Pivot 鍦ㄦ棆杞悗 Shape 鐨勪綅缃?
    // 锛堜笌 pending 璺緞涓€鑷?鈥?鐢ㄦ埛鍐崇瓥 Q5锛歅ivot 鍘熺偣瀵归綈榧犳爣锛?
    if (ShapePreviewCanvas && BackpackGridWidget)
    {
        const FIntPoint AnchorCell = PR.Rune.Shape.GetPivotOffset(PR.Rune.Rotation);
        const FVector2D GridSize   = BackpackGridWidget->GetGridGeometry().GetLocalSize();
        const int32 GW = (CachedBackpack.IsValid() && CachedBackpack->GridWidth > 0) ? CachedBackpack->GridWidth : 5;
        const float CellPx = (GridSize.X > 0.f) ? (GridSize.X / GW) : 64.f;
        ShowShapePreview(PR.Rune, AnchorCell, MouseDragTex, CellPx);
        // 棣栧抚绔嬪嵆瀵逛綅锛岄伩鍏嶅嚭鐜板湪 (0,0) 涓€甯?
        UpdateShapePreviewPosition(InGeometry, InMouseEvent.GetScreenSpacePosition());
    }

    PendingDragCol = PendingDragRow = -1;
    OutOperation = DragOp;
}

bool UBackpackScreenWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (!Cast<URuneDragDropOperation>(InOperation))
        return false;

    LastMouseAbsPos = InDragDropEvent.GetScreenSpacePosition();

    // Phase 2: 璁╁畬鏁?Shape 棰勮璺熼殢榧犳爣
    UpdateShapePreviewPosition(InGeometry, InDragDropEvent.GetScreenSpacePosition());

    int32 Col, Row;
    if (GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), Col, Row))
    {
        if (HoverCol != Col || HoverRow != Row)
        {
            HoverCol = Col;
            HoverRow = Row;
            OnGridNeedsRefresh();
        }
    }
    else if (HoverCol != -1 || HoverRow != -1)
    {
        HoverCol = HoverRow = -1;
        OnGridNeedsRefresh();
    }

    return true;
}

bool UBackpackScreenWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (bIsPreviewMode)
    {
        HideShapePreview();  // 棰勮妯″紡鎻愭棭杩斿洖涔熻娓呯悊 Shape 棰勮
        return false;
    }

    bMouseDragging = false;
    MouseDragTex   = nullptr;
    HoverCol = HoverRow = -1;
    HideShapePreview();  // Phase 2: 鎵€鏈?Drop 璺緞缁熶竴鍦ㄥ叆鍙ｆ竻鎺?Shape 棰勮

    URuneDragDropOperation* RuneOp = Cast<URuneDragDropOperation>(InOperation);
    if (!RuneOp)
    {
        OnGridNeedsRefresh();
        return false;
    }

    // 鈹€鈹€ 涓绘牸瀛愮鏂囨嫋鍥炲緟鏀剧疆鍖猴紙鍙栧洖锛?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
    // 鍙钀界偣鍦ㄤ富鏍煎瓙闈㈡澘宸︿晶锛堝惈 Pending 鍖哄煙鍜屼袱鑰呬箣闂寸殑绌虹櫧锛夛紝閮借Е鍙戝彇鍥?
    if (RuneOp->PendingSourceIndex < 0 && RuneOp->DraggedRune.RuneGuid.IsValid())
    {
        bool bShouldUnplace = false;

        // 绮剧‘鍛戒腑 Pending 闈㈡澘
        int32 PendingTargetIdx;
        bShouldUnplace = GetPendingSlotAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), PendingTargetIdx);

        // 鎵╁睍锛氳惤鐐瑰湪涓绘牸瀛愰潰鏉垮乏渚т篃绠楀彇鍥?
        if (!bShouldUnplace && BackpackGridWidget)
        {
            const FGeometry GridGeo  = BackpackGridWidget->GetGridGeometry();
            const FVector2D LocalInGrid = GridGeo.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
            bShouldUnplace = (LocalInGrid.X < 0.f);
        }

        if (bShouldUnplace)
        {
            UBackpackGridComponent* Backpack = GetBackpack();
            if (Backpack)
            {
                // 鎵剧洰鏍囨牸瀛愶細鍏堝皾璇曟渶杩戞牸锛岃嫢宸插崰鐢ㄥ垯鎵剧涓€涓┖鏍?
                int32 TargetSlot = -1;
                if (PendingGridWidget)
                {
                    int32 NearestIdx;
                    if (PendingGridWidget->GetNearestSlotAtScreenPos(
                            InDragDropEvent.GetScreenSpacePosition(), NearestIdx)
                        && PendingGrid.IsValidIndex(NearestIdx)
                        && !PendingGrid[NearestIdx].RuneGuid.IsValid())
                    {
                        TargetSlot = NearestIdx;
                    }
                }
                if (TargetSlot < 0)
                {
                    for (int32 i = 0; i < PendingGrid.Num(); i++)
                        if (!PendingGrid[i].RuneGuid.IsValid()) { TargetSlot = i; break; }
                }
                if (TargetSlot < 0)
                {
                    OnStatusMessage(NSLOCTEXT("Backpack", "PendingFull", "Pending rune list is full."));
                    OnGridNeedsRefresh();
                    return false;
                }

                Backpack->RemoveRune(RuneOp->DraggedRune.RuneGuid);
                PendingGrid[TargetSlot] = RuneOp->DraggedRune;
                SyncPendingToPlayer();
                PendingSelectedIdx = TargetSlot;
                RefreshPendingGrid();
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "UnplaceOK", "宸插彇鍥烇細{0}"),
                    FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName)));
                return true;
            }
        }
    }

    if (RuneOp->PendingSourceIndex >= 0)
    {
        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack || !PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex))
        {
            OnGridNeedsRefresh();
            return false;
        }

        // 鈹€鈹€ 浼樺厛妫€娴嬩富鏍煎瓙钀界偣锛堥槻姝㈣竟鐣屾ā绯婅瑙?pending鈫抪ending 璺緞锛?鈹€鈹€鈹€鈹€
        int32 TargetCol, TargetRow;
        if (GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
        {
            const FRuneInstance PendingRune = PendingGrid[RuneOp->PendingSourceIndex];
            if (!PendingRune.RuneGuid.IsValid()) { OnGridNeedsRefresh(); return false; }

            if (Backpack->TryPlaceRune(PendingRune, FIntPoint(TargetCol, TargetRow)))
            {
                PendingGrid[RuneOp->PendingSourceIndex] = FRuneInstance();
                SyncPendingToPlayer();
                PendingSelectedIdx = -1;
                RefreshPendingGrid();
                OnRuneListChanged();
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "PendingPlaceOK", "宸叉斁缃細{0}"),
                    FText::FromName(PendingRune.RuneConfig.RuneName)));
                return true;
            }

            OnStatusMessage(NSLOCTEXT("Backpack", "PendingPlaceFail", "Could not place rune here."));
            OnGridNeedsRefresh();
            return false;
        }

        // 鈹€鈹€ 涓绘牸瀛愭湭鍛戒腑锛氭鏌ユ槸鍚﹁惤鍦ㄥ緟鏀剧疆鍖哄唴锛坧ending 鈫?pending 浜ゆ崲锛夆攢鈹€
        {
            int32 PendingTargetIdx;
            if (PendingGridWidget && PendingGridWidget->GetSlotAtScreenPos(
                    InDragDropEvent.GetScreenSpacePosition(), PendingTargetIdx)
                && PendingTargetIdx != RuneOp->PendingSourceIndex)
            {
                const FRuneInstance Src = PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex)
                    ? PendingGrid[RuneOp->PendingSourceIndex] : FRuneInstance();
                const FRuneInstance Dst = PendingGrid.IsValidIndex(PendingTargetIdx)
                    ? PendingGrid[PendingTargetIdx] : FRuneInstance();
                if (PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex))
                    PendingGrid[RuneOp->PendingSourceIndex] = Dst;
                if (PendingGrid.IsValidIndex(PendingTargetIdx))
                    PendingGrid[PendingTargetIdx] = Src;
                SyncPendingToPlayer();
                PendingSelectedIdx = PendingTargetIdx;
                PendingCursorIdx   = PendingTargetIdx;
                RefreshPendingGrid();
                return true;
            }
        }

        OnGridNeedsRefresh();
        return false;
    }

    int32 TargetCol, TargetRow;
    if (!GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
    {
        OnGridNeedsRefresh();
        return false;
    }

    if (TargetCol == RuneOp->SrcCol && TargetRow == RuneOp->SrcRow)
    {
        OnGridNeedsRefresh();
        return false;
    }

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack)
    {
        OnGridNeedsRefresh();
        return false;
    }

    const FIntPoint GrabOffset = FIntPoint(RuneOp->SrcCol, RuneOp->SrcRow) - RuneOp->SrcPivot;
    const FIntPoint NewPivot   = FIntPoint(TargetCol, TargetRow) - GrabOffset;

    if (Backpack->MoveRune(RuneOp->DraggedRune.RuneGuid, NewPivot))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(
            NSLOCTEXT("Backpack", "MoveOK", "宸茬Щ鍔細{0}"),
            FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName)));
        return true;
    }

    const int32 DstIdx = Backpack->GetRuneIndexAtCell(FIntPoint(TargetCol, TargetRow));
    if (DstIdx >= 0)
    {
        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        if (Placed.IsValidIndex(DstIdx))
        {
            FRuneInstance RuneB  = Placed[DstIdx].Rune;
            FIntPoint     PivotA = RuneOp->SrcPivot;
            FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneOp->DraggedRune.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);

            const bool bPlaceA = Backpack->TryPlaceRune(RuneOp->DraggedRune, PivotB);
            const bool bPlaceB = Backpack->TryPlaceRune(RuneB, PivotA);

            if (bPlaceA && bPlaceB)
            {
                // 浜掓崲鎴愬姛 鈫?鑷姩鎶撳彇琚浛鎹㈢殑绗︽枃锛圧uneB 鐜板湪鍦?PivotA锛?
                bGrabbingRune   = true;
                GrabbedFromCell = PivotA;
                SelectedCell    = PivotA;
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "SwapOK", "宸蹭簰鎹細{0} 鈫?{1}"),
                    FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName),
                    FText::FromName(RuneB.RuneConfig.RuneName)));
                return true;
            }

            Backpack->TryPlaceRune(RuneOp->DraggedRune, PivotA);
            Backpack->TryPlaceRune(RuneB, PivotB);
            OnStatusMessage(NSLOCTEXT("Backpack", "SwapFail", "Could not swap these runes."));
            OnGridNeedsRefresh();
            return false;
        }
    }

    OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "鏃犳硶绉诲姩锛氱洰鏍囦綅缃鍗犵敤"));
    OnGridNeedsRefresh();
    return false;
}

void UBackpackScreenWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    bMouseDragging = false;
    MouseDragTex   = nullptr;
    HoverCol       = HoverRow       = -1;
    PendingDragCol = PendingDragRow = -1;
    HideShapePreview();  // Phase 2: 鎷栨嫿鍙栨秷锛堝 ESC / 鍙抽敭锛変篃娓呮帀 Shape 棰勮
    OnGridNeedsRefresh();
}

// ============================================================
//  鎵嬫焺杈撳叆
// ============================================================

FReply UBackpackScreenWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const FVector2D NewPos = InMouseEvent.GetScreenSpacePosition();
    // 鍙湁鍏夋爣鐪熷疄绉诲姩瓒呰繃 2px 鎵嶇畻"鐪熷疄绉诲姩"锛涢浂浣嶇Щ鎴栨瀬灏忓亸绉昏涓哄悎鎴愪簨浠讹紝
    // 淇濇寔鎵嬫焺妯″紡锛岄槻姝㈠娆″悎鎴愪簨浠跺弽澶嶅皢 HoverCol/Row 娓呴浂瀵艰嚧楂樹寒闂儊銆?
    const bool bRealMove = (NewPos - LastMouseAbsPos).SizeSquared() > 4.f;

    if (bIsGamepadInputMode)
    {
        LastMouseAbsPos = NewPos;
        if (!bRealMove)
            return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
        bIsGamepadInputMode = false;
        if (CombatDeckEditWidget)
        {
            CombatDeckEditWidget->NotifyPointerNavigationInput();
        }
    }
    else
    {
        LastMouseAbsPos = NewPos;
        if (bRealMove && CombatDeckEditWidget)
        {
            CombatDeckEditWidget->NotifyPointerNavigationInput();
        }
    }

    // 鏃犳姄鍙?鎷栨嫿鏃惰拷韪富鏍煎瓙鎮诞鏍硷紝椹卞姩缁挎楂樹寒
    if (!bGrabbingRune && !bMouseDragging)
    {
        int32 NewHCol = -1, NewHRow = -1;
        int32 Col, Row;
        if (GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        {
            NewHCol = Col;
            NewHRow = Row;
        }
        if (HoverCol != NewHCol || HoverRow != NewHRow)
        {
            HoverCol = NewHCol;
            HoverRow = NewHRow;
            OnGridNeedsRefresh();
        }
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UBackpackScreenWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackKeyUp] Key=%s HasDeckWidget=%d CanDeck=%d"),
        *InKeyEvent.GetKey().ToString(),
        CombatDeckEditWidget ? 1 : 0,
        (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput()) ? 1 : 0);

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput() &&
        InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom)
    {
        SuppressDeckVirtualMouseSelectUntilTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + 0.25f;
        return HandleCombatDeckSelectButtonState(false, TEXT("KeyUp"));
    }

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput() &&
        (InKeyEvent.GetKey() == EKeys::Gamepad_DPad_Left ||
         InKeyEvent.GetKey() == EKeys::Gamepad_DPad_Right ||
         InKeyEvent.GetKey() == EKeys::Gamepad_DPad_Up ||
         InKeyEvent.GetKey() == EKeys::Gamepad_DPad_Down ||
         InKeyEvent.GetKey() == EKeys::Gamepad_LeftStick_Left ||
         InKeyEvent.GetKey() == EKeys::Gamepad_LeftStick_Right ||
         InKeyEvent.GetKey() == EKeys::Gamepad_LeftStick_Up ||
         InKeyEvent.GetKey() == EKeys::Gamepad_LeftStick_Down))
    {
        if (InKeyEvent.GetKey() == HeldDirKey)
        {
            bDirKeyHeld    = false;
            HeldKeyTime    = 0.f;
            LastRepeatCount = 0;
        }
        return FReply::Handled();
    }

    if (InKeyEvent.GetKey() == HeldDirKey)
    {
        bDirKeyHeld    = false;
        HeldKeyTime    = 0.f;
        LastRepeatCount = 0;
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UBackpackScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshActionButtonHints();
    UpdateOperationHintVisibility();

    PollCombatDeckSelectButtonState();

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        CombatDeckEditWidget->TickDeckGamepadInput(InDeltaTime);
    }

    if (GrabbedRuneIcon)
    {
        const UBackpackStyleDataAsset* Style = BackpackGridWidget ? BackpackGridWidget->StyleDA.Get() : nullptr;
        const float CellPx  = Style ? Style->CellSize : 64.f;
        const float HalfPx  = CellPx * 0.5f;
        const float FloatY  = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) * 5.f;

        // Phase 2: 濡傛灉 ShapePreview 宸叉縺娲伙紙WBP 缁戜簡 ShapePreviewCanvas锛夛紝涓嶅啀鏄剧ず鍗曞浘鏍?GrabbedRuneIcon锛?
        // 閬垮厤鎷栨嫿鏃跺弻鍥炬爣鍙犲姞
        if (bShapePreviewActive)
        {
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
        else if (bMouseDragging && MouseDragTex)
        {
            GrabbedRuneIcon->SetBrushFromTexture(MouseDragTex, false);
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

            const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(LastMouseAbsPos);
            FWidgetTransform T;
            T.Translation = LocalPos + FVector2D(-HalfPx, -HalfPx + FloatY);
            GrabbedRuneIcon->SetRenderTransform(T);
        }
        else if ((bGrabbingRune || bGrabbingFromPending) && BackpackGridWidget)
        {
            UTexture2D* Tex = bGrabbingFromPending
                ? (PendingGrid.IsValidIndex(PendingGrabbedIdx) ? PendingGrid[PendingGrabbedIdx].RuneConfig.RuneIcon.Get() : nullptr)
                : GetRuneIconAtCell(GrabbedFromCell.X, GrabbedFromCell.Y);
            if (Tex)
            {
                GrabbedRuneIcon->SetBrushFromTexture(Tex, false);
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

                if (bCursorInPendingArea
                    && PendingGridWidget && PendingGridWidget->PendingRuneGrid)
                {
                    // 鎵嬫焺鍦ㄥ緟鏀剧疆鍖烘姄璧风鏂囨椂锛氭诞绌哄浘鏍囪窡闅忓緟鏀剧疆鍖哄厜鏍囨牸
                    const FGeometry& PGeo  = PendingGridWidget->PendingRuneGrid->GetCachedGeometry();
                    const FVector2D  PSize = PGeo.GetLocalSize();
                    const int32 PCols = FMath::Max(1, PendingCols);
                    const int32 PRows = FMath::Max(1, PendingRows);
                    const float PCW   = PSize.X / PCols;
                    const float PCH   = PSize.Y / PRows;
                    const int32 CurCol = PendingCursorIdx % PCols;
                    const int32 CurRow = PendingCursorIdx / PCols;
                    const FVector2D SlotCenter((CurCol + 0.5f) * PCW, (CurRow + 0.5f) * PCH);
                    const FVector2D AbsPos   = PGeo.LocalToAbsolute(SlotCenter);
                    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(AbsPos);
                    const float HalfSize = PCW * 0.35f;
                    FWidgetTransform T;
                    T.Translation = LocalPos + FVector2D(-HalfSize, -HalfSize + FloatY);
                    T.Scale       = FVector2D(0.7f, 0.7f);
                    GrabbedRuneIcon->SetRenderTransform(T);
                }
                else
                {
                    // 鎵嬫焺鍦ㄤ富鏍煎瓙鏃讹細娴┖鍥炬爣璺熼殢涓绘牸瀛愬厜鏍囨牸
                    const FGeometry GridGeo  = BackpackGridWidget->GetGridGeometry();
                    const FVector2D GridSize = GridGeo.GetLocalSize();
                    if (GridSize.X > 0.f && GridSize.Y > 0.f)
                    {
                        UBackpackGridComponent* BGComp = GetBackpack();
                        const int32 TGW = BGComp ? BGComp->GridWidth  : 5;
                        const int32 TGH = BGComp ? BGComp->GridHeight : 5;

                        const float CellW = GridSize.X / TGW;
                        const float CellH = GridSize.Y / TGH;
                        const FVector2D CellCenter(
                            (GamepadCursorCell.X + 0.5f) * CellW,
                            (GamepadCursorCell.Y + 0.5f) * CellH);

                        const FVector2D AbsPos   = GridGeo.LocalToAbsolute(CellCenter);
                        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(AbsPos);

                        const float HalfSize = CellW * 0.35f;
                        FWidgetTransform T;
                        T.Translation = LocalPos + FVector2D(-HalfSize, -HalfSize + FloatY);
                        T.Scale       = FVector2D(0.7f, 0.7f);
                        GrabbedRuneIcon->SetRenderTransform(T);
                    }
                }
            }
            else
            {
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        else
        {
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (!bDirKeyHeld) return;

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (!PC->IsInputKeyDown(HeldDirKey))
        {
            bDirKeyHeld    = false;
            HeldKeyTime    = 0.f;
            LastRepeatCount = 0;
            return;
        }
    }

    HeldKeyTime += InDeltaTime;
    if (HeldKeyTime < DirRepeatInitial) return;

    const int32 TargetCount = FMath::FloorToInt((HeldKeyTime - DirRepeatInitial) / DirRepeatRate);
    if (TargetCount <= LastRepeatCount) return;

    LastRepeatCount = TargetCount;

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRepeat] HeldKey=%s TargetCount=%d"),
            *HeldDirKey.ToString(),
            TargetCount);
        if      (HeldDirKey == EKeys::Gamepad_DPad_Left || HeldDirKey == EKeys::Gamepad_LeftStick_Left)   CombatDeckEditWidget->HandleDeckDirectionalInput(-1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right || HeldDirKey == EKeys::Gamepad_LeftStick_Right) CombatDeckEditWidget->HandleDeckDirectionalInput(1);
    }
    else if (bCursorInPendingArea)
    {
        // Up/Down navigation is disabled in the backpack 鈥?only horizontal repeats are honored.
        if      (HeldDirKey == EKeys::Gamepad_DPad_Left)  MovePendingCursor(-1,  0);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MovePendingCursor( 1,  0);
    }
    else
    {
        if      (HeldDirKey == EKeys::Gamepad_DPad_Left)  MoveGamepadCursor(-1,  0);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MoveGamepadCursor( 1,  0);
    }
}

FReply UBackpackScreenWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (!InKeyEvent.IsRepeat() &&
        (Key == EKeys::Gamepad_Special_Left ||
         Key == EKeys::Gamepad_Special_Right ||
         Key == EKeys::Escape ||
         Key == EKeys::Tab))
    {
        DeactivateWidget();
        return FReply::Handled();
    }

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        auto StartDeckDirRepeat = [&](int32 Direction) -> FReply
        {
            bIsGamepadInputMode = true;
            bDeckSelectFromVirtualMouse = false;
            CombatDeckEditWidget->NotifyGamepadNavigationInput();
            if (!InKeyEvent.IsRepeat())
            {
                UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackPreviewKeyDown] DPad Direction=%d"), Direction);
                CombatDeckEditWidget->HandleDeckDirectionalInput(Direction);
                HeldDirKey      = Key;
                bDirKeyHeld     = true;
                HeldKeyTime     = 0.f;
                LastRepeatCount = 0;
            }
            return FReply::Handled();
        };

        if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
        {
            return StartDeckDirRepeat(-1);
        }
        if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
        {
            return StartDeckDirRepeat(1);
        }

        if (!InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackPreviewKeyDown] A Press"));
            bIsGamepadInputMode = true;
            bDeckSelectFromVirtualMouse = false;
            SuppressDeckVirtualMouseSelectUntilTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + 0.25f;
            return HandleCombatDeckSelectButtonState(true, TEXT("PreviewKeyDown"));
        }

        if (!InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Right)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackPreviewKeyDown] B Press"));
            bIsGamepadInputMode = true;
            bDeckSelectFromVirtualMouse = false;
            if (CombatDeckEditWidget->CancelDeckGamepadDrag())
            {
                return FReply::Handled();
            }
        }

        if (!InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Left)
        {
            bIsGamepadInputMode = true;
            bDeckSelectFromVirtualMouse = false;
            return CombatDeckEditWidget->ToggleSelectedLinkOrientation() ? FReply::Handled() : FReply::Unhandled();
        }
    }

    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UBackpackScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackKeyDown] Key=%s Repeat=%d HasDeckWidget=%d CanDeck=%d"),
        *Key.ToString(),
        InKeyEvent.IsRepeat() ? 1 : 0,
        CombatDeckEditWidget ? 1 : 0,
        (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput()) ? 1 : 0);

    // 鎽囨潌杞翠簨浠朵笉瑙﹀彂杈撳叆妯″紡鍒囨崲锛岀洿鎺ュ拷鐣?
    if (Key == EKeys::Gamepad_RightStick_Up   || Key == EKeys::Gamepad_RightStick_Down  ||
        Key == EKeys::Gamepad_RightStick_Left  || Key == EKeys::Gamepad_RightStick_Right)
    {
        return FReply::Handled();
    }

    // 棣栨鍒囨崲鍒版墜鏌勬ā寮忔椂绔嬪埢鏄剧ず鎿嶄綔鎻愮ず
    bIsGamepadInputMode = true;
    if (Key == EKeys::Gamepad_Special_Left ||
        Key == EKeys::Gamepad_Special_Right ||
        Key == EKeys::Escape ||
        Key == EKeys::Tab)
    {
        DeactivateWidget();
        return FReply::Handled();
    }

    if (CombatDeckEditWidget && CombatDeckEditWidget->IsVisible() && !InKeyEvent.IsRepeat() &&
        (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top))
    {
        const bool bVisible = CombatDeckEditWidget->ToggleDetailPreview();
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] DetailPreview Key=%s Visible=%d"),
            *Key.ToString(),
            bVisible ? 1 : 0);
        return FReply::Handled();
    }

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        auto StartDeckDirRepeat = [&](int32 Direction) -> FReply
        {
            CombatDeckEditWidget->NotifyGamepadNavigationInput();
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] DPad Direction=%d"), Direction);
            CombatDeckEditWidget->HandleDeckDirectionalInput(Direction);
            HeldDirKey      = Key;
            bDirKeyHeld     = true;
            HeldKeyTime     = 0.f;
            LastRepeatCount = 0;
            return FReply::Handled();
        };

        if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
        {
            if (InKeyEvent.IsRepeat())
            {
                UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] IgnoreNativeRepeat Key=%s"), *Key.ToString());
                return FReply::Handled();
            }
            return StartDeckDirRepeat(-1);
        }
        if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
        {
            if (InKeyEvent.IsRepeat())
            {
                UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] IgnoreNativeRepeat Key=%s"), *Key.ToString());
                return FReply::Handled();
            }
            return StartDeckDirRepeat(1);
        }
        if (!InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] A Press"));
            SuppressDeckVirtualMouseSelectUntilTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + 0.25f;
            return HandleCombatDeckSelectButtonState(true, TEXT("KeyDown"));
        }
        if (!InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Right)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] B Press"));
            if (CombatDeckEditWidget->CancelDeckGamepadDrag())
            {
                return FReply::Handled();
            }
        }
        if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left)
        {
            if (Key == EKeys::Gamepad_FaceButton_Left)
            {
                CombatDeckEditWidget->NotifyGamepadNavigationInput();
            }
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] Reverse Key=%s"), *Key.ToString());
            return CombatDeckEditWidget->ToggleSelectedLinkOrientation() ? FReply::Handled() : FReply::Unhandled();
        }
    }
    else if (CombatDeckEditWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] Deck widget present but CanHandleDeckInput=false"));
    }

    if (!BackpackGridWidget && !PendingGridWidget)
    {
        if (!InKeyEvent.IsRepeat() && (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter || Key == EKeys::Virtual_Accept))
        {
            DeactivateWidget();
            return FReply::Handled();
        }
        if (!InKeyEvent.IsRepeat() && (Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Escape || Key == EKeys::Virtual_Back))
        {
            DeactivateWidget();
            return FReply::Handled();
        }

        return FReply::Handled();
    }

    // 鈹€鈹€ Gamepad grid navigation 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
    auto StartDirRepeat = [&](int32 DC, int32 DR) -> FReply
    {
        if (bCursorInPendingArea) MovePendingCursor(DC, DR);
        else                      MoveGamepadCursor(DC, DR);
        HeldDirKey      = Key;
        bDirKeyHeld     = true;
        HeldKeyTime     = 0.f;
        LastRepeatCount = 0;
        return FReply::Handled();
    };

    // DPad / left-stick up & down are intentionally inert in the backpack screen 鈥?swallow
    // the event so it doesn't trigger CommonUI focus navigation, but don't move the cursor.
    if (Key == EKeys::Gamepad_DPad_Up   || Key == EKeys::Gamepad_LeftStick_Up   ||
        Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
    {
        return FReply::Handled();
    }
    if (Key == EKeys::Gamepad_DPad_Left)  return StartDirRepeat(-1,  0);
    if (Key == EKeys::Gamepad_DPad_Right) return StartDirRepeat( 1,  0);

    if (!InKeyEvent.IsRepeat())
    {
        if (Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            if (bCursorInPendingArea) PendingGamepadConfirm();
            else                      GamepadConfirm();
            return FReply::Handled();
        }
        if (Key == EKeys::Gamepad_FaceButton_Right)
        {
            if (bCursorInPendingArea) PendingGamepadCancel();
            else                      GamepadCancel();
            return FReply::Handled();
        }
    }

    // 鈹€鈹€ 绗︽枃鏃嬭浆 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
    if (Key == EKeys::R)
    {
        if (bCursorInPendingArea || PendingSelectedIdx >= 0)
            RotatePendingRune();
        else
            RotateSelectedRune();
        return FReply::Handled();
    }
    if (Key == EKeys::Gamepad_FaceButton_Left) // X 閿?
    {
        if (bCursorInPendingArea)
            RotatePendingRune();
        else
            RotateSelectedRune();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UBackpackScreenWidget::HandleCombatDeckSelectButtonState(bool bPressed, const TCHAR* Source)
{
    const bool bCanDeck = CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput();
    UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackAState] Source=%s Pressed=%d WasDown=%d CanDeck=%d"),
        Source,
        bPressed ? 1 : 0,
        bDeckSelectButtonWasDown ? 1 : 0,
        bCanDeck ? 1 : 0);

    if (!bCanDeck)
    {
        bDeckSelectButtonWasDown = bPressed;
        return FReply::Unhandled();
    }

    if (bPressed)
    {
        if (bDeckSelectButtonWasDown)
        {
            // 鎷栨嫿杩涜涓椂鍏佽绗簩娆?A 鎸変笅鐩存帴鎻愪氦锛屽惁鍒欏繀椤荤瓑鍒版澗閿墠鑳芥斁涓嬪崱鐗?
            if (CombatDeckEditWidget && CombatDeckEditWidget->IsGamepadDragActive())
            {
                return CombatDeckEditWidget->HandleDeckSelectPressed() ? FReply::Handled() : FReply::Unhandled();
            }
            return FReply::Handled();
        }

        bDeckSelectButtonWasDown = true;
        return CombatDeckEditWidget->HandleDeckSelectPressed() ? FReply::Handled() : FReply::Unhandled();
    }

    if (!bDeckSelectButtonWasDown)
    {
        if (CombatDeckEditWidget && CombatDeckEditWidget->IsGamepadDragActive())
        {
            return CombatDeckEditWidget->HandleDeckSelectReleased() ? FReply::Handled() : FReply::Unhandled();
        }

        return FReply::Handled();
    }

    bDeckSelectButtonWasDown = false;
    return CombatDeckEditWidget->HandleDeckSelectReleased() ? FReply::Handled() : FReply::Unhandled();
}

void UBackpackScreenWidget::PollCombatDeckSelectButtonState()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    if (bDeckSelectFromVirtualMouse)
    {
        return;
    }

    const bool bPressed = PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
    if (bPressed == bDeckSelectButtonWasDown)
    {
        return;
    }

    HandleCombatDeckSelectButtonState(bPressed, TEXT("TickPoll"));
}

void UBackpackScreenWidget::MoveGamepadCursor(int32 DCol, int32 DRow)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 W = Backpack ? Backpack->GridWidth  : 5;
    const int32 H = Backpack ? Backpack->GridHeight : 5;

    // DPad Left 浠庣 0 鍒楀嚭杈圭晫 鈫?杩涘叆寰呮斁缃尯
    if (DCol == -1 && GamepadCursorCell.X == 0)
    {
        bCursorInPendingArea = true;
        const int32 PCols = FMath::Max(1, PendingCols);
        const int32 PRows = FMath::Max(1, PendingRows);
        const int32 Row   = FMath::Clamp(GamepadCursorCell.Y, 0, PRows - 1);
        PendingCursorIdx  = Row * PCols + (PCols - 1);

        if (!bGrabbingRune && !bGrabbingFromPending)
        {
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
        }
        RefreshPendingGrid();
        OnGridNeedsRefresh();
        return;
    }

    GamepadCursorCell.X = FMath::Clamp(GamepadCursorCell.X + DCol, 0, W - 1);
    GamepadCursorCell.Y = FMath::Clamp(GamepadCursorCell.Y + DRow, 0, H - 1);

    if (!bGrabbingRune)
    {
        // 浠呮偓娴珮浜紙缁挎锛夛紝涓嶈Е鍙戦€変腑/淇℃伅鍗?
        HoverCol = GamepadCursorCell.X;
        HoverRow = GamepadCursorCell.Y;
        OnGridNeedsRefresh();
    }
    else
    {
        OnGridNeedsRefresh();
    }

    UpdateTooltipForCell(GamepadCursorCell.X, GamepadCursorCell.Y, FVector2D::ZeroVector);
}

void UBackpackScreenWidget::GamepadConfirm()
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈡姄鍙?鏀剧疆

    // 浠庡緟鏀剧疆鍖烘姄璧峰悗锛屽湪涓绘牸瀛愯惤鐐?
    if (bGrabbingFromPending)
    {
        if (!PendingGrid.IsValidIndex(PendingGrabbedIdx) || !PendingGrid[PendingGrabbedIdx].RuneGuid.IsValid())
        {
            bGrabbingFromPending = false; PendingGrabbedIdx = -1; return;
        }
        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack) return;

        const FRuneInstance PendingRune = PendingGrid[PendingGrabbedIdx];
        if (Backpack->TryPlaceRune(PendingRune, GamepadCursorCell))
        {
            PendingGrid[PendingGrabbedIdx] = FRuneInstance();
            SyncPendingToPlayer();
            bGrabbingFromPending = false;
            PendingGrabbedIdx    = -1;
            PendingSelectedIdx   = -1;
            RefreshPendingGrid();
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "PendingPlaceOK", "宸叉斁缃細{0}"),
                FText::FromName(PendingRune.RuneConfig.RuneName)));
        }
        else
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GamepadCursorCell.X, GamepadCursorCell.Y);
            OnStatusMessage(NSLOCTEXT("Backpack", "PendingPlaceFail", "Could not place rune here."));
        }
        return;
    }

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    if (!bGrabbingRune)
    {
        int32 RuneIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);
        if (RuneIdx >= 0)
        {
            if (IsInCombatPhase())
            {
                if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GamepadCursorCell.X, GamepadCursorCell.Y);
                OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat."));
                return;
            }

            bGrabbingRune    = true;
            GrabbedFromCell  = GamepadCursorCell;
            SelectedCell     = GamepadCursorCell;
            HoverCol = HoverRow = -1;
            OnSelectionChanged();
            OnStatusMessage(NSLOCTEXT("Backpack", "GrabOK", "宸叉姄鍙栫鏂囷紝绉诲姩鍏夋爣鍚庢寜纭鏀剧疆"));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack", "GrabEmpty", "No rune in this cell."));
        }
    }
    else
    {
        if (GamepadCursorCell == GrabbedFromCell)
        {
            GamepadCancel();
            return;
        }

        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        int32 SrcIdx = Backpack->GetRuneIndexAtCell(GrabbedFromCell);
        int32 DstIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);

        if (SrcIdx < 0)
        {
            bGrabbingRune = false;
            GrabbedFromCell = FIntPoint(-1, -1);
            return;
        }

        const FRuneInstance RuneA  = Placed[SrcIdx].Rune;
        const FIntPoint     PivotA = Placed[SrcIdx].Pivot;

        if (DstIdx >= 0)
        {
            // 浜掓崲 鈫?鑷姩鎶撳彇琚浛鎹㈢鏂囷紙RuneB 鐜板湪鍦?PivotA锛?
            const FRuneInstance RuneB  = Placed[DstIdx].Rune;
            const FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneA.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);
            Backpack->TryPlaceRune(RuneA, PivotB);
            Backpack->TryPlaceRune(RuneB, PivotA);

            GrabbedFromCell   = PivotA;
            SelectedCell      = PivotA;
            GamepadCursorCell = PivotA;
            // bGrabbingRune 淇濇寔 true
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "SwapOK", "宸蹭簰鎹細{0} 鈫?{1}"),
                FText::FromName(RuneA.RuneConfig.RuneName),
                FText::FromName(RuneB.RuneConfig.RuneName)));
            OnGridNeedsRefresh();
        }
        else
        {
            // 绉诲姩鍒扮┖鏍?鈫?鏀剧疆鎴愬姛鍚庣粨鏉熸姄鍙?
            const FIntPoint Offset   = GrabbedFromCell - PivotA;
            const FIntPoint NewPivot = GamepadCursorCell - Offset;

            if (Backpack->MoveRune(RuneA.RuneGuid, NewPivot))
            {
                bGrabbingRune   = false;
                GrabbedFromCell = FIntPoint(-1,-1);
                SelectedCell    = FIntPoint(-1,-1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "MoveOK", "宸茬Щ鍔細{0}"),
                    FText::FromName(RuneA.RuneConfig.RuneName)));
            }
            else
            {
                if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GamepadCursorCell.X, GamepadCursorCell.Y);
                OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "鏃犳硶鏀剧疆锛氱洰鏍囦綅缃鍗犵敤"));
                OnGridNeedsRefresh();
            }
        }
    }
}

void UBackpackScreenWidget::GamepadCancel()
{
    if (bGrabbingFromPending)
    {
        bGrabbingFromPending = false;
        PendingGrabbedIdx    = -1;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "Grab cancelled."));
        return;
    }

    if (bGrabbingRune)
    {
        bGrabbingRune   = false;
        GrabbedFromCell = FIntPoint(-1, -1);
        SelectedCell    = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "Grab cancelled."));
        return;
    }

    // 鏈姄鍙栨椂 B 閿洿鎺ュ叧闂儗鍖?
    DeactivateWidget();
}

// 鈹€鈹€ 寰呮斁缃尯鎵嬫焺鏂规硶 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

void UBackpackScreenWidget::MovePendingCursor(int32 DCol, int32 DRow)
{
    const int32 PCols = FMath::Max(1, PendingCols);
    const int32 PRows = FMath::Max(1, PendingRows);

    int32 Col = PendingCursorIdx % PCols + DCol;
    int32 Row = PendingCursorIdx / PCols + DRow;

    // 鍚戝彸鍑鸿竟鐣?鈫?鍒囨崲鍒颁富鏍煎瓙
    if (DCol == 1 && Col >= PCols)
    {
        bCursorInPendingArea = false;
        UBackpackGridComponent* Backpack = GetBackpack();
        const int32 GH = Backpack ? Backpack->GridHeight : 5;
        GamepadCursorCell = FIntPoint(0, FMath::Clamp(Row, 0, GH - 1));

        if (!bGrabbingRune && !bGrabbingFromPending)
        {
            SelectedCell = GamepadCursorCell;
            OnSelectionChanged();
        }
        else
        {
            OnGridNeedsRefresh();
        }
        RefreshPendingGrid();
        return;
    }

    Col = FMath::Clamp(Col, 0, PCols - 1);
    Row = FMath::Clamp(Row, 0, PRows - 1);
    PendingCursorIdx = Row * PCols + Col;
    RefreshPendingGrid();
}

void UBackpackScreenWidget::PendingGamepadConfirm()
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈡搷浣?

    // 涓绘牸瀛愭姄鍙栫姸鎬佷笅杩涘叆寰呮斁缃尯锛欰 閿皢绗︽枃閫佸洖寰呮斁缃Ы
    if (bGrabbingRune)
    {
        if (IsInCombatPhase()) { OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat.")); return; }

        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack) return;

        FPlacedRune PR = GetRuneAtCell(GrabbedFromCell.X, GrabbedFromCell.Y);
        if (!PR.Rune.RuneGuid.IsValid()) { bGrabbingRune = false; return; }

        // 浼樺厛鏀惧叆鍏夋爣鏍硷紙鑻ヤ负绌猴級锛屽惁鍒欐壘绗竴涓┖鏍?
        int32 TargetSlot = -1;
        if (PendingGrid.IsValidIndex(PendingCursorIdx) && !PendingGrid[PendingCursorIdx].RuneGuid.IsValid())
            TargetSlot = PendingCursorIdx;
        if (TargetSlot < 0)
            for (int32 i = 0; i < PendingGrid.Num(); i++)
                if (!PendingGrid[i].RuneGuid.IsValid()) { TargetSlot = i; break; }

        if (TargetSlot < 0) { OnStatusMessage(NSLOCTEXT("Backpack", "PendingFull", "Pending rune list is full.")); return; }

        Backpack->RemoveRune(PR.Rune.RuneGuid);
        PendingGrid[TargetSlot] = PR.Rune;
        SyncPendingToPlayer();

        bGrabbingRune      = false;
        GrabbedFromCell    = FIntPoint(-1, -1);
        SelectedCell       = FIntPoint(-1, -1);
        PendingSelectedIdx = TargetSlot;
        PendingCursorIdx   = TargetSlot;
        RefreshPendingGrid();
        OnSelectionChanged();
        OnStatusMessage(FText::Format(
            NSLOCTEXT("Backpack", "UnplaceOK", "宸插彇鍥烇細{0}"),
            FText::FromName(PR.Rune.RuneConfig.RuneName)));
        return;
    }

    const bool bHasRune = PendingGrid.IsValidIndex(PendingCursorIdx)
        && PendingGrid[PendingCursorIdx].RuneGuid.IsValid();

    if (!bGrabbingFromPending)
    {
        if (!bHasRune) { OnStatusMessage(NSLOCTEXT("Backpack", "PendingGrabEmpty", "No rune in this pending slot.")); return; }
        if (IsInCombatPhase()) { OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "Backpack editing is locked during combat.")); return; }

        bGrabbingFromPending = true;
        PendingGrabbedIdx    = PendingCursorIdx;
        PendingSelectedIdx   = PendingCursorIdx;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "PendingGrabOK", "Pending rune grabbed."));
    }
    else
    {
        if (PendingCursorIdx == PendingGrabbedIdx) { PendingGamepadCancel(); return; }

        // 鍦ㄥ緟鏀剧疆鍖哄唴浜ゆ崲
        const FRuneInstance Src = PendingGrid[PendingGrabbedIdx];
        const FRuneInstance Dst = PendingGrid.IsValidIndex(PendingCursorIdx)
            ? PendingGrid[PendingCursorIdx] : FRuneInstance();
        PendingGrid[PendingGrabbedIdx] = Dst;
        if (PendingGrid.IsValidIndex(PendingCursorIdx)) PendingGrid[PendingCursorIdx] = Src;

        SyncPendingToPlayer();
        bGrabbingFromPending = false;
        PendingGrabbedIdx    = -1;
        PendingSelectedIdx   = PendingCursorIdx;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "PendingMoved", "Pending rune moved."));
    }
}

void UBackpackScreenWidget::PendingGamepadCancel()
{
    if (bGrabbingFromPending)
    {
        bGrabbingFromPending = false;
        PendingGrabbedIdx    = -1;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "Grab cancelled."));
        return;
    }
    // 鏈姄鍙栨椂 B 閿€€鍑哄緟鏀剧疆鍖猴紝鍥炲埌涓绘牸瀛愭渶宸﹀垪
    bCursorInPendingArea = false;
    PendingSelectedIdx   = -1;
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 GH = Backpack ? Backpack->GridHeight : 5;
    const int32 Row = FMath::Clamp(PendingCursorIdx / FMath::Max(1, PendingCols), 0, GH - 1);
    GamepadCursorCell = FIntPoint(0, Row);
    SelectedCell = GamepadCursorCell;
    OnSelectionChanged();
    RefreshPendingGrid();
}

// ============================================================
//  绗︽枃鏃嬭浆
// ============================================================

void UBackpackScreenWidget::RotateSelectedRune()
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈡棆杞?

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    const FIntPoint TargetCell = bGrabbingRune ? GrabbedFromCell : SelectedCell;
    if (TargetCell == FIntPoint(-1, -1)) return;

    const int32 Idx = Backpack->GetRuneIndexAtCell(TargetCell);
    const TArray<FPlacedRune>& AllPlaced = Backpack->GetAllPlacedRunes();
    if (!AllPlaced.IsValidIndex(Idx)) return;

    const FPlacedRune PR = AllPlaced[Idx];
    FRuneInstance NewRune = PR.Rune;
    NewRune.Rotation = (NewRune.Rotation + 1) % 4;

    // 浠ョ鏂?(0,0) 鏍间负鏃嬭浆涓績锛氳绠楁棆杞悗浣?icon 鏍间繚鎸佸師浣嶇疆鐨勬柊 Pivot
    const FIntPoint IconAbsCell = PR.Pivot + PR.Rune.Shape.GetPivotOffset(PR.Rune.Rotation);
    const FIntPoint NewPivot    = IconAbsCell - NewRune.Shape.GetPivotOffset(NewRune.Rotation);

    Backpack->RemoveRune(PR.Rune.RuneGuid);
    bool bSuccess = Backpack->TryPlaceRune(NewRune, NewPivot);
    if (!bSuccess)
        bSuccess = Backpack->TryPlaceRune(NewRune, PR.Pivot);  // 閫€鑰屾眰鍏舵锛氬師 Pivot
    if (!bSuccess)
        Backpack->TryPlaceRune(PR.Rune, PR.Pivot);              // 杩樺師

    // icon 鏍间繚鎸佸湪 IconAbsCell锛屽皢閫変腑/鎶撳彇鎸囬拡鏇存柊鍒拌鏍?
    if (bSuccess)
    {
        SelectedCell = IconAbsCell;
        if (bGrabbingRune) GrabbedFromCell = IconAbsCell;
        OnSelectionChanged();
    }
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::RotatePendingRune()
{
    if (bIsPreviewMode) return;  // 鍙棰勮妯″紡锛氱姝㈡棆杞?pending 绗︽枃
    const int32 Idx = bCursorInPendingArea ? PendingCursorIdx : PendingSelectedIdx;
    if (!PendingGrid.IsValidIndex(Idx)) return;
    if (PendingGrid[Idx].RuneGuid == FGuid()) return; // 绌烘牸

    PendingGrid[Idx].Rotation = (PendingGrid[Idx].Rotation + 1) % 4;
    SyncPendingToPlayer();
    RefreshPendingGrid();
}

void UBackpackScreenWidget::UpdateTooltipForCell(int32 Col, int32 Row, const FVector2D& LocalPos)
{
    if (!RuneTooltip) return;

    if (IsCellOccupied(Col, Row))
    {
        FPlacedRune PR = GetRuneAtCell(Col, Row);
        RuneTooltip->ShowRuneInfo(PR.Rune);

        const FVector2D Offset(16.f, -8.f);
        RuneTooltip->SetRenderTranslation(LocalPos + Offset);
    }
    else
    {
        RuneTooltip->HideTooltip();
    }
}
