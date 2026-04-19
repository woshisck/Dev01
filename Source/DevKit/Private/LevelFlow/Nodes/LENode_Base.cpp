#include "LevelFlow/Nodes/LENode_Base.h"
#include "Tutorial/TutorialManager.h"
#include "Kismet/GameplayStatics.h"

ULENode_Base::ULENode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

APlayerController* ULENode_Base::GetPlayerController() const
{
	if (UWorld* World = GetWorld())
		return World->GetFirstPlayerController();
	return nullptr;
}

UTutorialManager* ULENode_Base::GetTutorialManager() const
{
	if (UWorld* World = GetWorld())
		if (UGameInstance* GI = World->GetGameInstance())
			return GI->GetSubsystem<UTutorialManager>();
	return nullptr;
}
