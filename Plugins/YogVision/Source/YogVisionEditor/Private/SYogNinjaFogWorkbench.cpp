#include "Editor.h"
#include "YogNinjaFogEnvironmentActor.h"
#include "YogNinjaGlobalFogActor.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "IDetailsView.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "LevelEditorViewport.h"
#include "LevelUtils.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "TickableEditorObject.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Script.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

namespace NinjaFogWorkbench
{
namespace
{
const FName TabName(TEXT("YogNinjaFogWorkbench"));
const FName SmokeTag(TEXT("YogNinjaSmokeExact"));
const FName LegacyDisabledByGlobalTag(TEXT("YogNinjaLegacyDisabledByGlobal"));
const TCHAR* SmokeBlueprintPath = TEXT("/Game/FluidNinja/Usecases/Volumetrics/BP_VolumeSmokeContainer.BP_VolumeSmokeContainer");
const TCHAR* SmokeMaterialPath = TEXT("/Game/FluidNinja/Usecases/Volumetrics/BaseMaterials/Output/M_VolumeSmoke.M_VolumeSmoke");
const TCHAR* OwnedFogLevel = TEXT("/Game/Art/Rendering/Fog/Ninja/L_Corridor_NinjaLighting");
const TCHAR* SceneInkPath = TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_SceneInk.MI_Corridor_SceneInk");
const TCHAR* FogFinishPath = TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_FogFinish.MI_Corridor_FogFinish");
const TCHAR* OutlinePath = TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_Outline_NoFog.MI_Corridor_Outline_NoFog");
const TCHAR* ArtistOutlinePath = TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline");
const FName FogDebugParameter(TEXT("FogHatchDebugMode"));

bool IsKnownOutline(const UObject* Object)
{
    if (!Object) return false;
    const FString Path = Object->GetPathName();
    return Path == OutlinePath || Path == ArtistOutlinePath || Path == FogFinishPath;
}
// Retained only for the explicit legacy PrepareAssets console command. The
// original VolumeSmoke page never prepares or activates these FVOL assets.
const TCHAR* MaterialPath = TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_NinjaFVOL.MI_Corridor_NinjaFVOL");
const TCHAR* OwnedSystemPackage = TEXT("/Game/Art/Rendering/Fog/Ninja/NS_Corridor_NinjaFVOL");
const TCHAR* OwnedSystemPath = TEXT("/Game/Art/Rendering/Fog/Ninja/NS_Corridor_NinjaFVOL.NS_Corridor_NinjaFVOL");
const TCHAR* VendorSystemPath = TEXT("/Game/FluidNinjaLive/Volumetrics_v2/Niagara_VolumeGeneric/NS_FVOL_with_RVTsampler.NS_FVOL_with_RVTsampler");
int32 AssetsPrepared = 0;
FAutoConsoleVariableRef PreparedStatus(TEXT("Yog.NinjaFog.AssetsPrepared"), AssetsPrepared,
    TEXT("Read-only result of the last Ninja FVOL asset preparation command."), ECVF_ReadOnly);

void PrepareAssets()
{
    // Set through the CVar API: FConsoleVariableRef's game-thread getter reads
    // MainValue, so changing only the referenced integer leaves a stale result.
    IConsoleVariable* Result = IConsoleManager::Get().FindConsoleVariable(TEXT("Yog.NinjaFog.AssetsPrepared"));
    if (Result) Result->Set(0, ECVF_SetByCode);
    if (!GEditor || GEditor->PlayWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Ninja fog assets must be prepared in the editor, outside PIE."));
        return;
    }
    UMaterialInstanceConstant* Material = LoadObject<UMaterialInstanceConstant>(nullptr, MaterialPath);
    if (!Material || !Material->GetMaterial() || Material->GetMaterial()->MaterialDomain != MD_Volume)
    {
        UE_LOG(LogTemp, Error, TEXT("Prepare the owned Volume material first: %s"), MaterialPath);
        return;
    }
    UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, OwnedSystemPath);
    if (!System)
    {
        UNiagaraSystem* Vendor = LoadObject<UNiagaraSystem>(nullptr, VendorSystemPath);
        if (!Vendor)
        {
            UE_LOG(LogTemp, Error, TEXT("Missing Ninja source system: %s"), VendorSystemPath);
            return;
        }
        IAssetTools& Assets = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
        System = Cast<UNiagaraSystem>(Assets.DuplicateAsset(TEXT("NS_Corridor_NinjaFVOL"), TEXT("/Game/Art/Rendering/Fog/Ninja"), Vendor));
    }
    if (!System || System->GetOutermost()->GetName() != OwnedSystemPackage)
    {
        UE_LOG(LogTemp, Error, TEXT("Ninja fog system preparation requires an owned asset: %s"), OwnedSystemPackage);
        return;
    }
    // Inspect every target before editing any renderer; inherited vendor objects
    // must never become accidental write targets after a duplicate or merge.
    TArray<UNiagaraMeshRendererProperties*> Targets;
    for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
    {
        if (!Handle.GetIsEnabled()) continue;
        const FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData();
        if (!Data) continue;
        for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
        {
            auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer);
            if (!Mesh || !Mesh->GetIsEnabled()) continue;
            if (Mesh->GetOutermost() != System->GetOutermost())
            {
                UE_LOG(LogTemp, Error, TEXT("Refusing to edit a non-owned Ninja renderer: %s"), *Mesh->GetPathName());
                return;
            }
            Targets.Add(Mesh);
        }
    }
    if (Targets.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("No active owned FVOL mesh renderer found in %s"), OwnedSystemPath);
        return;
    }
    FProperty* Property = FindFProperty<FProperty>(UNiagaraMeshRendererProperties::StaticClass(), GET_MEMBER_NAME_CHECKED(UNiagaraMeshRendererProperties, OverrideMaterials));
    if (!Property)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot find the Niagara mesh material override property."));
        return;
    }
    const FNiagaraVariable Binding(FNiagaraTypeDefinition::GetUMaterialDef(), TEXT("User.FogMaterial"));
    System->Modify();
    System->GetExposedParameters().SetUObject(Material, Binding);
    for (UNiagaraMeshRendererProperties* Mesh : Targets)
    {
        Mesh->Modify();
        Mesh->PreEditChange(Property);
        Mesh->bOverrideMaterials = true;
        if (Mesh->OverrideMaterials.IsEmpty()) Mesh->OverrideMaterials.AddDefaulted();
        Mesh->OverrideMaterials[0].ExplicitMat = Material;
        Mesh->OverrideMaterials[0].UserParamBinding.Parameter = Binding;
        // MaterialParameters carries Ninja's extents, local pivot, camera and
        // texture bindings. Leave that complete original contract untouched.
        FPropertyChangedEvent Event(Property, EPropertyChangeType::ValueSet);
        Mesh->PostEditChangeProperty(Event);
    }
    System->MarkPackageDirty();
    if (Result) Result->Set(1, ECVF_SetByCode);
    UE_LOG(LogTemp, Display, TEXT("Prepared %s: %d active FVOL renderer(s) bind User.FogMaterial; save only this owned system."), OwnedSystemPath, Targets.Num());
}

UWorld* CurrentWorld()
{
    return GEditor ? (GEditor->PlayWorld ? GEditor->PlayWorld.Get() : GEditor->GetEditorWorldContext().World()) : nullptr;
}

bool IsOriginalSmokePropertyVisible(const FPropertyAndParent& Item)
{
    if (Item.Property.GetOwnerClass()==AYogNinjaFogEnvironmentActor::StaticClass()) return true;
    // These are the original BP properties, not a second parameter mapping.
    static const TSet<FName> Names = {
        TEXT("FlipbookPlayerMaterial"), TEXT("VolumeSmokeMaterial"),
        TEXT("TrackThisActorAsPointLight"), TEXT("TickPointLightPositionPerFrame"),
        TEXT("FlipbookFrameResolution"), TEXT("CameraFacing"), TEXT("CameraFacingLockY"),
        TEXT("RelativeLocation"), TEXT("RelativeRotation"), TEXT("RelativeScale3D")
    };
    if (Names.Contains(Item.Property.GetFName())) return true;
    for (const FProperty* Parent : Item.ParentProperties)
        if (Parent && Names.Contains(Parent->GetFName())) return true;
    return false;
}

UObject* OriginalSmokeObjectProperty(const AActor* Actor, const FName Name)
{
    if (!IsValid(Actor)) return nullptr;
    const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name);
    return Property ? Property->GetObjectPropertyValue_InContainer(Actor) : nullptr;
}

bool HasExistingFogFinish(UWorld* World, const AActor* ExcludedActor)
{
    const auto HasFinish = [](const FPostProcessSettings& Settings)
    {
        for (const FWeightedBlendable& Blend : Settings.WeightedBlendables.Array)
        {
            if (Blend.Weight <= 0.f) continue;
            const UObject* Object = Blend.Object.Get();
            // Both an authored MIC and its runtime MID count as the same
            // finish. Inspect only; never change another actor's blend list.
            for (int32 Depth = 0; Object && Depth < 8; ++Depth)
            {
                if (Object->GetPathName() == FogFinishPath) return true;
                const UMaterialInstance* Instance = Cast<UMaterialInstance>(Object);
                Object = Instance ? Instance->Parent.Get() : nullptr;
            }
        }
        return false;
    };
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor == ExcludedActor || Actor->IsActorBeingDestroyed()) continue;
        if (const APostProcessVolume* Volume = Cast<APostProcessVolume>(Actor))
            if (Volume->bEnabled && Volume->BlendWeight > 0.f && HasFinish(Volume->Settings)) return true;
        TInlineComponentArray<UPostProcessComponent*> Components(Actor);
        for (const UPostProcessComponent* Component : Components)
            if (Component && Component->bEnabled && Component->BlendWeight > 0.f
                && HasFinish(Component->Settings)) return true;
    }
    return false;
}

void CopyEnvironmentSettings(const AYogNinjaFogEnvironmentActor& Source, AYogNinjaGlobalFogActor& Target)
{
    Target.bEnvironmentEnabled = Source.bEnvironmentEnabled;
    Target.ManualGroundHeight = Source.ManualGroundHeight;
    Target.GroundBodyHeight = Source.GroundBodyHeight;
    Target.GroundAmount = Source.GroundAmount;
    Target.UpperAmount = Source.UpperAmount;
    Target.SurfaceFade = Source.SurfaceFade;
    Target.SurfaceBand = Source.SurfaceBand;
    Target.ContactBoost = Source.ContactBoost;
    Target.PlayerRadius = Source.PlayerRadius;
    Target.PlayerStrength = Source.PlayerStrength;
    Target.PlayerFeather = Source.PlayerFeather;
    Target.PlayerHeight = Source.PlayerHeight;
    Target.LightStrength = Source.LightStrength;
    Target.LightRadiusLimit = Source.LightRadiusLimit;
    Target.LightFeather = Source.LightFeather;
    Target.bUseCachedEnvironment = Source.bUseCachedEnvironment;
    Target.MaskResolution = Source.MaskResolution;
    Target.MaskHeightSlices = Source.MaskHeightSlices;
    Target.MaskUpdatesPerSecond = Source.MaskUpdatesPerSecond;
    Target.WallRange = Source.WallRange;
    // Global height is a continuous atmosphere. Do not import the optional
    // local height-cap mode which would reintroduce a visible top boundary.
    Target.bSpatialHeightEnabled = false;
}

bool SwitchSceneToGlobalFog(FString& Result)
{
    if (!GEditor || GEditor->PlayWorld || GEditor->bIsSimulatingInEditor)
    {
        Result = TEXT("请先退出 PIE / SIE，再切换当前场景的雾渲染入口。");
        return false;
    }
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World || World->WorldType != EWorldType::Editor || !World->GetCurrentLevel())
    {
        Result = TEXT("请先打开有效的编辑关卡。");
        return false;
    }
    struct FLegacySmoke { AActor* Actor; USceneComponent* DepthSlicer; };
    TArray<FLegacySmoke> Legacy;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor->IsActorBeingDestroyed() || !Actor->ActorHasTag(SmokeTag)
            || Actor->ActorHasTag(LegacyDisabledByGlobalTag) || Actor->IsHidden()
            || Actor->IsHiddenEd() || Actor->GetClass()->GetFName() != TEXT("BP_VolumeSmokeContainer_C")) continue;
        TInlineComponentArray<USceneComponent*> Components(Actor);
        for (USceneComponent* Component : Components)
            if (Component && Component->GetFName() == TEXT("DepthSlicer") && Component->IsVisible())
            {
                Legacy.Add({Actor, Component});
                break;
            }
    }
    Legacy.Sort([](const FLegacySmoke& A, const FLegacySmoke& B)
    { return A.Actor->GetPathName() < B.Actor->GetPathName(); });
    TArray<AYogNinjaFogEnvironmentActor*> Environments;
    for (TActorIterator<AYogNinjaFogEnvironmentActor> It(World); It; ++It)
    {
        AYogNinjaFogEnvironmentActor* Environment = *It;
        if (Environment->IsActorBeingDestroyed() || Environment->IsA<AYogNinjaGlobalFogActor>()) continue;
        if (Legacy.ContainsByPredicate([Environment](const FLegacySmoke& Item)
            { return Item.Actor == Environment->FogCarrier; })) Environments.Add(Environment);
    }
    Environments.Sort([](const AYogNinjaFogEnvironmentActor& A, const AYogNinjaFogEnvironmentActor& B)
    { return A.GetPathName() < B.GetPathName(); });
    AYogNinjaGlobalFogActor* Global = nullptr;
    for (TActorIterator<AYogNinjaGlobalFogActor> It(World); It; ++It)
    {
        AYogNinjaGlobalFogActor* Candidate = *It;
        if (!Candidate->bFogEnabled || Candidate->IsActorBeingDestroyed()) continue;
        if (!Global || Candidate->GlobalPriority > Global->GlobalPriority
            || (Candidate->GlobalPriority == Global->GlobalPriority && Candidate->GetPathName() < Global->GetPathName())) Global = Candidate;
    }
    ULevel* TargetLevel = Global ? Global->GetLevel()
        : (!Legacy.IsEmpty() ? Legacy[0].Actor->GetLevel() : World->GetCurrentLevel());
    // Complete the ownership/lock preflight before making any transaction edits.
    if (!TargetLevel || FLevelUtils::IsLevelLocked(TargetLevel))
    {
        Result = TEXT("全局雾所在关卡已锁定，请解锁后再切换。");
        return false;
    }
    for (const FLegacySmoke& Item : Legacy)
        if (FLevelUtils::IsLevelLocked(Item.Actor->GetLevel()))
        {
            Result = TEXT("旧 Ninja 雾所在关卡已锁定，未切换任何雾。");
            return false;
        }
    for (const AYogNinjaFogEnvironmentActor* Environment : Environments)
        if (FLevelUtils::IsLevelLocked(Environment->GetLevel()))
        {
            Result = TEXT("旧雾环境控制器所在关卡已锁定，未切换任何雾。");
            return false;
        }
    FScopedTransaction Transaction(FText::FromString(TEXT("切换当前场景到全局雾（停用旧容器）")));
    const bool bCreated = !Global;
    if (!Global)
    {
        Global = Cast<AYogNinjaGlobalFogActor>(GEditor->AddActor(TargetLevel,
            AYogNinjaGlobalFogActor::StaticClass(), FTransform::Identity, false, RF_Transactional, false));
        if (!Global)
        {
            Transaction.Cancel();
            Result = TEXT("未能创建全局雾，旧容器保持原状态。");
            return false;
        }
        Global->Modify();
        Global->SetActorLabel(TEXT("YOG 全局雾 · 连续高度衰减"));
        if (!Environments.IsEmpty()) CopyEnvironmentSettings(*Environments[0], *Global);
        Global->bEnableFinish = !HasExistingFogFinish(World, Global);
    }
    for (const FLegacySmoke& Item : Legacy)
    {
        Item.Actor->Modify();
        Item.DepthSlicer->Modify();
        Item.Actor->SetActorHiddenInGame(true);
        Item.DepthSlicer->SetVisibility(false, false);
        Item.Actor->PrimaryActorTick.bStartWithTickEnabled = false;
        Item.Actor->SetActorTickEnabled(false);
        Item.Actor->Tags.AddUnique(LegacyDisabledByGlobalTag);
    }
    for (AYogNinjaFogEnvironmentActor* Environment : Environments)
    {
        Environment->Modify();
        Environment->bEnvironmentEnabled = false;
        Environment->PrimaryActorTick.bStartWithTickEnabled = false;
        Environment->SetActorTickEnabled(false);
        Environment->Tags.AddUnique(LegacyDisabledByGlobalTag);
        Environment->RefreshEnvironment();
    }
    Global->RefreshGlobalFog();
    Global->EnableEditorPreview();
    GEditor->SelectNone(false, true);
    GEditor->SelectActor(Global, true, true);
    GEditor->RedrawLevelEditingViewports(false);
    Result = FString::Printf(TEXT("已%s全局雾，停用 %d 个旧容器及 %d 个关联环境控制器；旧Actor保留，关卡尚未保存。"),
        bCreated ? TEXT("创建并启用") : TEXT("选择已有"), Legacy.Num(), Environments.Num());
    UE_LOG(LogTemp, Display, TEXT("Global fog scene switch: %s; disabled legacy containers=%d, environments=%d; no asset save."),
        *Global->GetPathName(), Legacy.Num(), Environments.Num());
    return true;
}

void UseGlobalFog()
{
    FString Result;
    const bool bSuccess = SwitchSceneToGlobalFog(Result);
    UE_LOG(LogTemp, Display, TEXT("Yog.NinjaFog.UseGlobal %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Result);
}

TAutoConsoleVariable<int32> EditorDensityAnimation(TEXT("Yog.NinjaFog.EditorPreview"), 1,
    TEXT("Update original tagged Ninja VolumeSmoke density RTs in realtime editor viewports. 0 keeps the last density frame; PIE always uses the original BP Tick."));

class FSmokeEditorPreview : public FTickableEditorObject
{
public:
    FSmokeEditorPreview()
    {
        MapChanged = FEditorDelegates::MapChange.AddLambda([this](uint32) { bRescan = true; });
        ActorAdded = GEngine->OnLevelActorAdded().AddLambda([this](AActor*) { bRescan = true; });
        LevelAdded = FWorldDelegates::LevelAddedToWorld.AddLambda([this](ULevel*, UWorld*) { bRescan = true; });
    }

    ~FSmokeEditorPreview()
    {
        FEditorDelegates::MapChange.Remove(MapChanged);
        if (GEngine) GEngine->OnLevelActorAdded().Remove(ActorAdded);
        FWorldDelegates::LevelAddedToWorld.Remove(LevelAdded);
    }

    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FSmokeEditorPreview, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return GEditor && !IsRunningCommandlet() && !IsGarbageCollecting(); }

    void Refresh() { bRescan = true; bRefreshFrame = true; }

    virtual void Tick(float DeltaTime) override
    {
        // Do not tick gameplay, run BeginPlay, or touch the PIE copy. The vendor
        // BP remains the sole owner of its runtime update path.
        if (!GEditor || GEditor->PlayWorld || GEditor->bIsSimulatingInEditor) return;
        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        if (!EditorWorld || EditorWorld->WorldType != EWorldType::Editor || EditorWorld->bIsTearingDown) return;
        if (World.Get() != EditorWorld || bRescan)
        {
            World = EditorWorld;
            Actors.Reset();
            InitializedTargets.Reset();
            for (TActorIterator<AActor> It(EditorWorld); It; ++It)
                if (It->ActorHasTag(SmokeTag) && It->GetClass()->GetFName() == FName(TEXT("BP_VolumeSmokeContainer_C")))
                    Actors.Add(*It);
            bRescan = false;
        }
        Elapsed += DeltaTime;
        if (Elapsed < 1.f / 30.f) return;
        Elapsed = 0.f;
        bool bRealtime = false;
        for (const FLevelEditorViewportClient* Viewport : GEditor->GetLevelViewportClients())
            if (Viewport && Viewport->GetWorld() == EditorWorld && Viewport->IsRealtime()) bRealtime = true;
        const bool bAnimate = EditorDensityAnimation.GetValueOnGameThread() != 0 && bRealtime;
        bool bDrew = false;
        for (const TWeakObjectPtr<AActor>& WeakActor : Actors)
        {
            AActor* Actor = WeakActor.Get();
            if (!IsValid(Actor) || Actor->GetWorld() != EditorWorld || Actor->IsHiddenEd()
                || Actor->ActorHasTag(LegacyDisabledByGlobalTag)) continue;
            auto* Target = Cast<UTextureRenderTarget2D>(OriginalSmokeObjectProperty(Actor, TEXT("IntermedierRenderTarget")));
            auto* Input = Cast<UMaterialInterface>(OriginalSmokeObjectProperty(Actor, TEXT("FlipbookPlayerMaterial")));
            if (!IsValid(Target) || !IsValid(Input) || !Target->GetResource()) continue;
            const bool bNewTarget = !InitializedTargets.Contains(Target);
            if (bAnimate || bRefreshFrame || bNewTarget)
            {
                // This is precisely the DrawMaterialToRenderTarget operation in
                // the original ReceiveTick graph, with its original input and RT.
                UKismetRenderingLibrary::DrawMaterialToRenderTarget(Actor, Target, Input);
                InitializedTargets.Add(Target);
                bDrew = true;
            }
            const FBoolProperty* TrackLight = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("TickPointLightPositionPerFrame"));
            if (bRefreshFrame || bNewTarget || (TrackLight && TrackLight->GetPropertyValue_InContainer(Actor)))
            {
                if (UFunction* RefreshLighting = Actor->FindFunction(TEXT("RefreshLighting")))
                {
                    FEditorScriptExecutionGuard AllowEditorScript;
                    Actor->ProcessEvent(RefreshLighting, nullptr);
                }
            }
        }
        bRefreshFrame = false;
        if (bDrew) GEditor->RedrawLevelEditingViewports(false);
    }

private:
    TWeakObjectPtr<UWorld> World;
    TArray<TWeakObjectPtr<AActor>> Actors;
    TSet<TWeakObjectPtr<UTextureRenderTarget2D>> InitializedTargets;
    FDelegateHandle MapChanged, ActorAdded, LevelAdded;
    float Elapsed = 0.f;
    bool bRescan = true;
    bool bRefreshFrame = true;
};

TUniquePtr<FSmokeEditorPreview> EditorPreview;
FDelegateHandle PreviewPostEngineInit;

struct FSmokeChoice
{
    TWeakObjectPtr<AActor> Actor;
    FString Label;
};

class SWorkbench : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWorkbench) {}
    SLATE_END_ARGS()

    virtual ~SWorkbench() override
    {
        FEditorDelegates::PreSaveWorldWithContext.Remove(DebugPreSave);
        RestoreDebugPreview();
    }

    void Construct(const FArguments&)
    {
        DebugPreSave = FEditorDelegates::PreSaveWorldWithContext.AddLambda([this](UWorld*, FObjectPreSaveContext)
        {
            if (!FogDebugRestore.IsEmpty())
            {
                RestoreDebugPreview();
                Message = TEXT("保存前已退出临时雾数据预览，原合成绑定已恢复。 ");
            }
        });
        FDetailsViewArgs Args;
        Args.bAllowSearch = true;
        Args.bHideSelectionTip = true;
        Args.bLockable = false;
        Args.bUpdatesFromSelection = false;
        FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
        SmokeDetails = Properties.CreateDetailView(Args);
        SmokeDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&IsOriginalSmokePropertyVisible));

        ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(8)
            [SNew(STextBlock).Text(FText::FromString(TEXT("Ninja / 全局雾与原版 VolumeSmoke")))]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(SButton).Text(FText::FromString(TEXT("添加 / 选择全局雾")))
                .ToolTipText(FText::FromString(TEXT("在当前编辑关卡添加一个 YOG 全局体积雾，或选中已有设置；不自动保存关卡。")))
                .OnClicked_Lambda([this]() { AddOrSelectGlobalFog(); return FReply::Handled(); })]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(SButton).Text(FText::FromString(TEXT("切换当前场景到全局雾（停用旧容器）")))
                .ToolTipText(FText::FromString(TEXT("创建或选择全局雾，沿用旧雾的环境与清雾设置，并停用当前启用的已标记旧Ninja容器及其环境控制器。旧Actor保留，可撤销；不自动保存关卡。")))
                .OnClicked_Lambda([this]() { SwitchSceneToGlobalFog(Message); return FReply::Handled(); })]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("全局模式：放置一个“YOG 全局体积雾”即可覆盖画面，无需设置容器范围。在主编辑器的 Details 面板调整密度、云团、玩家与灯光退雾；也可在放置Actor面板搜索此名称。下方保留原版容器工具。")))]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("保留原版 Ninja 容器与体积步进。混合版以输入贴图的密度和速度驱动形态，少量三维细节补充纵深；距离场与清雾计算低频缓存。输入、输出实例控制美术，环境按钮控制玩家、灯光和贴地雾量。")))]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [
                SAssignNew(SmokeSelector, SComboBox<TSharedPtr<FSmokeChoice>>)
                .OptionsSource(&SmokeChoices)
                .OnGenerateWidget_Lambda([](TSharedPtr<FSmokeChoice> Item)
                { return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? Item->Label : FString())); })
                .OnSelectionChanged_Lambda([this](TSharedPtr<FSmokeChoice> Item, ESelectInfo::Type)
                {
                    Smoke = Item.IsValid() ? Item->Actor : TWeakObjectPtr<AActor>();
                    SmokeDetails->SetObject(Smoke.Get());
                    Message = Smoke.IsValid() ? TEXT("正在调整：") + Smoke->GetActorLabel() : TEXT("请选择雾体。 ");
                })
                [SNew(STextBlock).Text_Lambda([this]()
                { return FText::FromString(Smoke.IsValid() ? Smoke->GetActorLabel() : FString(TEXT("当前关卡未找到原版雾体"))); })]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).Text(FText::FromString(TEXT("应用材质 / 刷新容器"))).OnClicked_Lambda([this]() { ApplyMaterialAndRefresh(); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                [SNew(SButton).Text(FText::FromString(TEXT("选中原版容器"))).OnClicked_Lambda([this]() { SelectActor(); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).Text(FText::FromString(TEXT("查看原版 BP"))).OnClicked_Lambda([this]() { OpenReference(SmokeBlueprintPath); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                [SNew(SButton).Text(FText::FromString(TEXT("查看原版 Shader"))).OnClicked_Lambda([this]() { OpenReference(SmokeMaterialPath); return FReply::Handled(); })]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).Text(FText::FromString(TEXT("打开当前输入实例"))).OnClicked_Lambda([this]() { OpenPreset(TEXT("FlipbookPlayerMaterial")); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                [SNew(SButton).Text(FText::FromString(TEXT("打开当前输出实例"))).OnClicked_Lambda([this]() { OpenPreset(TEXT("VolumeSmokeMaterial")); return FReply::Handled(); })]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(SButton).Text(FText::FromString(TEXT("环境与清雾参数")))
                .OnClicked_Lambda([this]()
                {
                    if (UWorld* ActiveWorld=CurrentWorld())
                        for (TActorIterator<AYogNinjaFogEnvironmentActor> It(ActiveWorld);It;++It)
                            if (It->FogCarrier==Smoke.Get())
                            {
                                SmokeDetails->SetObject(*It);
                                if (GEditor) { GEditor->SelectNone(false,true);GEditor->SelectActor(*It,true,true); }
                                Message=TEXT("环境参数立即作用于当前雾体；材质中的缓存字段由此 Actor 驱动。");
                                return FReply::Handled();
                            }
                    Message=TEXT("当前雾体尚未绑定环境控制器。");
                    return FReply::Handled();
                })]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(SButton).Text(FText::FromString(TEXT("打开场景描边与笔触")))
                .OnClicked_Lambda([this]()
                {
                    OpenAsset(LoadObject<UObject>(nullptr, TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_SceneInk.MI_Corridor_SceneInk")), true);
                    return FReply::Handled();
                })]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(SButton).Text(FText::FromString(TEXT("打开雾后结构描边")))
                .OnClicked_Lambda([this]()
                {
                    OpenAsset(LoadObject<UObject>(nullptr, FogFinishPath), true);
                    return FReply::Handled();
                })]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("场景排线在后处理画面中绘制，已锁定世界参考平面，玩家或镜头移动不会带着纹样滑动；已移除细碎干刷噪点。雾内排线随云动画缓动，在当前输出实例中调整 FogHatchStrength 与 FogHatchSpacing。通过“打开雾后结构描边”调整末端结构线，让建筑轮廓在雾后仍可辨认。")))]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).Text(FText::FromString(TEXT("成图")))
                    .ToolTipText(FText::FromString(TEXT("退出临时雾数据预览，恢复原场景合成；不保存材质或关卡。")))
                    .OnClicked_Lambda([this]() { SetDebugPreview(0); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                [SNew(SButton).Text(FText::FromString(TEXT("雾遮罩")))
                    .OnClicked_Lambda([this]() { SetDebugPreview(1); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).Text(FText::FromString(TEXT("雾流向")))
                    .OnClicked_Lambda([this]() { SetDebugPreview(2); return FReply::Handled(); })]
                + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                [SNew(SButton).Text(FText::FromString(TEXT("雾受光")))
                    .OnClicked_Lambda([this]() { SetDebugPreview(3); return FReply::Handled(); })]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("雾数据预览只临时修改当前容器的动态材质与自有验证关卡的合成绑定；成图、关闭工作台或切换世界时自动恢复。遮罩为实际雾透明度，流向与受光为透明度加权显示。")))]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([]() { return EditorDensityAnimation.GetValueOnGameThread() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                .OnCheckStateChanged_Lambda([](ECheckBoxState State)
                {
                    EditorDensityAnimation->Set(State == ECheckBoxState::Checked ? 1 : 0, ECVF_SetByConsole);
                    if (State == ECheckBoxState::Checked && GEditor && !GEditor->PlayWorld)
                        for (FLevelEditorViewportClient* Viewport : GEditor->GetLevelViewportClients())
                            if (Viewport && Viewport->GetWorld() == CurrentWorld()) Viewport->SetRealtime(true);
                    if (EditorPreview) EditorPreview->Refresh();
                })
                [SNew(STextBlock).Text(FText::FromString(TEXT("输入贴图动画（仅控制二维密度帧）")))]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return StatusText(); })]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("混合版：输入实例调整二维密度/速度播放；输出实例调整 Density、Extrude、阴影采样和弱三维细节。“环境与清雾参数”调整贴地雾量、玩家/灯光退散及缓存频率。缓存字段由 Actor 更新，无需在材质里手动改。")))]
            + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
            [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("动画需要视口实时更新（Ctrl+R）；环境面板也可开启实时预览。原版 BP 播放输入密度帧，环境缓存按设定频率更新。调整后保存当前实例与关卡；Ninja 原始资源只读打开。")))]
            + SVerticalBox::Slot().FillHeight(1).Padding(8, 0)
            [
                SmokeDetails.ToSharedRef()
            ]
        ];
        RefreshObjects();
    }

    virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
        // Scanning is limited to explicit refresh or an editor/PIE world switch.
        if (World.Get() != CurrentWorld()) RefreshObjects();
    }

private:
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> Smoke;
    TSharedPtr<IDetailsView> SmokeDetails;
    TArray<TSharedPtr<FSmokeChoice>> SmokeChoices;
    TSharedPtr<SComboBox<TSharedPtr<FSmokeChoice>>> SmokeSelector;
    FString Message;
    int32 SmokeCount = 0;

    struct FFogDebugRestore
    {
        TWeakObjectPtr<UMaterialInstanceDynamic> Material;
        float Value = 0.f;
    };
    struct FPostProcessDebugRestore
    {
        TWeakObjectPtr<UObject> Owner;
        bool bEnabled = false;
    };
    struct FBlendableDebugRestore
    {
        TWeakObjectPtr<UObject> Owner;
        TStrongObjectPtr<UMaterialInterface> Original;
        TWeakObjectPtr<UMaterialInstanceDynamic> Preview;
        float Weight = 0.f;
    };
    TArray<FFogDebugRestore> FogDebugRestore;
    TArray<FPostProcessDebugRestore> PostProcessDebugRestore;
    TArray<FBlendableDebugRestore> BlendableDebugRestore;
    FDelegateHandle DebugPreSave;

    static FPostProcessSettings* DebugSettings(UObject* Owner)
    {
        if (auto* Volume = Cast<APostProcessVolume>(Owner)) return &Volume->Settings;
        if (auto* Component = Cast<UPostProcessComponent>(Owner)) return &Component->Settings;
        return nullptr;
    }

    void RestoreDebugPreview()
    {
        // Restore only objects/entries this preview changed. No asset editor
        // calls, transactions, package dirtying, or disk saves are involved.
        for (const FFogDebugRestore& Saved : FogDebugRestore)
            if (UMaterialInstanceDynamic* Material = Saved.Material.Get())
                Material->SetScalarParameterValue(FogDebugParameter, Saved.Value);
        for (const FBlendableDebugRestore& Saved : BlendableDebugRestore)
            if (FPostProcessSettings* Settings = DebugSettings(Saved.Owner.Get()))
                for (FWeightedBlendable& Blendable : Settings->WeightedBlendables.Array)
                {
                    UObject* Expected = Saved.Preview.IsValid() ? Saved.Preview.Get() : Saved.Original.Get();
                    if (Blendable.Object == Expected)
                    {
                        Blendable.Object = Saved.Original.Get();
                        Blendable.Weight = Saved.Weight;
                    }
                }
        for (const FPostProcessDebugRestore& Saved : PostProcessDebugRestore)
        {
            if (auto* Volume = Cast<APostProcessVolume>(Saved.Owner.Get())) Volume->bEnabled = Saved.bEnabled;
            if (auto* Component = Cast<UPostProcessComponent>(Saved.Owner.Get())) Component->bEnabled = Saved.bEnabled;
        }
        FogDebugRestore.Reset();
        BlendableDebugRestore.Reset();
        PostProcessDebugRestore.Reset();
        if (GEditor) GEditor->RedrawLevelEditingViewports(false);
    }

    void PreviewPostProcess(UObject* Owner)
    {
        FPostProcessSettings* Settings = DebugSettings(Owner);
        if (!Settings) return;
        bool bOutline = false;
        bool bOtherActive = false;
        for (const FWeightedBlendable& Blendable : Settings->WeightedBlendables.Array)
        {
            if (!Blendable.Object || Blendable.Weight <= 0.f) continue;
            if (IsKnownOutline(Blendable.Object.Get())) bOutline = true;
            else bOtherActive = true;
        }
        if (bOutline && !bOtherActive)
        {
            FPostProcessDebugRestore& Saved = PostProcessDebugRestore.AddDefaulted_GetRef();
            Saved.Owner = Owner;
            if (auto* Volume = Cast<APostProcessVolume>(Owner)) { Saved.bEnabled = Volume->bEnabled; Volume->bEnabled = false; }
            if (auto* Component = Cast<UPostProcessComponent>(Owner)) { Saved.bEnabled = Component->bEnabled; Component->bEnabled = false; }
        }
        for (FWeightedBlendable& Blendable : Settings->WeightedBlendables.Array)
        {
            auto* Material = Cast<UMaterialInterface>(Blendable.Object.Get());
            if (!Material) continue;
            const bool bSceneInk = Material->GetPathName() == SceneInkPath;
            const bool bMixedOutline = bOutline && bOtherActive && IsKnownOutline(Material);
            if (!bSceneInk && !bMixedOutline) continue;
            FBlendableDebugRestore& Saved = BlendableDebugRestore.AddDefaulted_GetRef();
            Saved.Owner = Owner;
            Saved.Original.Reset(Material);
            Saved.Weight = Blendable.Weight;
            if (bMixedOutline)
            {
                // Never disable the main PP when it also carries scene color
                // or the black backing pass: suppress only this owned ink.
                Blendable.Weight = 0.f;
            }
            else
            {
                UMaterialInstanceDynamic* Preview = UMaterialInstanceDynamic::Create(Material, Owner);
                Preview->SetFlags(RF_Transient);
                Preview->SetScalarParameterValue(TEXT("DebugMode"), 4.f);
                Saved.Preview = Preview;
                Blendable.Object = Preview;
            }
        }
    }

    void SetDebugPreview(int32 Mode)
    {
        // A click is an explicit one-shot scan. Existing editor-preview caches
        // and the vendor BP's runtime Tick remain unchanged.
        RestoreDebugPreview();
        if (Mode == 0)
        {
            Message = TEXT("已恢复成图与原合成绑定；未保存材质或关卡。 ");
            return;
        }
        UWorld* ActiveWorld = CurrentWorld();
        if (!ActiveWorld) return;
        for (TActorIterator<AActor> It(ActiveWorld); It; ++It)
        {
            AActor* Actor = *It;
            if (!Actor->ActorHasTag(SmokeTag)) continue;
            auto* Output = Cast<UMaterialInterface>(OriginalSmokeObjectProperty(Actor, TEXT("VolumeSmokeMaterial")));
            auto* Dynamic = Cast<UMaterialInstanceDynamic>(OriginalSmokeObjectProperty(Actor, TEXT("DynamicMaterialInstance")));
            float Original = 0.f;
            if (!Output || !Output->GetOutermost()->GetName().StartsWith(TEXT("/Game/Art/Rendering/Fog/Ninja/"))
                || !Dynamic || !Dynamic->GetScalarParameterValue(FMaterialParameterInfo(FogDebugParameter), Original)) continue;
            // Shared MIDs are recorded once so restoration cannot capture a
            // diagnostic value already written by another tagged container.
            if (FogDebugRestore.ContainsByPredicate([Dynamic](const FFogDebugRestore& Saved) { return Saved.Material.Get() == Dynamic; })) continue;
            FFogDebugRestore& Saved = FogDebugRestore.AddDefaulted_GetRef();
            Saved.Material = Dynamic;
            Saved.Value = Original;
            Dynamic->SetScalarParameterValue(FogDebugParameter, static_cast<float>(Mode));
        }
        if (FogDebugRestore.IsEmpty())
        {
            Message = TEXT("当前雾体尚无 FogHatchDebugMode 动态输出；请先构建并应用雾内排线材质。 ");
            return;
        }
        for (TActorIterator<AActor> It(ActiveWorld); It; ++It)
        {
            AActor* Actor = *It;
            if (!Actor->GetLevel() || UWorld::RemovePIEPrefix(Actor->GetLevel()->GetOutermost()->GetName()) != OwnedFogLevel) continue;
            if (auto* Volume = Cast<APostProcessVolume>(Actor)) PreviewPostProcess(Volume);
            TInlineComponentArray<UPostProcessComponent*> Components;
            Actor->GetComponents(Components);
            for (UPostProcessComponent* Component : Components) PreviewPostProcess(Component);
        }
        Message = FString::Printf(TEXT("临时雾数据预览 %d：已设置 %d 个动态输出；点“成图”恢复。未修改材质资产。"), Mode, FogDebugRestore.Num());
        if (GEditor) GEditor->RedrawLevelEditingViewports(false);
    }

    void AddOrSelectGlobalFog()
    {
        if (!GEditor || GEditor->PlayWorld || GEditor->bIsSimulatingInEditor)
        {
            Message = TEXT("请先退出 PIE / SIE，再添加或选择全局雾设置。");
            return;
        }
        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        ULevel* CurrentLevel = EditorWorld ? EditorWorld->GetCurrentLevel() : nullptr;
        if (!EditorWorld || EditorWorld->WorldType != EWorldType::Editor || !CurrentLevel)
        {
            Message = TEXT("请先打开有效的编辑关卡。");
            return;
        }
        AYogNinjaGlobalFogActor* GlobalFog = nullptr;
        for (TActorIterator<AYogNinjaGlobalFogActor> It(EditorWorld); It; ++It)
        {
            AYogNinjaGlobalFogActor* Candidate = *It;
            if (Candidate->GetLevel() != CurrentLevel || Candidate->IsActorBeingDestroyed()) continue;
            if (!GlobalFog || (Candidate->bFogEnabled && !GlobalFog->bFogEnabled)
                || (Candidate->bFogEnabled == GlobalFog->bFogEnabled
                    && (Candidate->GlobalPriority > GlobalFog->GlobalPriority
                        || (Candidate->GlobalPriority == GlobalFog->GlobalPriority
                            && Candidate->GetPathName() < GlobalFog->GetPathName())))) GlobalFog = Candidate;
        }
        bool bCreated = false;
        if (!GlobalFog)
        {
            if (FLevelUtils::IsLevelLocked(CurrentLevel))
            {
                Message = TEXT("当前关卡已锁定，请解锁后添加全局雾。");
                return;
            }
            const FScopedTransaction Transaction(FText::FromString(TEXT("添加 YOG 全局体积雾")));
            GlobalFog = Cast<AYogNinjaGlobalFogActor>(GEditor->AddActor(CurrentLevel,
                AYogNinjaGlobalFogActor::StaticClass(), FTransform::Identity, false, RF_Transactional, false));
            if (!GlobalFog)
            {
                Message = TEXT("未能在当前关卡添加全局雾。");
                return;
            }
            GlobalFog->Modify();
            GlobalFog->SetActorLabel(TEXT("YOG 全局体积雾"));
            GlobalFog->RefreshGlobalFog();
            GlobalFog->EnableEditorPreview();
            bCreated = true;
        }
        // Existing actors are only selected. Their parameters, enable state,
        // material bindings and the legacy preview actors are left untouched.
        GEditor->SelectNone(false, true);
        GEditor->SelectActor(GlobalFog, true, true);
        Message = bCreated
            ? TEXT("已添加 YOG 全局体积雾；请在主编辑器 Details 调整。关卡尚未自动保存。")
            : TEXT("已选中当前关卡的 YOG 全局体积雾；请在主编辑器 Details 调整。");
    }

    void ApplyMaterialAndRefresh()
    {
        RestoreDebugPreview();
        if (!GEditor || GEditor->PlayWorld || GEditor->bIsSimulatingInEditor)
        {
            RefreshObjects();
            Message = TEXT("运行中仅刷新当前绑定；请退出 PIE / SIE 后应用材质并保存关卡。");
            return;
        }
        AActor* Actor = Smoke.Get();
        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        if (!IsValid(Actor) || !EditorWorld || EditorWorld->WorldType != EWorldType::Editor
            || Actor->GetWorld() != EditorWorld || Actor->GetClass()->GetFName() != FName(TEXT("BP_VolumeSmokeContainer_C")))
        {
            RefreshObjects();
            Message = TEXT("请先选择编辑关卡中的原版 VolumeSmoke 容器，再应用材质。");
            return;
        }
        {
            const FScopedTransaction Transaction(FText::FromString(TEXT("应用 Ninja 雾材质与容器范围")));
            Actor->Modify();
            FEditorScriptExecutionGuard AllowEditorScript;
            // The original construction graph derives its box bounds from the
            // selected material's Extrude / ExtrudeSoften. Rebuild only the
            // chosen authoring actor, explicitly, after material edits.
            Actor->RerunConstructionScripts();
            Actor->MarkPackageDirty();
        }
        RefreshObjects();
        Message = TEXT("已应用当前雾体的材质与范围；请保存材质实例和所在关卡。");
    }

    void RefreshObjects()
    {
        if (EditorPreview) EditorPreview->Refresh();
        UWorld* ActiveWorld = CurrentWorld();
        if (World.Get() != ActiveWorld) RestoreDebugPreview();
        World = ActiveWorld;
        const TWeakObjectPtr<AActor> PreviousSmoke = Smoke;
        const FString PreviousLabel = Smoke.IsValid() ? Smoke->GetActorLabel() : FString();
        Smoke.Reset();
        SmokeChoices.Reset();
        SmokeCount = 0;
        if (ActiveWorld)
        {
            for (TActorIterator<AActor> It(ActiveWorld); It; ++It)
            {
                AActor* Actor = *It;
                if (Actor->ActorHasTag(SmokeTag) || Actor->GetClass()->GetFName() == FName(TEXT("BP_VolumeSmokeContainer_C")))
                {
                    ++SmokeCount;
                    const TSharedPtr<FSmokeChoice> Choice = MakeShared<FSmokeChoice>();
                    Choice->Actor = Actor;
                    Choice->Label = Actor->GetActorLabel();
                    SmokeChoices.Add(Choice);
                }
            }
        }
        SmokeChoices.Sort([](const TSharedPtr<FSmokeChoice>& A, const TSharedPtr<FSmokeChoice>& B)
        {
            const bool bATagged = A->Actor.IsValid() && A->Actor->ActorHasTag(SmokeTag);
            const bool bBTagged = B->Actor.IsValid() && B->Actor->ActorHasTag(SmokeTag);
            if (bATagged != bBTagged) return bATagged;
            return A->Label < B->Label;
        });
        TSharedPtr<FSmokeChoice> Selected;
        for (const TSharedPtr<FSmokeChoice>& Choice : SmokeChoices)
            if (Choice->Actor == PreviousSmoke || (!PreviousLabel.IsEmpty() && Choice->Label == PreviousLabel))
            { Selected = Choice; break; }
        if (!Selected.IsValid() && !SmokeChoices.IsEmpty()) Selected = SmokeChoices[0];
        SmokeSelector->RefreshOptions();
        if (Selected.IsValid()) SmokeSelector->SetSelectedItem(Selected);
        else
        {
            SmokeSelector->ClearSelection();
            SmokeDetails->SetObject(nullptr);
        }
        Message = Smoke.IsValid() ? TEXT("已找到原版 VolumeSmoke 容器。") : TEXT("当前关卡未找到原版 VolumeSmoke 容器。");
        if (SmokeCount > 1) Message += TEXT("可用上方下拉框切换雾体。");
    }

    void SelectActor()
    {
        AActor* Actor = Smoke.Get();
        if (!GEditor || !IsValid(Actor) || Actor->GetWorld() != CurrentWorld())
        {
            Message = TEXT("容器不可用，请先刷新当前关卡。 ");
            return;
        }
        GEditor->SelectNone(false, true);
        GEditor->SelectActor(Actor, true, true);
        SmokeDetails->SetObject(Actor);
        Message = TEXT("已选择：") + Actor->GetActorLabel();
    }

    void OpenAsset(UObject* Asset, bool bAllowOwnedEdit = false)
    {
        UAssetEditorSubsystem* Editors = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
        if (!Editors || !IsValid(Asset))
        {
            Message = TEXT("原版资源尚不可用，请先检查预设或资源是否已导入。");
            return;
        }
        const bool bOwnedInstance = bAllowOwnedEdit && Cast<UMaterialInstanceConstant>(Asset)
            && Asset->GetOutermost()->GetName().StartsWith(TEXT("/Game/Art/Rendering/Fog/Ninja/"));
        const EAssetTypeActivationOpenedMethod Method = bOwnedInstance
            ? EAssetTypeActivationOpenedMethod::Edit : EAssetTypeActivationOpenedMethod::View;
        FText Reason;
        if (!Editors->CanOpenEditorForAsset(Asset, Method, &Reason))
        {
            Message = Reason.IsEmpty() ? TEXT("当前资源编辑器无法以请求的模式打开。") : Reason.ToString();
            return;
        }
        const bool bOpened = Editors->OpenEditorForAsset(Asset, EToolkitMode::Standalone, {}, true, Method);
        Message = bOpened ? (bOwnedInstance ? TEXT("已打开场景自有实例，可编辑：") : TEXT("已只读打开：")) + Asset->GetPathName() : TEXT("未能打开资源编辑器。");
    }

    void OpenReference(const TCHAR* Path)
    {
        OpenAsset(LoadObject<UObject>(nullptr, Path));
    }

    void OpenPreset(FName Property)
    {
        OpenAsset(OriginalSmokeObjectProperty(Smoke.Get(), Property), true);
    }

    FText StatusText() const
    {
        const auto PresetName = [this](FName Property)
        { const UObject* Asset = OriginalSmokeObjectProperty(Smoke.Get(), Property); return Asset ? Asset->GetPathName() : FString(TEXT("未设置")); };
        return FText::FromString(FString::Printf(TEXT("%s\n当前容器：%s\n输入：%s\n输出：%s"), *Message,
            Smoke.IsValid() ? *Smoke->GetActorLabel() : TEXT("未找到"),
            *PresetName(TEXT("FlipbookPlayerMaterial")), *PresetName(TEXT("VolumeSmokeMaterial"))));
    }
};
}

void Open()
{
    if (!FGlobalTabmanager::Get()->HasTabSpawner(TabName))
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName, FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
        { return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SWorkbench)]; }))
        .SetDisplayName(FText::FromString(TEXT("Ninja 体积雾")));
    FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}

void Startup()
{
    if (IsRunningCommandlet()) return;
    if (GEditor && GEngine) EditorPreview = MakeUnique<FSmokeEditorPreview>();
    else PreviewPostEngineInit = FCoreDelegates::GetOnPostEngineInit().AddLambda([]()
    {
        if (GEditor && GEngine) EditorPreview = MakeUnique<FSmokeEditorPreview>();
    });
}

// Called by the editor module on shutdown, so an open tab never retains code
// or delegates from an unloaded module.
void Shutdown()
{
    FCoreDelegates::GetOnPostEngineInit().Remove(PreviewPostEngineInit);
    PreviewPostEngineInit.Reset();
    EditorPreview.Reset();
    if (const TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabName)) Tab->RequestCloseTab();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

FAutoConsoleCommand Command(TEXT("Yog.NinjaFog.Open"), TEXT("Open original Ninja VolumeSmoke container and preset controls."), FConsoleCommandDelegate::CreateStatic(&Open));
FAutoConsoleCommand PrepareCommand(TEXT("Yog.NinjaFog.PrepareAssets"), TEXT("Prepare an owned Ninja FVOL system with a working User.FogMaterial binding; source Ninja assets are untouched."), FConsoleCommandDelegate::CreateStatic(&PrepareAssets));
FAutoConsoleCommand UseGlobalCommand(TEXT("Yog.NinjaFog.UseGlobal"), TEXT("Switch the current editor world to global fog and stop its active tagged legacy Ninja containers; does not save assets."), FConsoleCommandDelegate::CreateStatic(&UseGlobalFog));
}
