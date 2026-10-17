#include "LookDev/YogLookDevMigrateCommandlet.h"

#include "LookDev/YogLookDevLightingRig.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/SceneComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/LevelEditorViewportSettings.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogYogLookDevMigration, Log, All);

namespace
{
const FString SourceRoot(TEXT("/Game/Art/Map/LookDev"));
const FString TargetRoot(TEXT("/YogLookDev"));
const FString BlueprintSuffix(TEXT("/Assets/AssetsPreview_BP.AssetsPreview_BP"));
const FString NeutralSuffix(TEXT("/LightDev/LookDev_Light_Neutral.LookDev_Light_Neutral"));

bool IsWithin(const FString& Package, const FString& Root)
{
    return Package == Root || Package.StartsWith(Root + TEXT("/"));
}

FString NewPackagePath(const FString& OldPackage)
{
    return TargetRoot + OldPackage.RightChop(SourceRoot.Len());
}

FString FileHash(const FString& Filename)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Filename)) return FString();
    FSHAHash Hash;
    FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
    return Hash.ToString();
}

bool SaveJson(const FString& Filename, const TSharedRef<FJsonObject>& Object)
{
    FString Text;
    return FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text))
        && FFileHelper::SaveStringToFile(Text, *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool IsRedirectorPackage(IAssetRegistry& Registry, FName Package)
{
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPackageName(Package, Assets, true);
    return !Assets.IsEmpty() && !Assets.ContainsByPredicate([](const FAssetData& Asset) { return !Asset.IsRedirector(); });
}

bool GatherSource(IAssetRegistry& Registry, int32 ExpectedCount, TArray<FAssetData>& Assets,
    TMap<FString, FString>& OriginalFiles, TArray<FString>& ExternalRedirectors)
{
    Registry.GetAssetsByPath(FName(*SourceRoot), Assets, true, true);
    Assets.RemoveAll([](const FAssetData& Asset) { return Asset.IsRedirector(); });
    Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
    if (Assets.Num() != ExpectedCount)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("Expected %d source assets, found %d. No migration was started."), ExpectedCount, Assets.Num());
        return false;
    }
    bool bValid = true;
    for (const FAssetData& Asset : Assets)
    {
        const FString OldPackage = Asset.PackageName.ToString();
        const FString NewPackage = NewPackagePath(OldPackage);
        FString OriginalFile;
        if (OriginalFiles.Contains(OldPackage) || !FPackageName::DoesPackageExist(OldPackage, &OriginalFile))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Source package is missing or contains more than one top-level asset: %s"), *OldPackage);
            bValid = false;
            continue;
        }
        OriginalFiles.Add(OldPackage, OriginalFile);
        if (FPackageName::DoesPackageExist(NewPackage) || FindPackage(nullptr, *NewPackage))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Target already exists: %s. Use -VerifyOnly for an already completed migration."), *NewPackage);
            bValid = false;
        }
        UE_LOG(LogYogLookDevMigration, Display, TEXT("PLAN %s -> %s"), *Asset.GetSoftObjectPath().ToString(), *NewPackage);
        TArray<FName> Referencers;
        Registry.GetReferencers(Asset.PackageName, Referencers);
        for (FName Referencer : Referencers)
        {
            if (IsWithin(Referencer.ToString(), SourceRoot)) continue;
            if (IsRedirectorPackage(Registry, Referencer))
            {
                ExternalRedirectors.AddUnique(Referencer.ToString());
                UE_LOG(LogYogLookDevMigration, Display, TEXT("EXTERNAL_REDIRECTOR_ALLOWED %s -> %s"), *Referencer.ToString(), *OldPackage);
                continue;
            }
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Unexpected external referencer %s -> %s. Review it explicitly; this commandlet does not migrate external assets."), *Referencer.ToString(), *OldPackage);
            bValid = false;
        }
    }
    return bValid;
}

bool BackupSources(const TMap<FString, FString>& OriginalFiles, const TArray<FString>& ExternalRedirectors, FString& OutDirectory)
{
    OutDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("LookDevMigration")
        / (FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    if (IFileManager::Get().DirectoryExists(*OutDirectory) || !IFileManager::Get().MakeDirectory(*OutDirectory, true)) return false;
    const FString SourceDirectory = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(SourceRoot));
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *SourceDirectory, TEXT("*"), true, false);
    for (const FString& Redirector : ExternalRedirectors)
    {
        FString Filename;
        if (FPackageName::DoesPackageExist(Redirector, &Filename)) Files.AddUnique(FPaths::ConvertRelativePathToFull(Filename));
    }
    Files.Sort();
    TArray<TSharedPtr<FJsonValue>> ManifestFiles;
    for (const FString& File : Files)
    {
        FString Relative = FPaths::ConvertRelativePathToFull(File);
        FPaths::MakePathRelativeTo(Relative, *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
        if (Relative.StartsWith(TEXT("../")))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Backup source is outside project: %s"), *File);
            return false;
        }
        const FString Backup = OutDirectory / TEXT("Files") / Relative;
        if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true)
            || IFileManager::Get().Copy(*Backup, *File, false, true) != COPY_OK)
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Backup failed for %s"), *File);
            return false;
        }
        const FString SHA1 = FileHash(File);
        if (SHA1.IsEmpty() || SHA1 != FileHash(Backup))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Backup checksum mismatch: %s"), *File);
            return false;
        }
        TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("source"), FPaths::ConvertRelativePathToFull(File));
        Entry->SetStringField(TEXT("backup"), Backup);
        Entry->SetStringField(TEXT("sha1"), SHA1);
        ManifestFiles.Add(MakeShared<FJsonValueObject>(Entry));
    }
    TSharedRef<FJsonObject> Manifest = MakeShared<FJsonObject>();
    Manifest->SetStringField(TEXT("source_root"), SourceRoot);
    Manifest->SetStringField(TEXT("target_root"), TargetRoot);
    Manifest->SetNumberField(TEXT("package_count"), OriginalFiles.Num());
    Manifest->SetArrayField(TEXT("files"), ManifestFiles);
    if (!SaveJson(OutDirectory / TEXT("backup-manifest.json"), Manifest)) return false;
    UE_LOG(LogYogLookDevMigration, Display, TEXT("BACKUP_COMPLETE=%s files=%d"), *OutDirectory, Files.Num());
    return true;
}

struct FComponentSnapshot
{
    FName Name;
    FTransform RelativeTransform;
    TStrongObjectPtr<UStaticMesh> StaticMesh;
    TArray<TStrongObjectPtr<UMaterialInterface>> Materials;
    TOptional<FPostProcessSettings> PostProcessSettings;
    TOptional<float> SourceCubemapAngle;
};

bool SnapshotNeutral(UBlueprint* Blueprint, UWorld* Neutral, TArray<FComponentSnapshot>& OutComponents)
{
    if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->SimpleConstructionScript || !Neutral || !Neutral->PersistentLevel) return false;
    AActor* Rig = nullptr;
    for (AActor* Actor : Neutral->PersistentLevel->Actors)
    {
        if (!Actor || !Actor->IsA(Blueprint->GeneratedClass)) continue;
        if (Rig)
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Neutral map has multiple AssetsPreview_BP actors; cannot choose an authoritative calibration layout."));
            return false;
        }
        Rig = Actor;
    }
    if (!Rig)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("Neutral map has no serialized AssetsPreview_BP actor."));
        return false;
    }
    TInlineComponentArray<USceneComponent*> Components;
    Rig->GetComponents(Components);
    for (USceneComponent* Component : Components)
    {
        if (!Component) continue;
        FComponentSnapshot& Snapshot = OutComponents.AddDefaulted_GetRef();
        Snapshot.Name = Component->GetFName();
        Snapshot.RelativeTransform = Component->GetRelativeTransform();
        if (const UPostProcessComponent* Post = Cast<UPostProcessComponent>(Component))
        {
            Snapshot.PostProcessSettings = Post->Settings;
            for (FWeightedBlendable& Blendable : Snapshot.PostProcessSettings.GetValue().WeightedBlendables.Array)
            {
                // The original checker owns a MID inside the neutral-map actor.
                // A Blueprint SCS template must not reference that private object
                // across packages, even when the checker component is disabled.
                UObject* Object = Blendable.Object.Get();
                while (UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Object)) Object = Dynamic->Parent;
                Blendable.Object = Object;
            }
        }
        if (const USkyLightComponent* Sky = Cast<USkyLightComponent>(Component)) Snapshot.SourceCubemapAngle = Sky->SourceCubemapAngle;
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
        {
            Snapshot.StaticMesh.Reset(Mesh->GetStaticMesh());
            for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
            {
                UMaterialInterface* Material = Mesh->GetMaterial(Slot);
                // Instance-generated MIDs belong to the old actor. Preserve their
                // persistent parent; native lighting reapplies only the three backdrop scalars.
                while (UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Material)) Material = Dynamic->Parent;
                Snapshot.Materials.Emplace(Material);
            }
        }
        UE_LOG(LogYogLookDevMigration, Display, TEXT("SNAPSHOT component=%s transform=%s"), *Snapshot.Name.ToString(), *Snapshot.RelativeTransform.ToHumanReadableString());
    }
    return OutComponents.ContainsByPredicate([](const auto& Item) { return Item.Name == TEXT("Skydome"); });
}

void RestoreComponent(USceneComponent* Component, const FComponentSnapshot& Snapshot)
{
    Component->SetRelativeTransform(Snapshot.RelativeTransform);
    if (UPostProcessComponent* Post = Cast<UPostProcessComponent>(Component))
        if (Snapshot.PostProcessSettings.IsSet()) Post->Settings = Snapshot.PostProcessSettings.GetValue();
    if (USkyLightComponent* Sky = Cast<USkyLightComponent>(Component))
        if (Snapshot.SourceCubemapAngle.IsSet()) Sky->SourceCubemapAngle = Snapshot.SourceCubemapAngle.GetValue();
    if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
    {
        Mesh->SetStaticMesh(Snapshot.StaticMesh.Get());
        for (int32 Slot = 0; Slot < Snapshot.Materials.Num(); ++Slot) Mesh->SetMaterial(Slot, Snapshot.Materials[Slot].Get());
    }
}

bool SaveSnapshotReport(const FString& Directory, const TArray<FComponentSnapshot>& Snapshots)
{
    TArray<TSharedPtr<FJsonValue>> Components;
    for (const FComponentSnapshot& Snapshot : Snapshots)
    {
        TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"), Snapshot.Name.ToString());
        Entry->SetStringField(TEXT("relative_transform"), Snapshot.RelativeTransform.ToString());
        Entry->SetStringField(TEXT("static_mesh"), GetPathNameSafe(Snapshot.StaticMesh.Get()));
        if (Snapshot.PostProcessSettings.IsSet()) Entry->SetNumberField(TEXT("vignette_intensity"), Snapshot.PostProcessSettings.GetValue().VignetteIntensity);
        if (Snapshot.SourceCubemapAngle.IsSet()) Entry->SetNumberField(TEXT("source_cubemap_angle"), Snapshot.SourceCubemapAngle.GetValue());
        TArray<TSharedPtr<FJsonValue>> Materials;
        for (const auto& Material : Snapshot.Materials) Materials.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Material.Get())));
        Entry->SetArrayField(TEXT("persistent_materials"), Materials);
        Components.Add(MakeShared<FJsonValueObject>(Entry));
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("source_map"), SourceRoot + NeutralSuffix);
    Report->SetStringField(TEXT("source_blueprint"), SourceRoot + BlueprintSuffix);
    Report->SetArrayField(TEXT("components"), Components);
    return SaveJson(Directory / TEXT("neutral-component-snapshot.json"), Report);
}

void ApplyNativeDefaults(AYogLookDevLightingRig* Rig)
{
    Rig->LightingScenario = EYogLookDevLightingScenario::MidContrast;
    Rig->BackgroundCompensation = 0.f;
    Rig->ExposureCompensation = 0.f;
    Rig->bGrounded = true;
    Rig->bCalibrator = true;
    Rig->DirectionalSettings = FYogLookDevDirectionalSettings();
    Rig->SkySettings = FYogLookDevSkySettings();
}

void AuditBlueprint(UBlueprint* Blueprint)
{
    UE_LOG(LogYogLookDevMigration, Display, TEXT("BLUEPRINT_AUDIT=%s parent=%s variables=%d"), *Blueprint->GetPathName(),
        *GetPathNameSafe(Blueprint->ParentClass), Blueprint->NewVariables.Num());
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
        UE_LOG(LogYogLookDevMigration, Display, TEXT("REMOVE_VARIABLE name=%s default=%s"), *Variable.VarName.ToString(), *Variable.DefaultValue);
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
        if (Graph) UE_LOG(LogYogLookDevMigration, Display, TEXT("REMOVE_GRAPH name=%s nodes=%d"), *Graph->GetName(), Graph->Nodes.Num());
}

bool ReparentAndCleanBlueprint(UBlueprint* Blueprint, const TArray<FComponentSnapshot>& Snapshots)
{
    const int32 OriginalSCSCount = Blueprint->SimpleConstructionScript->GetAllNodes().Num();
    Blueprint->Modify();
    TArray<UEdGraph*> Graphs;
    for (UEdGraph* Graph : Blueprint->UbergraphPages) Graphs.Add(Graph);
    for (UEdGraph* Graph : Blueprint->FunctionGraphs) Graphs.Add(Graph);
    for (UEdGraph* Graph : Blueprint->MacroGraphs) Graphs.Add(Graph);
    for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs) Graphs.Add(Graph);
    for (UEdGraph* Graph : Graphs) if (Graph) FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph, EGraphRemoveFlags::MarkTransient);
    Blueprint->NewVariables.Reset();
    Blueprint->ParentClass = AYogLookDevLightingRig::StaticClass();
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (!Node) continue;
        const FComponentSnapshot* Snapshot = Snapshots.FindByPredicate([Node](const auto& Item) { return Item.Name == Node->GetVariableName(); });
        if (Snapshot)
            if (USceneComponent* Template = Cast<USceneComponent>(Node->ComponentTemplate)) RestoreComponent(Template, *Snapshot);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Results);
    if (Results.NumErrors > 0 || Blueprint->Status == BS_Error || !Blueprint->GeneratedClass
        || !Blueprint->GeneratedClass->IsChildOf(AYogLookDevLightingRig::StaticClass()))
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("Native LookDev Blueprint compile failed (%d errors). Use the backup; migration has not been reported as complete."), Results.NumErrors);
        return false;
    }
    if (Blueprint->SimpleConstructionScript->GetAllNodes().Num() != OriginalSCSCount)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("Blueprint SCS component count changed during reparent."));
        return false;
    }
    ApplyNativeDefaults(CastChecked<AYogLookDevLightingRig>(Blueprint->GeneratedClass->GetDefaultObject()));
    Blueprint->MarkPackageDirty();
    return true;
}

bool SaveAsset(UObject* Asset, const FString& Filename = FString())
{
    UPackage* Package = Asset->GetOutermost();
    const FString TargetFilename = Filename.IsEmpty() ? FPackageName::LongPackageNameToFilename(Package->GetName(),
        Asset->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension()) : Filename;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(TargetFilename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    Args.bSlowTask = false;
    if (!UPackage::SavePackage(Package, Asset, *TargetFilename, Args))
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("Failed to save %s"), *TargetFilename);
        return false;
    }
    Package->SetDirtyFlag(false);
    return true;
}

bool IsEmptyConstructionGraph(const UEdGraph* Graph)
{
    if (!Graph || Graph->GetFName() != TEXT("UserConstructionScript")) return false;
    for (const UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || Node->GetClass()->GetFName() != TEXT("K2Node_FunctionEntry")) return false;
        for (const UEdGraphPin* Pin : Node->Pins)
            if (Pin && !Pin->LinkedTo.IsEmpty()) return false;
    }
    return true;
}

bool Verify(IAssetRegistry& Registry, int32 ExpectedCount)
{
    Registry.ScanPathsSynchronous({ TargetRoot, SourceRoot }, true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(*TargetRoot), Assets, true, true);
    Assets.RemoveAll([](const FAssetData& Asset) { return Asset.IsRedirector(); });
    bool bValid = Assets.Num() == ExpectedCount;
    if (!bValid) UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY target expected=%d actual=%d"), ExpectedCount, Assets.Num());
    for (const FAssetData& Asset : Assets)
    {
        TArray<FName> Dependencies;
        Registry.GetDependencies(Asset.PackageName, Dependencies);
        for (FName Dependency : Dependencies)
        {
            if (IsWithin(Dependency.ToString(), SourceRoot))
            {
                UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY stale dependency: %s -> %s"), *Asset.PackageName.ToString(), *Dependency.ToString());
                bValid = false;
            }
        }
    }
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *(TargetRoot + BlueprintSuffix));
    TArray<UEdGraph*> Graphs;
    if (Blueprint) Blueprint->GetAllGraphs(Graphs);
    if (!Blueprint || Blueprint->ParentClass != AYogLookDevLightingRig::StaticClass()
        || !Blueprint->NewVariables.IsEmpty()
        || Graphs.ContainsByPredicate([](const UEdGraph* Graph) { return !IsEmptyConstructionGraph(Graph); }))
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY Blueprint must have native LookDev parent, zero legacy variables and no executable graph logic (an unconnected UCS entry is allowed)."));
        bValid = false;
    }
    UWorld* Main = LoadObject<UWorld>(nullptr, TEXT("/YogLookDev/LookDevMain.LookDevMain"));
    if (!Main || Main->GetStreamingLevels().Num() != 8)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY main map must reference four reviews and four light maps."));
        bValid = false;
    }
    else for (const ULevelStreaming* Streaming : Main->GetStreamingLevels())
    {
        // AssetTools can rewrite the soft path without refreshing the streaming
        // object's cached package name. Validate the serialized field directly.
        const FSoftObjectPath WorldPath = Streaming ? Streaming->GetWorldAsset().ToSoftObjectPath() : FSoftObjectPath();
        UE_LOG(LogYogLookDevMigration, Display, TEXT("VERIFY main streaming reference: %s"), *WorldPath.ToString());
        if (!Streaming || !IsWithin(WorldPath.GetLongPackageName(), TargetRoot))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY invalid main streaming reference: %s"), *WorldPath.ToString());
            bValid = false;
        }
    }
    UWorld* Neutral = LoadObject<UWorld>(nullptr, *(TargetRoot + NeutralSuffix));
    int32 NativeRigCount = 0;
    if (Neutral && Neutral->PersistentLevel)
        for (AActor* Actor : Neutral->PersistentLevel->Actors)
            if (const AYogLookDevLightingRig* Rig = Cast<AYogLookDevLightingRig>(Actor))
            {
                ++NativeRigCount;
                if (Rig->LightingScenario != EYogLookDevLightingScenario::MidContrast || Rig->BackgroundCompensation != 0.f
                    || Rig->ExposureCompensation != 0.f || !Rig->bGrounded || !Rig->bCalibrator
                    || !FMath::IsNearlyEqual(Rig->DirectionalSettings.Intensity, 2.3f) || !FMath::IsNearlyEqual(Rig->SkySettings.Intensity, 1.2f))
                {
                    UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY neutral rig does not match the requested screenshot defaults."));
                    bValid = false;
                }
            }
    if (NativeRigCount != 1)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY expected one native neutral rig, got %d."), NativeRigCount);
        bValid = false;
    }
    TArray<FAssetData> OldAssets;
    Registry.GetAssetsByPath(FName(*SourceRoot), OldAssets, true, true);
    TSet<FName> OldPackages;
    for (const FAssetData& Asset : OldAssets) OldPackages.Add(Asset.PackageName);
    if (OldPackages.Num() != ExpectedCount)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY expected %d retained source packages, got %d (%d alias assets)."), ExpectedCount, OldPackages.Num(), OldAssets.Num());
        bValid = false;
    }
    for (const auto& Asset : OldAssets)
        if (!Asset.IsRedirector())
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY source retains a non-redirector asset: %s"), *Asset.GetSoftObjectPath().ToString());
            bValid = false;
        }
    // A moved Blueprint may retain additional generated/skeleton-class aliases
    // in its source package. Count packages, but still require the canonical
    // original asset-name alias corresponding to every migrated target asset.
    for (const FAssetData& TargetAsset : Assets)
    {
        const FString OriginalPackage = SourceRoot + TargetAsset.PackageName.ToString().RightChop(TargetRoot.Len());
        const FName OriginalPackageName(*OriginalPackage);
        const bool bHasCanonicalAlias = OldAssets.ContainsByPredicate([&](const FAssetData& OldAsset)
        {
            return OldAsset.PackageName == OriginalPackageName && OldAsset.AssetName == TargetAsset.AssetName && OldAsset.IsRedirector();
        });
        if (!bHasCanonicalAlias)
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("VERIFY missing canonical source redirector: %s.%s"), *OriginalPackage, *TargetAsset.AssetName.ToString());
            bValid = false;
        }
    }
    UE_LOG(LogYogLookDevMigration, Display, TEXT("LOOKDEV_MIGRATION_VERIFY=%s target_count=%d old_packages=%d old_redirectors=%d"), bValid ? TEXT("true") : TEXT("false"), Assets.Num(), OldPackages.Num(), OldAssets.Num());
    return bValid;
}
}

UYogLookDevMigrateCommandlet::UYogLookDevMigrateCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UYogLookDevMigrateCommandlet::Main(const FString& Params)
{
    int32 ExpectedCount = 45;
    FParse::Value(*Params, TEXT("ExpectedCount="), ExpectedCount);
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerifyOnly = FParse::Param(*Params, TEXT("VerifyOnly"));
    if (ExpectedCount <= 0 || (bApply && bVerifyOnly) || !FPackageName::MountPointExists(TargetRoot))
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("YogLookDev must be mounted, ExpectedCount must be positive, and Apply / VerifyOnly are mutually exclusive."));
        return 1;
    }
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    Registry.ScanPathsSynchronous({ SourceRoot, TargetRoot }, true);
    if (bVerifyOnly) return Verify(Registry, ExpectedCount) ? 0 : 2;
    TArray<FAssetData> Assets;
    TMap<FString, FString> OriginalFiles;
    TArray<FString> ExternalRedirectors;
    if (!GatherSource(Registry, ExpectedCount, Assets, OriginalFiles, ExternalRedirectors)) return 3;
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *(SourceRoot + BlueprintSuffix));
    if (!Blueprint || !Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("AssetsPreview_BP or its authored SCS is unavailable."));
        return 4;
    }
    AuditBlueprint(Blueprint);
    if (!bApply)
    {
        UE_LOG(LogYogLookDevMigration, Display, TEXT("DRY_RUN_COMPLETE assets=%d. No backup, reparent, rename, save or P4 action performed. -Apply requires -nop4 and writable source files."), Assets.Num());
        return 0;
    }
    if (ISourceControlModule::Get().IsEnabled())
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("Run this scoped migration with -nop4. P4 opened/change ownership must be managed before the commandlet; AssetTools must not auto-checkout files."));
        return 5;
    }
    for (const auto& File : OriginalFiles)
        if (IFileManager::Get().IsReadOnly(*File.Value))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Source package is read-only: %s. Resolve the scoped P4 writable state first."), *File.Value);
            return 6;
        }
    for (const FString& ExternalRedirector : ExternalRedirectors)
    {
        FString Filename;
        if (!FPackageName::DoesPackageExist(ExternalRedirector, &Filename) || IFileManager::Get().IsReadOnly(*Filename))
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Allowed external redirector is missing or read-only: %s. Resolve its scoped P4 state first."), *ExternalRedirector);
            return 6;
        }
    }
    FString BackupDirectory;
    if (!BackupSources(OriginalFiles, ExternalRedirectors, BackupDirectory)) return 7;

    // Load serialized worlds without opening them in GEditor or registering their
    // actors. The old construction script must not run before its graphs are removed.
    TArray<TStrongObjectPtr<UObject>> LoadedAssets;
    for (const FAssetData& Asset : Assets)
    {
        UObject* Object = Asset.GetAsset();
        if (!Object)
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Cannot load source asset: %s"), *Asset.GetSoftObjectPath().ToString());
            return 8;
        }
        LoadedAssets.Emplace(Object);
    }
    UWorld* Neutral = LoadObject<UWorld>(nullptr, *(SourceRoot + NeutralSuffix));
    TArray<FComponentSnapshot> Snapshots;
    if (!SnapshotNeutral(Blueprint, Neutral, Snapshots)) return 9;
    if (!SaveSnapshotReport(BackupDirectory, Snapshots)) return 9;
    if (!ReparentAndCleanBlueprint(Blueprint, Snapshots)) return 10;

    TArray<FAssetRenameData> Renames;
    for (int32 Index = 0; Index < Assets.Num(); ++Index)
    {
        const FString NewPackage = NewPackagePath(Assets[Index].PackageName.ToString());
        Renames.Emplace(LoadedAssets[Index].Get(), FPackageName::GetLongPackagePath(NewPackage), Assets[Index].AssetName.ToString(), false, false);
    }
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    bool bRenameSucceeded = false;
    {
        // AssetTools warns about every native CDO soft reference, including saved
        // viewport camera history. In an unattended commandlet that warning
        // defaults to Cancel. Isolate only exact migrating world keys from this
        // editor preference while retaining all other CDO/reference checks.
        // Restore the original history on both success and failure; never save
        // this config object. The retained old package redirectors keep it valid.
        ULevelEditorViewportSettings* ViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
        TMap<TSoftObjectPtr<UWorld>, FLevelEditorViewporEditorViews> SavedViewportHistory;
        ON_SCOPE_EXIT
        {
            for (const auto& History : SavedViewportHistory)
            {
                ViewportSettings->EditorViews.Add(History.Key, History.Value);
            }
            UE_LOG(LogYogLookDevMigration, Display, TEXT("Restored %d saved viewport history entries in memory; no editor config was saved."), SavedViewportHistory.Num());
        };

        TSet<FSoftObjectPath> MigratingWorldPaths;
        for (int32 Index = 0; Index < Assets.Num(); ++Index)
        {
            if (LoadedAssets[Index]->IsA<UWorld>())
            {
                MigratingWorldPaths.Add(Assets[Index].GetSoftObjectPath());
            }
        }
        for (auto It = ViewportSettings->EditorViews.CreateIterator(); It; ++It)
        {
            if (MigratingWorldPaths.Contains(It.Key().ToSoftObjectPath()))
            {
                UE_LOG(LogYogLookDevMigration, Display, TEXT("Temporarily isolating viewport camera history for migrating world: %s"), *It.Key().ToSoftObjectPath().ToString());
                SavedViewportHistory.Add(It.Key(), It.Value());
                It.RemoveCurrent();
            }
        }
        bRenameSucceeded = AssetTools.RenameAssets(Renames);
    }
    if (!bRenameSucceeded)
    {
        UE_LOG(LogYogLookDevMigration, Error, TEXT("AssetTools rename was not fully successful. Backup: %s. Do not rerun Apply blindly; inspect partial source/target state."), *BackupDirectory);
        return 11;
    }
    int32 Saved = 0;
    for (int32 Index = 0; Index < LoadedAssets.Num(); ++Index)
    {
        UObject* Asset = LoadedAssets[Index].Get();
        const FString ExpectedPackage = NewPackagePath(Assets[Index].PackageName.ToString());
        if (Asset->GetOutermost()->GetName() != ExpectedPackage)
        {
            UE_LOG(LogYogLookDevMigration, Error, TEXT("Unexpected post-rename package: %s expected %s"), *Asset->GetOutermost()->GetName(), *ExpectedPackage);
            return 12;
        }
        if (UWorld* World = Cast<UWorld>(Asset))
        {
            for (ULevelStreaming* Streaming : World->GetStreamingLevels())
            {
                if (!Streaming) continue;
                const FString StreamingPackage = Streaming->GetWorldAsset().ToSoftObjectPath().GetLongPackageName();
                if (IsWithin(StreamingPackage, SourceRoot))
                {
                    const FString NewStreamingPackage = NewPackagePath(StreamingPackage);
                    UE_LOG(LogYogLookDevMigration, Display, TEXT("Remapping streaming reference: %s -> %s"), *StreamingPackage, *NewStreamingPackage);
                    Streaming->SetWorldAssetByPackageName(FName(*NewStreamingPackage));
                }
            }
            if (World->PersistentLevel)
                for (AActor* Actor : World->PersistentLevel->Actors)
                    if (AYogLookDevLightingRig* Rig = Cast<AYogLookDevLightingRig>(Actor))
                    {
                        ApplyNativeDefaults(Rig);
                        Rig->RerunConstructionScripts();
                        TInlineComponentArray<USceneComponent*> Components;
                        Rig->GetComponents(Components);
                        for (USceneComponent* Component : Components)
                        {
                            const auto* Snapshot = Snapshots.FindByPredicate([Component](const auto& Item) { return Component && Item.Name == Component->GetFName(); });
                            if (Snapshot) RestoreComponent(Component, *Snapshot);
                        }
                        Rig->RefreshLighting();
                    }
        }
        if (!SaveAsset(Asset)) return 13;
        ++Saved;
    }
    // Do not run a global redirector fixup. Keep the exact source package aliases,
    // including assets for which AssetTools found no remaining referencers.
    for (int32 Index = 0; Index < Assets.Num(); ++Index)
    {
        const FString OldPackageName = Assets[Index].PackageName.ToString();
        UPackage* OldPackage = FindPackage(nullptr, *OldPackageName);
        if (!OldPackage) OldPackage = CreatePackage(*OldPackageName);
        // Match ObjectTools::RenameSingleObject: map aliases retain the map
        // package flag as well as their original .umap filename.
        if (LoadedAssets[Index]->IsA<UWorld>()) OldPackage->ThisContainsMap();
        UObjectRedirector* Redirector = FindObject<UObjectRedirector>(OldPackage, *Assets[Index].AssetName.ToString());
        if (!Redirector)
        {
            if (FindObject<UObject>(OldPackage, *Assets[Index].AssetName.ToString()))
            {
                UE_LOG(LogYogLookDevMigration, Error, TEXT("Cannot retain redirector: original name is occupied in %s"), *OldPackageName);
                return 14;
            }
            Redirector = NewObject<UObjectRedirector>(OldPackage, Assets[Index].AssetName, RF_Public | RF_Standalone);
            Redirector->DestinationObject = LoadedAssets[Index].Get();
            FAssetRegistryModule::AssetCreated(Redirector);
        }
        Redirector->DestinationObject = LoadedAssets[Index].Get();
        if (!SaveAsset(Redirector, OriginalFiles[OldPackageName])) return 15;
    }
    UE_LOG(LogYogLookDevMigration, Display, TEXT("MIGRATION_SAVED=%d BACKUP=%s"), Saved, *BackupDirectory);
    const bool bValid = Verify(Registry, ExpectedCount);
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("verified"), bValid);
    Result->SetNumberField(TEXT("saved_target_packages"), Saved);
    Result->SetStringField(TEXT("backup_directory"), BackupDirectory);
    SaveJson(BackupDirectory / TEXT("result.json"), Result);
    return bValid ? 0 : 16;
}
