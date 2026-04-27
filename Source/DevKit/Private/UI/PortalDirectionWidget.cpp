#include "UI/PortalDirectionWidget.h"
#include "Map/Portal.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"

namespace
{
    FLinearColor GetTypeAccentColor(const FGameplayTag& Tag)
    {
        if (!Tag.IsValid()) return FLinearColor(0.95f, 0.85f, 0.55f, 0.92f);
        const FName Name = Tag.GetTagName();
        if (Name == FName("Room.Type.Elite"))  return FLinearColor(0.85f, 0.35f, 0.29f, 0.92f);
        if (Name == FName("Room.Type.Shop"))   return FLinearColor(0.85f, 0.69f, 0.28f, 0.92f);
        if (Name == FName("Room.Type.Event"))  return FLinearColor(0.48f, 0.36f, 0.79f, 0.92f);
        return FLinearColor(0.95f, 0.85f, 0.55f, 0.92f);
    }
}

void UPortalDirectionWidget::SetActive(bool bInActive, const TArray<APortal*>& OpenPortals)
{
    bActive = bInActive;

    TrackedPortals.Reset();
    if (bActive)
    {
        for (APortal* P : OpenPortals)
        {
            if (P) TrackedPortals.Add(P);
        }
    }

    ClearArrowUnits();
    if (bActive && TrackedPortals.Num() > 0)
    {
        RebuildArrowUnits();
    }

    SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPortalDirectionWidget::ClearArrowUnits()
{
    for (FArrowUnit& U : Units)
    {
        if (U.Container) U.Container->RemoveFromParent();
    }
    Units.Reset();
}

void UPortalDirectionWidget::RebuildArrowUnits()
{
    if (!RootCanvas) return;

    for (const TWeakObjectPtr<APortal>& W : TrackedPortals)
    {
        APortal* P = W.Get();
        if (!P) continue;

        UHorizontalBox* Container = NewObject<UHorizontalBox>(this);

        // 箭头 Image
        USizeBox* ArrowBox = NewObject<USizeBox>(this);
        ArrowBox->SetWidthOverride(ArrowSize);
        ArrowBox->SetHeightOverride(ArrowSize);
        UImage* Arrow = NewObject<UImage>(this);
        if (ArrowTexture)
            Arrow->SetBrushFromTexture(ArrowTexture, false);
        Arrow->SetColorAndOpacity(ArrowColor);
        ArrowBox->AddChild(Arrow);

        UHorizontalBoxSlot* ArrowSlot = Container->AddChildToHorizontalBox(ArrowBox);
        ArrowSlot->SetVerticalAlignment(VAlign_Center);
        ArrowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ArrowSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));

        // 标签 TextBlock（房间名 — 颜色按类型）
        UTextBlock* Label = NewObject<UTextBlock>(this);
        Label->SetText(P->CachedPreviewInfo.RoomDisplayName);
        Label->SetColorAndOpacity(FSlateColor(GetTypeAccentColor(P->CachedPreviewInfo.RoomTypeTag)));
        UHorizontalBoxSlot* LabelSlot = Container->AddChildToHorizontalBox(Label);
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

        UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Container);
        CanvasSlot->SetSize(FVector2D(220.f, ArrowSize));   // 估宽，足够容纳房间名
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetAutoSize(false);

        FArrowUnit U;
        U.Portal     = P;
        U.Container  = Container;
        U.ArrowImage = Arrow;
        U.LabelText  = Label;
        Units.Add(U);
    }
}

void UPortalDirectionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bActive || Units.Num() == 0) return;

    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->GetPawn()) return;

    // 玩家进入任意 Portal Box → 全隐
    if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(PC->GetPawn()))
    {
        if (Player->PendingPortal != nullptr)
        {
            for (FArrowUnit& U : Units)
                if (U.Container) U.Container->SetVisibility(ESlateVisibility::Collapsed);
            return;
        }
    }

    UGameViewportClient* GVC = GetWorld()->GetGameViewport();
    if (!GVC) return;
    FVector2D ViewportSize;
    GVC->GetViewportSize(ViewportSize);
    const FVector2D Center = ViewportSize * 0.5f;

    for (FArrowUnit& U : Units)
    {
        APortal* P = U.Portal.Get();
        if (!P || !U.Container) continue;

        const FVector ProjectPoint = P->GetActorLocation() + FVector(0.f, 0.f, ArrowProjectionZOffset);
        FVector2D SP;
        const bool bOn = IsOnScreen(ProjectPoint, SP);

        if (bOn)
        {
            // 屏幕内：让浮窗接手，箭头隐藏
            U.Container->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }

        U.Container->SetVisibility(ESlateVisibility::HitTestInvisible);

        const FVector2D EdgePos = ClampToScreenEdge(SP, Center, ViewportSize);
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(U.Container->Slot))
            CanvasSlot->SetPosition(EdgePos);

        if (U.ArrowImage)
            U.ArrowImage->SetRenderTransformAngle(CalcArrowAngle(SP, Center));
    }
}

bool UPortalDirectionWidget::IsOnScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return false;

    FVector CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);
    const bool bInFront = FVector::DotProduct(CamRot.Vector(), WorldPos - CamLoc) > 0.f;

    PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, false);

    if (!bInFront)
    {
        FVector2D ViewSize;
        if (UGameViewportClient* GVC = GetWorld()->GetGameViewport())
            GVC->GetViewportSize(ViewSize);
        OutScreenPos = ViewSize - OutScreenPos;
        return false;
    }

    FVector2D ViewSize;
    if (UGameViewportClient* GVC = GetWorld()->GetGameViewport())
        GVC->GetViewportSize(ViewSize);
    return OutScreenPos.X >= OnScreenShrink && OutScreenPos.X <= ViewSize.X - OnScreenShrink
        && OutScreenPos.Y >= OnScreenShrink && OutScreenPos.Y <= ViewSize.Y - OnScreenShrink;
}

FVector2D UPortalDirectionWidget::ClampToScreenEdge(const FVector2D& ScreenPos,
                                                     const FVector2D& Center,
                                                     const FVector2D& ViewportSize) const
{
    const FVector2D Dir = ScreenPos - Center;
    if (Dir.IsNearlyZero()) return Center;

    const float HalfW = ViewportSize.X * 0.5f - EdgeMargin;
    const float HalfH = ViewportSize.Y * 0.5f - EdgeMargin;

    const float Scale = FMath::Min(
        HalfW / (FMath::Abs(Dir.X) + KINDA_SMALL_NUMBER),
        HalfH / (FMath::Abs(Dir.Y) + KINDA_SMALL_NUMBER));

    return Center + Dir * Scale;
}

float UPortalDirectionWidget::CalcArrowAngle(const FVector2D& TargetScreenPos,
                                              const FVector2D& Center) const
{
    const FVector2D Dir = TargetScreenPos - Center;
    return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)) + ArrowAngleOffset;
}
