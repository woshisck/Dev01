#pragma once

#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DevKitArtToolUI
{
	inline TSharedRef<SWidget> MakeHeader(const FText& Title, const FText& Description)
	{
		return SNew(SBorder)
			.Padding(FMargin(12.f, 10.f))
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			.BorderBackgroundColor(FLinearColor(0.055f, 0.075f, 0.095f, 1.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(Title)
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 5.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(Description)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
	}

	inline TSharedRef<SWidget> MakeSectionHeader(int32 Step, const FText& Title, const FText& Description = FText::GetEmpty())
	{
		return SNew(SBorder)
			.Padding(FMargin(10.f, 7.f))
			.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Header")))
			.BorderBackgroundColor(FLinearColor(0.10f, 0.20f, 0.27f, 1.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				.Padding(0.f, 0.f, 10.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(Step))
					.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					.ColorAndOpacity(FLinearColor(0.25f, 0.75f, 1.f))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(Title)
						.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(Description)
						.AutoWrapText(true)
						.Visibility(Description.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			];
	}

	inline TSharedRef<SWidget> MakeStatus(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Color = FSlateColor::UseForeground())
	{
		return SNew(SBorder)
			.Padding(FMargin(10.f, 8.f))
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			[
				SNew(STextBlock)
				.Text(Text)
				.ColorAndOpacity(Color)
				.AutoWrapText(true)
			];
	}
}
