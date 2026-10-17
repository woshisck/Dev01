#include "LookDev/DevKitLookDevSetupCommandlet.h"

#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"

UDevKitLookDevSetupCommandlet::UDevKitLookDevSetupCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UDevKitLookDevSetupCommandlet::Main(const FString& Params)
{
	TArray<FString> Packages{ FDevKitLookDevLevelService::MainPackagePath() };
	for (const auto& Entry : FDevKitLookDevLevelService::GetReviewEntries()) Packages.AddUnique(Entry.PackagePath);
	for (const auto& Entry : FDevKitLookDevLevelService::GetLightingEntries()) Packages.AddUnique(Entry.PackagePath);
	for (const FString& Package : Packages)
	{
		UE_LOG(LogTemp, Display, TEXT("LookDev setup %s %s"), FPackageName::DoesPackageExist(Package) ? TEXT("PRESERVE") : TEXT("CREATE"), *Package);
	}
	if (!FParse::Param(*Params, TEXT("Apply")))
	{
		UE_LOG(LogTemp, Display, TEXT("LookDev dry-run complete. -Apply creates missing maps; -PruneRetiredLights removes retired entries from LookDevMain."));
		return 0;
	}
	FText Message;
	TArray<FString> Created;
	if (!FDevKitLookDevLevelService::CreateMissingAssets(Message, &Created))
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Message.ToString());
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("%s"), *Message.ToString());
	for (const FString& Package : Created) UE_LOG(LogTemp, Display, TEXT("LOOKDEV_CREATED=%s"), *Package);
	UE_LOG(LogTemp, Display, TEXT("LOOKDEV_CREATED_COUNT=%d"), Created.Num());
	if (FParse::Param(*Params, TEXT("PruneRetiredLights")))
	{
		if (!FDevKitLookDevLevelService::PruneRetiredLightingReferences(Message))
		{
			UE_LOG(LogTemp, Error, TEXT("%s"), *Message.ToString());
			return 3;
		}
		UE_LOG(LogTemp, Display, TEXT("%s"), *Message.ToString());
		UE_LOG(LogTemp, Display, TEXT("LOOKDEV_MAIN_RETIRED_LIGHT_REFS=%d"), FDevKitLookDevLevelService::GetRetiredLightingPackagePaths().Num());
	}
	if (FParse::Param(*Params, TEXT("VerifyOpen")))
	{
		// This deliberately opens in the same process as first-time setup: it catches stale,
		// cleaned-up bootstrap worlds being reused instead of loading the saved packages.
		if (!FDevKitLookDevLevelService::OpenMain(Message)
			|| !FDevKitLookDevLevelService::IsMainWorld()
			|| FDevKitLookDevLevelService::GetCurrentReviewPath().IsEmpty()
			|| FDevKitLookDevLevelService::GetCurrentLightingPath().IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("LOOKDEV_VERIFY_OPEN=false: %s"), *Message.ToString());
			return 2;
		}
		UE_LOG(LogTemp, Display, TEXT("LOOKDEV_VERIFY_OPEN=true"));
		UE_LOG(LogTemp, Display, TEXT("LOOKDEV_REVIEW=%s"), *FDevKitLookDevLevelService::GetCurrentReviewPath());
		UE_LOG(LogTemp, Display, TEXT("LOOKDEV_LIGHT=%s"), *FDevKitLookDevLevelService::GetCurrentLightingPath());
	}
	return 0;
}
