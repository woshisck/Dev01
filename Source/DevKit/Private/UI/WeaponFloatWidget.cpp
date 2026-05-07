#include "UI/WeaponFloatWidget.h"
#include "UI/YogCommonRichTextBlock.h"
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

	SetRenderTransform(FWidgetTransform());
	SetRenderOpacity(1.f);
	// 折叠动画把 InfoContainer 透明度降到 0、Visibility=Collapsed，必须两个都还原
	bCollapsing   = false;
	CollapseTimer = 0.f;
	if (InfoContainer)
	{
		InfoContainer->SetRenderOpacity(1.f);
		InfoContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

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
		if (bHas)
		{
			WeaponDescText->SetText(Info->WeaponDescription);
			WeaponDescText->SetAutoWrapText(true);
		}
	}

	// ── 子描述 ─────────────────────────────────
	if (WeaponSubDescText)
	{
		const bool bHas = Info && !Info->WeaponSubDescription.IsEmpty();
		WeaponSubDescText->SetVisibility(bHas ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bHas)
		{
			WeaponSubDescText->SetText(Info->WeaponSubDescription);
			WeaponSubDescText->SetAutoWrapText(true);
		}
	}

	// ── 激活区 ─────────────────────────────────
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

	BuildZonePanel(ZoneGrid1, Zone1Image, GetZoneImg(0), nullptr, 5, 5);
	BuildZonePanel(ZoneGrid2, Zone2Image, GetZoneImg(1), nullptr, 5, 5);
	BuildZonePanel(ZoneGrid3, Zone3Image, GetZoneImg(2), nullptr, 5, 5);

	// ── 初始符文列表 ───────────────────────────
	BuildRuneList(Def->InitialRunes);
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

// ─────────────────────────────────────────────────────────────────────────────
//  折叠动画
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::StartCollapse(float InDuration)
{
	CollapseDuration = FMath::Max(InDuration, 0.05f);
	CollapseTimer    = 0.f;
	bCollapsing      = true;
}

void UWeaponFloatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bCollapsing) return;

	CollapseTimer += InDeltaTime;
	const float Alpha = FMath::Clamp(CollapseTimer / CollapseDuration, 0.f, 1.f);

	// InfoContainer 淡出（点阵 + 符文列表），WeaponThumbnail 保持不变
	if (InfoContainer)
		InfoContainer->SetRenderOpacity(1.f - Alpha);

	if (Alpha >= 1.f)
	{
		bCollapsing = false;
		if (InfoContainer)
			InfoContainer->SetVisibility(ESlateVisibility::Collapsed);

		// 获取缩略图的屏幕绝对中心坐标作为飞行起点
		FVector2D ThumbnailCenter = FVector2D::ZeroVector;
		if (WeaponThumbnail)
		{
			const FGeometry& G = WeaponThumbnail->GetCachedGeometry();
			ThumbnailCenter = G.LocalToAbsolute(G.GetLocalSize() * 0.5f);
		}

		if (OnCollapseComplete.IsBound())
			OnCollapseComplete.Execute(ThumbnailCenter);
	}
}
