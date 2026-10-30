#if WITH_DEV_AUTOMATION_TESTS

#include "Data/RoomDataAsset.h"
#include "Map/Portal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PropertyHandle.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/AppStyle.h"
#include "Tools/SceneWorkbench/SDevKitSceneRoomConfigWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SBorder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneRoomConfigWidgetContractTest,
	"DevKit.SceneWorkbench.RoomConfig.NativeCards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneRoomConfigWidgetContractTest::RunTest(const FString& Parameters)
{
	const TStrongObjectPtr<URoomDataAsset> Room(NewObject<URoomDataAsset>(GetTransientPackage(), NAME_None, RF_Transactional));
	Room->RoomName = TEXT("L1_CommonLevel_Test");
	const TSharedRef<SDevKitSceneRoomConfigWidget> Widget = SNew(SDevKitSceneRoomConfigWidget);
	Widget->SetRoom(Room.Get());
	TestEqual(TEXT("The form edits the existing asset, not a data copy"), Widget->GetRoom(), Room.Get());
	TestTrue(TEXT("Defaults panel constructs"), Widget->MakeDefaultsPanel()->GetType() != NAME_None);
	TestTrue(TEXT("Gameplay panel constructs"), Widget->MakeGameplayPanel()->GetType() != NAME_None);
	const FName RequiredFields[] = {
		TEXT("RoomName"), TEXT("DisplayName"), TEXT("RoomTags"), TEXT("LevelAsset")
	};
	for (FName Field : RequiredFields)
	{
		TestTrue(FString::Printf(TEXT("Existing RoomData field %s is available"), *Field.ToString()),
			Widget->HasPropertyForTesting(Field));
	}
	const FName RemovedPortalFields[] = {
		TEXT("PortalDestinations"), TEXT("bForceSinglePortal"), TEXT("ForcedPortalIndex")
	};
	for (FName Field : RemovedPortalFields)
	{
		TestFalse(FString::Printf(TEXT("Removed per-room portal field %s is not exposed"), *Field.ToString()),
			Widget->HasPropertyForTesting(Field));
	}
	TestNotNull(TEXT("Portal destinations remain owned by placed portal actors"),
		FindFProperty<FNameProperty>(APortal::StaticClass(), GET_MEMBER_NAME_CHECKED(APortal, DestinationLevel)));
	Widget->SetRoom(Room.Get());
	if (FParse::Param(FCommandLine::Get(), TEXT("SceneWorkbenchRender")))
	{
		// Render representative rows using transient objects only. No production asset is loaded or saved.
		const TSharedRef<SWidget> Gameplay = SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(12)
			[Widget->MakeGameplayPanel()];
		constexpr int32 Width = 1600, Height = 1000;
		FWidgetRenderer Renderer(false);
		UTextureRenderTarget2D* Target = nullptr;
		for (int32 Frame = 0; Frame < 3; ++Frame)
		{
			Gameplay->SlatePrepass();
			Target = Renderer.DrawWidget(Gameplay, FVector2D(Width, Height));
			FlushRenderingCommands();
		}
		if (!TestNotNull(TEXT("Native gameplay component target"), Target)) return false;
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
		if (!TestTrue(TEXT("Native gameplay component pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags))) return false;
		for (FColor& Pixel : Pixels) Pixel = Pixel.ReinterpretAsLinear().ToFColor(true);
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Width, Height, Pixels, Png);
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports");
		IFileManager::Get().MakeDirectory(*Directory, true);
		TestTrue(TEXT("Native gameplay PNG saved"), FFileHelper::SaveArrayToFile(Png, *(Directory / TEXT("SceneGameplay.png"))));
	}
	Widget->SetRoom(nullptr);
	TestNull(TEXT("Clearing selection releases the room"), Widget->GetRoom());
	return true;
}

#endif
