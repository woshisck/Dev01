#include "YogPlanarFogActor.h"
#include "YogFogPlanarSimulationComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

namespace PlanarFogWorkbench
{
const FName TabName(TEXT("YogPlanarFogWorkbench"));
FString Folder(){return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Rendering/Fog/Planar/Presets"));}
TSharedPtr<FJsonObject> Export(UObject* Object)
{
    auto Result=MakeShared<FJsonObject>();if(!Object)return Result;
    for(TFieldIterator<FProperty> It(Object->GetClass(),EFieldIterationFlags::None);It;++It)
    {
        FProperty* P=*It;if(!P->HasAnyPropertyFlags(CPF_Edit)||P->HasAnyPropertyFlags(CPF_Transient))continue;
        FString Value;P->ExportText_InContainer(0,Value,Object,Object,Object,PPF_None);Result->SetStringField(P->GetName(),Value);
    }
    return Result;
}
void Import(UObject* Object,const TSharedPtr<FJsonObject>& Fields)
{
    if(!Object||!Fields)return;Object->Modify();
    for(TFieldIterator<FProperty> It(Object->GetClass(),EFieldIterationFlags::None);It;++It)
    {
        FProperty* P=*It;FString Text;
        if(!P->HasAnyPropertyFlags(CPF_Edit)||P->HasAnyPropertyFlags(CPF_Transient)||!Fields->TryGetStringField(P->GetName(),Text))continue;
        P->ImportText_Direct(*Text,P->ContainerPtrToValuePtr<void>(Object),Object,PPF_None);
    }
}
class SWorkbench:public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWorkbench){} SLATE_END_ARGS()
    void Construct(const FArguments&)
    {
        FDetailsViewArgs Args;Args.bAllowSearch=true;Args.bHideSelectionTip=true;Args.bLockable=false;Args.bUpdatesFromSelection=false;
        FPropertyEditorModule& PE=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
        ActorDetails=PE.CreateDetailView(Args);SimDetails=PE.CreateDetailView(Args);
        SimDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& P)
        {return P.Property.GetFName()!=TEXT("DensityTexture")&&P.Property.GetFName()!=TEXT("DensityTileSize");}));
        ChildSlot[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("画意环境雾 · 先体积雾 / 再映射雾笔触")))]
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("选择 / 刷新Actor"))).OnClicked_Lambda([this](){FindFog(true);return FReply::Handled();})]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("刷新参数"))).OnClicked_Lambda([this](){if(auto* F=FindFog())F->RefreshPlanarFog();return FReply::Handled();})]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("重置二维流动"))).OnClicked_Lambda([this](){if(auto* F=FindFog())F->PlanarSimulation->ResetSimulation();return FReply::Handled();})]]
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("基础雾 / 无笔触"))).OnClicked_Lambda([this](){SetBrush(0);return FReply::Handled();})]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("映射笔触 / 雾散射层"))).OnClicked_Lambda([this](){SetBrush(.45f);return FReply::Handled();})]]
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("全部笔触只在Volume材质内刻画雾团转折与疏密边缘；后处理仅保留建筑方向塑形。密度图与笔刷图在Actor选择；下方二维组件调整风速、旋流和播放。PIE内调整属于运行实例。")))]
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SAssignNew(PresetName,SEditableTextBox).Text(FText::FromString(TEXT("浓雾_粗排刷")))]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("命名保存草稿"))).OnClicked_Lambda([this](){Save();return FReply::Handled();})]]
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SAssignNew(Picker,SComboBox<TSharedPtr<FString>>).OptionsSource(&Drafts)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> S){return SNew(STextBlock).Text(FText::FromString(S.IsValid()?*S:TEXT("")));})
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> S,ESelectInfo::Type){Selected=S;})
                [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Selected.IsValid()?*Selected:TEXT("选择已保存草稿"));})]]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("读取所选草稿"))).OnClicked_Lambda([this](){Load();return FReply::Handled();})]]
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this](){return FText::FromString(Message+TEXT("\n草稿目录：")+FPaths::ConvertRelativePathToFull(Folder()));})]
        +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)
            +SScrollBox::Slot()[ActorDetails.ToSharedRef()]
            +SScrollBox::Slot()[SNew(STextBlock).Text(FText::FromString(TEXT("二维流动组件（世界空间）")))]
            +SScrollBox::Slot()[SimDetails.ToSharedRef()]
            +SScrollBox::Slot()[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this](){auto* F=FindFog();return FText::FromString(F?F->GetPlanarFogStatus():TEXT("当前关卡未找到画意环境雾Actor。"));})]]];
        RefreshDrafts();FindFog(true);
    }
    void SaveNamed(const FString& Name){PresetName->SetText(FText::FromString(Name));Save();}
    void LoadNamed(const FString& Name)
    {RefreshDrafts();for(auto D:Drafts)if(*D==Name){Selected=D;Picker->SetSelectedItem(D);Load();return;}}
private:
    TWeakObjectPtr<AYogPlanarFogActor> Fog;
    TSharedPtr<IDetailsView> ActorDetails,SimDetails;
    TSharedPtr<SEditableTextBox> PresetName;
    TArray<TSharedPtr<FString>> Drafts;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> Picker;
    TSharedPtr<FString> Selected;
    FString Message;
    AYogPlanarFogActor* FindFog(bool Force=false)
    {
        UWorld* W=GEditor?(GEditor->PlayWorld?GEditor->PlayWorld.Get():GEditor->GetEditorWorldContext().World()):nullptr;
        if(!Force&&Fog.IsValid()&&Fog->GetWorld()==W)return Fog.Get();
        AYogPlanarFogActor* F=nullptr;if(W)for(TActorIterator<AYogPlanarFogActor> It(W);It;++It){F=*It;break;}
        if(F!=Fog.Get()||Force)
        {Fog=F;ActorDetails->SetObject(F);SimDetails->SetObject(F?F->PlanarSimulation.Get():nullptr);}
        if(F&&Force){GEditor->SelectNone(false,true);GEditor->SelectActor(F,true,true);}
        return F;
    }
    void SetBrush(float Strength){if(auto* F=FindFog()){F->Modify();F->BrushStrength=Strength;F->RefreshPlanarFog();ActorDetails->ForceRefresh();}}
    void RefreshDrafts()
    {
        TArray<FString> Names;IFileManager::Get().FindFiles(Names,*FPaths::Combine(Folder(),TEXT("*.json")),true,false);Names.Sort();
        Drafts.Empty();for(const auto& N:Names)Drafts.Add(MakeShared<FString>(FPaths::GetBaseFilename(N)));
        if(Picker)Picker->RefreshOptions();
    }
    void Save()
    {
        auto* F=FindFog();if(!F){Message=TEXT("未找到雾Actor，未保存。");return;}
        FString Name=FPaths::MakeValidFileName(PresetName->GetText().ToString());if(Name.IsEmpty()){Message=TEXT("请先填写草稿名称。");return;}
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("schema"),1);J->SetStringField(TEXT("name"),Name);
        J->SetStringField(TEXT("actor"),F->GetPathName());J->SetObjectField(TEXT("mist"),Export(F));J->SetObjectField(TEXT("simulation"),Export(F->PlanarSimulation));
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));IFileManager::Get().MakeDirectory(*Folder(),true);
        const FString File=FPaths::Combine(Folder(),Name+TEXT(".json"));
        Message=FFileHelper::SaveStringToFile(Text,*File)?TEXT("已保存：")+File:TEXT("保存失败：")+File;RefreshDrafts();
        for(auto S:Drafts)if(*S==Name){Selected=S;Picker->SetSelectedItem(S);break;}
    }
    void Load()
    {
        auto* F=FindFog();if(!F||!Selected){Message=TEXT("先选择雾Actor和草稿。");return;}
        FString Text;TSharedPtr<FJsonObject> J;
        const FString File=FPaths::Combine(Folder(),*Selected+TEXT(".json"));
        if(!FFileHelper::LoadFileToString(Text,*File)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J.IsValid())
        {Message=TEXT("草稿无法读取：")+File;return;}
        const TSharedPtr<FJsonObject>* Fields=nullptr;
        if(J->TryGetObjectField(TEXT("simulation"),Fields))Import(F->PlanarSimulation,*Fields);
        if(J->TryGetObjectField(TEXT("mist"),Fields))Import(F,*Fields);
        F->RefreshPlanarFog();F->PlanarSimulation->ResetSimulation();ActorDetails->ForceRefresh();SimDetails->ForceRefresh();
        PresetName->SetText(FText::FromString(*Selected));Message=TEXT("已读取：")+File;
    }
};
void Open()
{
    if(!FGlobalTabmanager::Get()->HasTabSpawner(TabName))
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName,FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
        {return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SWorkbench)];})).SetDisplayName(FText::FromString(TEXT("画意环境雾")));
    FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}
FAutoConsoleCommand Command(TEXT("Yog.PlanarFog.Open"),TEXT("Open world-space painterly fog workbench."),FConsoleCommandDelegate::CreateStatic(&Open));
void Named(const TArray<FString>& Args,bool Save)
{
    if(Args.IsEmpty())return;Open();
    if(auto Tab=FGlobalTabmanager::Get()->FindExistingLiveTab(TabName))
    {auto Widget=StaticCastSharedRef<SWorkbench>(Tab->GetContent());if(Save)Widget->SaveNamed(Args[0]);else Widget->LoadNamed(Args[0]);}
}
FAutoConsoleCommand SaveCommand(TEXT("Yog.PlanarFog.SaveDraft"),TEXT("Save named fog workbench draft."),FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& A){Named(A,true);}));
FAutoConsoleCommand LoadCommand(TEXT("Yog.PlanarFog.LoadDraft"),TEXT("Load named fog workbench draft."),FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& A){Named(A,false);}));
}
