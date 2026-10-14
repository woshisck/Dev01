#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/StyleColors.h"
#include "Framework/Application/SlateApplication.h"
#include "GameplayTagsManager.h"
#include "Tools/ActionWorkbench/SDevKitActionWorkbenchWidget.h"
#include "UObject/Package.h"

namespace
{
/** Allow normal editor frames to settle auto-wrap, font atlases and SVG resources before sampling. */
class FActionWorkbenchCaptureCommand final : public IAutomationLatentCommand
{
public:
	FActionWorkbenchCaptureCommand(FAutomationTestBase* InTest, TSharedRef<SDevKitActionWorkbenchWidget> InWidget)
		: Test(InTest), Widget(InWidget), Renderer(MakeUnique<FWidgetRenderer>(false, true)) {}
	virtual bool Update() override
	{
		const FIntPoint Sizes[] = { FIntPoint(1600, 900), FIntPoint(1024, 720), FIntPoint(1920, 1080) };
		const float Scales[] = { 1.f, 1.f, 1.5f };
		const FIntPoint Size = Sizes[Index];
		if (!Target.IsValid())
		{
			// Linear Slate shader output + sRGB target gives exactly one display conversion.
			Target.Reset(FWidgetRenderer::CreateTargetFor(FVector2D(Size), TF_Bilinear, true));
			if (!Test->TestTrue(TEXT("Create native Slate capture target"), Target.IsValid())) return true;
			Began = FPlatformTime::Seconds();
			Draws = 0;
		}
		Widget->Invalidate(EInvalidateWidgetReason::Layout);
		Renderer->DrawWidget(Target.Get(), Widget, Scales[Index], FVector2D(Size), 0.f);
		if (++Draws < 5 || FPlatformTime::Seconds() - Began < 0.4) return false;
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false); // Preserve the Slate vertex/display colors without double gamma.
		if (!Test->TestTrue(TEXT("Read settled Slate render pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))) return true;
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
		Test->AddInfo(FString::Printf(TEXT("Slate panel color: expected=%s captured=%s"),
			*FStyleColors::Panel.GetSpecifiedColor().ToFColor(true).ToHex(), *Pixels[10 * Size.X + 10].ToHex()));
		const FColor ExpectedPanel = FStyleColors::Panel.GetSpecifiedColor().ToFColor(true);
		const FColor CapturedPanel = Pixels[10 * Size.X + 10];
		Test->TestTrue(TEXT("Capture preserves actual UE theme panel color within 8-bit quantization"),
			FMath::Abs(int32(ExpectedPanel.R) - CapturedPanel.R) <= 3
			&& FMath::Abs(int32(ExpectedPanel.G) - CapturedPanel.G) <= 3
			&& FMath::Abs(int32(ExpectedPanel.B) - CapturedPanel.B) <= 3);
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("ActionWorkbenchValidation/SlateCaptures/Final");
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FString Filename = Directory / FString::Printf(TEXT("Matrix_%dx%d_%d.png"), Size.X, Size.Y, FMath::RoundToInt(Scales[Index] * 100));
		Test->TestTrue(TEXT("Save generated native Slate capture"), FFileHelper::SaveArrayToFile(Png, *Filename));
		Test->AddInfo(TEXT("Settled native UE Slate render (not desktop screenshot): ") + Filename);
		Target.Reset();
		return ++Index >= UE_ARRAY_COUNT(Sizes);
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<SDevKitActionWorkbenchWidget> Widget;
	TUniquePtr<FWidgetRenderer> Renderer;
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	int32 Index = 0;
	int32 Draws = 0;
	double Began = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchWidgetTest,
	"DevKit.ActionWorkbench.WidgetConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchWidgetTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate is initialized"), FSlateApplication::IsInitialized())) return false;
	UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Temp/ActionWorkbenchWidget/%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewObject<UStateConflictDataAsset>(Package, NAME_None, RF_Transactional));
	FGameplayTagContainer Inventory;
	UGameplayTagsManager::Get().RequestAllGameplayTags(Inventory, true);
	for (const FGameplayTag& Identity : Inventory)
	{
		if (Source->Rules.Num() >= 30) break;
		Source->Rules.AddDefaulted_GetRef().ActiveTag = Identity;
	}
	if (!TestEqual(TEXT("Thirty registered labels available for large catalog fixture"), Source->Rules.Num(), 30)) return false;
	const FString Before = FDevKitActionWorkbenchModel::Snapshot(Source.Get());
	const bool bWasDirty = Package->IsDirty();
	{
		TSharedRef<SDevKitActionWorkbenchWidget> Widget = SNew(SDevKitActionWorkbenchWidget).InitialAsset(Source.Get());
		Widget->SlatePrepass(1.f);
		Widget->SlatePrepass(1.5f);
		TestTrue(TEXT("Workbench has a nonzero desired size"), Widget->GetDesiredSize().X > 0 && Widget->GetDesiredSize().Y > 0);
		TestTrue(TEXT("Read-only construction can close without a discard dialog"), Widget->CanClose());
	}
	TestEqual(TEXT("UI construction/destruction never changes source data"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), Before);
	TestEqual(TEXT("UI construction/destruction preserves package dirty flag"), Package->IsDirty(), bWasDirty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchConfiguredSourceTest,
	"DevKit.ActionWorkbench.ConfiguredSourceReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchConfiguredSourceTest::RunTest(const FString& Parameters)
{
	FString ConfiguredPath;
	GConfig->GetString(TEXT("/Script/DevKit.DevAssetManager"), TEXT("StateConflictData"), ConfiguredPath, GGameIni);
	TStrongObjectPtr<UStateConflictDataAsset> Source(Cast<UStateConflictDataAsset>(FSoftObjectPath(ConfiguredPath).TryLoad()));
	if (!TestTrue(TEXT("Configured real source loads without mutation"), Source.IsValid())) return false;
	const FString Before = FDevKitActionWorkbenchModel::Snapshot(Source.Get());
	const bool bWasDirty = Source->GetOutermost()->IsDirty();
	{
		TSharedRef<SDevKitActionWorkbenchWidget> Widget = SNew(SDevKitActionWorkbenchWidget).InitialAsset(Source.Get());
		Widget->SlatePrepass(1.f);
		TestTrue(TEXT("Loaded source can be inspected without edit dialog"), Widget->CanClose());
	}
	{
		// Exercise the exact menu/console path: no caller-provided InitialAsset. Slate arguments
		// must initialize this pointer, otherwise the widget can treat stack garbage as a source.
		TSharedRef<SDevKitActionWorkbenchWidget> Widget = SNew(SDevKitActionWorkbenchWidget);
		Widget->SlatePrepass(1.f);
		TestTrue(TEXT("Default constructor loads configured source safely"), Widget->CanClose());
		if (FParse::Param(FCommandLine::Get(), TEXT("ActionWorkbenchCapture")))
		{
			if (!TestFalse(TEXT("Native Slate capture requires a real RHI"), GUsingNullRHI)) return false;
			FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FActionWorkbenchCaptureCommand>(this, Widget));
		}
	}
	TestEqual(TEXT("Loaded source fields unchanged"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), Before);
	TestEqual(TEXT("Loaded source dirty state unchanged"), Source->GetOutermost()->IsDirty(), bWasDirty);
	return true;
}

#endif
