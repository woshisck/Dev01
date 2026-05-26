#include "Story/Flow/Nodes/SNode_Base.h"

#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Engine/LocalPlayer.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/Encounter/StoryFlowProxy.h"
#include "Kismet/GameplayStatics.h"
#include "Story/StoryRuleTypes.h"

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

FStoryEventContext USNode_Base::MakeStoryEventContext() const
{
	FStoryEventContext Context;
	const AStoryFlowProxy* Proxy = GetStoryProxy();
	Context.SourceActor = Proxy ? Proxy->GetContextSourceActor() : TryGetRootFlowActorOwner();
	Context.PlayerController = Proxy ? Proxy->GetContextPlayerController() : GetPlayerController();
	if (const UWorld* World = GetWorld())
	{
		Context.MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}
	if (Context.SourceActor)
	{
		Context.SourceName = Context.SourceActor->GetFName();
	}
	return Context;
}

FText USNode_Base::ResolveInputAwareText(const FText& DefaultText, bool bUseInputTextVariants,
	const FText& KeyboardMouseText, const FText& GamepadText) const
{
	if (!bUseInputTextVariants)
	{
		return DefaultText;
	}

	const APlayerController* PlayerController = GetStoryProxy()
		? GetStoryProxy()->GetContextPlayerController()
		: GetPlayerController();
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	const UCommonInputSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(LocalPlayer)
		: nullptr;
	if (!InputSubsystem)
	{
		return DefaultText;
	}

	if (InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad && !GamepadText.IsEmpty())
	{
		return GamepadText;
	}

	if (InputSubsystem->GetCurrentInputType() == ECommonInputType::MouseAndKeyboard && !KeyboardMouseText.IsEmpty())
	{
		return KeyboardMouseText;
	}

	return DefaultText;
}
