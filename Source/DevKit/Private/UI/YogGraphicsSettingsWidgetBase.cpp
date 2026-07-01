#include "UI/YogGraphicsSettingsWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputModeTypes.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"
#include "System/YogPerformanceSettingsLibrary.h"

namespace
{
UTextBlock* MakeSettingsText(UWidgetTree* Tree, const FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = Tree ? Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name) : nullptr;
	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.90f, 0.92f, 1.f)));
	return TextBlock;
}

UButton* MakeSettingsButton(UWidgetTree* Tree, const FName Name, const FText& Label)
{
	UButton* Button = Tree ? Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name) : nullptr;
	if (!Button)
	{
		return nullptr;
	}

	Button->SetContent(MakeSettingsText(Tree, FName(*(Name.ToString() + TEXT("_Label"))), Label, 18));
	return Button;
}

void AddRow(UVerticalBox* Root, UWidget* Widget, const FMargin& Padding = FMargin(0.f, 0.f, 0.f, 10.f))
{
	if (Root && Widget)
	{
		if (UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Widget))
		{
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
	}
}

void AddInline(UHorizontalBox* Row, UWidget* Widget)
{
	if (Row && Widget)
	{
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Widget))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
}

int32 ClampQuality(int32 Quality)
{
	return FMath::Clamp(Quality, 0, 3);
}

int32 RoundQuality(float Quality)
{
	return ClampQuality(FMath::RoundToInt(Quality));
}

FText QualityDisplayName(int32 Quality)
{
	switch (ClampQuality(Quality))
	{
	case 3:
		return NSLOCTEXT("DevKitGraphicsSettings", "QualityEpic", "Epic");
	case 2:
		return NSLOCTEXT("DevKitGraphicsSettings", "QualityHigh", "High");
	case 1:
		return NSLOCTEXT("DevKitGraphicsSettings", "QualityMid", "Mid");
	case 0:
	default:
		return NSLOCTEXT("DevKitGraphicsSettings", "QualityLow", "Low");
	}
}

int32 LightInfoCountForMaterialLightQuality(int32 Quality)
{
	switch (ClampQuality(Quality))
	{
	case 0:
		return 0;
	case 1:
		return 1;
	case 2:
		return 2;
	case 3:
	default:
		return 4;
	}
}

void ConfigureQualitySlider(USlider* Slider)
{
	if (!Slider)
	{
		return;
	}

	Slider->SetMinValue(0.f);
	Slider->SetMaxValue(3.f);
	Slider->SetStepSize(1.f / 3.f);
}

FText FormatQualityText(const FText& Label, int32 Quality)
{
	return FText::Format(
		NSLOCTEXT("DevKitGraphicsSettings", "QualityLabel", "{0}: {1} ({2})"),
		Label,
		QualityDisplayName(Quality),
		FText::AsNumber(ClampQuality(Quality)));
}
}

TArray<FName> UYogGraphicsSettingsWidgetBase::GetRequiredDesignerWidgetNames()
{
	return {
		TEXT("BtnTierEpic"),
		TEXT("BtnTierHigh"),
		TEXT("BtnTierMid"),
		TEXT("BtnTierLow"),
		TEXT("BtnFrame30"),
		TEXT("BtnFrame40"),
		TEXT("BtnFrame60"),
		TEXT("BtnFrame120"),
		TEXT("BtnFrameUnlimited"),
		TEXT("BtnApplyCustom"),
		TEXT("BtnBack"),
		TEXT("ResolutionScaleSlider"),
		TEXT("ModelQualitySlider"),
		TEXT("ShadowQualitySlider"),
		TEXT("ReflectionQualitySlider"),
		TEXT("TextureQualitySlider"),
		TEXT("MaterialQualitySlider"),
		TEXT("DynamicLightQualitySlider"),
		TEXT("MaterialLightQualitySlider"),
		TEXT("LumenLiteCheckBox"),
		TEXT("BatchProxiesCheckBox"),
		TEXT("CurrentProfileText"),
		TEXT("ResolutionScaleText"),
		TEXT("FrameRateText"),
		TEXT("ModelQualityText"),
		TEXT("ShadowQualityText"),
		TEXT("ReflectionQualityText"),
		TEXT("TextureQualityText"),
		TEXT("MaterialQualityText"),
		TEXT("DynamicLightQualityText"),
		TEXT("MaterialLightQualityText")
	};
}

FName UYogGraphicsSettingsWidgetBase::GetDefaultFocusWidgetName()
{
	return TEXT("BtnApplyCustom");
}

void UYogGraphicsSettingsWidgetBase::RefreshFromSavedSettings()
{
	SetPendingSettings(UYogPerformanceSettingsLibrary::GetSavedGraphicsSettings(this));
}

void UYogGraphicsSettingsWidgetBase::SetPendingSettings(const FYogGraphicsSettings& InSettings)
{
	PendingSettings = InSettings;
	PendingSettings.ResolutionScalePercent = FMath::Clamp(PendingSettings.ResolutionScalePercent, 25.f, 100.f);
	PendingSettings.FrameRateLimit = FMath::Clamp(PendingSettings.FrameRateLimit, 0, 240);
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::ApplyProfile(EYogPerformanceProfile Profile)
{
	SetPendingSettings(UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForProfile(Profile));
	ApplyPendingSettings(true);
}

void UYogGraphicsSettingsWidgetBase::ApplyTargetTier(EYogPerformanceTargetTier TargetTier)
{
	SetPendingSettings(UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(TargetTier));
	ApplyPendingSettings(true);
}

bool UYogGraphicsSettingsWidgetBase::ApplyPendingSettings(bool bSaveToDisk)
{
	return UYogPerformanceSettingsLibrary::ApplyGraphicsSettings(this, PendingSettings, bSaveToDisk);
}

void UYogGraphicsSettingsWidgetBase::SetPendingResolutionScale(float ResolutionScalePercent)
{
	PendingSettings.ResolutionScalePercent = FMath::Clamp(ResolutionScalePercent, 25.f, 100.f);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingFrameRateLimit(int32 FrameRateLimit)
{
	PendingSettings.FrameRateLimit = FMath::Clamp(FrameRateLimit, 0, 240);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingLumenLite(bool bUseLumenLite)
{
	PendingSettings.bUseLumenLite = bUseLumenLite;
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingBatchProxies(bool bPreferBatchedGeometryProxies)
{
	PendingSettings.bPreferBatchedGeometryProxies = bPreferBatchedGeometryProxies;
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingModelQuality(int32 Quality)
{
	PendingSettings.ViewDistanceQuality = ClampQuality(Quality);
	PendingSettings.FoliageQuality = ClampQuality(Quality);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingShadowQuality(int32 Quality)
{
	PendingSettings.ShadowQuality = ClampQuality(Quality);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingReflectionQuality(int32 Quality)
{
	PendingSettings.ReflectionQuality = ClampQuality(Quality);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingTextureQuality(int32 Quality)
{
	PendingSettings.TextureQuality = ClampQuality(Quality);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingMaterialQuality(int32 Quality)
{
	const int32 ClampedQuality = ClampQuality(Quality);
	PendingSettings.ShadingQuality = ClampedQuality;
	PendingSettings.EffectsQuality = ClampedQuality;
	PendingSettings.MaterialQuality = ClampedQuality;
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingDynamicLightQuality(int32 Quality)
{
	PendingSettings.DynamicLightQuality = ClampQuality(Quality);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::SetPendingMaterialLightQuality(int32 Quality)
{
	PendingSettings.MaterialLightQuality = ClampQuality(Quality);
	PendingSettings.MaterialLightMaxLightInfoCount = LightInfoCountForMaterialLightQuality(Quality);
	MarkPendingAsCustom();
	SyncControlsFromPendingSettings();
}

void UYogGraphicsSettingsWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();
	BindControls();
	RefreshFromSavedSettings();
	SetIsFocusable(true);
}

void UYogGraphicsSettingsWidgetBase::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshFromSavedSettings();
	if (BtnApplyCustom)
	{
		BtnApplyCustom->SetKeyboardFocus();
	}
}

UWidget* UYogGraphicsSettingsWidgetBase::NativeGetDesiredFocusTarget() const
{
	return BtnApplyCustom ? BtnApplyCustom.Get() : BtnBack.Get();
}

TOptional<FUIInputConfig> UYogGraphicsSettingsWidgetBase::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UYogGraphicsSettingsWidgetBase::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsSettingsRoot"));
	WidgetTree->RootWidget = Root;

	AddRow(Root, MakeSettingsText(WidgetTree, TEXT("TitleText"), NSLOCTEXT("DevKitGraphicsSettings", "Title", "Graphics Settings"), 32));
	CurrentProfileText = MakeSettingsText(WidgetTree, TEXT("CurrentProfileText"), FText::GetEmpty(), 20);
	AddRow(Root, CurrentProfileText);

	UHorizontalBox* TargetRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TargetTierRow"));
	BtnTierEpic = MakeSettingsButton(WidgetTree, TEXT("BtnTierEpic"), NSLOCTEXT("DevKitGraphicsSettings", "TierEpic", "Epic"));
	BtnTierHigh = MakeSettingsButton(WidgetTree, TEXT("BtnTierHigh"), NSLOCTEXT("DevKitGraphicsSettings", "TierHigh", "High"));
	BtnTierMid = MakeSettingsButton(WidgetTree, TEXT("BtnTierMid"), NSLOCTEXT("DevKitGraphicsSettings", "TierMid", "Mid"));
	BtnTierLow = MakeSettingsButton(WidgetTree, TEXT("BtnTierLow"), NSLOCTEXT("DevKitGraphicsSettings", "TierLow", "Low"));
	AddInline(TargetRow, BtnTierEpic);
	AddInline(TargetRow, BtnTierHigh);
	AddInline(TargetRow, BtnTierMid);
	AddInline(TargetRow, BtnTierLow);
	AddRow(Root, TargetRow);

	ResolutionScaleText = MakeSettingsText(WidgetTree, TEXT("ResolutionScaleText"), FText::GetEmpty(), 18);
	AddRow(Root, ResolutionScaleText);
	ResolutionScaleSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ResolutionScaleSlider"));
	if (ResolutionScaleSlider)
	{
		ResolutionScaleSlider->SetMinValue(25.f);
		ResolutionScaleSlider->SetMaxValue(100.f);
		AddRow(Root, ResolutionScaleSlider);
	}

	FrameRateText = MakeSettingsText(WidgetTree, TEXT("FrameRateText"), FText::GetEmpty(), 18);
	AddRow(Root, FrameRateText);
	UHorizontalBox* FrameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FrameRow"));
	BtnFrame30 = MakeSettingsButton(WidgetTree, TEXT("BtnFrame30"), NSLOCTEXT("DevKitGraphicsSettings", "Fps30", "30"));
	BtnFrame40 = MakeSettingsButton(WidgetTree, TEXT("BtnFrame40"), NSLOCTEXT("DevKitGraphicsSettings", "Fps40", "40"));
	BtnFrame60 = MakeSettingsButton(WidgetTree, TEXT("BtnFrame60"), NSLOCTEXT("DevKitGraphicsSettings", "Fps60", "60"));
	BtnFrame120 = MakeSettingsButton(WidgetTree, TEXT("BtnFrame120"), NSLOCTEXT("DevKitGraphicsSettings", "Fps120", "120"));
	BtnFrameUnlimited = MakeSettingsButton(WidgetTree, TEXT("BtnFrameUnlimited"), NSLOCTEXT("DevKitGraphicsSettings", "FpsUnlimited", "Unlimited"));
	AddInline(FrameRow, BtnFrame30);
	AddInline(FrameRow, BtnFrame40);
	AddInline(FrameRow, BtnFrame60);
	AddInline(FrameRow, BtnFrame120);
	AddInline(FrameRow, BtnFrameUnlimited);
	AddRow(Root, FrameRow);

	LumenLiteCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("LumenLiteCheckBox"));
	BatchProxiesCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("BatchProxiesCheckBox"));
	AddRow(Root, LumenLiteCheckBox);
	AddRow(Root, BatchProxiesCheckBox);

	ModelQualityText = MakeSettingsText(WidgetTree, TEXT("ModelQualityText"), FText::GetEmpty(), 16);
	ModelQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ModelQualitySlider"));
	ConfigureQualitySlider(ModelQualitySlider);
	AddRow(Root, ModelQualityText);
	AddRow(Root, ModelQualitySlider);

	ShadowQualityText = MakeSettingsText(WidgetTree, TEXT("ShadowQualityText"), FText::GetEmpty(), 16);
	ShadowQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ShadowQualitySlider"));
	ConfigureQualitySlider(ShadowQualitySlider);
	AddRow(Root, ShadowQualityText);
	AddRow(Root, ShadowQualitySlider);

	ReflectionQualityText = MakeSettingsText(WidgetTree, TEXT("ReflectionQualityText"), FText::GetEmpty(), 16);
	ReflectionQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ReflectionQualitySlider"));
	ConfigureQualitySlider(ReflectionQualitySlider);
	AddRow(Root, ReflectionQualityText);
	AddRow(Root, ReflectionQualitySlider);

	TextureQualityText = MakeSettingsText(WidgetTree, TEXT("TextureQualityText"), FText::GetEmpty(), 16);
	TextureQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("TextureQualitySlider"));
	ConfigureQualitySlider(TextureQualitySlider);
	AddRow(Root, TextureQualityText);
	AddRow(Root, TextureQualitySlider);

	MaterialQualityText = MakeSettingsText(WidgetTree, TEXT("MaterialQualityText"), FText::GetEmpty(), 16);
	MaterialQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MaterialQualitySlider"));
	ConfigureQualitySlider(MaterialQualitySlider);
	AddRow(Root, MaterialQualityText);
	AddRow(Root, MaterialQualitySlider);

	DynamicLightQualityText = MakeSettingsText(WidgetTree, TEXT("DynamicLightQualityText"), FText::GetEmpty(), 16);
	DynamicLightQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("DynamicLightQualitySlider"));
	ConfigureQualitySlider(DynamicLightQualitySlider);
	AddRow(Root, DynamicLightQualityText);
	AddRow(Root, DynamicLightQualitySlider);

	MaterialLightQualityText = MakeSettingsText(WidgetTree, TEXT("MaterialLightQualityText"), FText::GetEmpty(), 16);
	MaterialLightQualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MaterialLightQualitySlider"));
	ConfigureQualitySlider(MaterialLightQualitySlider);
	AddRow(Root, MaterialLightQualityText);
	AddRow(Root, MaterialLightQualitySlider);

	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
	BtnApplyCustom = MakeSettingsButton(WidgetTree, TEXT("BtnApplyCustom"), NSLOCTEXT("DevKitGraphicsSettings", "ApplyCustom", "Apply Custom"));
	BtnBack = MakeSettingsButton(WidgetTree, TEXT("BtnBack"), NSLOCTEXT("DevKitGraphicsSettings", "Back", "Back"));
	AddInline(ActionRow, BtnApplyCustom);
	AddInline(ActionRow, BtnBack);
	AddRow(Root, ActionRow, FMargin(0.f));
}

void UYogGraphicsSettingsWidgetBase::BindControls()
{
	if (BtnTierEpic)
	{
		BtnTierEpic->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierEpicClicked);
		BtnTierEpic->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierEpicClicked);
	}
	if (BtnTierHigh)
	{
		BtnTierHigh->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierHighClicked);
		BtnTierHigh->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierHighClicked);
	}
	if (BtnTierMid)
	{
		BtnTierMid->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierMidClicked);
		BtnTierMid->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierMidClicked);
	}
	if (BtnTierLow)
	{
		BtnTierLow->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierLowClicked);
		BtnTierLow->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTierLowClicked);
	}
	if (BtnFrame30)
	{
		BtnFrame30->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame30Clicked);
		BtnFrame30->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame30Clicked);
	}
	if (BtnFrame40)
	{
		BtnFrame40->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame40Clicked);
		BtnFrame40->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame40Clicked);
	}
	if (BtnFrame60)
	{
		BtnFrame60->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame60Clicked);
		BtnFrame60->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame60Clicked);
	}
	if (BtnFrame120)
	{
		BtnFrame120->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame120Clicked);
		BtnFrame120->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrame120Clicked);
	}
	if (BtnFrameUnlimited)
	{
		BtnFrameUnlimited->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrameUnlimitedClicked);
		BtnFrameUnlimited->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleFrameUnlimitedClicked);
	}
	if (BtnApplyCustom)
	{
		BtnApplyCustom->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleApplyCustomClicked);
		BtnApplyCustom->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleApplyCustomClicked);
	}
	if (BtnBack)
	{
		BtnBack->OnClicked.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleBackClicked);
		BtnBack->OnClicked.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleBackClicked);
	}
	if (ResolutionScaleSlider)
	{
		ResolutionScaleSlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleResolutionScaleChanged);
		ResolutionScaleSlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleResolutionScaleChanged);
	}
	if (LumenLiteCheckBox)
	{
		LumenLiteCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleLumenLiteChanged);
		LumenLiteCheckBox->OnCheckStateChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleLumenLiteChanged);
	}
	if (BatchProxiesCheckBox)
	{
		BatchProxiesCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleBatchProxiesChanged);
		BatchProxiesCheckBox->OnCheckStateChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleBatchProxiesChanged);
	}
	if (ModelQualitySlider)
	{
		ModelQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleModelQualityChanged);
		ModelQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleModelQualityChanged);
	}
	if (ShadowQualitySlider)
	{
		ShadowQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleShadowQualityChanged);
		ShadowQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleShadowQualityChanged);
	}
	if (ReflectionQualitySlider)
	{
		ReflectionQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleReflectionQualityChanged);
		ReflectionQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleReflectionQualityChanged);
	}
	if (TextureQualitySlider)
	{
		TextureQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTextureQualityChanged);
		TextureQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleTextureQualityChanged);
	}
	if (MaterialQualitySlider)
	{
		MaterialQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleMaterialQualityChanged);
		MaterialQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleMaterialQualityChanged);
	}
	if (DynamicLightQualitySlider)
	{
		DynamicLightQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleDynamicLightQualityChanged);
		DynamicLightQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleDynamicLightQualityChanged);
	}
	if (MaterialLightQualitySlider)
	{
		MaterialLightQualitySlider->OnValueChanged.RemoveDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleMaterialLightQualityChanged);
		MaterialLightQualitySlider->OnValueChanged.AddDynamic(this, &UYogGraphicsSettingsWidgetBase::HandleMaterialLightQualityChanged);
	}
}

void UYogGraphicsSettingsWidgetBase::SyncControlsFromPendingSettings()
{
	TGuardValue<bool> Guard(bBindingControls, true);

	if (CurrentProfileText)
	{
		const FText CurrentName = PendingSettings.SelectedTargetTier != EYogPerformanceTargetTier::Custom
			? UYogPerformanceSettingsLibrary::GetPerformanceTargetTierDisplayName(PendingSettings.SelectedTargetTier)
			: UYogPerformanceSettingsLibrary::GetPerformanceProfileDisplayName(PendingSettings.PerformanceProfile);

		CurrentProfileText->SetText(FText::Format(
			NSLOCTEXT("DevKitGraphicsSettings", "CurrentProfile", "Current: {0}"),
			CurrentName));
	}
	if (ResolutionScaleText)
	{
		ResolutionScaleText->SetText(FText::Format(
			NSLOCTEXT("DevKitGraphicsSettings", "ResolutionScale", "Resolution Scale: {0}%"),
			FText::AsNumber(FMath::RoundToInt(PendingSettings.ResolutionScalePercent))));
	}
	if (FrameRateText)
	{
		FrameRateText->SetText(PendingSettings.FrameRateLimit > 0
			? FText::Format(NSLOCTEXT("DevKitGraphicsSettings", "FrameLimit", "Frame Limit: {0}"), FText::AsNumber(PendingSettings.FrameRateLimit))
			: NSLOCTEXT("DevKitGraphicsSettings", "FrameLimitUnlimited", "Frame Limit: Unlimited"));
	}
	if (ResolutionScaleSlider)
	{
		ResolutionScaleSlider->SetValue(PendingSettings.ResolutionScalePercent);
	}
	if (LumenLiteCheckBox)
	{
		LumenLiteCheckBox->SetIsChecked(PendingSettings.bUseLumenLite);
	}
	if (BatchProxiesCheckBox)
	{
		BatchProxiesCheckBox->SetIsChecked(PendingSettings.bPreferBatchedGeometryProxies);
	}
	if (ModelQualityText)
	{
		ModelQualityText->SetText(FormatQualityText(NSLOCTEXT("DevKitGraphicsSettings", "ModelQuality", "Model Quality"), PendingSettings.ViewDistanceQuality));
	}
	if (ShadowQualityText)
	{
		ShadowQualityText->SetText(FormatQualityText(NSLOCTEXT("DevKitGraphicsSettings", "ShadowQuality", "Shadow Quality"), PendingSettings.ShadowQuality));
	}
	if (ReflectionQualityText)
	{
		ReflectionQualityText->SetText(FormatQualityText(NSLOCTEXT("DevKitGraphicsSettings", "ReflectionQuality", "Reflection Quality"), PendingSettings.ReflectionQuality));
	}
	if (TextureQualityText)
	{
		TextureQualityText->SetText(FormatQualityText(NSLOCTEXT("DevKitGraphicsSettings", "TextureQuality", "Texture Quality"), PendingSettings.TextureQuality));
	}
	if (MaterialQualityText)
	{
		MaterialQualityText->SetText(FormatQualityText(NSLOCTEXT("DevKitGraphicsSettings", "MaterialQuality", "Material Quality"), PendingSettings.MaterialQuality));
	}
	if (DynamicLightQualityText)
	{
		DynamicLightQualityText->SetText(FormatQualityText(NSLOCTEXT("DevKitGraphicsSettings", "DynamicLightQuality", "Dynamic Light Quality"), PendingSettings.DynamicLightQuality));
	}
	if (MaterialLightQualityText)
	{
		MaterialLightQualityText->SetText(FText::Format(
			NSLOCTEXT("DevKitGraphicsSettings", "MaterialLightQuality", "Material Light Quality: {0} ({1} lights)"),
			FText::AsNumber(ClampQuality(PendingSettings.MaterialLightQuality)),
			FText::AsNumber(FMath::Clamp(PendingSettings.MaterialLightMaxLightInfoCount, 0, 4))));
	}
	if (ModelQualitySlider)
	{
		ModelQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.ViewDistanceQuality)));
	}
	if (ShadowQualitySlider)
	{
		ShadowQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.ShadowQuality)));
	}
	if (ReflectionQualitySlider)
	{
		ReflectionQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.ReflectionQuality)));
	}
	if (TextureQualitySlider)
	{
		TextureQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.TextureQuality)));
	}
	if (MaterialQualitySlider)
	{
		MaterialQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.MaterialQuality)));
	}
	if (DynamicLightQualitySlider)
	{
		DynamicLightQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.DynamicLightQuality)));
	}
	if (MaterialLightQualitySlider)
	{
		MaterialLightQualitySlider->SetValue(static_cast<float>(ClampQuality(PendingSettings.MaterialLightQuality)));
	}
}

void UYogGraphicsSettingsWidgetBase::MarkPendingAsCustom()
{
	PendingSettings = UYogPerformanceSettingsLibrary::MakeCustomGraphicsSettings(
		PendingSettings,
		PendingSettings.ResolutionScalePercent,
		PendingSettings.FrameRateLimit,
		PendingSettings.bUseLumenLite,
		PendingSettings.bPreferBatchedGeometryProxies);
}

void UYogGraphicsSettingsWidgetBase::HandleTierEpicClicked()
{
	ApplyTargetTier(EYogPerformanceTargetTier::Epic);
}

void UYogGraphicsSettingsWidgetBase::HandleTierHighClicked()
{
	ApplyTargetTier(EYogPerformanceTargetTier::High);
}

void UYogGraphicsSettingsWidgetBase::HandleTierMidClicked()
{
	ApplyTargetTier(EYogPerformanceTargetTier::Mid);
}

void UYogGraphicsSettingsWidgetBase::HandleTierLowClicked()
{
	ApplyTargetTier(EYogPerformanceTargetTier::Low);
}

void UYogGraphicsSettingsWidgetBase::HandleFrame30Clicked()
{
	SetPendingFrameRateLimit(30);
}

void UYogGraphicsSettingsWidgetBase::HandleFrame40Clicked()
{
	SetPendingFrameRateLimit(40);
}

void UYogGraphicsSettingsWidgetBase::HandleFrame60Clicked()
{
	SetPendingFrameRateLimit(60);
}

void UYogGraphicsSettingsWidgetBase::HandleFrame120Clicked()
{
	SetPendingFrameRateLimit(120);
}

void UYogGraphicsSettingsWidgetBase::HandleFrameUnlimitedClicked()
{
	SetPendingFrameRateLimit(0);
}

void UYogGraphicsSettingsWidgetBase::HandleApplyCustomClicked()
{
	ApplyPendingSettings(true);
}

void UYogGraphicsSettingsWidgetBase::HandleBackClicked()
{
	OnBackRequested.Broadcast();
	DeactivateWidget();
}

void UYogGraphicsSettingsWidgetBase::HandleResolutionScaleChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingResolutionScale(NewValue);
	}
}

void UYogGraphicsSettingsWidgetBase::HandleLumenLiteChanged(bool bIsChecked)
{
	if (!bBindingControls)
	{
		SetPendingLumenLite(bIsChecked);
	}
}

void UYogGraphicsSettingsWidgetBase::HandleBatchProxiesChanged(bool bIsChecked)
{
	if (!bBindingControls)
	{
		SetPendingBatchProxies(bIsChecked);
	}
}

void UYogGraphicsSettingsWidgetBase::HandleModelQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingModelQuality(RoundQuality(NewValue));
	}
}

void UYogGraphicsSettingsWidgetBase::HandleShadowQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingShadowQuality(RoundQuality(NewValue));
	}
}

void UYogGraphicsSettingsWidgetBase::HandleReflectionQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingReflectionQuality(RoundQuality(NewValue));
	}
}

void UYogGraphicsSettingsWidgetBase::HandleTextureQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingTextureQuality(RoundQuality(NewValue));
	}
}

void UYogGraphicsSettingsWidgetBase::HandleMaterialQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingMaterialQuality(RoundQuality(NewValue));
	}
}

void UYogGraphicsSettingsWidgetBase::HandleDynamicLightQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingDynamicLightQuality(RoundQuality(NewValue));
	}
}

void UYogGraphicsSettingsWidgetBase::HandleMaterialLightQualityChanged(float NewValue)
{
	if (!bBindingControls)
	{
		SetPendingMaterialLightQuality(RoundQuality(NewValue));
	}
}
