#include "UI/EnemyArrowWidget.h"
#include "GameModes/YogGameMode.h"
#include "Character/EnemyCharacterBase.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"

void UEnemyArrowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            if (UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(
                    Pawn->FindComponentByClass<UAbilitySystemComponent>()))
            {
                CachedPlayerASC = ASC;
                ASC->ReceivedDamage.AddDynamic(this, &UEnemyArrowWidget::OnPlayerDamageTaken);
            }
        }
    }

    RebuildArrowPool();
}

void UEnemyArrowWidget::RebuildArrowPool()
{
    for (UImage* Img : ArrowImages)
        if (Img) Img->RemoveFromParent();
    ArrowImages.Empty();

    if (!RootCanvas) return;

    for (int32 i = 0; i < MaxArrows; i++)
    {
        UImage* Img = NewObject<UImage>(this);
        if (ArrowTexture)
            Img->SetBrushFromTexture(ArrowTexture, false);
        Img->SetColorAndOpacity(ArrowColor);
        Img->SetVisibility(ESlateVisibility::Collapsed);

        UCanvasPanelSlot* ArrowSlot = RootCanvas->AddChildToCanvas(Img);
        ArrowSlot->SetSize(FVector2D(ArrowSize, ArrowSize));
        ArrowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        ArrowImages.Add(Img);
    }
}

void UEnemyArrowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    auto HideAll = [this]
    {
        for (UImage* A : ArrowImages)
            if (A) A->SetVisibility(ESlateVisibility::Collapsed);
    };

    AYogGameMode* GM = GetWorld()->GetAuthGameMode<AYogGameMode>();
    if (!GM || !GM->HasAliveEnemies()) { HideAll(); return; }

    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->GetPawn()) { HideAll(); return; }

    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    const FVector2D Center = ViewportSize * 0.5f;

    TArray<AEnemyCharacterBase*> AllEnemies = GM->GetAllAliveEnemies();

    bool bAnyOnScreen = false;
    TArray<TPair<AEnemyCharacterBase*, FVector2D>> EnemyScreenData;
    EnemyScreenData.Reserve(AllEnemies.Num());

    const FVector PlayerPos = PC->GetPawn()->GetActorLocation();
    for (AEnemyCharacterBase* E : AllEnemies)
    {
        FVector2D SP;
        // 抬高投影点到胶囊中段，修正斜视角下脚底投影偏差
        const FVector ProjectPoint = E->GetActorLocation() + FVector(0.f, 0.f, ArrowProjectionZOffset);
        bool bOn = IsOnScreen(ProjectPoint, SP);
        // 世界距离超限 → 强制视为离屏
        if (bOn && ForceOffScreenDistance > 0.f)
        {
            if (FVector::Dist(ProjectPoint, PlayerPos) > ForceOffScreenDistance)
                bOn = false;
        }
        EnemyScreenData.Add({ E, SP });
        if (bOn) bAnyOnScreen = true;
    }

    if (bAnyOnScreen)
        LastCombatEventTime = GetWorld()->GetTimeSeconds();

    const float Now = GetWorld()->GetTimeSeconds();
    if (bAnyOnScreen || (Now - LastCombatEventTime < AppearDelay))
    {
        HideAll();
        return;
    }

    // 按距离排序，取最近的 MaxArrows 个
    EnemyScreenData.Sort([&PlayerPos](const TPair<AEnemyCharacterBase*, FVector2D>& A,
                                      const TPair<AEnemyCharacterBase*, FVector2D>& B)
    {
        return FVector::DistSquared(A.Key->GetActorLocation(), PlayerPos)
             < FVector::DistSquared(B.Key->GetActorLocation(), PlayerPos);
    });

    int32 ArrowIdx = 0;
    for (const TPair<AEnemyCharacterBase*, FVector2D>& Pair : EnemyScreenData)
    {
        if (ArrowIdx >= MaxArrows) break;

        UImage* Arrow = ArrowImages[ArrowIdx++];
        Arrow->SetVisibility(ESlateVisibility::HitTestInvisible);

        const FVector2D EdgePos = ClampToScreenEdge(Pair.Value, Center, ViewportSize);
        if (UCanvasPanelSlot* ArrowSlot = Cast<UCanvasPanelSlot>(Arrow->Slot))
            ArrowSlot->SetPosition(EdgePos);

        Arrow->SetRenderTransformAngle(CalcArrowAngle(Pair.Value, Center));
    }

    for (int32 i = ArrowIdx; i < ArrowImages.Num(); i++)
        if (ArrowImages[i]) ArrowImages[i]->SetVisibility(ESlateVisibility::Collapsed);
}

bool UEnemyArrowWidget::IsOnScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const
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
        // 敌人在摄像机后方：镜像翻转坐标，使箭头出现在玩家"背向"的屏幕边缘
        FVector2D ViewSize;
        GetWorld()->GetGameViewport()->GetViewportSize(ViewSize);
        OutScreenPos = ViewSize - OutScreenPos;
        return false;
    }

    FVector2D ViewSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewSize);
    return OutScreenPos.X >= OnScreenShrink && OutScreenPos.X <= ViewSize.X - OnScreenShrink
        && OutScreenPos.Y >= OnScreenShrink && OutScreenPos.Y <= ViewSize.Y - OnScreenShrink;
}

FVector2D UEnemyArrowWidget::ClampToScreenEdge(const FVector2D& ScreenPos,
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

float UEnemyArrowWidget::CalcArrowAngle(const FVector2D& EnemyScreenPos,
                                         const FVector2D& Center) const
{
    const FVector2D Dir = EnemyScreenPos - Center;
    // atan2 在屏幕坐标下（Y 向下）给出相对 +X 轴的角度
    // 加 90° 是因为贴图约定顶点朝上（-Y），对应旋转 0° 时朝右（+X）需要补偿
    return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)) + 90.f;
}

void UEnemyArrowWidget::OnPlayerDamageTaken(UYogAbilitySystemComponent* /*SourceASC*/, float Damage)
{
    if (Damage > 0.f)
        LastCombatEventTime = GetWorld()->GetTimeSeconds();
}
