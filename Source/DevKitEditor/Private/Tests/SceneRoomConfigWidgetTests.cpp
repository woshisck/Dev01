#if WITH_DEV_AUTOMATION_TESTS

#include "Data/RoomDataAsset.h"
#include "Data/EnemyData.h"
#include "Data/RuneDataAsset.h"
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
		TEXT("RoomName"), TEXT("DisplayName"), TEXT("RoomTags"), TEXT("bIsHubRoom"),
		TEXT("EnemyPool"), TEXT("BuffPool"), TEXT("LootPool"),
		TEXT("LowDifficulty"), TEXT("MediumDifficulty"), TEXT("HighDifficulty"),
		TEXT("bSpawnSpecialRewardEnemy"), TEXT("SpecialRewardEnemyClass"),
		TEXT("SpecialRewardEnemyPreSpawnFX"), TEXT("SpecialRewardEnemyPreSpawnFXDuration"),
		TEXT("bEnableTimedClearObjective"), TEXT("TimedClearSeconds"),
		TEXT("SacrificeEventAltarData"), TEXT("SacrificeEventAltarClass"),
		TEXT("SacrificeEventWidgetClass"), TEXT("SacrificeEventAltarSpawnOffset"),
		TEXT("ShopData"), TEXT("ShopActorClass"), TEXT("ShopWidgetClass"), TEXT("ShopSpawnOffset"),
		TEXT("bUseFixedRewardOptions"), TEXT("FixedRewardOptions"), TEXT("MetaCurrencyRewards")
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
	TestTrue(TEXT("Add enemy entry uses the native property handle"), Widget->AddArrayItemForTesting(TEXT("EnemyPool")));
	TestEqual(TEXT("The authored array is updated directly"), Room->EnemyPool.Num(), 1);
	TestEqual(TEXT("Adding an enemy does not change the runtime room name"), Room->RoomName, FName(TEXT("L1_CommonLevel_Test")));
	// Test populated arrays as well as empty headers: inner struct properties reuse
	// their array's name, and must never replace the editable top-level array handle.
	if (!TestTrue(TEXT("Preview has an enemy row"), Room->EnemyPool.Num() == 1)) return false;
	Room->EnemyPool[0].EnemyData = NewObject<UEnemyData>(Room.Get(), TEXT("DA_PreviewEnemy"));
	Room->EnemyPool[0].MaxCountPerLevel = 12;
	URuneDataAsset* Rune = NewObject<URuneDataAsset>(Room.Get(), TEXT("DA_PreviewBuffAndLoot"));
	FBuffEntry& Buff = Room->BuffPool.AddDefaulted_GetRef();
	Buff.RuneDA = Rune; Buff.DifficultyScore = 2; Buff.ApplyChance = 0.75f;
	Room->LootPool.Add(Rune);
	Widget->SetRoom(Room.Get());
	const auto CheckChild = [this, &Widget](FName ArrayName, FName FieldName)
	{
		const TSharedPtr<IPropertyHandle> Field = Widget->GetArrayElementPropertyForTesting(ArrayName, 0, FieldName);
		TestTrue(FString::Printf(TEXT("Populated %s exposes editable child %s, not a struct summary"),
			*ArrayName.ToString(), *FieldName.ToString()), Field.IsValid() && Field->IsValidHandle());
		return Field;
	};
	const TSharedPtr<IPropertyHandle> EnemyDataHandle = CheckChild(TEXT("EnemyPool"), TEXT("EnemyData"));
	const TSharedPtr<IPropertyHandle> EnemyMaxHandle = CheckChild(TEXT("EnemyPool"), TEXT("MaxCountPerLevel"));
	CheckChild(TEXT("BuffPool"), TEXT("RuneDA"));
	const TSharedPtr<IPropertyHandle> BuffScoreHandle = CheckChild(TEXT("BuffPool"), TEXT("DifficultyScore"));
	const TSharedPtr<IPropertyHandle> BuffChanceHandle = CheckChild(TEXT("BuffPool"), TEXT("ApplyChance"));
	if (EnemyDataHandle.IsValid())
	{
		UObject* EnemyValue = nullptr;
		TestTrue(TEXT("Enemy data picker reads the authored object"), EnemyDataHandle->GetValue(EnemyValue) == FPropertyAccess::Success
			&& EnemyValue == Room->EnemyPool[0].EnemyData.Get());
	}
	if (EnemyMaxHandle.IsValid())
	{
		int32 MaxCount = 0;
		TestTrue(TEXT("Enemy limit value reads the authored field"), EnemyMaxHandle->GetValue(MaxCount) == FPropertyAccess::Success && MaxCount == 12);
		TestTrue(TEXT("Enemy limit is editable through the native field control"), EnemyMaxHandle->SetValue(15) == FPropertyAccess::Success);
		TestEqual(TEXT("Enemy field edit writes back to the real array entry"), Room->EnemyPool[0].MaxCountPerLevel, 15);
	}
	if (BuffScoreHandle.IsValid())
	{
		int32 Score = 0;
		TestTrue(TEXT("Buff difficulty is an editable child value"), BuffScoreHandle->GetValue(Score) == FPropertyAccess::Success && Score == 2);
	}
	if (BuffChanceHandle.IsValid())
	{
		float Chance = 0;
		TestTrue(TEXT("Buff probability is an editable child value"), BuffChanceHandle->GetValue(Chance) == FPropertyAccess::Success && FMath::IsNearlyEqual(Chance, 0.75f));
	}
	const TSharedPtr<IPropertyHandle> LootHandle = Widget->GetArrayElementPropertyForTesting(TEXT("LootPool"), 0);
	TestTrue(TEXT("Loot remains an editable asset array"), LootHandle.IsValid() && LootHandle->IsValidHandle());
	TestTrue(TEXT("Populated enemy array still provides Add"), Widget->AddArrayItemForTesting(TEXT("EnemyPool")));
	TestEqual(TEXT("Add writes a second real enemy entry"), Room->EnemyPool.Num(), 2);
	TestTrue(TEXT("Populated enemy array still provides Delete"), Widget->DeleteArrayItemForTesting(TEXT("EnemyPool"), 1));
	TestEqual(TEXT("Delete removes only the selected array entry"), Room->EnemyPool.Num(), 1);
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
