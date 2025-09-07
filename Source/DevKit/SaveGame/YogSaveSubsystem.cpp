
#include "YogSaveSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

UYogSaveSubsystem::UYogSaveSubsystem()
{
	

}




void UYogSaveSubsystem::UObjectArraySaver(UPARAM(ref) TArray<UObject*>& SaveObjects)
{
	//for (UObject* SaveObject : SaveObjects)
	//{
	//	SaveData(SaveObject, CurrentSaveGame->WorldObjects);
	//}
}

void UYogSaveSubsystem::AutoSave()
{
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (player)
	{
		CurrentSaveGame->PlayerTransform = player->GetActorTransform();
		SaveData(player, CurrentSaveGame->PlayerCharacter);
	}

	UYogWorldSubsystem* worldsubsystem = this->GetWorld()->GetSubsystem<UYogWorldSubsystem>();
	UWorld* current_world = worldsubsystem->GetCurrentWorld();
	CurrentSaveGame->LevelName = current_world->GetPathName();

	//SaveData(current_world, CurrentSaveGame->CurrentLevel.ByteData);


	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, DefaultUserIndex_SOLID);

	UE_LOG(DevKitGame, Display, TEXT("Yogger autosave SaveGame"));

	//bool UGameplayStatics::DoesSaveGameExist(const FString& SlotName, const int32 UserIndex)
	//if (!UGameplayStatics::DoesSaveGameExist(SlotName, DefaultUserIndex_SOLID))
	//{
	//	/*CurrentSaveGame->Initialize(SlotName);*/
	//	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSaveGame->SlotName, DefaultUserIndex_SOLID);
	//	UE_LOG(DevKitGame, Display, TEXT("INIT SaveGame"));
	//}
	//else
	//{
	//	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	//	if (player)
	//	{
	//		CurrentSaveGame->PlayerTransform = player->GetActorTransform();
	//		SaveData(player, CurrentSaveGame->PlayerCharacter);
	//	}
	//	UYogWorldSubsystem* worldsubsystem = this->GetWorld()->GetSubsystem<UYogWorldSubsystem>();
	//	UWorld* current_world = worldsubsystem->GetCurrentWorld();
	//	CurrentSaveGame->LevelName = current_world->GetPathName();
	//	
	//	//SaveData(current_world, CurrentSaveGame->CurrentLevel.ByteData);
	//	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, DefaultUserIndex_SOLID);

	//	UE_LOG(DevKitGame, Display, TEXT("Yogger autosave SaveGame"));
	//}



}

void UYogSaveSubsystem::AutoLoad()
{

	UYogSaveGame* current_save = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, DefaultUserIndex_SOLID));
	if (current_save)
	{
		FString level_name;
		level_name = current_save->LevelName;
		//(const UObject * WorldContextObject, FName LevelName, bool bAbsolute, FString Options)
		UGameplayStatics::OpenLevel(GetWorld(), FName(*level_name));
		APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		LoadData(player, current_save->PlayerCharacter);
		//APlayerCharacterBase* player = current_save->
		player->SetActorTransform(current_save->PlayerTransform);
		
		UE_LOG(DevKitGame, Display, TEXT("Yogger autoload game"));
	}
}





void UYogSaveSubsystem::SaveData(UObject* Object, TArray<uint8>& Data)
{
	if (Object == nullptr)
	{
		return;
	}

	FMemoryWriter MemoryWriter = FMemoryWriter(Data, true);
	FYogSaveGameArchive Archive = FYogSaveGameArchive(MemoryWriter);

	Object->Serialize(Archive);
}


void UYogSaveSubsystem::LoadData(UObject* Object, UPARAM(ref)TArray<uint8>& Data)
{

	if (!Object)
	{
		return;
	}
	FMemoryReader MemoryReader(Data, true);

	FYogSaveGameArchive Ar(MemoryReader);
	Object->Serialize(Ar);
}


//void UYogSaveSubsystem::PopulateCurrentSlot()
//{
//	// Player
//	UYogWorldSubsystem* worldsubsystem = this->GetWorld()->GetSubsystem<UYogWorldSubsystem>();
//	UWorld* current_world = worldsubsystem->GetCurrentWorld();
//	if (current_world)
//	{
//		SerializeObject(current_world, CurrentSaveGame->CurrentLevel.ByteData);
//
//
//		//FString level_name = current_world->GetPathName();
//		//CurrentSaveGame->CurrentLevel.
//
//		//FLevelRecord level_detail = CurrentSaveGame->GenerateLevelRecord(current_world);
//		
//		//SerializeObject(level_detail, CurrentSaveGame->LevelPathName);
//
//	}
//
//
//	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
//	if (Player)
//	{
//		SerializeObject(Player, CurrentSaveGame->PlayerCharacter);
//	}
//
//} 

//void UYogSaveSubsystem::PopulateFromCurrentSlot()
//{
//	if (!CurrentSaveGame)
//	{
//		return;
//	}
//
//	// Player controller.
//	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
//	if (PC)
//	{
//		
//		//SerializeObject(Cast<UObject>(PC), CurrentSaveGame->PlayerController);
//	}
//
//
//	TArray<AActor*> WorldObjects;
//	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), WorldObjects);
//
//}
