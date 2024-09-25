// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "UI/Styling/GSCEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)
#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( Style, RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#define JPG_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush(Style->RootToContentDir( RelativePath, TEXT(".jpg")), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FGSCEditorStyle::StyleInstance = nullptr;

void FGSCEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FGSCEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FGSCEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("GASCompanionEditorStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon180x100(180.0f, 100.0f);
const FVector2D Icon256x256(256.0f, 256.0f);

TSharedRef<FSlateStyleSet> FGSCEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = CreateStyle("GASCompanionEditorStyle");
	Style->Set("GASCompanionEditor.OpenPluginWindow", new IMAGE_BRUSH(TEXT("GSC40x40"), Icon40x40));
	Style->Set("GASCompanionEditor.OpenPluginWindow.Small", new IMAGE_BRUSH(TEXT("GSC20x20"), Icon20x20));

	const FTextBlockStyle NormalText = FTextBlockStyle()
	   .SetFont(DEFAULT_FONT("Regular", FCoreStyle::RegularTextSize))
	   .SetColorAndOpacity(FSlateColor::UseForeground())
	   .SetShadowOffset(FVector2D::ZeroVector)
	   .SetShadowColorAndOpacity(FLinearColor::Black)
	   .SetHighlightColor( FLinearColor( 0.02f, 0.3f, 0.0f ) )
	   .SetHighlightShape(BOX_BRUSH(Style, "Common/TextBlockHighlightShape", FMargin(3.f/8.f)));

	Style->Set(
		"GASCompanionEditor.h1",
		FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("BoldCondensed", 28))
			// .SetShadowOffset(FVector2D(1,1))
			.SetShadowColorAndOpacity(FLinearColor(0,0,0,0.9f))
	);

	Style->Set(
		"GASCompanionEditor.h2",
		FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("BoldCondensed", 18))
			// .SetShadowOffset(FVector2D(1,1))
			.SetShadowColorAndOpacity(FLinearColor(0,0,0,0.9f))
	);

	Style->Set("Icons.Discord", new IMAGE_BRUSH("icons/discord_icon_16x", Icon16x16));

	// Common text styles
	{
		constexpr FLinearColor Badass = FLinearColor(0.729412f, 0.854902f, 0.333333f, 1.f);
		constexpr FLinearColor Tomato = FLinearColor(0.854902, 0.168741, 0.160237, 1.f);

		Style->Set(
			"Text.Badass",
			FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 10))
			.SetColorAndOpacity(FSlateColor(Badass))
		);

		Style->Set(
			"Text.Tomato",
			FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 10))
			.SetColorAndOpacity(FSlateColor(Tomato))
		);

		Style->Set(
			"Text.Bold",
			FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 11))
		);
	}

	// Common styles for launch pad actions
	{

		constexpr FLinearColor RoyalBlue = FLinearColor(0.254152f, 0.412543f, 0.887923f, 1.f);
		const FTextBlockStyle InheritedFromNativeTextStyle = FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 10))
			.SetColorAndOpacity(FSlateColor(RoyalBlue));

		// Go to native class hyperlink
		const FButtonStyle EditNativeHyperlinkButton = FButtonStyle()
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f)))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f)));

		const FHyperlinkStyle EditNativeHyperlinkStyle = FHyperlinkStyle()
			.SetUnderlineStyle(EditNativeHyperlinkButton)
			.SetTextStyle(InheritedFromNativeTextStyle)
			.SetPadding(FMargin(0.0f));
	}

	// AttributeSet Wizard styles
	{
		Style->Set("NewClassDialog.SelectedParentClassLabel", FTextBlockStyle(NormalText)
		  .SetFont(DEFAULT_FONT("Regular", 12))
		  .SetShadowOffset(FVector2D(0, 1))
		  .SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f))
		);

		Style->Set("NewClassDialog.ErrorLabelFont", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 10))
		);

		Style->Set("NewClassDialog.PageTitle", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("BoldCondensed", 28))
		    .SetShadowOffset(FVector2D(1, 1))
		    .SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f))
		);

		const FLinearColor AccentRed = FLinearColor::FromSRGBColor(FColor::FromHex("#FF4040FF"));
		Style->Set("NewClassDialog.ErrorLabelBorder", new FSlateColorBrush(AccentRed));
	}
	return Style;
}

TSharedRef<FSlateStyleSet> FGSCEditorStyle::CreateStyle(const FName StyleSetName)
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(StyleSetName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("GASCompanion")->GetBaseDir() / TEXT("Resources"));
	return Style;
}

void FGSCEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FGSCEditorStyle::Get()
{
	return *StyleInstance;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
#undef JPG_IMAGE_BRUSH
