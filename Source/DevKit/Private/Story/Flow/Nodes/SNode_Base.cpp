#include "Story/Flow/Nodes/SNode_Base.h"

#include "Story/Flow/StoryFlowAsset.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/Encounter/StoryFlowProxy.h"
#include "Kismet/GameplayStatics.h"

USNode_Base::USNode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	AllowedAssetClasses = { UStoryFlowAsset::StaticClass() };
#endif
}

UStoryEngineSubsystem* USNode_Base::GetStoryEngine() const
{
	if (UWorld* World = GetWorld())
		if (UGameInstance* GI = World->GetGameInstance())
			return GI->GetSubsystem<UStoryEngineSubsystem>();
	return nullptr;
}

APlayerController* USNode_Base::GetPlayerController() const
{
	if (UWorld* World = GetWorld())
		return World->GetFirstPlayerController();
	return nullptr;
}

AStoryFlowProxy* USNode_Base::GetStoryProxy() const
{
	return Cast<AStoryFlowProxy>(TryGetRootFlowActorOwner());
}
