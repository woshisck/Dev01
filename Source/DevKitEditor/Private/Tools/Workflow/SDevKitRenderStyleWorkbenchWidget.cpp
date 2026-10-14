#include "Tools/Workflow/SDevKitRenderStyleWorkbenchWidget.h"
#include "Tools/Workflow/DevKitWorkbenchUI.h"

#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    TSharedRef<SWidget> Heading(const TCHAR* Text)
    {
        return SNew(STextBlock).Text(FText::FromString(Text))
            .Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall"))).ColorAndOpacity(FStyleColors::AccentBlue);
    }
    TSharedRef<SWidget> Description(const TCHAR* Text)
    {
        return SNew(STextBlock).Text(FText::FromString(Text)).AutoWrapText(true)
            .ColorAndOpacity(FSlateColor::UseSubduedForeground());
    }
    const FString NativePresetName = TEXT("关闭场景分层对照");
    const FString LayerPresetName = TEXT("二分光影默认");
    float Safe(float Value, float Min, float Max, float Fallback)
    {
        return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Fallback;
    }

}

// Edit only the new top-level bool in the saved first profile. Preserve all
// other fields verbatim, including compatibility data and unknown future fields.
static bool SetMetalProfileField(FString& Profile, const FString& FieldName, const FString& Value)
{
    const FString Flag = FieldName + TEXT("=") + Value;
    if (!Profile.StartsWith(TEXT("(")) || !Profile.EndsWith(TEXT(")"))) return false;
    int32 Depth = 0, FieldStart = 1, FlagStart = INDEX_NONE, FlagEnd = INDEX_NONE;
    bool bQuoted = false;
    for (int32 Index = 0; Index < Profile.Len(); ++Index)
    {
        const TCHAR Character = Profile[Index];
        if (Character == TEXT('"'))
        {
            int32 EscapeCount = 0;
            for (int32 Previous = Index - 1; Previous >= 0 && Profile[Previous] == TEXT('\\'); --Previous) ++EscapeCount;
            if ((EscapeCount % 2) == 0) bQuoted = !bQuoted;
        }
        if (bQuoted) continue;
        if (Character == TEXT('(')) ++Depth;
        if (Character == TEXT(')'))
        {
            --Depth;
            if (Depth < 0 || (Depth == 0 && Index != Profile.Len() - 1)) return false;
        }
        if ((Character == TEXT(',') && Depth == 1) || (Character == TEXT(')') && Depth == 0))
        {
            FString Name, ExistingValue;
            if (Profile.Mid(FieldStart, Index - FieldStart).Split(TEXT("="), &Name, &ExistingValue)
                && Name.TrimStartAndEnd().Equals(FieldName, ESearchCase::IgnoreCase))
            {
                if (FlagStart != INDEX_NONE) return false;
                FlagStart = FieldStart;
                FlagEnd = Index;
            }
            FieldStart = Index + 1;
        }
    }
    // Validate the entire record before touching even an early matching field.
    if (bQuoted || Depth != 0) return false;
    if (FlagStart != INDEX_NONE)
        Profile = Profile.Left(FlagStart) + Flag + Profile.Mid(FlagEnd);
    else
        Profile = Profile.LeftChop(1) + (Profile.Mid(1, Profile.Len() - 2).TrimStartAndEnd().IsEmpty() ? TEXT("") : TEXT(",")) + Flag + TEXT(")");
    return true;
}

bool SDevKitRenderStyleWorkbenchWidget::SetSavedDefaultNPRMetal(TArray<FString>& Profiles, bool bEnabled)
{
    if(Profiles.IsEmpty()) Profiles.Add(TEXT("(ProfileName=\"Default\")"));
    return SetMetalProfileField(Profiles[0],TEXT("bEnableNPRMetal"),bEnabled?TEXT("True"):TEXT("False"));
}

FReply SDevKitRenderStyleWorkbenchWidget::SaveMetalArtDefaults()
{
    if(FDevKitCloudLightingFinishTrial::IsActive()) return FReply::Handled();
    auto* S=GetMutableDefault<UStylizedLightingSettings>();
    const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()/TEXT("DefaultGame.ini"));
    const FString Section=S->GetClass()->GetPathName(), Branch=S->GetClass()->ClassConfigName.ToString();
    FConfigFile Existing;
    if(!Existing.Combine(Path)){Status=TEXT("配置读取失败，未保存。");return FReply::Handled();}
    TArray<FString> Profiles;
    Existing.GetArray(*Section,TEXT("CharacterLightingProfiles"),Profiles);
    for(int32 I=0;I<S->CharacterLightingProfiles.Num();++I)
    {
        if(!Profiles.IsValidIndex(I)) Profiles.Add(TEXT("()"));
        const auto& P=S->CharacterLightingProfiles[I];
        FString Art;
        FStylizedMetalArtSettings::StaticStruct()->ExportText(Art,&P.MetalArt,nullptr,nullptr,PPF_SerializedAsImportText,nullptr);
        if(!SetMetalProfileField(Profiles[I],TEXT("bOverrideMetalArt"),P.bOverrideMetalArt?TEXT("True"):TEXT("False"))
            || !SetMetalProfileField(Profiles[I],TEXT("MetalArt"),Art))
        {Status=TEXT("Profile 格式异常，未写入配置。");return FReply::Handled();}
    }
    TMap<FName,FString> Expected;
    for(FName Name:{GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings,MetalArt),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings,NonMetalReflectionAlpha),GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings,MetalReflectionAlpha)})
    {
        const FProperty* P=FindFProperty<FProperty>(S->GetClass(),Name);FString Value;
        P->ExportTextItem_Direct(Value,P->ContainerPtrToValuePtr<void>(S),nullptr,S,PPF_SerializedAsImportText);
        Expected.Add(Name,Value);
    }
    for(const auto& Pair:Expected) GConfig->SetInSectionOfStaticLayer(*Section,Pair.Key,Pair.Value,Branch,Path,true);
    bool Saved=GConfig->RemoveKeyFromSectionOfStaticLayer(*Section,TEXT("CharacterLightingProfiles"),Branch,Path,false);
    for(int32 I=0;Saved&&I<Profiles.Num();++I) Saved=GConfig->AddToSectionOfStaticLayer(*Section,TEXT("CharacterLightingProfiles"),Profiles[I],Branch,Path,I==Profiles.Num()-1);
    FConfigFile Readback;Saved &= Readback.Combine(Path);
    for(const auto& Pair:Expected){FString V;Saved &= Readback.GetString(*Section,*Pair.Key.ToString(),V)&&V==Pair.Value;}
    TArray<FString> Actual;Readback.GetArray(*Section,TEXT("CharacterLightingProfiles"),Actual);Saved &= Actual==Profiles;
    Status=Saved?TEXT("金属形状、Profile 覆写与反射 Alpha 已保存；不修改材质或地图。") : TEXT("配置回读不一致，请检查写入权限。");
    return FReply::Handled();
}

void SDevKitRenderStyleWorkbenchWidget::Construct(const FArguments& InArgs)
{
    FinishParameters = FDevKitCloudLightingFinishTrial::GetParameters();
    bLastFinishActive = FDevKitCloudLightingFinishTrial::IsActive();
    ReadSceneDraft();
    RefreshPresets();
    Status = TEXT("暗部固定两档，亮面连续受光。分组可折叠，修改实时生效；仅显式保存才写入配置。");
    ChildSlot
    [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Background"))).Padding(12.f)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [Heading(TEXT("二分光影设置"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [Description(TEXT("只对暗部受光做两档分色，亮面保持连续，不按粗糙度抬亮分界。取消交界加法补光，保留原材质高光与投影。曝光、后处理和阴影算法不变；需要本次新版源引擎。"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed"))).Padding(10.f)
                [SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(this, &SDevKitRenderStyleWorkbenchWidget::GetSourceText).AutoWrapText(true).ColorAndOpacity(FStyleColors::AccentBlue)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[SNew(STextBlock).Text(this, &SDevKitRenderStyleWorkbenchWidget::GetAppliedText).AutoWrapText(true)]]]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
                [SAssignNew(PresetCombo, SComboBox<TSharedPtr<FString>>).OptionsSource(&PresetNames)
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString())); })
                    .OnSelectionChanged(this, &SDevKitRenderStyleWorkbenchWidget::OnPresetSelected)
                    [SNew(STextBlock).Text_Lambda([this] { return FText::FromString(SelectedPreset.IsValid() ? *SelectedPreset : TEXT("当前配置 / 自定义")); })]]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
                [SAssignNew(PresetName, SEditableTextBox).MinDesiredWidth(140.f).HintText(FText::FromString(TEXT("二分光影预设名称")))]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
                [SNew(SButton).IsEnabled_Lambda([] { return !FDevKitCloudLightingFinishTrial::IsActive(); })
                    .Text(FText::FromString(TEXT("保存二分光影预设"))).OnClicked(this, &SDevKitRenderStyleWorkbenchWidget::SavePreset)]]
            + SVerticalBox::Slot().FillHeight(1.f)
            [SNew(SScrollBox)
                + SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)
                [DevKitWorkbenchUI::MakeSection(TEXT("01 二分光影设置"),
                    SNew(SBox).IsEnabled_Lambda([] { return !FDevKitCloudLightingFinishTrial::IsActive(); })[MakeLightingControls()], false,
                    TEXT("暗部两档，亮面不分层；柔度只控制交界过渡。保留既有 Lumen Cache 分层入口，不替换 GI 算法。试调期间锁定编辑及保存。"))]
                + SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)
                [DevKitWorkbenchUI::MakeSection(TEXT("02 角色受光开关"),
                    SNew(SBox).IsEnabled_Lambda([] { return !FDevKitCloudLightingFinishTrial::IsActive(); })[MakeCharacterShadowControls()], false,
                    TEXT("仅作用于 StylizedCharacterLit。按当前确认效果使用原生 VSM 接收投影与自投影；独立角色阴影拆分固定关闭，不增加角色专用阴影图。"))]
                + SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)
                [DevKitWorkbenchUI::MakeSection(TEXT("03 角色金属美术参数"), MakeMetalArtControls())]
                + SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)
                [DevKitWorkbenchUI::MakeSection(TEXT("04 LookDev 与场景资源"), MakePreviewLinks(), true,
                    TEXT("在 LookDev 和实际游戏俯视镜头分别验收。角色覆写只影响 StylizedCharacterLit，不自动转换旧 Unlit 材质。"))]
                + SScrollBox::Slot()
                [DevKitWorkbenchUI::MakeSection(TEXT("05 平台与性能验收"),
                    SNew(SButton).Text(FText::FromString(TEXT("性能工作台 / 画质与阴影"))).OnClicked_Lambda([]
                    { FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitPerformanceWorkbench"))); return FReply::Handled(); }), true,
                    TEXT("Windows / DX12 / SDR，默认 Lumen + VSM；性能工作台提供原生 VSM / Lumen 精度与诊断，光源尺度仍在灯光属性中。60 / 120 FPS 需目标设备验证。"))]]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 8.f)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
                [SNew(SButton).Text(FText::FromString(TEXT("Default · 二分光影默认"))).OnClicked(this, &SDevKitRenderStyleWorkbenchWidget::ResetDraft)]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
                [SNew(SButton).Text(FText::FromString(TEXT("关闭场景分层对照"))).OnClicked_Lambda([this] {LoadBuiltInPreset(true); return FReply::Handled();})]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f)
                [SNew(STextBlock).Text(FText::FromString(TEXT("实时预览 · 修改即生效"))).ColorAndOpacity(FStyleColors::AccentBlue)]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
                [SNew(SButton).IsEnabled_Lambda([] { return !FDevKitCloudLightingFinishTrial::IsActive(); })
                    .ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton")).Text(FText::FromString(TEXT("保存项目全局默认"))).OnClicked(this, &SDevKitRenderStyleWorkbenchWidget::SaveProjectDefaults)]]
            + SVerticalBox::Slot().AutoHeight()
            [SNew(STextBlock).Text_Lambda([this] { return FText::FromString(Status); }).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]]];
}

void SDevKitRenderStyleWorkbenchWidget::ReadSceneDraft()
{
    const UStylizedLightingSettings* Settings = GetDefault<UStylizedLightingSettings>();
    bDraftEnabled = Settings->bEnableStylizedLumenLighting;
    DraftBands = 2.f; DraftSoftness = Safe(Settings->BandSoftness, 0.f, .49f, .18f);
    DraftGlossInfluence = Settings->GlossInfluence; DraftSpecularIntensity = Settings->SpecularIntensity;
    DraftSpecularOffset = Settings->SpecularOffset; DraftIndirectBlend = Settings->IndirectBlend;
    DraftDirectBlend = Settings->DirectBlend;
}

void SDevKitRenderStyleWorkbenchWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    const bool bFinishActive = FDevKitCloudLightingFinishTrial::IsActive();
    if (bLastFinishActive && !bFinishActive)
    {
        ReadSceneDraft();
        Status = TEXT("试调已结束，已恢复试调前的受控参数；保存和切换关卡不会保留会话试调。");
    }
    if (bFinishActive) FinishParameters = FDevKitCloudLightingFinishTrial::GetParameters();
    bLastFinishActive = bFinishActive;
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeFinishControls()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
            [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
                .Text(FText::FromString(TEXT("试调 · 高光 / 质感 / 阴影"))).OnClicked(this, &SDevKitRenderStyleWorkbenchWidget::ApplyFinishTrial)]
            + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
            [SNew(SButton).IsEnabled_Lambda([] { return FDevKitCloudLightingFinishTrial::IsActive(); })
                .Text(FText::FromString(TEXT("恢复试调前"))).OnClicked(this, &SDevKitRenderStyleWorkbenchWidget::RestoreFinishTrial)]]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
        [SNew(STextBlock).Text(this, &SDevKitRenderStyleWorkbenchWidget::GetFinishStatus).AutoWrapText(true).ColorAndOpacity(FStyleColors::AccentBlue)]
        + SVerticalBox::Slot().AutoHeight()[MakeFinishParameterRow(TEXT("场景高光倍率"), TEXT("相对试调前强度，默认 0.90：轻压高光峰值，不改漫反射分色。"), &FinishParameters.SceneSpecularScale, 0.f, 2.f, .01f)]
        + SVerticalBox::Slot().AutoHeight()[MakeFinishParameterRow(TEXT("场景高光偏移增量"), TEXT("默认 +0.005：微调原有高光瓣范围；负值更收紧、正值稍展开。不是曝光。"), &FinishParameters.SceneSpecularOffsetDelta, -1.f, 1.f, .001f)]
        + SVerticalBox::Slot().AutoHeight()[MakeFinishParameterRow(TEXT("角色直接高光倍率"), TEXT("默认 0.85：各角色 Profile 独立乘以原强度，保留 GGX、MixMap、粗糙度及金属区别。"), &FinishParameters.CharacterSpecularScale, 0.f, 2.f, .01f)]
        + SVerticalBox::Slot().AutoHeight()[MakeFinishParameterRow(TEXT("角色环境反射倍率"), TEXT("默认 0.90：轻收反射亮斑；不关 Lumen / SSR、不改反射颜色或 GI。"), &FinishParameters.CharacterReflectionScale, 0.f, 2.f, .01f)]
        + SVerticalBox::Slot().AutoHeight()[MakeFinishParameterRow(TEXT("角色细碎遮蔽倍率"), TEXT("默认 0.85：稍减 SSAO / Lumen 短程遮蔽。也会减弱反射遮蔽，过低可能漏光；不取消物件投影。"), &FinishParameters.CharacterOcclusionScale, 0.f, 1.f, .01f)]
        + SVerticalBox::Slot().AutoHeight()[MakeFinishParameterRow(TEXT("角色接收场景投影倍率"), TEXT("仅 Toon 模式方向光 + StylizedCharacterLit 材质；调整角色接收投影的压暗响应，不改变 VSM 清晰度或角色投向地面的阴影。"), &FinishParameters.CharacterProjectedShadowScale, 0.f, 1.f, .01f)];
}

SDevKitRenderStyleWorkbenchWidget::~SDevKitRenderStyleWorkbenchWidget()
{
    if (MetalColorPicker) MetalColorPicker->CancelForOwnerShutdown();
}

FStylizedMetalArtSettings& SDevKitRenderStyleWorkbenchWidget::EditMetalArt() const
{
    auto* S = GetMutableDefault<UStylizedLightingSettings>();
    return S->CharacterLightingProfiles.IsValidIndex(MetalProfileIndex)
        ? S->CharacterLightingProfiles[MetalProfileIndex].MetalArt : S->MetalArt;
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeReflectionAlphaControls()
{
    auto Body = SNew(SVerticalBox);
    for (bool bMetal : {false, true})
        Body->AddSlot().AutoHeight().Padding(4.f)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f)[Description(bMetal ? TEXT("金属环境反射 Alpha · 默认 0.7") : TEXT("非金属环境反射 Alpha · 默认 0.3"))]
            + SHorizontalBox::Slot().FillWidth(.45f)[SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(.01f)
                .Value_Lambda([bMetal] { const auto* S=GetDefault<UStylizedLightingSettings>(); return bMetal ? S->MetalReflectionAlpha : S->NonMetalReflectionAlpha; })
                .OnValueChanged_Lambda([this,bMetal](float V) { auto* S=GetMutableDefault<UStylizedLightingSettings>();
                    (bMetal ? S->MetalReflectionAlpha : S->NonMetalReflectionAlpha)=Safe(V,0,1,bMetal?.7f:.3f);
                    S->bEnableEnvironmentReflections=true; ApplyCharacterShadowChanges(); })]];
    Body->AddSlot().AutoHeight()[Description(TEXT("0 移除间接镜面，1 完整保留；中间金属度连续插值。不会削弱直接高光或漫反射 GI。点击下方保存金属与反射配置。"))];
    return Body;
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeMetalArtControls()
{
    auto Body=SNew(SVerticalBox);
    MetalProfileOptions.Add(MakeShared<FString>(TEXT("全局默认")));
    const auto& Profiles=GetDefault<UStylizedLightingSettings>()->CharacterLightingProfiles;
    for(int32 I=0;I<Profiles.Num();++I) MetalProfileOptions.Add(MakeShared<FString>(FString::Printf(TEXT("Profile %d · %s"),I,*Profiles[I].ProfileName.ToString())));
    Body->AddSlot().AutoHeight()[SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&MetalProfileOptions)
        .IsEnabled_Lambda([this]{return !MetalColorPicker || !MetalColorPicker->IsPickerOpen();})
        .OnGenerateWidget_Lambda([](TSharedPtr<FString> V){ return SNew(STextBlock).Text(FText::FromString(*V)); })
        .OnSelectionChanged_Lambda([this](TSharedPtr<FString> V,ESelectInfo::Type){ MetalProfileIndex=MetalProfileOptions.IndexOfByKey(V)-1; })
        [SNew(STextBlock).Text_Lambda([this]{ return FText::FromString(*MetalProfileOptions[FMath::Clamp(MetalProfileIndex+1,0,MetalProfileOptions.Num()-1)]); })]];
    Body->AddSlot().AutoHeight().Padding(4.f)[SNew(SCheckBox)
        .Visibility_Lambda([this]{ return MetalProfileIndex>=0 ? EVisibility::Visible : EVisibility::Collapsed; })
        .IsChecked_Lambda([this]{ const auto& P=GetDefault<UStylizedLightingSettings>()->CharacterLightingProfiles; return P.IsValidIndex(MetalProfileIndex)&&P[MetalProfileIndex].bOverrideMetalArt ? ECheckBoxState::Checked:ECheckBoxState::Unchecked; })
        .OnCheckStateChanged_Lambda([this](ECheckBoxState V){ auto* S=GetMutableDefault<UStylizedLightingSettings>(); if(S->CharacterLightingProfiles.IsValidIndex(MetalProfileIndex)) { auto& P=S->CharacterLightingProfiles[MetalProfileIndex]; if(V==ECheckBoxState::Checked&&!P.bOverrideMetalArt) P.MetalArt=S->MetalArt; P.bOverrideMetalArt=V==ECheckBoxState::Checked; ApplyCharacterShadowChanges(); } })
        [SNew(STextBlock).Text(FText::FromString(TEXT("覆写此 Profile 的金属美术参数（关闭则继承全局）")))]];
    auto Controls=SNew(SVerticalBox).IsEnabled_Lambda([this]{ const auto& P=GetDefault<UStylizedLightingSettings>()->CharacterLightingProfiles;
        return !FDevKitCloudLightingFinishTrial::IsActive() && (MetalProfileIndex<0 || (P.IsValidIndex(MetalProfileIndex)&&P[MetalProfileIndex].bOverrideMetalArt)); });
    auto Add=[this](TSharedRef<SVerticalBox> Group,const TCHAR* Label,float FStylizedMetalArtSettings::*Field,float Min,float Max,float Step)
    {
        Group->AddSlot().AutoHeight().Padding(4.f)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(Label))]
            +SHorizontalBox::Slot().FillWidth(.45f)[SNew(SSpinBox<float>).MinValue(Min).MaxValue(Max).Delta(Step)
                .Value_Lambda([this,Field]{return EditMetalArt().*Field;})
                .OnValueChanged_Lambda([this,Field,Min,Max](float V){EditMetalArt().*Field=Safe(V,Min,Max,EditMetalArt().*Field);ApplyCharacterShadowChanges();})]];
    };
    auto Response=SNew(SVerticalBox);
    auto ModeRow=SNew(SHorizontalBox);
    for (EStylizedMetalHighlightMode Mode : {EStylizedMetalHighlightMode::PBR, EStylizedMetalHighlightMode::Stylized})
        ModeRow->AddSlot().AutoWidth().Padding(4.f)[SNew(SCheckBox).Style(FAppStyle::Get(),"RadioButton")
            .IsChecked_Lambda([this,Mode]{return EditMetalArt().HighlightMode==Mode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
            .OnCheckStateChanged_Lambda([this,Mode](ECheckBoxState State){
                if(State==ECheckBoxState::Checked && !FDevKitCloudLightingFinishTrial::IsActive()) {
                    EditMetalArt().HighlightMode=Mode; ApplyCharacterShadowChanges();
                }
            })
            [SNew(STextBlock).Text(FText::FromString(Mode==EStylizedMetalHighlightMode::PBR ? TEXT("PBR 响应") : TEXT("风格化块面")))]];
    Response->AddSlot().AutoHeight()[ModeRow];
    Add(Response,TEXT("风格化混合强度 · 0=PBR / 1=块面"),&FStylizedMetalArtSettings::Stylization,0,1,.01f);
    Response->AddSlot().AutoHeight()[Description(TEXT("默认风格化模式、强度0.5。PBR 模式保留当前的物理高光响应，并忽略但记住混合强度；风格化模式把真实 GGX 高光整理为亮带和亮芯，粗糙金属仍会变宽、变弱、柔化。可按 Profile 覆写，修改立即预览。"))];
    Controls->AddSlot().AutoHeight()[DevKitWorkbenchUI::MakeSection(TEXT("金属模式与混合"),Response)];
    auto Faces=SNew(SVerticalBox);
    Add(Faces,TEXT("亮面强度"),&FStylizedMetalArtSettings::LightFace,0,2,.01f);
    Add(Faces,TEXT("暗部强化 · 0=不压暗 / 1=压低块面间反射"),&FStylizedMetalArtSettings::DarkSideStrength,0,1,.01f);
    Faces->AddSlot().AutoHeight()[Description(TEXT("暗部强化默认0.35，仅在风格化端点压低亮块以外的真实镜面尾部，随混合强度生效；不改变原生 VSM、环境反射或整屏对比度。亮面增益不添加额外底色。"))];
    Controls->AddSlot().AutoHeight()[DevKitWorkbenchUI::MakeSection(TEXT("金属明暗"),Faces)];
    auto Highlights=SNew(SVerticalBox);
    Add(Highlights,TEXT("基础色块大小 · 远景块面尺度"),&FStylizedMetalArtSettings::BaseBlockSize,0,1,.01f);
    Add(Highlights,TEXT("默认高光宽度倍率 · 随粗糙度（材质可覆写）"),&FStylizedMetalArtSettings::HighlightWidth,.1f,3,.01f);
    Highlights->AddSlot().AutoHeight()[Description(TEXT("默认基础色块0.30、宽度倍率1.5、高光3.2、亮芯4.5。基础色块独立扩大风格化高光的结构范围，并保留远景像素覆盖，不扩大金属遮罩或轮廓；0仍保留必要的像素抗锯齿。宽度倍率作用于粗糙度 Alpha，材质覆写优先。PBR模式不使用基础色块与暗部强化；高光仍读取真实光强、金属F0和粗糙度，不添加灯带、补光或屏幕模糊。"))];
    Add(Highlights,TEXT("高光倍率 × 实际光强"),&FStylizedMetalArtSettings::HighlightStrength,0,10,.05f);
    Add(Highlights,TEXT("亮芯倍率 × 实际光强 / 原生 Bloom"),&FStylizedMetalArtSettings::CoreStrength,0,20,.05f);
    Highlights->AddSlot().AutoHeight().Padding(4.f)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1.f)[Description(TEXT("默认高光色调 · 材质 RGB/贴图覆写优先"))]
        +SHorizontalBox::Slot().FillWidth(.45f)[SAssignNew(MetalColorPicker,SDevKitWorkbenchColorPicker).ClampValue(true)
            .Color_Lambda([this]{return EditMetalArt().HighlightTint;})
            .OnColorChanged_Lambda([this](FLinearColor C){EditMetalArt().HighlightTint=C;ApplyCharacterShadowChanges();})]];
    Controls->AddSlot().AutoHeight()[DevKitWorkbenchUI::MakeSection(TEXT("高光与对比"),Highlights)];
    Controls->AddSlot().AutoHeight()[Description(TEXT("环境反射仅使用真实光照 / Lumen / 反射捕获及原生粗糙度 BRDF，再乘角色收光中的反射混合系数；不再生成人为环境灯带。"))];
    Body->AddSlot().AutoHeight()[Controls];
    Body->AddSlot().AutoHeight()[Description(TEXT("仅作用于启用金属风格混合的 StylizedCharacterLit（沿用原 NPR Profile 开关）。M_CharacterMaster 的 NPR Metal 分组可覆写宽度和高光颜色/贴图；默认关闭，继承当前效果。引擎兼容模式0/1/2不使用此面板；本面板中的 PBR 模式与风格化模式都保持原生环境反射和 VSM。"))];
    Body->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("恢复当前项的金属形状默认"))).OnClicked_Lambda([this]{ EditMetalArt()=FStylizedMetalArtSettings();ApplyCharacterShadowChanges();return FReply::Handled(); })];
    Body->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("保存金属与反射配置"))).OnClicked(this,&SDevKitRenderStyleWorkbenchWidget::SaveMetalArtDefaults)];
    return Body;
}

void SDevKitRenderStyleWorkbenchWidget::ApplyCharacterShadowChanges()
{
    if (FDevKitCloudLightingFinishTrial::IsActive()) return;
    auto* Settings = GetMutableDefault<UStylizedLightingSettings>();
    Settings->bEnableCharacterShadowSeparation = false;
    Settings->bEnableCharacterHalfVectorShadow = true;
    Settings->ApplyToConsoleVariables();
    Status = TEXT("角色受光开关已实时应用，未保存配置或资产。角色接收投影与自投影保持原生 VSM；独立阴影拆分固定关闭，半程向量设置保留但不参与当前渲染。");
    if (GEditor) GEditor->RedrawLevelEditingViewports();
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeCharacterShadowControls()
{
    const TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
    const auto AddToggle = [this, &Body](const TCHAR* Label, bool UStylizedLightingSettings::* Member,
        const TCHAR* Help = TEXT(""), bool bRequiresIndependentSplit = false)
    {
        Body->AddSlot().AutoHeight().Padding(4.f)[SNew(SCheckBox)
            .IsEnabled_Lambda([bRequiresIndependentSplit]
            { return !bRequiresIndependentSplit; })
            .ToolTipText(FText::FromString(Help))
            .IsChecked_Lambda([Member] { return GetDefault<UStylizedLightingSettings>()->*Member ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([this, Member, bRequiresIndependentSplit](ECheckBoxState State)
            {
                if (bRequiresIndependentSplit) return;
                GetMutableDefault<UStylizedLightingSettings>()->*Member = State == ECheckBoxState::Checked;
                ApplyCharacterShadowChanges();
            })
            [SNew(STextBlock).Text(FText::FromString(Label))]];
    };
    AddToggle(TEXT("接收投影（原生 VSM）"), &UStylizedLightingSettings::bEnableSceneShadowPartition,
        TEXT("接收场景与其他物件的投影，并保留原生 VSM 自投影；不改变角色投向地面的阴影。"));
    AddToggle(TEXT("角色独立细节自阴影（当前不参与）"), &UStylizedLightingSettings::bEnableCharacterIndependentSelfShadow,
        TEXT("保留勾选状态。独立角色阴影拆分固定关闭，因此此项不参与当前渲染；角色自投影仍由原生 VSM 提供。"), true);
    AddToggle(TEXT("保留 Lumen 漫反射环境光"), &UStylizedLightingSettings::bEnableLumenDiffuseIndirect);
    Body->AddSlot().AutoHeight()[MakeReflectionAlphaControls()];
    AddToggle(TEXT("额外细碎遮蔽（默认关闭；不关闭 VSM）"), &UStylizedLightingSettings::bEnableIndirectOcclusion);
    Body->AddSlot().AutoHeight().Padding(4.f)[SNew(SCheckBox)
        .ToolTipText(FText::FromString(TEXT("只修改默认 Lighting Profile 0 的金属外观。修改立即预览；保存只写该开关，不改贴图、材质或其他 Profile。")))
        .IsChecked_Lambda([]
        {
            const auto& Profiles = GetDefault<UStylizedLightingSettings>()->CharacterLightingProfiles;
            return !Profiles.IsEmpty() && Profiles[0].bEnableNPRMetal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        })
        .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
        {
            if (FDevKitCloudLightingFinishTrial::IsActive()) return;
            auto* Settings = GetMutableDefault<UStylizedLightingSettings>();
            if (Settings->CharacterLightingProfiles.IsEmpty()) Settings->CharacterLightingProfiles.AddDefaulted();
            Settings->CharacterLightingProfiles[0].bEnableNPRMetal = State == ECheckBoxState::Checked;
            ApplyCharacterShadowChanges();
        })
        [SNew(STextBlock).Text(FText::FromString(TEXT("金属风格混合（默认 Profile 0）")))]];
    Body->AddSlot().AutoHeight().Padding(4.f)[Description(TEXT("启用默认配置的金属模式与混合面板，不改变非金属。其他材质通过 Lighting Profile 选择各自配置；新增 Profile 的金属增强默认关闭。"))];
    Body->AddSlot().AutoHeight().Padding(4.f)[Description(TEXT("当前保持原生 VSM 接收投影 / 自投影与 Lumen GI；独立阴影拆分关闭，半程向量和独立自阴影不参与。材质环境反射提供金属所需的环境形状与反射颜色，不是额外补光。额外细碎遮蔽默认关闭。"))];
    Body->AddSlot().AutoHeight().Padding(4.f)[SNew(SButton).Text(FText::FromString(TEXT("保存角色受光开关"))).OnClicked(this, &SDevKitRenderStyleWorkbenchWidget::SaveCharacterShadowDefaults)];
    return Body;
}

FReply SDevKitRenderStyleWorkbenchWidget::SaveCharacterShadowDefaults()
{
    if (FDevKitCloudLightingFinishTrial::IsActive()) return FReply::Handled();
    auto* Settings = GetMutableDefault<UStylizedLightingSettings>();
    Settings->bEnableCharacterShadowSeparation = false;
    Settings->bEnableCharacterHalfVectorShadow = true;
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"));
    const FString Section = Settings->GetClass()->GetPathName();
    FConfigFile Existing;
    TArray<FString> SavedProfiles;
    if (!Existing.Combine(Path))
    {
        Status = TEXT("无法读取现有配置，未保存角色开关。");
        return FReply::Handled();
    }
    Existing.GetArray(*Section, TEXT("CharacterLightingProfiles"), SavedProfiles);
    const bool bNPRMetal = !Settings->CharacterLightingProfiles.IsEmpty() && Settings->CharacterLightingProfiles[0].bEnableNPRMetal;
    if (!SetSavedDefaultNPRMetal(SavedProfiles, bNPRMetal))
    {
        Status = TEXT("已有角色 Profile 格式无法安全更新，未保存角色开关。");
        return FReply::Handled();
    }
    TMap<FName, FString> Expected;
    for (const FName Name : {
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableSceneShadowPartition),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableLumenDiffuseIndirect),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableEnvironmentReflections),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, NonMetalReflectionAlpha),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, MetalReflectionAlpha),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableIndirectOcclusion),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterShadowSeparation),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterIndependentSelfShadow),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterHalfVectorShadow),
        })
    {
        const FProperty* Property = FindFProperty<FProperty>(Settings->GetClass(), Name);
        FString Value;
        Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Settings), nullptr, Settings, PPF_SerializedAsImportText);
        GConfig->SetInSectionOfStaticLayer(*Section, Name, Value, Settings->GetClass()->ClassConfigName.ToString(), Path, true);
        Expected.Add(Name, Value);
    }
    FConfigFile Saved;
    // Profile indices are material-facing IDs. ArrayAddUnique would collapse
    // identical authored rows and silently shift those IDs; use ordered ArrayAdd.
    const FName ProfilesKey = GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterLightingProfiles);
    const FString Branch = Settings->GetClass()->ClassConfigName.ToString();
    bool bProfilesSaved = GConfig->RemoveKeyFromSectionOfStaticLayer(*Section, ProfilesKey, Branch, Path, false);
    for (int32 Index = 0; bProfilesSaved && Index < SavedProfiles.Num(); ++Index)
        bProfilesSaved = GConfig->AddToSectionOfStaticLayer(*Section, ProfilesKey, SavedProfiles[Index], Branch, Path, Index == SavedProfiles.Num() - 1);
    bool bVerified = bProfilesSaved && Saved.Combine(Path);
    for (const auto& Pair : Expected) { FString Actual; bVerified &= Saved.GetString(*Section, *Pair.Key.ToString(), Actual) && Actual == Pair.Value; }
    TArray<FString> ActualProfiles;
    Saved.GetArray(*Section, TEXT("CharacterLightingProfiles"), ActualProfiles);
    bVerified &= ActualProfiles == SavedProfiles;
    Status = bVerified ? TEXT("已保存角色受光及默认 Profile 0 金属 GGX 增强开关；原有 Profile 倍率、其他配置、材质和地图不变。") : TEXT("部分角色开关未回读一致，请检查权限；未保存材质或地图资产。");
    return FReply::Handled();
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeFinishParameterRow(const TCHAR* Label, const TCHAR* Help,
    float* Value, float Min, float Max, float Delta)
{
    return SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed"))).Padding(8.f)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Label))]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Description(Help)]]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f)
            [SNew(SBox).WidthOverride(140.f)
                [SNew(SSpinBox<float>).MinValue(Min).MaxValue(Max).Delta(Delta)
                    .Value_Lambda([Value] {return *Value;})
                    .OnValueChanged_Lambda([this, Value](float NewValue)
                    {
                        *Value = NewValue;
                        ApplyFinishTrial();
                    })]]];
}

FReply SDevKitRenderStyleWorkbenchWidget::ApplyFinishTrial()
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        Status = TEXT("没有可用的编辑器关卡，未应用试调。");
        return FReply::Handled();
    }
    FDevKitCloudLightingFinishTrial::Apply(World, FinishParameters);
    bLastFinishActive = FDevKitCloudLightingFinishTrial::IsActive();
    ReadSceneDraft();
    SelectedPreset.Reset();
    if (PresetCombo.IsValid()) PresetCombo->ClearSelection();
    Status = TEXT("高光 / 质感 / 阴影试调已生效。修改倍率实时更新；未改分层、曝光、GI、自投影参数、阴影算法或材质资产。仅会话试调，恢复按钮可还原试调前值；保存入口暂锁定。");
    return FReply::Handled();
}

FReply SDevKitRenderStyleWorkbenchWidget::RestoreFinishTrial()
{
    FDevKitCloudLightingFinishTrial::Restore();
    bLastFinishActive = false;
    ReadSceneDraft();
    Status = TEXT("已恢复试调前的高光、角色反射 / 遮蔽和方向光投影参数；保留你原来的分层与其它修改，未保存关卡或配置。");
    return FReply::Handled();
}

FText SDevKitRenderStyleWorkbenchWidget::GetFinishStatus() const
{
    if (!FDevKitCloudLightingFinishTrial::IsActive())
        return FText::FromString(TEXT("未试调 · 修改即实时预览，恢复按钮可还原。角色参数仅作用于 StylizedCharacterLit 材质；场景参数不作用于该角色模型。投影清晰度由 VSM / 光源尺度决定，此处不提供阴影贴图配置。"));
    const UStylizedLightingSettings* Settings = GetDefault<UStylizedLightingSettings>();
    const FStylizedCharacterLightingProfile* Profile = Settings->CharacterLightingProfiles.IsEmpty() ? nullptr : &Settings->CharacterLightingProfiles[0];
    return FText::FromString(FString::Printf(TEXT("会话试调中 · 场景高光 %.3f / 偏移 %.3f · 首个角色 Profile 高光 %.3f / 反射 %.3f / 遮蔽 %.3f\n各 Profile 保留自身比例；投影只调整当前世界中投射阴影的方向光。不会自动写入项目默认或关卡。"),
        Settings->SpecularIntensity, Settings->SpecularOffset, Profile ? Profile->SpecularIntensity : 0.f,
        Profile ? Profile->ReflectionIntensity : 0.f, Profile ? Profile->IndirectOcclusionStrength : 0.f));
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeLightingControls()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
        [SNew(SCheckBox).IsChecked_Lambda([this] {return bDraftEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {bDraftEnabled = State == ECheckBoxState::Checked; MarkDraftChanged();})
            [SNew(STextBlock).Text(FText::FromString(TEXT("启用二分光影（暗部两档 / 亮面连续）")))]]
        + SVerticalBox::Slot().AutoHeight()[Description(TEXT("层数固定为 2，不再提供多层或光泽推动分界。亮面 N·L ≥ 0.5 保持原始受光；暗部档位为 0 / 0.25，GI 与反射仍提供原有环境底色。"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
        [DevKitWorkbenchUI::MakeSection(TEXT("暗部二分与边界"), SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[MakeLayerParameterRow(TEXT("暗部边界柔度"), TEXT("有效范围 0–0.49；0 为硬边，仅柔化暗部两档与连续亮面的交界，不改变阴影精度。"), &DraftSoftness, 0.f, .49f, .01f)]
            + SVerticalBox::Slot().AutoHeight()[MakeLayerParameterRow(TEXT("Lumen Cache 二分混合"), TEXT("既有 Cache 受光阶段二分混合，0 保留原生响应；不是最终 GI 能量乘数。"), &DraftIndirectBlend, 0.f, 1.f, .01f)]
            + SVerticalBox::Slot().AutoHeight()[MakeLayerParameterRow(TEXT("直接光二分混合"), TEXT("0 保留原生直接光，1 完整应用暗部二分；亮面始终连续。"), &DraftDirectBlend, 0.f, 1.f, .01f)])]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
        [DevKitWorkbenchUI::MakeSection(TEXT("原有场景高光微调"), SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[MakeLayerParameterRow(TEXT("场景高光强度"), TEXT("既有场景高光强度，默认 1；独立于暗部二分，不改角色 Profile 高光。"), &DraftSpecularIntensity, 0.f, 4.f, .01f)]
            + SVerticalBox::Slot().AutoHeight()[MakeLayerParameterRow(TEXT("场景高光偏移"), TEXT("既有高光响应偏移，默认 0；不是曝光补偿。"), &DraftSpecularOffset, -1.f, 1.f, .01f)], true)];
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakeLayerParameterRow(const TCHAR* Label, const TCHAR* Help,
    float* Value, float Min, float Max, float Delta)
{
    return SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed"))).Padding(8.f)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Label))]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Description(Help)]]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f)
            [SNew(SBox).WidthOverride(140.f)
                [SNew(SSpinBox<float>).MinValue(Min).MaxValue(Max).Delta(Delta).IsEnabled_Lambda([this] {return bDraftEnabled;})
                    .Value_Lambda([Value] {return *Value;})
                    .OnValueChanged_Lambda([this, Value](float NewValue) {*Value = NewValue; MarkDraftChanged();})]]];
}

TSharedRef<SWidget> SDevKitRenderStyleWorkbenchWidget::MakePreviewLinks()
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
        [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton")).Text(FText::FromString(TEXT("LookDev 关卡工作台")))
            .OnClicked_Lambda([] {FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitCharacterLookDev"))); return FReply::Handled();})]
        + SHorizontalBox::Slot().AutoWidth().Padding(2.f)
        [SNew(SButton).Text(FText::FromString(TEXT("场景资源与子关卡")))
            .OnClicked_Lambda([] {FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitSceneWorkbench"))); return FReply::Handled();})];
}

void SDevKitRenderStyleWorkbenchWidget::MarkDraftChanged(bool bKeepPresetSelection)
{
    if (!bKeepPresetSelection)
    {
        SelectedPreset.Reset();
        if (PresetCombo.IsValid()) PresetCombo->ClearSelection();
    }
    bHasPendingChanges = true;
    ApplyDraft();
}

void SDevKitRenderStyleWorkbenchWidget::RefreshPresets()
{
    PresetNames.Reset();
    PresetNames.Add(MakeShared<FString>(NativePresetName));
    PresetNames.Add(MakeShared<FString>(LayerPresetName));
    for (const FYogRenderStylePreset& Preset : GetDefault<UYogRenderStyleSettings>()->Presets)
        if (!Preset.Name.IsNone() && Preset.Name.ToString() != NativePresetName && Preset.Name.ToString() != LayerPresetName)
            PresetNames.Add(MakeShared<FString>(Preset.Name.ToString()));
    if (PresetCombo.IsValid()) PresetCombo->RefreshOptions();
}

void SDevKitRenderStyleWorkbenchWidget::LoadBuiltInPreset(bool bDisableScene)
{
    FDevKitCloudLightingFinishTrial::Restore();
    bDraftEnabled = !bDisableScene;
    DraftBands = 2.f; DraftSoftness = .18f; DraftGlossInfluence = .65f;
    DraftSpecularIntensity = 1.f; DraftSpecularOffset = 0.f;
    DraftIndirectBlend = 1.f; DraftDirectBlend = 1.f;
    MarkDraftChanged(true);
}

void SDevKitRenderStyleWorkbenchWidget::OnPresetSelected(TSharedPtr<FString> Name, ESelectInfo::Type SelectInfo)
{
    if (!Name.IsValid()) return;
    SelectedPreset = Name;
    if (*Name == NativePresetName) LoadBuiltInPreset(true);
    else if (*Name == LayerPresetName) LoadBuiltInPreset(false);
    else for (const FYogRenderStylePreset& Preset : GetDefault<UYogRenderStyleSettings>()->Presets)
    {
        if (Preset.Name.ToString() != *Name) continue;
        if (!Preset.bIncludesCloudLighting || (Preset.CloudLightingVersion != 1 && Preset.CloudLightingVersion != 2))
        {
            Status = TEXT("这是旧兼容预设或不支持的版本：未包含云端八参数契约。四参数 Strength 不会迁移为间接光混合；未应用旧角色 / 后处理数据，当前画面未改变。");
            return;
        }
        FDevKitCloudLightingFinishTrial::Restore();
        bDraftEnabled = Preset.bEnableCloudLighting;
        DraftBands = 2.f; DraftSoftness = Preset.CloudLightingSoftness;
        DraftGlossInfluence = Preset.CloudLightingGlossInfluence; DraftSpecularIntensity = Preset.CloudLightingSpecularIntensity;
        DraftSpecularOffset = Preset.CloudLightingSpecularOffset; DraftIndirectBlend = Preset.CloudLightingIndirectBlend;
        DraftDirectBlend = Preset.CloudLightingDirectBlend;
        MarkDraftChanged(true);
        Status = TEXT("已实时切换二分光影预设；旧多层数据自动按两档暗部解释，亮面连续。缓存渐进刷新，未保存项目默认或资产。");
        return;
    }
}

FReply SDevKitRenderStyleWorkbenchWidget::ResetDraft()
{
    LoadBuiltInPreset(false);
    SelectedPreset = PresetNames[1];
    if (PresetCombo.IsValid()) PresetCombo->SetSelectedItem(SelectedPreset);
    Status = TEXT("Default 已恢复二分光影默认，暗部两档、亮面连续；未改变角色 Profile、相机曝光、后处理或阴影算法，也未保存配置。缓存会渐进收敛。");
    return FReply::Handled();
}

FReply SDevKitRenderStyleWorkbenchWidget::ApplyDraft()
{
    if (FDevKitCloudLightingFinishTrial::IsActive())
    {
        Status = TEXT("请先恢复试调前，再编辑或保存二分光影；当前试调未改变。");
        return FReply::Handled();
    }
    DraftBands = 2.f;
    DraftSoftness = Safe(DraftSoftness, 0.f, .49f, .18f);
    DraftGlossInfluence = Safe(DraftGlossInfluence, 0.f, 1.f, .65f);
    DraftSpecularIntensity = Safe(DraftSpecularIntensity, 0.f, 4.f, 1.f);
    DraftSpecularOffset = Safe(DraftSpecularOffset, -1.f, 1.f, 0.f);
    DraftIndirectBlend = Safe(DraftIndirectBlend, 0.f, 1.f, 1.f);
    DraftDirectBlend = Safe(DraftDirectBlend, 0.f, 1.f, 1.f);
    UStylizedLightingSettings* Settings = GetMutableDefault<UStylizedLightingSettings>();
    Settings->bEnableStylizedLumenLighting = bDraftEnabled;
    Settings->BandCount = DraftBands; Settings->BandSoftness = DraftSoftness;
    Settings->GlossInfluence = DraftGlossInfluence; Settings->SpecularIntensity = DraftSpecularIntensity;
    Settings->SpecularOffset = DraftSpecularOffset; Settings->IndirectBlend = DraftIndirectBlend;
    Settings->DirectBlend = DraftDirectBlend;
    Settings->ApplyToConsoleVariables();
    Status = TEXT("二分光影已实时上传，暗部两档、亮面连续；缓存渐进刷新，未修改曝光、后处理、材质资产或配置文件。");
    if (GEditor) GEditor->RedrawLevelEditingViewports();
    return FReply::Handled();
}

FReply SDevKitRenderStyleWorkbenchWidget::SaveProjectDefaults()
{
    if (FDevKitCloudLightingFinishTrial::IsActive())
    {
        Status = TEXT("会话试调不会保存为项目默认。请先恢复试调前；验收试调后再决定正式配置。");
        return FReply::Handled();
    }
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"));
    ApplyDraft();
    if (SaveCloudLightingDefaultsToConfig(GetMutableDefault<UStylizedLightingSettings>(), Path))
    {
        bHasPendingChanges = false;
        Status = TEXT("已保存二分光影默认（层数固定为 2）；未写入曝光、角色 Profile 或资产。");
    }
    else Status = TEXT("当前会话已实时应用，但八个默认键未全部写入成功；请检查 DefaultGame.ini 的权限 / 签出状态。未保存资产。");
    return FReply::Handled();
}

FReply SDevKitRenderStyleWorkbenchWidget::SavePreset()
{
    if (FDevKitCloudLightingFinishTrial::IsActive())
    {
        Status = TEXT("会话试调包含角色与灯光数据，不能伪装为八参数场景预设保存。请先恢复试调前。");
        return FReply::Handled();
    }
    const FName Name(*PresetName->GetText().ToString().TrimStartAndEnd());
    if (Name.IsNone() || Name.ToString() == NativePresetName || Name.ToString() == LayerPresetName)
    {
        Status = TEXT("请填写新的二分光影预设名称；内置默认与原生受光对照不能覆盖。");
        return FReply::Handled();
    }
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"));
    UYogRenderStyleSettings* Settings = GetMutableDefault<UYogRenderStyleSettings>();
    const TArray<FYogRenderStylePreset> Previous = Settings->Presets;
    FYogRenderStylePreset* Preset = Settings->Presets.FindByPredicate([Name](const FYogRenderStylePreset& Item) {return Item.Name == Name;});
    if (!Preset) {Preset = &Settings->Presets.AddDefaulted_GetRef(); Preset->Name = Name;}
    Preset->bIncludesCloudLighting = true; Preset->CloudLightingVersion = 2; Preset->bEnableCloudLighting = bDraftEnabled;
    Preset->CloudLightingBands = 2.f;
    Preset->CloudLightingSoftness = Safe(DraftSoftness, 0.f, .49f, .18f);
    Preset->CloudLightingGlossInfluence = Safe(DraftGlossInfluence, 0.f, 1.f, .65f);
    Preset->CloudLightingSpecularIntensity = Safe(DraftSpecularIntensity, 0.f, 4.f, 1.f);
    Preset->CloudLightingSpecularOffset = Safe(DraftSpecularOffset, -1.f, 1.f, 0.f);
    Preset->CloudLightingIndirectBlend = Safe(DraftIndirectBlend, 0.f, 1.f, 1.f);
    Preset->CloudLightingDirectBlend = Safe(DraftDirectBlend, 0.f, 1.f, 1.f);
    // Preserve any serialized legacy fields; they are never applied by this UI.
    if (!SavePresetTableToConfig(Settings, Path))
    {
        Settings->Presets = Previous;
        Status = TEXT("二分光影预设写入失败，已恢复预设表；当前画面和项目默认未改变。");
        return FReply::Handled();
    }
    RefreshPresets();
    Status = TEXT("已保存二分光影预设（版本 2），只写入 Presets 表；未保存全局默认、关卡或材质。选择预设会实时切换。");
    return FReply::Handled();
}

bool SDevKitRenderStyleWorkbenchWidget::SaveCloudLightingDefaultsToConfig(UStylizedLightingSettings* Settings, const FString& Path,
    FConfigCacheIni* ConfigSystem)
{
    if (!Settings || IFileManager::Get().IsReadOnly(*Path)) return false;
    ConfigSystem = ConfigSystem ? ConfigSystem : GConfig;
    if (!ConfigSystem) return false;
    const FString Section = UStylizedLightingSettings::StaticClass()->GetPathName();
    const TPair<FName, FString> Keys[] = {
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableStylizedLumenLighting), Settings->bEnableStylizedLumenLighting ? TEXT("True") : TEXT("False")},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandCount), TEXT("2.000000")},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandSoftness), FString::Printf(TEXT("%.6f"), Safe(Settings->BandSoftness, 0.f, .49f, .18f))},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, GlossInfluence), FString::Printf(TEXT("%.6f"), Safe(Settings->GlossInfluence, 0.f, 1.f, .65f))},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularIntensity), FString::Printf(TEXT("%.6f"), Safe(Settings->SpecularIntensity, 0.f, 4.f, 1.f))},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularOffset), FString::Printf(TEXT("%.6f"), Safe(Settings->SpecularOffset, -1.f, 1.f, 0.f))},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, IndirectBlend), FString::Printf(TEXT("%.6f"), Safe(Settings->IndirectBlend, 0.f, 1.f, 1.f))},
        {GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, DirectBlend), FString::Printf(TEXT("%.6f"), Safe(Settings->DirectBlend, 0.f, 1.f, 1.f))}};
    for (const auto& Key : Keys)
        ConfigSystem->SetInSectionOfStaticLayer(*Section, Key.Key, Key.Value, Settings->GetClass()->ClassConfigName.ToString(), Path, true);
    FConfigFile Saved;
    if (!Saved.Combine(Path)) return false;
    for (const auto& Key : Keys)
    {
        FString Actual;
        if (!Saved.GetString(*Section, *Key.Key.ToString(), Actual) || Actual != Key.Value) return false;
    }
    return true;
}

bool SDevKitRenderStyleWorkbenchWidget::SavePresetTableToConfig(UYogRenderStyleSettings* Settings, const FString& Path,
    FConfigCacheIni* ConfigSystem)
{
    if (!Settings || IFileManager::Get().IsReadOnly(*Path)) return false;
    const FArrayProperty* Property = FindFProperty<FArrayProperty>(UYogRenderStyleSettings::StaticClass(), GET_MEMBER_NAME_CHECKED(UYogRenderStyleSettings, Presets));
    if (!Property) return false;
    ConfigSystem = ConfigSystem ? ConfigSystem : GConfig;
    if (!ConfigSystem) return false;
    TArray<FString> Expected;
    for (const FYogRenderStylePreset& Item : Settings->Presets)
    {
        FString Value;
        Property->Inner->ExportTextItem_Direct(Value, &Item, &Item, Settings, PPF_SerializedAsImportText);
        Expected.Add(MoveTemp(Value));
    }
    const FString Section = UYogRenderStyleSettings::StaticClass()->GetPathName();
    if (!ConfigSystem->SetArrayInSectionOfStaticLayer(*Section, Property->GetFName(), Expected, true,
        Settings->GetClass()->ClassConfigName.ToString(), Path, true)) return false;
    FConfigFile Saved;
    if (!Saved.Combine(Path)) return false;
    TArray<FString> Actual;
    Saved.GetArray(*Section, TEXT("Presets"), Actual);
    return Actual == Expected;
}

FText SDevKitRenderStyleWorkbenchWidget::GetSourceText() const
{
    return FText::FromString(TEXT("基于云端 CL149 / Engine76，本地已改为暗部二分 / 亮面连续，尚未云端发布。\n既有直接光 + Lumen Cache 受光入口，不是 GI 最终合成量化；旧关卡曝光覆盖保持退役，本页不创建或改写关卡组件。"));
}

FText SDevKitRenderStyleWorkbenchWidget::GetAppliedText() const
{
    const UStylizedLightingSettings* Settings = GetDefault<UStylizedLightingSettings>();
    float Applied[] = {Settings->bEnableStylizedLumenLighting ? 1.f : 0.f, Settings->BandCount, Settings->BandSoftness,
        Settings->GlossInfluence, Settings->SpecularIntensity, Settings->SpecularOffset, Settings->IndirectBlend, Settings->DirectBlend};
    const TCHAR* Names[] = {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
        TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.GlossInfluence"),
        TEXT("r.StylizedLumenLighting.SpecularIntensity"), TEXT("r.StylizedLumenLighting.SpecularOffset"),
        TEXT("r.StylizedLumenLighting.IndirectBlend"), TEXT("r.StylizedLumenLighting.DirectBlend")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
        if (const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Names[Index])) Applied[Index] = Variable->GetFloat();
    return FText::FromString(FString::Printf(TEXT("当前渲染参数（实时读取）：%s · 暗部固定两档 · 亮面连续 · 柔度 %.2f\n高光强度 %.2f · 高光偏移 %.2f · Cache 混合 %.2f · 直接混合 %.2f\n修改即上传；缓存会渐进刷新。不改变相机曝光、后处理、GI 方法或阴影算法。%s"),
        Applied[0] > 0.f ? TEXT("开启二分") : TEXT("原生受光对照"), Applied[2],
        Applied[4], Applied[5], Applied[6], Applied[7],
        bHasPendingChanges ? TEXT("\n当前会话已生效，尚未保存项目全局默认。") : TEXT("")));
}
