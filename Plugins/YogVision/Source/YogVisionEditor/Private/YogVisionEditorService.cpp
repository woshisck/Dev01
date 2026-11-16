#include "YogVisionEditorService.h"
#include "YogVisionSettings.h"
#include "YogVisionSubsystem.h"
#include "YogVisionObserverComponent.h"
#include "YogVisionSourceComponent.h"
#include "YogVisionTargetComponent.h"
#include "YogVisionMotionProbe.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Factories/TextureFactory.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "EditorLevelUtils.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/PointLight.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/PlayerStart.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "LevelEditorViewport.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "ImageUtils.h"
#include "UnrealClient.h"

namespace
{
    const TCHAR* MaterialPackage = TEXT("/YogVision/Materials/M_VisionFog");
    template<class T> T* Node(UMaterial* M)
    {
        T* N = NewObject<T>(M);
        M->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(N);
        N->MaterialExpressionEditorX = -800;
        N->MaterialExpressionEditorY = M->GetEditorOnlyData()->ExpressionCollection.Expressions.Num() * 70;
        return N;
    }
    void Input(UMaterialExpressionCustom* C, FName Name, UMaterialExpression* E, int32 Output = 0)
    {
        auto& I = C->Inputs.AddDefaulted_GetRef(); I.InputName = Name; I.Input.Connect(Output, E);
    }
    void Scalar(UMaterial* M, UMaterialExpressionCustom* C, FName Name, float Value)
    {
        auto* N = Node<UMaterialExpressionScalarParameter>(M); N->ParameterName = Name;
        N->DefaultValue = Value; N->Group = TEXT("Vision"); Input(C, Name, N);
    }
    void Vector(UMaterial* M, UMaterialExpressionCustom* C, FName Name, FLinearColor Value)
    {
        auto* N = Node<UMaterialExpressionVectorParameter>(M); N->ParameterName = Name;
        N->DefaultValue = Value; N->Group = TEXT("Vision"); Input(C, Name, N, 0);
        // Vector parameter output 0 is RGB. Sources and echoes need the RGBA output.
        C->Inputs.Last().Input.Mask = 0;
        if (Name.ToString().StartsWith(TEXT("Source")) || Name.ToString().StartsWith(TEXT("Echo")) || Name == TEXT("Observer"))
            C->Inputs.Last().Input.Connect(5, N);
    }
    bool Save(UObject* Asset)
    {
        FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        Asset->MarkPackageDirty(); return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args);
    }
    UTexture2D* ImportStrokes(const FString& File)
    {
        const TCHAR* Path = TEXT("/YogVision/Textures/T_VisionStrokes.T_VisionStrokes");
        if (FPackageName::DoesPackageExist(TEXT("/YogVision/Textures/T_VisionStrokes")))
            if (auto* Existing = LoadObject<UTexture2D>(nullptr, Path))
            {
                if (FParse::Param(FCommandLine::Get(), TEXT("YogVisionRebuildGenerated")))
                {
                    UTexture* Asset = Existing;
                    FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Asset,1));
                    Existing->PowerOfTwoMode = ETexturePowerOfTwoSetting::StretchToPowerOfTwo;
                    Existing->MipGenSettings = TMGS_FromTextureGroup;
                    Existing->PostEditChange();
                    FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Asset,1));
                    if (!Save(Existing)) return nullptr;
                }
                return Existing;
            }
        if (!FPaths::FileExists(File)) return nullptr;
        UPackage* Package = CreatePackage(TEXT("/YogVision/Textures/T_VisionStrokes"));
        UTextureFactory* Factory = NewObject<UTextureFactory>(); Factory->SuppressImportOverwriteDialog();
        Factory->ColorSpaceMode = ETextureSourceColorSpace::Linear;
        Factory->CompressionSettings = TC_Masks;
        bool Cancelled = false;
        auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), Package, TEXT("T_VisionStrokes"), RF_Public | RF_Standalone, File, nullptr, GWarn, Cancelled));
        if (!Texture) return nullptr;
        UTexture* Imported = Texture;
        FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Imported, 1));
        Texture->SRGB = false; Texture->CompressionSettings = TC_Masks;
        Texture->AddressX = TA_Wrap; Texture->AddressY = TA_Wrap;
        Texture->PowerOfTwoMode = ETexturePowerOfTwoSetting::StretchToPowerOfTwo;
        Texture->MipGenSettings = TMGS_FromTextureGroup;
        Texture->LODGroup = TEXTUREGROUP_World; Texture->PostEditChange();
        FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Imported, 1));
        FAssetRegistryModule::AssetCreated(Texture);
        return Save(Texture) ? Texture : nullptr;
    }
    template<class T> T* AddComponent(AActor* Actor, FName Name)
    {
        T* C = NewObject<T>(Actor, Name, RF_Transactional);
        if constexpr (TIsDerivedFrom<T, USceneComponent>::IsDerived) C->SetupAttachment(Actor->GetRootComponent());
        Actor->AddInstanceComponent(C); C->RegisterComponent(); return C;
    }
    AStaticMeshActor* Cube(UWorld* W, UStaticMesh* Mesh, FVector P, FVector Scale, const TCHAR* Label)
    {
        auto* A = W->SpawnActor<AStaticMeshActor>(P, FRotator::ZeroRotator);
        A->SetActorLabel(Label); A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        A->GetStaticMeshComponent()->SetStaticMesh(Mesh); A->SetActorScale3D(Scale); return A;
    }
}

bool FYogVisionEditorService::BuildAssets(FText& Message)
{
    auto Plugin = IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    FString Code;
    if (!Plugin || !FFileHelper::LoadFileToString(Code, *(Plugin->GetBaseDir() / TEXT("Resources/VisionFog.hlsl"))))
    { Message = FText::FromString(TEXT("找不到视野迷雾材质源文件。")); return false; }
    UTexture2D* Strokes = ImportStrokes(Plugin->GetBaseDir() / TEXT("Resources/T_VisionStrokes.png"));
    if (!Strokes) { Message = FText::FromString(TEXT("笔触纹理导入失败。")); return false; }
    UMaterial* M = FPackageName::DoesPackageExist(MaterialPackage)
        ? LoadObject<UMaterial>(nullptr, TEXT("/YogVision/Materials/M_VisionFog.M_VisionFog")) : nullptr;
    if (M)
    {
        if (!FParse::Param(FCommandLine::Get(), TEXT("YogVisionRebuildGenerated")))
        {
            Message = FText::FromString(TEXT("基础资产已存在，保留当前美术调整。"));
            return true;
        }
        // Rebuilding is deliberate, but never replace an artist's modified material graph.
        bool Owned = false;
        for (UMaterialExpression* E : M->GetEditorOnlyData()->ExpressionCollection.Expressions)
            if (auto* C = Cast<UMaterialExpressionCustom>(E)) Owned |= C->Description == TEXT("YogVision.Generated.v1");
        if (!Owned) { Message = FText::FromString(TEXT("现有材质已由美术修改；请保留并创建新的版本。")); return false; }
        M->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    }
    else
    {
        M = NewObject<UMaterial>(CreatePackage(MaterialPackage), TEXT("M_VisionFog"), RF_Public | RF_Standalone | RF_Transactional);
        FAssetRegistryModule::AssetCreated(M);
    }
    M->MaterialDomain = MD_PostProcess; M->BlendableLocation = BL_SceneColorAfterTonemapping; M->BlendablePriority = 50;
    auto* C = Node<UMaterialExpressionCustom>(M); C->Inputs.Empty(); C->Code = Code;
    C->Description = TEXT("YogVision.Generated.v1"); C->OutputType = CMOT_Float3;
    C->MaterialExpressionEditorX = 100;
    auto* Color = Node<UMaterialExpressionSceneTexture>(M); Color->SceneTextureId = PPI_PostProcessInput0; Input(C, TEXT("SceneColor"), Color);
    auto* Depth = Node<UMaterialExpressionSceneTexture>(M); Depth->SceneTextureId = PPI_SceneDepth; Input(C, TEXT("SceneDepth"), Depth);
    C->Inputs.Last().Input.Mask = 1; C->Inputs.Last().Input.MaskR = 1;
    C->Inputs.Last().Input.MaskG = 0; C->Inputs.Last().Input.MaskB = 0; C->Inputs.Last().Input.MaskA = 0;
    auto* Normal = Node<UMaterialExpressionSceneTexture>(M); Normal->SceneTextureId = PPI_WorldNormal; Input(C, TEXT("SceneNormal"), Normal);
    Input(C, TEXT("WorldPosition"), Node<UMaterialExpressionWorldPosition>(M));
    Input(C, TEXT("CameraPosition"), Node<UMaterialExpressionCameraPositionWS>(M));
    FYogVisionParameters Defaults;
    for (TFieldIterator<FProperty> It(FYogVisionParameters::StaticStruct()); It; ++It)
    {
        if (auto* Float = CastField<FFloatProperty>(*It)) Scalar(M, C, It->GetFName(), Float->GetPropertyValue_InContainer(&Defaults));
        else if (auto* Struct = CastField<FStructProperty>(*It); Struct && Struct->Struct == TBaseStructure<FLinearColor>::Get())
            Vector(M, C, It->GetFName(), *Struct->ContainerPtrToValuePtr<FLinearColor>(&Defaults));
    }
    Scalar(M, C, TEXT("TimeSeconds"), 0); Scalar(M, C, TEXT("SourceCount"), 0); Scalar(M, C, TEXT("EchoCount"), 0);
    Scalar(M, C, TEXT("PresentationFade"), 1);
    Vector(M, C, TEXT("Observer"), FLinearColor(0,0,100,420));
    for (int32 I = 0; I < 4; ++I)
    { Vector(M, C, *FString::Printf(TEXT("Source%d"), I), FLinearColor(0,0,100,0)); Scalar(M, C, *FString::Printf(TEXT("SourceStrength%d"), I), 1); }
    for (int32 I = 0; I < 8; ++I)
    { Vector(M, C, *FString::Printf(TEXT("Echo%d"), I), FLinearColor(0,0,0,0)); Vector(M, C, *FString::Printf(TEXT("EchoVelocity%d"), I), FLinearColor(0,0,55,0)); }
    for (int32 I = 0; I < 2; ++I)
    {
        auto* T = Node<UMaterialExpressionTextureObjectParameter>(M); T->ParameterName = I ? TEXT("StrokeTexture") : TEXT("VisibilityTexture");
        // Both defaults are linear masks. Runtime replaces VisibilityTexture with its private 64x4 fan.
        // SourceCount is zero until that fan is uploaded, so the authored fallback is never used for visibility.
        T->Texture = Strokes;
        T->SamplerType = SAMPLERTYPE_Masks; T->Group = TEXT("Vision"); Input(C, T->ParameterName, T);
    }
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,C); M->PostEditChange();
    bool Result = Save(M);
    Message = FText::FromString(Result ? TEXT("已创建视野迷雾材质和笔触纹理。") : TEXT("视野迷雾资产保存失败。"));
    UE_LOG(LogTemp, Display, TEXT("YOG_VISION_ASSETS success=%d material=%s"), Result, *M->GetPathName()); return Result;
}

bool FYogVisionEditorService::CreateValidationStage(FText& Message)
{
    TArray<UPackage*> DirtyWorlds;
    FEditorFileUtils::GetDirtyWorldPackages(DirtyWorlds);
    if (!GEditor || GEditor->PlayWorld || DirtyWorlds.Num() > 0)
    { Message = FText::FromString(TEXT("请先保存当前关卡，再打开视野验证场景。")); return false; }
    // This creates an unsaved review world, so no existing map or plugin asset is overwritten.
    UWorld* W = GEditor->NewMap();
    if (!W) return false;
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Mesh) return false;
    Cube(W, Mesh, FVector(0,0,-25), FVector(35,25,.5), TEXT("视野验证 / 地面"));
    Cube(W, Mesh, FVector(650,180,150), FVector(.5,13,3), TEXT("视野验证 / 遮挡墙"));
    Cube(W, Mesh, FVector(-900,-550,100), FVector(4,.8,2), TEXT("视野验证 / 结构 A"));
    Cube(W, Mesh, FVector(1300,650,100), FVector(4,.8,2), TEXT("视野验证 / 结构 B"));
    auto* Observer = W->SpawnActor<APlayerStart>(FVector(-150,0,110), FRotator::ZeroRotator);
    Observer->SetActorLabel(TEXT("视野验证 / 玩家观察者")); AddComponent<UYogVisionObserverComponent>(Observer, TEXT("VisionObserver"));
    auto* Enemy = W->SpawnActor<AYogVisionMotionProbe>(FVector(1050,220,75), FRotator::ZeroRotator);
    Enemy->SetActorLabel(TEXT("视野验证 / 墙后移动回响"));
    UStaticMesh* Character = LoadObject<UStaticMesh>(nullptr, TEXT("/YogArt/Character/Enemy/Elite/CellCaptain/SM_CellCaptain_01.SM_CellCaptain_01"));
    if (Character)
    {
        auto* PlayerSample = W->SpawnActor<AStaticMeshActor>(FVector(-150,0,0), FRotator(0,-90,0));
        PlayerSample->SetActorLabel(TEXT("视野验证 / 玩家可读性样本"));
        auto* Body = PlayerSample->GetStaticMeshComponent(); Body->SetMobility(EComponentMobility::Movable);
        Body->SetStaticMesh(Character); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->SetCustomPrimitiveDataFloat(34,1.f); Body->SetCustomPrimitiveDataFloat(35,1.f);
    }
    Enemy->Mesh->SetStaticMesh(Character ? Character : Mesh);
    if (!Character) Enemy->SetActorScale3D(FVector(.55,.55,1.5));
    Enemy->Tags.Add(TEXT("YogVision.Validation.Moving"));
    auto* Torch = W->SpawnActor<APointLight>(FVector(-400,-300,150), FRotator::ZeroRotator);
    auto* Lamp = CastChecked<UPointLightComponent>(Torch->GetLightComponent());
    Lamp->SetMobility(EComponentMobility::Movable);
    Torch->SetActorLabel(TEXT("视野验证 / 火把")); Lamp->SetIntensity(4000.f);
    Lamp->SetAttenuationRadius(850.f); Lamp->SetLightColor(FLinearColor(1.f,.5f,.15f));
    auto* TorchSource = AddComponent<UYogVisionSourceComponent>(Torch, TEXT("VisionSource"));
    TorchSource->BindLight(Lamp);
    auto* Sun = W->SpawnActor<ADirectionalLight>(FVector(0,0,800), FRotator(-50,-35,0));
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->SetActorLabel(TEXT("视野验证 / 微弱环境光")); Sun->GetLightComponent()->SetIntensity(.8f);
    auto* Exposure = W->SpawnActor<APostProcessVolume>(); Exposure->bUnbound = true;
    Exposure->Settings.bOverride_AutoExposureMethod = true; Exposure->Settings.AutoExposureMethod = AEM_Manual;
    Exposure->Settings.bOverride_AutoExposureBias = true; Exposure->Settings.AutoExposureBias = 0;
    Exposure->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Exposure->Settings.AutoExposureApplyPhysicalCameraExposure = false;
    if (GCurrentLevelEditingViewportClient)
    {
        FVector P(0,0,0), Cam(-1700,-1800,2350);
        GCurrentLevelEditingViewportClient->SetViewLocation(Cam); GCurrentLevelEditingViewportClient->SetViewRotation((P-Cam).Rotation());
        GCurrentLevelEditingViewportClient->SetViewMode(VMI_Lit);
        GCurrentLevelEditingViewportClient->ExposureSettings.bFixed = false;
        GCurrentLevelEditingViewportClient->EngineShowFlags.SetEyeAdaptation(true);
        GCurrentLevelEditingViewportClient->SetRealtime(true); GCurrentLevelEditingViewportClient->SetGameView(true);
    }
    auto* Vision = W->GetSubsystem<UYogVisionSubsystem>();
    Vision->SetPreviewObserver(Observer); Vision->SetEnabled(true);
    Message = FText::FromString(TEXT("已打开未保存的视野验证场景；可移动火把、观察者和墙后目标。")); return true;
}

bool FYogVisionEditorService::AuditScene(FText& Message)
{
    UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!W) return false;
    auto Root = MakeShared<FJsonObject>(); Root->SetStringField(TEXT("world"), W->GetPathName());
    TArray<TSharedPtr<FJsonValue>> Volumes;
    for (TActorIterator<APostProcessVolume> It(W); It; ++It)
    {
        auto V = MakeShared<FJsonObject>(); V->SetStringField(TEXT("actor"), It->GetPathName());
        V->SetNumberField(TEXT("priority"), It->Priority); V->SetNumberField(TEXT("weight"), It->BlendWeight);
        V->SetBoolField(TEXT("enabled"), It->bEnabled); V->SetBoolField(TEXT("unbound"), It->bUnbound);
        TArray<TSharedPtr<FJsonValue>> Blends;
        for (const FWeightedBlendable& B : It->Settings.WeightedBlendables.Array)
        {
            auto O = MakeShared<FJsonObject>(); O->SetStringField(TEXT("object"), GetPathNameSafe(B.Object)); O->SetNumberField(TEXT("weight"), B.Weight);
            if (auto* M = Cast<UMaterialInterface>(B.Object))
            {
                O->SetNumberField(TEXT("location"), M->GetBlendableLocation(M->GetMaterial()));
                TArray<FMaterialParameterInfo> Infos; TArray<FGuid> Ids; M->GetAllScalarParameterInfo(Infos, Ids);
                auto Params = MakeShared<FJsonObject>();
                for (const auto& Info : Infos) { float Val = 0; if (M->GetScalarParameterValue(Info, Val)) Params->SetNumberField(Info.Name.ToString(), Val); }
                O->SetObjectField(TEXT("scalars"), Params);
            }
            Blends.Add(MakeShared<FJsonValueObject>(O));
        }
        V->SetArrayField(TEXT("blendables"), Blends); Volumes.Add(MakeShared<FJsonValueObject>(V));
    }
    Root->SetArrayField(TEXT("volumes"), Volumes); FString Json;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
    FString Filename = FPaths::ProjectSavedDir() / TEXT("VisionFog/ScenePostProcessAudit.json");
    bool Result = FFileHelper::SaveStringToFile(Json, *Filename);
    Message = FText::FromString(Result ? TEXT("场景后处理检查已保存。") : TEXT("场景检查保存失败。")); return Result;
}

bool FYogVisionEditorService::CaptureCurrentView(const FString& Name)
{
    if (!GCurrentLevelEditingViewportClient || !GCurrentLevelEditingViewportClient->Viewport) return false;
    FViewport* View = GCurrentLevelEditingViewportClient->Viewport;
    View->Draw(false);
    TArray<FColor> Pixels; const FIntPoint Size = View->GetSizeXY();
    if (Size.GetMin() < 32 || !View->ReadPixels(Pixels)) return false;
    TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
    FString File = FPaths::ProjectSavedDir() / TEXT("VisionFog/Captures") / (FPaths::GetBaseFilename(Name)+TEXT(".png"));
    bool Result = FFileHelper::SaveArrayToFile(PNG,*File);
    UE_LOG(LogTemp,Display,TEXT("VISION_SCENE_CAPTURE success=%d file=%s"),Result,*File);return Result;
}
