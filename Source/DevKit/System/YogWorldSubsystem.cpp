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
}

UWorld* UYogWorldSubsystem::GetCurrentWorld()
{
	UWorld* world = nullptr;
#if WITH_EDITOR
	if (GIsEditor)
	{
		if (GPlayInEditorID == -1)
		{
			FWorldContext* worldContext = GEditor->GetPIEWorldContext(1);
			if (worldContext == nullptr)
			{
				if (UGameViewportClient* viewport = GEngine->GameViewport)
				{
					world = viewport->GetWorld();
				}
			}
			else
			{
				world = worldContext->World();
			}
		}
		else
		{
			FWorldContext* worldContext = GEditor->GetPIEWorldContext(GPlayInEditorID);
			if (worldContext == nullptr)
			{
				return nullptr;
			}
			world = worldContext->World();
		}
	}
	else
	{
		world = GEngine->GetCurrentPlayWorld(nullptr);
	}
#else
	world = GEngine->GetCurrentPlayWorld(nullptr);
#endif
	return world;
}
