#include "UI/WeaponFloatWidget.h"
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

namespace WeaponZoneColors
{
	static const FLinearColor Active  (0.20f, 0.80f, 1.00f, 1.0f);
	static const FLinearColor Inactive(0.15f, 0.15f, 0.18f, 0.7f);
}

void UWeaponFloatWidget::SetWeaponDefinition(const UWeaponDefinition* Def)
{
	if (!Def) return;

	const UWeaponInfoDA* Info = Def->WeaponInfo;

	// ── 缩略图 ───────────────────────────────────────────────────
	if (WeaponThumbnail)
	{
		if (Info && Info->Thumbnail)
		{
			WeaponThumbnail->SetBrushFromTexture(Info->Thumbnail, true);
			WeaponThumbnail->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			WeaponThumbnail->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ── 名称 ─────────────────────────────────────────────────────
	if (WeaponNameText)
		WeaponNameText->SetText(Info ? Info->WeaponName : FText::GetEmpty());

	// ── 描述（可选） ─────────────────────────────────────────────
	if (WeaponDescText)
	{
		const bool bHasDesc = Info && !Info->WeaponDescription.IsEmpty();
		WeaponDescText->SetVisibility(bHasDesc
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
		if (bHasDesc) WeaponDescText->SetText(Info->WeaponDescription);
	}

	// ── 子描述（可选） ───────────────────────────────────────────
	if (WeaponSubDescText)
	{
		const bool bHasSub = Info && !Info->WeaponSubDescription.IsEmpty();
		WeaponSubDescText->SetVisibility(bHasSub
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
		if (bHasSub) WeaponSubDescText->SetText(Info->WeaponSubDescription);
	}

	// ── 激活区点阵 / 图像 ─────────────────────────────────────────
	const FActivationZoneConfig& ZoneCfg = Def->BackpackConfig.ActivationZoneConfig;
	const int32 GW = Def->BackpackConfig.GridWidth;
	const int32 GH = Def->BackpackConfig.GridHeight;

	auto GetShape = [&](int32 Idx) -> const FRuneShape*
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

	// ── 初始符文列表 ─────────────────────────────────────────────
	BuildRuneList(Def->InitialRunes);
}

// ──────────────────────────────────────────────────────────────────────────────
//  激活区：图像覆盖 or 全格点阵（高亮激活格）
// ──────────────────────────────────────────────────────────────────────────────

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

	constexpr float DotSize = 8.f;
	constexpr float Gap     = 2.f;
	constexpr float Step    = DotSize + Gap;

	for (int32 Row = 0; Row < GH; Row++)
	{
		for (int32 Col = 0; Col < GW; Col++)
		{
			const bool bActive = ActiveCells.Contains(FIntPoint(Col, Row));

			UImage* Dot = NewObject<UImage>(this);
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.TintColor = FSlateColor(FLinearColor::White);
			Brush.OutlineSettings.CornerRadii  = FVector4(2.f, 2.f, 2.f, 2.f);
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Dot->SetBrush(Brush);
			Dot->SetColorAndOpacity(bActive ? WeaponZoneColors::Active : WeaponZoneColors::Inactive);

			UCanvasPanelSlot* DotSlot = GridPanel->AddChildToCanvas(Dot);
			DotSlot->SetPosition(FVector2D(Col * Step, Row * Step));
			DotSlot->SetSize(FVector2D(DotSize, DotSize));
		}
	}
}

// ──────────────────────────────────────────────────────────────────────────────
//  初始符文列表：每条 = [Icon 40×40] + [Name / Desc]
// ──────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::BuildRuneList(const TArray<TObjectPtr<URuneDataAsset>>& Runes)
{
	if (!RuneListBox) return;
	RuneListBox->ClearChildren();

	for (const TObjectPtr<URuneDataAsset>& RuneDA : Runes)
	{
		if (!RuneDA) continue;
		const FRuneConfig& Cfg = RuneDA->RuneInfo.RuneConfig;

		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

		// 图标（固定 40×40）
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

		// 名称 + 描述
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
