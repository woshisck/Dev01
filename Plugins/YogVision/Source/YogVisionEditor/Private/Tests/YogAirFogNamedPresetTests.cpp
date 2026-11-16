#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SYogAirFogWorkbench.h"
#include "YogAirFogEditorService.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"

namespace AirFogNamedPresetTests
{
constexpr const TCHAR* ArtistMaterialPath = TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline");

/** Own only unique test files. Never remove a directory or the user's legacy draft. */
struct FScopedPresetFiles
{
    FAutomationTestBase& Test;
    FString Prefix = TEXT("AirFogAutomation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString NameA = Prefix + TEXT("_A");
    FString NameB = Prefix + TEXT("_B");
    FString InvalidJsonName = Prefix + TEXT("_InvalidJson");
    FString FutureName = Prefix + TEXT("_Future");
    FString BadTypeName = Prefix + TEXT("_BadType");
    FString EscapedName = Prefix + TEXT("_Escaped");
    TArray<FString> OwnedPaths;
    bool bReady = true;

    explicit FScopedPresetFiles(FAutomationTestBase& InTest) : Test(InTest)
    {
        for (const FString& Name : {NameA, NameB, InvalidJsonName, FutureName, BadTypeName, EscapedName})
            OwnedPaths.Add(PathFor(Name));
        // If the save API ever regresses to accepting traversal, this is still
        // a unique, pre-checked test file, and teardown removes exactly it.
        OwnedPaths.Add(FPaths::ConvertRelativePathToFull(FYogAirFogEditorService::GetPresetDirectory() / TEXT("..") / (EscapedName + TEXT(".json"))));
        for (const FString& Path : OwnedPaths)
            bReady = Test.TestFalse(TEXT("Unique preset fixture does not replace an existing file: ") + Path,
                IFileManager::Get().FileExists(*Path)) && bReady;
    }

    FString PathFor(const FString& Name) const
    {
        return FPaths::ConvertRelativePathToFull(FYogAirFogEditorService::GetPresetDirectory() / (Name + TEXT(".json")));
    }

    bool Write(const FString& Name, const FString& Json)
    {
        const FString Path = PathFor(Name);
        if (!Test.TestTrue(TEXT("Malformed-input fixture belongs to this test"), OwnedPaths.Contains(Path))) return false;
        IFileManager::Get().MakeDirectory(*FYogAirFogEditorService::GetPresetDirectory(), true);
        return Test.TestTrue(TEXT("Write the owned JSON fixture"), FFileHelper::SaveStringToFile(Json, *Path,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    }

    ~FScopedPresetFiles()
    {
        if (!bReady) return;
        for (const FString& Path : OwnedPaths)
            if (IFileManager::Get().FileExists(*Path))
                Test.TestTrue(TEXT("Remove only the exact owned preset fixture: ") + Path,
                    IFileManager::Get().Delete(*Path, false, true, true));
    }
};

/** Preserve the user's exact legacy bytes and only own newly imported names. */
struct FScopedLegacyDraft
{
    FAutomationTestBase& Test;
    FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AirFog/ArtDraft.json"));
    bool bExisted = false;
    bool bCaptured = false;
    TArray<uint8> OriginalBytes;
    TMap<FString, TArray<uint8>> ExistingNamedFiles;
    TArray<FString> ImportedFiles;

    explicit FScopedLegacyDraft(FAutomationTestBase& InTest) : Test(InTest)
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        bCaptured = !bExisted || FFileHelper::LoadFileToArray(OriginalBytes, *Path);
        TArray<FString> Existing;
        IFileManager::Get().FindFiles(Existing, *(FYogAirFogEditorService::GetPresetDirectory() / TEXT("*.json")), true, false);
        for (const FString& Name : Existing)
        {
            const FString File = FPaths::ConvertRelativePathToFull(FYogAirFogEditorService::GetPresetDirectory() / Name);
            TArray<uint8> Bytes;
            if (!FFileHelper::LoadFileToArray(Bytes, *File)) bCaptured = false;
            else ExistingNamedFiles.Add(File, MoveTemp(Bytes));
        }
    }

    bool Write(const FString& Json)
    {
        if (!bCaptured) return false;
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
        return Test.TestTrue(TEXT("Write the legacy fixture only after preserving the user's bytes"),
            FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    }

    bool OwnImportedName(const FString& Id)
    {
        FText Message;
        if (!Test.TestTrue(TEXT("Import returns a valid named-preset identity"), FYogAirFogEditorService::ValidatePresetName(Id, Message))) return false;
        const FString File = FPaths::ConvertRelativePathToFull(FYogAirFogEditorService::GetPresetDirectory() / (Id + TEXT(".json")));
        if (!Test.TestFalse(TEXT("Import never returns or claims ownership of a pre-existing user file"), ExistingNamedFiles.Contains(File))) return false;
        if (!Test.TestTrue(TEXT("The returned imported file actually exists"), IFileManager::Get().FileExists(*File))) return false;
        ImportedFiles.AddUnique(File);
        return true;
    }

    ~FScopedLegacyDraft()
    {
        if (!bCaptured) return;
        for (const FString& File : ImportedFiles)
            if (IFileManager::Get().FileExists(*File))
                Test.TestTrue(TEXT("Remove only the exact newly imported preset: ") + File,
                    IFileManager::Get().Delete(*File, false, true, true));
        for (const auto& Existing : ExistingNamedFiles)
        {
            TArray<uint8> Current;
            Test.TestTrue(TEXT("Legacy migration leaves the complete pre-existing named file unchanged: ") + Existing.Key,
                FFileHelper::LoadFileToArray(Current, *Existing.Key) && Current == Existing.Value);
        }
        bool bRestored = false;
        if (bExisted) bRestored = FFileHelper::SaveArrayToFile(OriginalBytes, *Path);
        else bRestored = !IFileManager::Get().FileExists(*Path) || IFileManager::Get().Delete(*Path, false, false, true);
        Test.TestTrue(TEXT("Restore the user's exact legacy draft bytes or original absence"), bRestored);
        if (bExisted && bRestored)
        {
            TArray<uint8> Restored;
            Test.TestTrue(TEXT("The restored legacy draft is byte-for-byte identical"),
                FFileHelper::LoadFileToArray(Restored, *Path) && Restored == OriginalBytes);
        }
    }
};

bool GetPersistentPalettes(FAutomationTestBase& Test, FSoftObjectPath& ArtistPath, FSoftObjectPath& OtherPath)
{
    UMaterialInterface* Artist = LoadObject<UMaterialInterface>(nullptr, ArtistMaterialPath);
    UMaterialInterface* Other = UMaterial::GetDefaultMaterial(MD_PostProcess);
    if (!Test.TestNotNull(TEXT("The original persistent artist MI exists"), Artist)
        || !Test.TestNotNull(TEXT("A second persistent post-process material exists"), Other)) return false;
    ArtistPath = FSoftObjectPath(Artist->GetPathName());
    OtherPath = FSoftObjectPath(Other->GetPathName());
    Test.TestFalse(TEXT("The two palette identities differ"), ArtistPath == OtherPath);
    Test.TestFalse(TEXT("The original palette is not transient"), Artist->HasAnyFlags(RF_Transient));
    Test.TestFalse(TEXT("The second palette is not transient"), Other->HasAnyFlags(RF_Transient));
    return Test.TestTrue(TEXT("The original palette round-trips through its persistent soft path"), ArtistPath.TryLoad() == Artist)
        && Test.TestTrue(TEXT("The engine default post-process palette round-trips through its persistent soft path"), OtherPath.TryLoad() == Other);
}

FYogAirFogPresetData MakeSnapshot(const FSoftObjectPath& Palette, float Density, bool bFinish)
{
    FYogAirFogPresetData Result;
    Result.Parameters.Density = Density;
    Result.Parameters.Sanitize();
    Result.bFinishEnabled = bFinish;
    Result.PaletteMaterial = Palette;
    return Result;
}

void CheckSnapshot(FAutomationTestBase& Test, const FString& Phase, const FYogAirFogPresetData& Actual, const FYogAirFogPresetData& Expected)
{
    Test.TestTrue(Phase + TEXT(": every parameter is retained"),
        FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Actual.Parameters, &Expected.Parameters, 0));
    Test.TestEqual(Phase + TEXT(": finish switch is retained"), Actual.bFinishEnabled, Expected.bFinishEnabled);
    Test.TestEqual(Phase + TEXT(": persistent palette identity is retained"), Actual.PaletteMaterial.ToString(), Expected.PaletteMaterial.ToString());
}

bool FindPreset(FAutomationTestBase& Test, const FString& Name, FYogAirFogNamedPresetInfo& Out)
{
    TArray<FYogAirFogNamedPresetInfo> Presets;
    FText Message;
    if (!Test.TestTrue(TEXT("The preset list can be read"), FYogAirFogEditorService::ListNamedPresets(Presets, Message))) return false;
    for (const FYogAirFogNamedPresetInfo& Preset : Presets)
        if (Preset.DisplayName == Name)
        {
            Out = Preset;
            return true;
        }
    Test.AddError(TEXT("The named preset is absent from the list: ") + Name);
    return false;
}

void CheckSchema3(FAutomationTestBase& Test, const FString& Path, const FYogAirFogPresetData& Expected)
{
    FString Json;
    TSharedPtr<FJsonObject> Object;
    if (!Test.TestTrue(TEXT("Read the actual named-preset JSON"), FFileHelper::LoadFileToString(Json, *Path)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) && Object.IsValid())) return;
    double Version = 0;
    bool bFinish = !Expected.bFinishEnabled;
    FString Palette;
    if (Test.TestTrue(TEXT("Named JSON contains a schema version"), Object->TryGetNumberField(TEXT("version"), Version)))
        Test.TestEqual(TEXT("New named JSON uses schema 3"), Version, 3.0);
    Test.TestTrue(TEXT("Schema 3 stores a parameters object"), Object->HasTypedField<EJson::Object>(TEXT("parameters")));
    if (Test.TestTrue(TEXT("Schema 3 stores the finish boolean"), Object->TryGetBoolField(TEXT("finishEnabled"), bFinish)))
        Test.TestEqual(TEXT("The JSON finish switch is correct"), bFinish, Expected.bFinishEnabled);
    if (Test.TestTrue(TEXT("Schema 3 stores a persistent palette path"), Object->TryGetStringField(TEXT("paletteMaterial"), Palette)))
        Test.TestEqual(TEXT("The JSON palette identity is correct"), Palette, Expected.PaletteMaterial.ToString());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogNamedPresetRoundTripTest, "YogAirFog.Editor.NamedPresets.IsolatedRoundTripAndExplicitOverwrite", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogNamedPresetRoundTripTest::RunTest(const FString&)
{
    using namespace AirFogNamedPresetTests;
    FScopedPresetFiles Files(*this);
    if (!Files.bReady) return false;
    FSoftObjectPath Artist, Other;
    if (!GetPersistentPalettes(*this, Artist, Other)) return false;
    const FYogAirFogPresetData A = MakeSnapshot(Artist, .083f, true);
    FYogAirFogPresetData B = MakeSnapshot(Other, .14f, false);
    B.Parameters.StrokeScale = 1.25f;
    B.Parameters.FogBankCenter = FVector(2100.f, -1250.f, 875.f);
    B.Parameters.FogBankExtent = FVector(1800.f, 2300.f, 1400.f);
    B.Parameters.FogBankStrength = .85f;
    B.Parameters.Sanitize();
    FText Message;
    if (!TestTrue(TEXT("Save the first unique preset without replacement"), FYogAirFogEditorService::SaveNamedPreset(Files.NameA, A, false, Message))
        || !TestTrue(TEXT("Save a separate second preset without replacement"), FYogAirFogEditorService::SaveNamedPreset(Files.NameB, B, false, Message))) return false;

    FYogAirFogNamedPresetInfo InfoA, InfoB;
    if (!FindPreset(*this, Files.NameA, InfoA) || !FindPreset(*this, Files.NameB, InfoB)) return false;
    TestEqual(TEXT("Named preset A has its basename identity"), InfoA.Id, Files.NameA);
    TestEqual(TEXT("Named preset B has its basename identity"), InfoB.Id, Files.NameB);
    TestFalse(TEXT("A named preset is not the legacy entry"), InfoA.bLegacy || InfoB.bLegacy);
    TestTrue(TEXT("Preset A is stored inside its exact owned file"), FPaths::IsSamePath(InfoA.FilePath, Files.PathFor(Files.NameA)));
    TestTrue(TEXT("Preset B is stored inside its exact owned file"), FPaths::IsSamePath(InfoB.FilePath, Files.PathFor(Files.NameB)));
    FYogAirFogPresetData Loaded;
    if (TestTrue(TEXT("Read preset A by its listed identity"), FYogAirFogEditorService::LoadNamedPreset(InfoA.Id, Loaded, Message)))
        CheckSnapshot(*this, TEXT("First preset round trip"), Loaded, A);
    if (TestTrue(TEXT("Read preset B by its listed identity"), FYogAirFogEditorService::LoadNamedPreset(InfoB.Id, Loaded, Message)))
        CheckSnapshot(*this, TEXT("Second preset round trip"), Loaded, B);
    CheckSchema3(*this, InfoA.FilePath, A);
    CheckSchema3(*this, InfoB.FilePath, B);

    TArray<uint8> Before, After, BeforeB;
    if (!TestTrue(TEXT("Capture preset A bytes before attempted replacement"), FFileHelper::LoadFileToArray(Before, *InfoA.FilePath))) return false;
    if (!TestTrue(TEXT("Capture independent preset B bytes before replacing A"), FFileHelper::LoadFileToArray(BeforeB, *InfoB.FilePath))) return false;
    TMap<FString, TArray<uint8>> SentinelBytes;
    for (const FString& Suffix : {FString(TEXT(".owned-sentinel.bak")), FString(TEXT(".owned-sentinel.tmp"))})
    {
        const FString Sentinel = InfoA.FilePath + Suffix;
        if (!TestFalse(TEXT("An owned sidecar sentinel does not replace an existing file"), IFileManager::Get().FileExists(*Sentinel))) return false;
        Files.OwnedPaths.Add(Sentinel);
        if (!TestTrue(TEXT("Create a pre-existing same-prefix user-sidecar sentinel"), FFileHelper::SaveStringToFile(
            TEXT("Existing sidecar must survive named-preset replacement: ") + Suffix, *Sentinel,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))) return false;
        TArray<uint8> Bytes;
        if (!TestTrue(TEXT("Record exact sidecar sentinel bytes"), FFileHelper::LoadFileToArray(Bytes, *Sentinel))) return false;
        SentinelBytes.Add(Sentinel, MoveTemp(Bytes));
    }
    auto CollectOwnedSidecars = [&InfoA, &InfoB]()
    {
        TArray<FString> Names, Sidecars;
        IFileManager::Get().FindFiles(Names, *(FYogAirFogEditorService::GetPresetDirectory() / TEXT("*")), true, false);
        const FString PrefixA = FPaths::GetCleanFilename(InfoA.FilePath) + TEXT(".");
        const FString PrefixB = FPaths::GetCleanFilename(InfoB.FilePath) + TEXT(".");
        for (const FString& Name : Names)
            if (Name.StartsWith(PrefixA, ESearchCase::IgnoreCase) || Name.StartsWith(PrefixB, ESearchCase::IgnoreCase)) Sidecars.Add(Name);
        Sidecars.Sort();
        return Sidecars;
    };
    const TArray<FString> SidecarsBefore = CollectOwnedSidecars();
    FYogAirFogPresetData Replacement = A;
    Replacement.Parameters.Density = .19f;
    Replacement.Parameters.StrokeFlowSpeed = .31f;
    Replacement.Parameters.Sanitize();
    TestFalse(TEXT("Saving an existing name cannot silently replace it"), FYogAirFogEditorService::SaveNamedPreset(Files.NameA, Replacement, false, Message));
    TestFalse(TEXT("A Windows case alias cannot bypass the replacement guard"), FYogAirFogEditorService::SaveNamedPreset(Files.NameA.ToUpper(), Replacement, false, Message));
    if (TestTrue(TEXT("Read bytes after rejected replacements"), FFileHelper::LoadFileToArray(After, *InfoA.FilePath)))
        TestTrue(TEXT("Rejected replacements leave the complete original file unchanged"), Before == After);
    const bool bReplaced = FYogAirFogEditorService::SaveNamedPreset(Files.NameA, Replacement, true, Message);
    const TArray<FString> SidecarsAfter = CollectOwnedSidecars();
    for (const FString& Name : SidecarsAfter)
        if (!SidecarsBefore.Contains(Name))
            Files.OwnedPaths.AddUnique(FPaths::ConvertRelativePathToFull(FYogAirFogEditorService::GetPresetDirectory() / Name));
    if (!TestTrue(TEXT("Explicit replacement updates the bound name"), bReplaced)) return false;
    TestTrue(TEXT("A normal successful save adds no temporary or backup sidecar residue"), SidecarsAfter == SidecarsBefore);
    for (const auto& Sentinel : SentinelBytes)
    {
        TArray<uint8> Current;
        TestTrue(TEXT("Successful replacement does not overwrite or clean pre-existing same-prefix sidecars: ") + Sentinel.Key,
            FFileHelper::LoadFileToArray(Current, *Sentinel.Key) && Current == Sentinel.Value);
    }
    if (TestTrue(TEXT("Read explicitly replaced preset A"), FYogAirFogEditorService::LoadNamedPreset(InfoA.Id, Loaded, Message)))
        CheckSnapshot(*this, TEXT("Explicit replacement"), Loaded, Replacement);
    if (TestTrue(TEXT("Read preset A bytes after explicit replacement"), FFileHelper::LoadFileToArray(After, *InfoA.FilePath)))
        TestTrue(TEXT("Successful replacement writes the new file contents"), After != Before);
    if (TestTrue(TEXT("Preset B is still independently readable"), FYogAirFogEditorService::LoadNamedPreset(InfoB.Id, Loaded, Message)))
        CheckSnapshot(*this, TEXT("Replacing A leaves B unchanged"), Loaded, B);
    if (TestTrue(TEXT("Read independent preset B bytes after replacing A"), FFileHelper::LoadFileToArray(After, *InfoB.FilePath)))
        TestTrue(TEXT("Replacing A leaves every byte of preset B unchanged"), After == BeforeB);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogNamedPresetFailureTest, "YogAirFog.Editor.NamedPresets.InvalidNamesAndFailurePreserveOutput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogNamedPresetFailureTest::RunTest(const FString&)
{
    using namespace AirFogNamedPresetTests;
    FScopedPresetFiles Files(*this);
    if (!Files.bReady) return false;
    FText Message;
    TestTrue(TEXT("A unique Chinese display name is valid"), FYogAirFogEditorService::ValidatePresetName(TEXT("油画卷雾_") + Files.Prefix, Message));
    const TArray<FString> InvalidNames = {
        TEXT(""), TEXT("   "), TEXT("."), TEXT(".."), TEXT("CON"), TEXT("con.txt"), TEXT("LPT1"), TEXT("COM1"),
        TEXT("../") + Files.EscapedName, TEXT("..\\") + Files.EscapedName,
        TEXT("C:\\") + Files.Prefix, Files.Prefix + TEXT("/child"), Files.Prefix + TEXT("\\child"),
        Files.Prefix + TEXT(":"), Files.Prefix + TEXT("?"), Files.Prefix + TEXT("*"), Files.Prefix + TEXT("|"),
        Files.Prefix + TEXT("<"), Files.Prefix + TEXT(">"), Files.Prefix + TEXT("\""),
        Files.Prefix + TEXT("."), Files.Prefix + TEXT(" "), Files.Prefix + TEXT("\n")
    };
    for (const FString& Name : InvalidNames)
        TestFalse(TEXT("Reject an unsafe or ambiguous preset name: ") + Name, FYogAirFogEditorService::ValidatePresetName(Name, Message));

    FSoftObjectPath Artist, Other;
    if (!GetPersistentPalettes(*this, Artist, Other)) return false;
    const FYogAirFogPresetData Sentinel = MakeSnapshot(Other, .123f, false);
    FYogAirFogPresetData Out = Sentinel;
    const FString EscapePath = FPaths::ConvertRelativePathToFull(FYogAirFogEditorService::GetPresetDirectory() / TEXT("..") / (Files.EscapedName + TEXT(".json")));
    TestFalse(TEXT("The save API itself rejects traversal"), FYogAirFogEditorService::SaveNamedPreset(TEXT("..\\") + Files.EscapedName, Sentinel, false, Message));
    TestFalse(TEXT("Rejected traversal writes no file outside Presets"), IFileManager::Get().FileExists(*EscapePath));
    if (!Files.Write(Files.InvalidJsonName, TEXT("{ broken JSON"))
        || !Files.Write(Files.FutureName, TEXT("{\"version\":99,\"parameters\":{},\"finishEnabled\":false,\"paletteMaterial\":\"") + Artist.ToString() + TEXT("\"}"))
        || !Files.Write(Files.BadTypeName, TEXT("{\"version\":3,\"parameters\":{\"Density\":0.25,\"StrokeScale\":\"bad type\"},\"finishEnabled\":true,\"paletteMaterial\":\"") + Artist.ToString() + TEXT("\"}"))) return false;
    for (const FString& Id : {Files.InvalidJsonName, Files.FutureName, Files.BadTypeName, Files.Prefix + TEXT("_Missing")})
    {
        Out = Sentinel;
        TestFalse(TEXT("Malformed, unsupported or absent input cannot load: ") + Id, FYogAirFogEditorService::LoadNamedPreset(Id, Out, Message));
        CheckSnapshot(*this, TEXT("Failed load leaves the caller's full snapshot unchanged"), Out, Sentinel);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogNamedPresetWidgetTest, "YogAirFog.Editor.NamedPresets.WorkbenchSaveLoadIdentityAndUndo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogNamedPresetWidgetTest::RunTest(const FString&)
{
    using namespace AirFogNamedPresetTests;
    if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized()) return false;
    if (!TestFalse(TEXT("Named-draft regression does not take over an active preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    const bool bWorldDirty = World->GetOutermost()->IsDirty();
    FScopedPresetFiles Files(*this);
    if (!Files.bReady) return false;
    FSoftObjectPath Artist, Other;
    if (!GetPersistentPalettes(*this, Artist, Other)) return false;
    TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
    TestTrue(TEXT("A new workbench has no bound preset identity"), Widget->GetCurrentPresetId().IsEmpty());
    TestTrue(TEXT("An unsaved new draft starts dirty"), Widget->IsDraftDirty());
    Widget->ApplyPreset(3);
    Widget->SetSaveAsName(Files.NameA);
    Widget->SaveDraftAs();
    const FYogAirFogPresetData SavedA = Widget->GetDraftSnapshot();
    if (!TestEqual(TEXT("Save As binds the new name"), Widget->GetCurrentPresetId(), Files.NameA)
        || !TestFalse(TEXT("A successful Save As establishes a clean baseline"), Widget->IsDraftDirty())) return false;
    TestTrue(TEXT("Save As exposes its actual owned file path"), FPaths::IsSamePath(Widget->GetCurrentPresetFilePath(), Files.PathFor(Files.NameA)));

    Widget->ApplyPreset(2);
    const FYogAirFogPresetData Edited = Widget->GetDraftSnapshot();
    TestTrue(TEXT("Editing a saved draft marks it dirty"), Widget->IsDraftDirty());
    TestTrue(TEXT("The real editor transaction undoes the draft edit"), GEditor->UndoTransaction());
    CheckSnapshot(*this, TEXT("Undo restores the saved snapshot"), Widget->GetDraftSnapshot(), SavedA);
    TestFalse(TEXT("Undo to the saved contents is clean"), Widget->IsDraftDirty());
    TestTrue(TEXT("The real editor transaction redoes the edit"), GEditor->RedoTransaction());
    CheckSnapshot(*this, TEXT("Redo restores the unsaved edit"), Widget->GetDraftSnapshot(), Edited);
    TestTrue(TEXT("Redo to the edited contents is dirty"), Widget->IsDraftDirty());

    Widget->SaveDraft();
    TestFalse(TEXT("Save Current marks the updated contents clean"), Widget->IsDraftDirty());
    TestTrue(TEXT("Undo after saving still undoes the content edit"), GEditor->UndoTransaction());
    CheckSnapshot(*this, TEXT("Undo after save restores older contents"), Widget->GetDraftSnapshot(), SavedA);
    TestTrue(TEXT("The disk checkpoint is not undone with content"), Widget->IsDraftDirty());
    TestTrue(TEXT("Redo after saving restores the saved edit"), GEditor->RedoTransaction());
    CheckSnapshot(*this, TEXT("Redo matches the latest disk checkpoint"), Widget->GetDraftSnapshot(), Edited);
    TestFalse(TEXT("Redo to the latest disk contents is clean"), Widget->IsDraftDirty());

    FYogAirFogPresetData B = MakeSnapshot(Other, .073f, false);
    B.Parameters.StrokeScale = 2.2f;
    B.Parameters.Sanitize();
    FText Message;
    if (!TestTrue(TEXT("Prepare a separate preset with another finish/palette identity"), FYogAirFogEditorService::SaveNamedPreset(Files.NameB, B, false, Message))) return false;
    Widget->RefreshDrafts();
    if (!TestTrue(TEXT("Select the second saved preset without applying it"), Widget->SelectSavedPreset(Files.NameB))) return false;
    CheckSnapshot(*this, TEXT("Selection alone does not load or change the draft"), Widget->GetDraftSnapshot(), Edited);
    TestEqual(TEXT("Selection alone retains the bound identity"), Widget->GetCurrentPresetId(), Files.NameA);
    TestFalse(TEXT("Selection alone leaves the saved draft clean"), Widget->IsDraftDirty());
    Widget->LoadDraft();
    CheckSnapshot(*this, TEXT("Load applies parameters, finish and palette together"), Widget->GetDraftSnapshot(), B);
    TestEqual(TEXT("Load binds the selected identity"), Widget->GetCurrentPresetId(), Files.NameB);
    TestFalse(TEXT("A freshly loaded snapshot is clean"), Widget->IsDraftDirty());
    TestTrue(TEXT("Load exposes the selected owned file path"), FPaths::IsSamePath(Widget->GetCurrentPresetFilePath(), Files.PathFor(Files.NameB)));
    TestTrue(TEXT("The real editor transaction undoes preset loading"), GEditor->UndoTransaction());
    CheckSnapshot(*this, TEXT("Undo load restores the preceding complete snapshot"), Widget->GetDraftSnapshot(), Edited);
    TestEqual(TEXT("Undo load restores the preceding file identity"), Widget->GetCurrentPresetId(), Files.NameA);
    TestTrue(TEXT("Undo load restores the preceding file path"), FPaths::IsSamePath(Widget->GetCurrentPresetFilePath(), Files.PathFor(Files.NameA)));
    TestFalse(TEXT("Undo load uses that identity's latest disk baseline"), Widget->IsDraftDirty());
    TestTrue(TEXT("The real editor transaction redoes preset loading"), GEditor->RedoTransaction());
    CheckSnapshot(*this, TEXT("Redo load restores the second complete snapshot"), Widget->GetDraftSnapshot(), B);
    TestEqual(TEXT("Redo load restores the second file identity"), Widget->GetCurrentPresetId(), Files.NameB);
    TestFalse(TEXT("Redo load returns to a clean loaded snapshot"), Widget->IsDraftDirty());
    TestFalse(TEXT("Named save/load and undo never start a scene preview"), FYogAirFogEditorService::IsPreviewActive());
    TestEqual(TEXT("Named save/load and undo retain the map dirty state"), World->GetOutermost()->IsDirty(), bWorldDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogNamedPresetLegacyTest, "YogAirFog.Editor.NamedPresets.LegacyMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogNamedPresetLegacyTest::RunTest(const FString&)
{
    using namespace AirFogNamedPresetTests;
    FScopedPresetFiles Files(*this);
    if (!Files.bReady) return false;
    FScopedLegacyDraft Legacy(*this);
    if (!TestTrue(TEXT("The user's legacy and named files are preserved before migration fixtures"), Legacy.bCaptured)) return false;
    FSoftObjectPath Artist, Other;
    if (!GetPersistentPalettes(*this, Artist, Other)) return false;
    const FYogAirFogParameters Defaults;
    FText Message;
    FYogAirFogPresetData Loaded;

    if (!Legacy.Write(TEXT("{\"version\":1,\"Density\":0.17,\"NearOffset\":-250,\"FarStart\":300,\"FarEnd\":900,\"StrokeFlowSpeed\":0.33}"))
        || !TestTrue(TEXT("The explicit legacy identity reads a version-1 flat file"),
            FYogAirFogEditorService::LoadNamedPreset(FYogAirFogEditorService::GetLegacyPresetId(), Loaded, Message))) return false;
    TestEqual(TEXT("Version 1 retains the authored density"), Loaded.Parameters.Density, .17f);
    TestEqual(TEXT("Version 1 migrates the old relative near distance"), Loaded.Parameters.NearOffset, Defaults.NearOffset);
    TestEqual(TEXT("Version 1 migrates the old relative far start"), Loaded.Parameters.FarStart, Defaults.FarStart);
    TestEqual(TEXT("Version 1 migrates the old relative far end"), Loaded.Parameters.FarEnd, Defaults.FarEnd);
    TestEqual(TEXT("Version 1 retains unrelated authored parameters"), Loaded.Parameters.StrokeFlowSpeed, .33f);
    TestTrue(TEXT("Version 1 supplies the default enabled finish"), Loaded.bFinishEnabled);
    TestEqual(TEXT("Version 1 supplies the original persistent MI identity"), Loaded.PaletteMaterial.ToString(), Artist.ToString());
    TestTrue(TEXT("Version 1 missing scene-direction switch uses the current true default"), Loaded.Parameters.bUseSceneDirectionalLight);

    if (!Legacy.Write(TEXT("{\"version\":2,\"Density\":0.071,\"NearOffset\":1350,\"FarStart\":2400,\"FarEnd\":6100,\"StrokeScale\":1.6}"))
        || !TestTrue(TEXT("The explicit legacy identity reads a version-2 flat file"),
            FYogAirFogEditorService::LoadNamedPreset(FYogAirFogEditorService::GetLegacyPresetId(), Loaded, Message))) return false;
    TestEqual(TEXT("Version 2 retains the authored near distance"), Loaded.Parameters.NearOffset, 1350.f);
    TestEqual(TEXT("Version 2 retains the authored far start"), Loaded.Parameters.FarStart, 2400.f);
    TestEqual(TEXT("Version 2 retains the authored far end"), Loaded.Parameters.FarEnd, 6100.f);
    TestEqual(TEXT("Version 2 retains the authored density"), Loaded.Parameters.Density, .071f);
    TestEqual(TEXT("Version 2 retains the authored stroke scale"), Loaded.Parameters.StrokeScale, 1.6f);
    TestTrue(TEXT("Version 2 supplies the default enabled finish"), Loaded.bFinishEnabled);
    TestEqual(TEXT("Version 2 supplies the original persistent MI identity"), Loaded.PaletteMaterial.ToString(), Artist.ToString());
    TestTrue(TEXT("Version 2 missing scene-direction switch uses the current true default"), Loaded.Parameters.bUseSceneDirectionalLight);
    const FYogAirFogPresetData LegacyV2 = Loaded;
    TArray<uint8> LegacyBytes;
    if (!TestTrue(TEXT("Record the legacy input bytes before explicit import"), FFileHelper::LoadFileToArray(LegacyBytes, *Legacy.Path))) return false;

    FString FirstId, SecondId;
    if (!TestTrue(TEXT("Explicit import creates a new unique named preset"), FYogAirFogEditorService::ImportLegacyPreset(FirstId, Message))
        || !Legacy.OwnImportedName(FirstId)) return false;
    const FString FirstFile = FYogAirFogEditorService::GetPresetDirectory() / (FirstId + TEXT(".json"));
    if (TestTrue(TEXT("The imported version-3 preset can be read by its returned identity"), FYogAirFogEditorService::LoadNamedPreset(FirstId, Loaded, Message)))
        CheckSnapshot(*this, TEXT("Explicit import retains the complete migrated snapshot"), Loaded, LegacyV2);
    CheckSchema3(*this, FirstFile, LegacyV2);
    TArray<uint8> FirstBytes;
    if (!TestTrue(TEXT("Record the first imported file before repeating import"), FFileHelper::LoadFileToArray(FirstBytes, *FirstFile))) return false;
    if (!TestTrue(TEXT("Repeated explicit import chooses another available name"), FYogAirFogEditorService::ImportLegacyPreset(SecondId, Message))
        || !Legacy.OwnImportedName(SecondId)) return false;
    TestFalse(TEXT("Repeated import never silently reuses the first identity"), FirstId == SecondId);
    const FString SecondFile = FYogAirFogEditorService::GetPresetDirectory() / (SecondId + TEXT(".json"));
    TestFalse(TEXT("Repeated import creates a distinct file"), FPaths::IsSamePath(FirstFile, SecondFile));
    TArray<uint8> AfterFirst, AfterLegacy;
    if (TestTrue(TEXT("Read the first imported file after the second import"), FFileHelper::LoadFileToArray(AfterFirst, *FirstFile)))
        TestTrue(TEXT("The second import leaves the first complete file unchanged"), AfterFirst == FirstBytes);
    if (TestTrue(TEXT("Read the legacy input after both imports"), FFileHelper::LoadFileToArray(AfterLegacy, *Legacy.Path)))
        TestTrue(TEXT("Explicit imports never rewrite the legacy input bytes"), AfterLegacy == LegacyBytes);
    if (TestTrue(TEXT("The second imported identity is readable"), FYogAirFogEditorService::LoadNamedPreset(SecondId, Loaded, Message)))
        CheckSnapshot(*this, TEXT("Second import retains the complete migrated snapshot"), Loaded, LegacyV2);

    FYogAirFogPresetData Manual = LegacyV2;
    Manual.Parameters.bUseSceneDirectionalLight = false;
    Manual.bFinishEnabled = false;
    Manual.PaletteMaterial = Other;
    if (!TestTrue(TEXT("Schema 3 saves an explicitly disabled scene-direction switch"), FYogAirFogEditorService::SaveNamedPreset(Files.NameA, Manual, false, Message))
        || !TestTrue(TEXT("Schema 3 reloads the authored false switch"), FYogAirFogEditorService::LoadNamedPreset(Files.NameA, Loaded, Message))) return false;
    CheckSnapshot(*this, TEXT("New schema retains authored false rather than replacing it with the default"), Loaded, Manual);
    TestFalse(TEXT("The explicit scene-direction false value survives round trip"), Loaded.Parameters.bUseSceneDirectionalLight);
    CheckSchema3(*this, Files.PathFor(Files.NameA), Manual);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogFlowAssetIdentityTest, "YogVision.AirFog.Editor.NamedPresets.FlowAssetIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogFlowAssetIdentityTest::RunTest(const FString&)
{
    using namespace AirFogNamedPresetTests;
    FScopedPresetFiles Files(*this);
    if (!Files.bReady) return false;
    FYogAirFogPresetData Original;
    Original.PaletteMaterial = FSoftObjectPath(ArtistMaterialPath);
    Original.Parameters.StrokeTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/YogVision/Textures/T_VisionStrokes.T_VisionStrokes")));
    Original.Parameters.VisionRadius = .31f;
    FText Message;
    if (!TestTrue(TEXT("Save selected persistent stroke and flow assets"), FYogAirFogEditorService::SaveNamedPreset(Files.NameA, Original, false, Message))) return false;
    FYogAirFogPresetData Loaded;
    if (!TestTrue(TEXT("Reload named screen-flow draft"), FYogAirFogEditorService::LoadNamedPreset(Files.NameA, Loaded, Message))) return false;
    TestEqual(TEXT("Stroke selection survives JSON round trip"), Loaded.Parameters.StrokeTexture.ToSoftObjectPath(), Original.Parameters.StrokeTexture.ToSoftObjectPath());
    TestEqual(TEXT("Flow selection survives JSON round trip independently"), Loaded.Parameters.FlowTexture.ToSoftObjectPath(), Original.Parameters.FlowTexture.ToSoftObjectPath());
    TestEqual(TEXT("Screen range survives JSON round trip"), Loaded.Parameters.VisionRadius, .31f);
    FString Before, After;
    FFileHelper::LoadFileToString(Before, *Files.PathFor(Files.NameA));
    auto* Transient = NewObject<UTexture2D>(GetTransientPackage(), NAME_None, RF_Transient);
    Loaded.Parameters.FlowTexture = Transient;
    TestFalse(TEXT("A transient flow texture cannot overwrite a saved preset"), FYogAirFogEditorService::SaveNamedPreset(Files.NameA, Loaded, true, Message));
    FFileHelper::LoadFileToString(After, *Files.PathFor(Files.NameA));
    TestEqual(TEXT("Rejected transient resource preserves exact original file bytes"), After, Before);
    return true;
}

#endif
