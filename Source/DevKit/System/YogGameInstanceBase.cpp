// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogGameInstanceBase.h"
#include "Player/PlayerCharacterBase.h"
#include "DevKit/SaveGame/YogSaveGame.h"
#include "DevKit/SaveGame/YogSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"

#include "DevKit/System/YogWorldSubsystem.h"

#include "DevKit/Map/YogLevelScript.h"

#include "DevKit/Map/YogMapDefinition.h"


UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{
	bShouldLoadSaveAfterMap = false;
}

APlayerCharacterBase* UYogGameInstanceBase::GetPlayerCharacter()
{
	APlayerCharacterBase* PlayerCharacterBase = NewObject<APlayerCharacterBase>();
	UWorld* World = this->GetWorld();
	if (World)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			return Cast<APlayerCharacterBase>(PlayerController->GetPawn());
		}
	}

	return PlayerCharacterBase;
}


////////////// AI //////////////
void UYogGameInstanceBase::CallNextFunction()
{
	if (CurrentIndex >= FunctionArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Sequence finished"));
		return;
	}

	// Execute the current function
	FunctionArray[CurrentIndex].ExecuteIfBound();

	// Schedule next step after interval
	GetWorld()->GetTimerManager().SetTimer(
		StepTimerHandle,
		this,
		&UYogGameInstanceBase::ForceNextFunction,
		Interval,
		false
	);
}

void UYogGameInstanceBase::StartSequence()
{
	CurrentIndex = 0;
	CallNextFunction();
}

void UYogGameInstanceBase::ForceNextFunction()
{
	GetWorld()->GetTimerManager().ClearTimer(StepTimerHandle);

	// Advance index
	CurrentIndex++;

	// Call next
	CallNextFunction();
}

void UYogGameInstanceBase::StepOne()
{
}

void UYogGameInstanceBase::StepTwo()
{
}

void UYogGameInstanceBase::StepThree()
{
}



/* ASAP: SAMPLE USE -- */

//void AMyActor::BeginPlay()
//{
//	Super::BeginPlay();
//
//	// Fill function array
//	FunctionArray.Add(FMyStepDelegate::CreateUObject(this, &AMyActor::StepOne));
//	FunctionArray.Add(FMyStepDelegate::CreateUObject(this, &AMyActor::StepTwo));
//	FunctionArray.Add(FMyStepDelegate::CreateUObject(this, &AMyActor::StepThree));
//
//	StartSequence();
//}
//
//void AMyActor::StepOne() { UE_LOG(LogTemp, Warning, TEXT("Step One")); }
//void AMyActor::StepTwo() { UE_LOG(LogTemp, Warning, TEXT("Step Two")); }
//void AMyActor::StepThree() { UE_LOG(LogTemp, Warning, TEXT("Step Three")); }






void UYogGameInstanceBase::Init()
{
	Super::Init();
	UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::Init()"));

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UYogGameInstanceBase::OnPostLoadMap);

}

void UYogGameInstanceBase::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Shutdown();
}

void UYogGameInstanceBase::AddCurrentMapKill(int32 adder)
{

	CurrentMapKills += adder;

	if (CurrentMapKills >= TargetMapKills)
	{
		OnFinishLevel.Broadcast();
	}
}

void UYogGameInstanceBase::OpenMapAndLoadSave(const TSoftObjectPtr<UWorld> Level)
{
	// Set the pending save slot and flag
	PendingSaveSlot = SaveSlot;
	bShouldLoadSaveAfterMap = true;

	// Bind the delegate to know when the map is loaded

	// Open the map
	//void UGameplayStatics::OpenLevelBySoftObjectPtr(const UObject * WorldContextObject, const TSoftObjectPtr<UWorld> Level, bool bAbsolute, FString Options)
	UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::OpenMapAndLoadSave"));
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level, true);
}

void UYogGameInstanceBase::OnPostLoadMap(UWorld* World)
{
	
	// If we are supposed to load a save after the map, do it
	if (bShouldLoadSaveAfterMap && World)
	{

		UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::OnPostLoadMap"));

		UYogSaveSubsystem* SaveSubsystem = this->GetSubsystem<UYogSaveSubsystem>();

		if (SaveSubsystem->CurrentSaveGame)
		{
			SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
		}

		//Load map quest
		UYogWorldSubsystem* WorldSubsystem = GetWorld()->GetSubsystem<UYogWorldSubsystem>();
		
		AYogLevelScript* CurrentLevelScript = WorldSubsystem->GetCurrentLevelScript();

		if (CurrentLevelScript)
		{
			this->TargetMapKills = CurrentLevelScript->GetDefinition()->TargetMapKills;
			this->WaveCount = CurrentLevelScript->GetDefinition()->WaveCount;
			this->SpawnCount = CurrentLevelScript->GetDefinition()->SpawnCount;
			this->IntervalTime = CurrentLevelScript->GetDefinition()->IntervalTime;

			
			//COPY VAlUE TO Instance`
		}
		// Reset the flag
		bShouldLoadSaveAfterMap = false;

		//SpawnMob from Map
		StartSequence();

	}

	// Unbind the delegate to avoid multiple bindings
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

void UYogGameInstanceBase::SaveGame()
{
	if (!PersistentSaveData)
	{
		PersistentSaveData = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}
	APlayerCharacterBase* CurrentCharacter = GetPlayerCharacter();
	PersistentSaveData->SavedCharacterClass = CurrentCharacter->GetClass();

	UGameplayStatics::SaveGameToSlot(PersistentSaveData, "Slot1", 0);
}

UYogSaveGame* UYogGameInstanceBase::GetCurrentSaveGame()
{
	return CurrentSaveGame;
}

void UYogGameInstanceBase::SetSavingEnabled(bool bEnabled)
{
	bSavingEnabled = bEnabled;
}

bool UYogGameInstanceBase::LoadOrCreateSaveGame()
{
	return false;
}

bool UYogGameInstanceBase::HandleSaveGameLoaded(USaveGame* SaveGameObject)
{
	bool bLoaded = false;
	return bLoaded;
}

void UYogGameInstanceBase::GetSaveSlotInfo(FString& SlotName, int32& UserIndex) const
{

}

bool UYogGameInstanceBase::WriteSaveGame()
{
	if (bSavingEnabled)
	{
		if (bCurrentlySaving)
		{
			bPendingSaveRequested = true;
			return true;
		}
		bCurrentlySaving = true;
		USaveGame* currentSaveGame = Cast<USaveGame>(GetCurrentSaveGame());
		UGameplayStatics::AsyncSaveGameToSlot(currentSaveGame, SaveSlot, SaveUserIndex, FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UYogGameInstanceBase::HandleAsyncSave));
		return true;
	}
	else 
	{
		return false;
	}
}

void UYogGameInstanceBase::SpawnMobInMap()
{

	UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::SpawnMobInMap()"));

}





void UYogGameInstanceBase::HandleAsyncSave(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	ensure(bCurrentlySaving);
	bCurrentlySaving = false;

	if (bPendingSaveRequested)
	{

		bPendingSaveRequested = false;
		WriteSaveGame();
	}

}
