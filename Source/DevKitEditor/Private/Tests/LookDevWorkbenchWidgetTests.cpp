#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Layout/Children.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Tools/CharacterLookDev/SDevKitCharacterLookDevWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
using FPackageSnapshot = TMap<FString, bool>;

FPackageSnapshot SnapshotPackages()
{
	FPackageSnapshot Snapshot;
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		Snapshot.Add(It->GetName(), It->IsDirty());
	}
	return Snapshot;
}

void CheckPackagesUnchanged(FAutomationTestBase* Test, const FPackageSnapshot& Before)
{
	const FPackageSnapshot After = SnapshotPackages();
	Test->TestEqual(TEXT("Construct/destruct preserves loaded package count"), After.Num(), Before.Num());
	for (const auto& Pair : Before)
	{
		const bool* bDirtyAfter = After.Find(Pair.Key);
		if (Test->TestNotNull(*FString::Printf(TEXT("Existing package remains loaded: %s"), *Pair.Key), bDirtyAfter))
		{
			Test->TestEqual(*FString::Printf(TEXT("Package dirty state is unchanged: %s"), *Pair.Key), *bDirtyAfter, Pair.Value);
		}
	}
	for (const auto& Pair : After)
	{
		Test->TestTrue(*FString::Printf(TEXT("Construction did not load/create a package: %s"), *Pair.Key), Before.Contains(Pair.Key));
	}
}

void CollectText(const TSharedRef<SWidget>& Widget, TArray<FString>& OutText)
{
	if (Widget->GetType() == FName(TEXT("STextBlock")))
	{
		OutText.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
	}
	if (FChildren* Children = Widget->GetAllChildren())
	{
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			CollectText(Children->GetChildAt(Index), OutText);
		}
	}
}

void CollectButtonText(const TSharedRef<SWidget>& Widget, TArray<TArray<FString>>& OutButtons)
{
	if (Widget->GetType() == FName(TEXT("SButton")))
	{
		TArray<FString> Text;
		CollectText(Widget, Text);
		OutButtons.Add(MoveTemp(Text));
	}
	if (FChildren* Children = Widget->GetAllChildren())
	{
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			CollectButtonText(Children->GetChildAt(Index), OutButtons);
		}
	}
}

void CountLightingMemoBoxes(const TSharedRef<SWidget>& Widget, int32& Count)
{
	if (Widget->GetType() == FName(TEXT("SMultiLineEditableTextBox"))) ++Count;
	if (FChildren* Children = Widget->GetAllChildren())
	{
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			CountLightingMemoBoxes(Children->GetChildAt(Index), Count);
		}
	}
}

void CollectExpandableGroups(const TSharedRef<SWidget>& Widget, TArray<TSharedRef<SExpandableArea>>& Groups)
{
	if (Widget->GetType() == FName(TEXT("SExpandableArea")))
	{
		Groups.Add(StaticCastSharedRef<SExpandableArea>(Widget));
	}
	if (FChildren* Children = Widget->GetAllChildren())
	{
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			CollectExpandableGroups(Children->GetChildAt(Index), Groups);
		}
	}
}

void CheckGroupCollapseReadOnly(FAutomationTestBase* Test, const TSharedRef<SDevKitCharacterLookDevWidget>& Widget)
{
	TArray<TSharedRef<SExpandableArea>> Groups;
	CollectExpandableGroups(Widget, Groups);
	Test->TestEqual(TEXT("Review and lighting navigation groups collapse independently"), Groups.Num(), 2);
	Widget->SlatePrepass(1.f);
	const float ExpandedHeight = Widget->GetDesiredSize().Y;
	for (const TSharedRef<SExpandableArea>& Group : Groups)
	{
		Test->TestTrue(TEXT("LookDev controls are expanded on first open"), Group->IsExpanded());
		Group->SetExpanded(false);
		Test->TestFalse(TEXT("Group collapses without replacing its controls"), Group->IsExpanded());
	}
	Widget->SlatePrepass(1.f);
	Test->TestTrue(TEXT("Collapsing groups reduces panel height"), Widget->GetDesiredSize().Y < ExpandedHeight);
	for (const TSharedRef<SExpandableArea>& Group : Groups)
	{
		Group->SetExpanded(true);
		Test->TestTrue(TEXT("Group can be expanded again"), Group->IsExpanded());
	}
	Widget->SlatePrepass(1.f);
}

void CheckWorkbenchStructure(FAutomationTestBase* Test, const TSharedRef<SDevKitCharacterLookDevWidget>& Widget)
{
	TArray<FString> Text;
	CollectText(Widget, Text);
	Test->TestTrue(TEXT("Upper level section is present"), Text.Contains(TEXT("关卡")));
	Test->TestTrue(TEXT("Lower lighting section is present"), Text.Contains(TEXT("灯光关卡")));
	Test->TestTrue(TEXT("Panel is explicitly focused on lighting comparisons"), Text.Contains(TEXT("LookDev / 光照对比")));
	Test->TestTrue(TEXT("Main light has an independent details column"), Text.ContainsByPredicate([](const FString& Item)
	{
		return Item.StartsWith(TEXT("Directional Light / 主光")) || Item.StartsWith(TEXT("Key Light / 主光"));
	}));
	Test->TestTrue(TEXT("Sky or fill light has an independent details column"), Text.ContainsByPredicate([](const FString& Item)
	{
		return Item.StartsWith(TEXT("Sky Light / 环境光")) || Item.StartsWith(TEXT("Fill Light / 补光"));
	}));
	Test->TestFalse(TEXT("Camera controls are no longer part of LookDev lighting comparison"), Text.Contains(TEXT("镜头对照 / 游戏远景")));
	Test->TestFalse(TEXT("Pipeline diagnostics are no longer part of LookDev lighting comparison"), Text.Contains(TEXT("灯光 / 阴影对照与诊断")));
	TArray<TArray<FString>> Buttons;
	CollectButtonText(Widget, Buttons);
	int32 LoadButtons = 0;
	for (const auto& Button : Buttons)
	{
		if (Button.Contains(TEXT("加载关卡"))) ++LoadButtons;
	}
	Test->TestEqual(TEXT("One explicit action loads the selected review level"), LoadButtons, 1);
	const TCHAR* CameraAndDiagnosisButtons[] = { TEXT("所选物件近景"), TEXT("读取玩家 BP 相机"), TEXT("捕获 PIE 最终镜头"), TEXT("应用游戏参考镜头"), TEXT("应用手填参考"), TEXT("Lighting Only"), TEXT("VSM 阴影遮罩"), TEXT("VSM 层级 / Mip"), TEXT("VSM 缓存") };
	for (const TCHAR* Label : CameraAndDiagnosisButtons)
	{
		Test->TestFalse(*FString::Printf(TEXT("Removed camera/diagnostic action: %s"), Label), Buttons.ContainsByPredicate([Label](const TArray<FString>& Button) { return Button.Contains(Label); }));
	}
	int32 MemoBoxes = 0;
	CountLightingMemoBoxes(Widget, MemoBoxes);
	Test->TestEqual(TEXT("Lighting-only panel has no unrelated memo editors"), MemoBoxes, 0);
	const TCHAR* RemovedControls[] = { TEXT("Raytracing Shadow"), TEXT("Enable Transmission"), TEXT("Use Hard Ware Raytracing"), TEXT("Enable Checker"), TEXT("Turn Lighting") };
	for (const TCHAR* Label : RemovedControls)
	{
		Test->TestFalse(*FString::Printf(TEXT("Red-box control removed: %s"), Label), Text.Contains(Label));
	}
	const auto& Presets = FDevKitCharacterLookDevPresets::GetPresets();
	Test->TestEqual(TEXT("Neutral plus one standard lighting environment per level are available"), Presets.Num(), 4);
	for (const auto& Preset : Presets)
	{
		int32 Matches = 0;
		for (const auto& Button : Buttons)
		{
			if (Button.ContainsByPredicate([&Preset](const FString& Label) { return Label.Contains(Preset.Label); })) ++Matches;
		}
		Test->TestEqual(*FString::Printf(TEXT("Lighting %s has one individual switch button"), *Preset.Id), Matches, 1);
	}
}

/** Render the native panel without opening a map, clicking a button or reading the desktop. */
class FLookDevWorkbenchCaptureCommand final : public IAutomationLatentCommand
{
public:
	FLookDevWorkbenchCaptureCommand(FAutomationTestBase* InTest, TSharedRef<SDevKitCharacterLookDevWidget> InWidget)
		: Test(InTest), Widget(InWidget), Renderer(MakeUnique<FWidgetRenderer>(false, true)), Began(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Began > 60.0)
		{
			Test->AddError(TEXT("Native LookDev panel capture exceeded 60 seconds."));
			return true;
		}
		const FIntPoint Sizes[] = { FIntPoint(900, 900), FIntPoint(640, 900) };
		const FIntPoint Size = Sizes[Index];
		if (!Target.IsValid())
		{
			// Slate emits linear shader color; the sRGB target applies display conversion once.
			Target.Reset(FWidgetRenderer::CreateTargetFor(FVector2D(Size), TF_Bilinear, true));
			if (!Test->TestTrue(TEXT("Create native Slate capture target"), Target.IsValid())) return true;
			SizeBegan = FPlatformTime::Seconds();
			Draws = 0;
		}
		Widget->Invalidate(EInvalidateWidgetReason::Layout);
		Renderer->DrawWidget(Target.Get(), Widget, 1.f, FVector2D(Size), 0.f);
		if (++Draws < 5 || FPlatformTime::Seconds() - SizeBegan < 0.4) return false;
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		if (!Test->TestTrue(TEXT("Read settled native Slate pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))) return true;
		if (!Test->TestEqual(TEXT("Capture contains all requested pixels"), Pixels.Num(), Size.X * Size.Y)) return true;
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
		if (!Test->TestTrue(TEXT("PNG capture has nonempty image data"), Png.Num() > 32)) return true;
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/LookDevWorkbench");
		if (!Test->TestTrue(TEXT("Create LookDev capture output directory"), IFileManager::Get().MakeDirectory(*Directory, true))) return true;
		const FString Filename = Directory / FString::Printf(TEXT("LookDevWorkbench_%dx%d.png"), Size.X, Size.Y);
		if (!Test->TestTrue(TEXT("Save native LookDev workbench PNG"), FFileHelper::SaveArrayToFile(Png, *Filename))) return true;
		Test->AddInfo(FString::Printf(TEXT("LOOKDEV_WORKBENCH_NATIVE_CAPTURE file=%s width=%d height=%d bytes=%lld"), *Filename, Size.X, Size.Y, int64(Png.Num())));
		Target.Reset();
		return ++Index >= UE_ARRAY_COUNT(Sizes);
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<SDevKitCharacterLookDevWidget> Widget;
	TUniquePtr<FWidgetRenderer> Renderer;
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	int32 Index = 0;
	int32 Draws = 0;
	double Began = 0;
	double SizeBegan = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevWorkbenchConstructionReadOnlyTest,
	"DevKitEditor.LookDevWorkbench.WidgetConstructionReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevWorkbenchConstructionReadOnlyTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Code-driven LookDev entry is registered"), IConsoleManager::Get().FindConsoleObject(TEXT("DevKit.LookDev.Open")));
	if (!TestTrue(TEXT("Slate is initialized"), FSlateApplication::IsInitialized())) return false;
	UWorld* BeforeWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("Current editor world is available"), BeforeWorld)) return false;
	ULevel* BeforeLevel = BeforeWorld->GetCurrentLevel();
	const FPackageSnapshot BeforePackages = SnapshotPackages();
	{
		TSharedRef<SDevKitCharacterLookDevWidget> Widget = SNew(SDevKitCharacterLookDevWidget);
		Widget->SlatePrepass(1.f);
		Widget->SlatePrepass(1.5f);
		TestTrue(TEXT("Constructed panel has a nonzero desired size"), Widget->GetDesiredSize().X > 0.f && Widget->GetDesiredSize().Y > 0.f);
		CheckWorkbenchStructure(this, Widget);
		CheckGroupCollapseReadOnly(this, Widget);
		CheckWorkbenchStructure(this, Widget);
		TestTrue(TEXT("Construction preserves current editor world"), GEditor->GetEditorWorldContext().World() == BeforeWorld);
		TestTrue(TEXT("Construction preserves current editing sublevel"), BeforeWorld->GetCurrentLevel() == BeforeLevel);
	}
	TestTrue(TEXT("Panel destruction preserves current editor world"), GEditor->GetEditorWorldContext().World() == BeforeWorld);
	TestTrue(TEXT("Panel destruction preserves current editing sublevel"), BeforeWorld->GetCurrentLevel() == BeforeLevel);
	CheckPackagesUnchanged(this, BeforePackages);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevWorkbenchNativePanelRenderTest,
	"DevKitEditor.LookDevWorkbench.NativePanelRender",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevWorkbenchNativePanelRenderTest::RunTest(const FString& Parameters)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("Dev01LookDevWidgetRender")))
	{
		AddInfo(TEXT("Skipped: native panel capture requires -Unattended -Dev01LookDevWidgetRender in a dedicated editor process."));
		return true;
	}
	if (!TestTrue(TEXT("Slate is initialized"), FSlateApplication::IsInitialized())
		|| !TestFalse(TEXT("Native panel captures require a real RHI"), GUsingNullRHI)) return false;
	TSharedRef<SDevKitCharacterLookDevWidget> Widget = SNew(SDevKitCharacterLookDevWidget);
	Widget->SlatePrepass(1.f);
	CheckWorkbenchStructure(this, Widget);
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FLookDevWorkbenchCaptureCommand>(this, Widget));
	return true;
}

#endif
