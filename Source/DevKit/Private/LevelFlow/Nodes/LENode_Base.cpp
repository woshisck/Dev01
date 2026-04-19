#include "LevelFlow/Nodes/LENode_Base.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Tutorial/TutorialManager.h"
#include "Kismet/GameplayStatics.h"

ULENode_Base::ULENode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// 只在 LevelFlowAsset（关卡事件 Flow）编辑器中出现，不显示在 BuffFlow 里
	AllowedAssetClasses = { ULevelFlowAsset::StaticClass() };
#endif
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
