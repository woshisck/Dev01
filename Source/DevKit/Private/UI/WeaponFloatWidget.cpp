#include "UI/WeaponFloatWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Data/RuneDataAsset.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"

namespace WeaponZoneColors
{
	static const FLinearColor Active  (0.20f, 0.80f, 1.00f, 1.0f);
	static const FLinearColor Inactive(0.15f, 0.15f, 0.18f, 0.7f);
}

void UWeaponFloatWidget::SetWeaponDefinition(const UWeaponDefinition* Def)
{
	if (!Def) return;

	// 重置动画状态（每次拿到新武器从 Idle 开始）
	CurrentPhase = EWeaponFloatPhase::Idle;
	PhaseTimer   = 0.f;
	SetRenderTransform(FWidgetTransform());
	SetRenderOpacity(1.f);
	if (InfoContainer)
		InfoContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const UWeaponInfoDA* Info = Def->WeaponInfo;

	// ── 缩略图 ─────────────────────────────────
	CachedThumbnail = nullptr;
	if (WeaponThumbnail)
	{
		if (Info && Info->Thumbnail)
		{
			WeaponThumbnail->SetBrushFromTexture(Info->Thumbnail, true);
			WeaponThumbnail->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			CachedThumbnail = Info->Thumbnail;
		}
		else
		{
			WeaponThumbnail->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ── 名称 ───────────────────────────────────
	if (WeaponNameText)
		WeaponNameText->SetText(Info ? Info->WeaponName : FText::GetEmpty());

	// ── 描述 ───────────────────────────────────
	if (WeaponDescText)
	{
		const bool bHas = Info && !Info->WeaponDescription.IsEmpty();
		WeaponDescText->SetVisibility(bHas ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bHas) WeaponDescText->SetText(Info->WeaponDescription);
	}

	// ── 子描述 ─────────────────────────────────
	if (WeaponSubDescText)
	{
		const bool bHas = Info && !Info->WeaponSubDescription.IsEmpty();
		WeaponSubDescText->SetVisibility(bHas ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bHas) WeaponSubDescText->SetText(Info->WeaponSubDescription);
	}

	// ── 激活区 ─────────────────────────────────
	const FActivationZoneConfig& ZoneCfg = Def->BackpackConfig.ActivationZoneConfig;
	const int32 GW = Def->BackpackConfig.GridWidth;
	const int32 GH = Def->BackpackConfig.GridHeight;

	auto GetShape   = [&](int32 Idx) -> const FRuneShape*
	{
		return ZoneCfg.ZoneShapes.IsValidIndex(Idx) ? &ZoneCfg.ZoneShapes[Idx] : nullptr;
	};
	auto GetZoneImg = [&](int32 Idx) -> UTexture2D*
	{
		if (!Info) return nullptr;
		switch (Idx) {
			case 0: return Info->Zone1Image;
			case 1: return Info->Zone2Image;
			case 2: return Info->Zone3Image;
		}
		return nullptr;
	};

	BuildZonePanel(ZoneGrid1, Zone1Image, GetZoneImg(0), GetShape(0), GW, GH);
	BuildZonePanel(ZoneGrid2, Zone2Image, GetZoneImg(1), GetShape(1), GW, GH);
	BuildZonePanel(ZoneGrid3, Zone3Image, GetZoneImg(2), GetShape(2), GW, GH);

	// ── 初始符文列表 ───────────────────────────
	BuildRuneList(Def->InitialRunes);
}

// ─────────────────────────────────────────────────────────────────────────────
//  动画：折叠 → 缩小 → 飞行
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::StartCollapseAndFly(FVector2D TargetScreenCenter,
                                              const UWeaponGlassAnimDA* InAnimDA)
{
	if (!InAnimDA || CurrentPhase != EWeaponFloatPhase::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] StartCollapseAndFly 中止 — DA=%s Phase=%d"),
			InAnimDA ? TEXT("OK") : TEXT("NULL"), (int32)CurrentPhase);
		return;
	}

	AnimDA           = InAnimDA;
	PhaseTimer       = 0.f;
	bFlyStartCaptured = false;
	CurrentPhase     = EWeaponFloatPhase::Collapsing;

	// Phase 1：立即隐藏非缩略图区域（用 InfoContainer 整体折叠，或逐个隐藏）
	if (InfoContainer)
	{
		InfoContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// 没有 InfoContainer 时退化为逐个隐藏
		if (WeaponNameText)    WeaponNameText->SetVisibility(ESlateVisibility::Collapsed);
		if (WeaponDescText)    WeaponDescText->SetVisibility(ESlateVisibility::Collapsed);
		if (WeaponSubDescText) WeaponSubDescText->SetVisibility(ESlateVisibility::Collapsed);
		if (ZoneGrid1)         ZoneGrid1->SetVisibility(ESlateVisibility::Collapsed);
		if (ZoneGrid2)         ZoneGrid2->SetVisibility(ESlateVisibility::Collapsed);
		if (ZoneGrid3)         ZoneGrid3->SetVisibility(ESlateVisibility::Collapsed);
		if (Zone1Image)        Zone1Image->SetVisibility(ESlateVisibility::Collapsed);
		if (Zone2Image)        Zone2Image->SetVisibility(ESlateVisibility::Collapsed);
		if (Zone3Image)        Zone3Image->SetVisibility(ESlateVisibility::Collapsed);
		if (RuneListBox)       RuneListBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 计算缩小目标 Scale
	const FGeometry Geom     = GetCachedGeometry();
	const FVector2D LocalSize = Geom.GetLocalSize();
	if (LocalSize.X > 0.f && LocalSize.Y > 0.f)
	{
		TargetShrinkScale = FMath::Min(
			InAnimDA->GlassIconSize.X / LocalSize.X,
			InAnimDA->GlassIconSize.Y / LocalSize.Y);
	}
	else
	{
		TargetShrinkScale = 0.15f;
	}

	// 计算飞行位移（屏幕绝对坐标 Center → 目标中心）
	const FVector2D WidgetCenter = Geom.LocalToAbsolute(LocalSize * 0.5f);
	FlyDelta = TargetScreenCenter - WidgetCenter;

	UE_LOG(LogTemp, Warning,
		TEXT("[WeaponPickup] StartCollapseAndFly OK — LocalSize=(%.0f,%.0f) WidgetCenter=(%.0f,%.0f) TargetCenter=(%.0f,%.0f) ShrinkScale=%.3f FlyDelta=(%.0f,%.0f)"),
		LocalSize.X, LocalSize.Y, WidgetCenter.X, WidgetCenter.Y,
		TargetScreenCenter.X, TargetScreenCenter.Y,
		TargetShrinkScale, FlyDelta.X, FlyDelta.Y);
}

void UWeaponFloatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CurrentPhase == EWeaponFloatPhase::Idle || !AnimDA) return;

	PhaseTimer += InDeltaTime;

	const float T_Collapse = AnimDA->CollapseDuration;
	const float T_Shrink   = AnimDA->ShrinkDuration;
	const float T_Fly      = AnimDA->FlyDuration;

	FWidgetTransform WT;

	if (PhaseTimer < T_Collapse)
	{
		// Phase 1 Collapsing：等待，不做 transform（已瞬间隐藏 InfoContainer）
		if (CurrentPhase != EWeaponFloatPhase::Collapsing)
			UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] → Phase: Collapsing"));
		CurrentPhase = EWeaponFloatPhase::Collapsing;
		return;
	}

	const float ShrinkElapsed = PhaseTimer - T_Collapse;

	if (ShrinkElapsed < T_Shrink)
	{
		// Phase 2 Shrinking
		if (CurrentPhase != EWeaponFloatPhase::Shrinking)
			UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] → Phase: Shrinking (ShrinkScale=%.3f)"), TargetShrinkScale);
		CurrentPhase = EWeaponFloatPhase::Shrinking;
		const float Alpha = ShrinkElapsed / FMath::Max(T_Shrink, 0.001f);
		const float Scale = FMath::Lerp(1.f, TargetShrinkScale, Alpha);
		WT.Scale = FVector2D(Scale);

		// 缩略图在缩小过程中淡至飞行不透明度
		if (WeaponThumbnail)
			WeaponThumbnail->SetRenderOpacity(FMath::Lerp(1.f, AnimDA->ThumbnailFlyOpacity, Alpha));
	}
	else
	{
		const float FlyElapsed = ShrinkElapsed - T_Shrink;

		if (FlyElapsed < T_Fly)
		{
			// Phase 3 Flying
			if (CurrentPhase != EWeaponFloatPhase::Flying)
				UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] → Phase: Flying (FlyDelta=%.0f,%.0f)"), FlyDelta.X, FlyDelta.Y);
			CurrentPhase = EWeaponFloatPhase::Flying;
			const float Alpha = FlyElapsed / FMath::Max(T_Fly, 0.001f);
			WT.Scale       = FVector2D(TargetShrinkScale);
			WT.Translation = FMath::Lerp(FVector2D::ZeroVector, FlyDelta, Alpha);

			// 首帧捕获飞行起点（layout 绝对中心）
			if (!bFlyStartCaptured)
			{
				FlyAbsStart = MyGeometry.LocalToAbsolute(MyGeometry.GetLocalSize() * 0.5f);
				bFlyStartCaptured = true;
				UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] FlyAbsStart 捕获 — (%.0f,%.0f)"), FlyAbsStart.X, FlyAbsStart.Y);
			}
			OnFlyProgress.Broadcast(FlyAbsStart, FlyAbsStart + WT.Translation, Alpha);
		}
		else
		{
			// Done
			UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] → Phase: Done — 广播 OnFlyComplete"));
			CurrentPhase = EWeaponFloatPhase::Idle;
			SetVisibility(ESlateVisibility::Collapsed);
			SetRenderTransform(FWidgetTransform());
			if (WeaponThumbnail) WeaponThumbnail->SetRenderOpacity(1.f);

			OnFlyComplete.Broadcast(CachedThumbnail);
			return;
		}
	}

	SetRenderTransform(WT);
}

// ─────────────────────────────────────────────────────────────────────────────
//  激活区：图像覆盖 or 全格点阵
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::BuildZonePanel(UCanvasPanel* GridPanel, UImage* ImgWidget,
                                         UTexture2D* ZoneTexture, const FRuneShape* Shape,
                                         int32 GW, int32 GH)
{
	if (ZoneTexture && ImgWidget)
	{
		ImgWidget->SetBrushFromTexture(ZoneTexture, true);
		ImgWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (GridPanel) GridPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (ImgWidget) ImgWidget->SetVisibility(ESlateVisibility::Collapsed);
	if (!GridPanel) return;

	GridPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	GridPanel->ClearChildren();

	TSet<FIntPoint> ActiveCells;
	if (Shape) ActiveCells.Append(Shape->Cells);

	const int32 MaxDim  = FMath::Max(FMath::Max(GW, GH), 1);
	const float Step    = ZoneGridSize / MaxDim;
	const float DotSize = FMath::Max(Step - 2.f, 1.f);
	const float OffX    = (ZoneGridSize - (GW * Step - (Step - DotSize))) * 0.5f;
	const float OffY    = (ZoneGridSize - (GH * Step - (Step - DotSize))) * 0.5f;
	const float Radius  = FMath::Max(DotSize * 0.25f, 1.f);

	for (int32 Row = 0; Row < GH; Row++)
	{
		for (int32 Col = 0; Col < GW; Col++)
		{
			const bool bActive = ActiveCells.Contains(FIntPoint(Col, Row));

			UImage* Dot = NewObject<UImage>(this);
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.TintColor = FSlateColor(FLinearColor::White);
			Brush.OutlineSettings.CornerRadii  = FVector4(Radius, Radius, Radius, Radius);
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Dot->SetBrush(Brush);
			Dot->SetColorAndOpacity(bActive ? WeaponZoneColors::Active : WeaponZoneColors::Inactive);

			UCanvasPanelSlot* DotSlot = GridPanel->AddChildToCanvas(Dot);
			DotSlot->SetPosition(FVector2D(OffX + Col * Step, OffY + Row * Step));
			DotSlot->SetSize(FVector2D(DotSize, DotSize));
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  初始符文列表
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::BuildRuneList(const TArray<TObjectPtr<URuneDataAsset>>& Runes)
{
	if (!RuneListBox) return;
	RuneListBox->ClearChildren();

	for (const TObjectPtr<URuneDataAsset>& RuneDA : Runes)
	{
		if (!RuneDA) continue;
		const FRuneConfig& Cfg = RuneDA->RuneInfo.RuneConfig;

		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

		USizeBox* IconBox = NewObject<USizeBox>(this);
		IconBox->SetWidthOverride(40.f);
		IconBox->SetHeightOverride(40.f);
		UImage* Icon = NewObject<UImage>(this);
		if (Cfg.RuneIcon)
			Icon->SetBrushFromTexture(Cfg.RuneIcon, true);
		else
			Icon->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
		IconBox->AddChild(Icon);

		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UVerticalBox* TextCol = NewObject<UVerticalBox>(this);
		UTextBlock* NameTB = NewObject<UTextBlock>(this);
		NameTB->SetText(FText::FromName(Cfg.RuneName));
		TextCol->AddChildToVerticalBox(NameTB);

		if (!Cfg.RuneDescription.IsEmpty())
		{
			UTextBlock* DescTB = NewObject<UTextBlock>(this);
			DescTB->SetText(Cfg.RuneDescription);
			TextCol->AddChildToVerticalBox(DescTB);
		}

		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextCol);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBoxSlot* RowSlot = RuneListBox->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
}
