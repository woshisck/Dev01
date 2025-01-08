// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogWorldSubsystem.h"



UYogWorldSubsystem::UYogWorldSubsystem()
{
}

void UYogWorldSubsystem::InitLevel()
{
}

void UYogWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	
	UE_LOG(LogTemp, Warning,TEXT("YogSubsystem onWorldBeginPlay"));
	UWorld* world = GetCurrentWorld();
	if (world != nullptr)
	{
		FName worldName;
		worldName = world->GetFName();
		
		UE_LOG(LogTemp, Warning, TEXT("WORLD: %s"), *worldName.ToString());
	}


}

UWorld* UYogWorldSubsystem::GetCurrentWorld()
{
	UWorld* world = nullptr;
	FName worldName;
#if WITH_EDITOR
	world = GEditor->GetEditorWorldContext().World();
	return world;
#else
	world = GEngine->GetCurrentPlayWorld();
	return world;
#endif
	


//#if WITH_EDITOR
//	if (GIsEditor)
//	{
//		if (GPlayInEditorID == -1)
//		{
//			FWorldContext* worldContext = GEditor->GetPIEWorldContext(1);
//			if (worldContext == nullptr)
//			{
//				if (UGameViewportClient* viewport = GEngine->GameViewport)
//				{
//					world = viewport->GetWorld();
//				}
//			}
//			else
//			{
//				world = worldContext->World();
//			}
//		}
//		else
//		{
//			FWorldContext* worldContext = GEditor->GetPIEWorldContext(GPlayInEditorID);
//			if (worldContext == nullptr)
//			{
//				return nullptr;
//			}
//			world = worldContext->World();
//		}
//	}
//	else
//	{
//		world = GEngine->GetCurrentPlayWorld(nullptr);
//	}
//	worldName = world->GetFName();
//	UE_LOG(LogTemp, Warning, TEXT("WORLD: %s"), *worldName.ToString());
//#else
//	world = GEngine->GetCurrentPlayWorld(nullptr);
//#endif
//	world = GEngine->GetWorld();
//	worldName = world->GetFName();
//	UE_LOG(LogTemp, Warning, TEXT("WORLD: %s"),*worldName.ToString());

}
