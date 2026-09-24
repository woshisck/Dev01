#include "Interface/YogBubbleSpeaker.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "System/YogBubbleSubsystem.h"
#include "UI/BubbleMessageTypes.h"

namespace
{
	/** _getUObject is the only route from the interface back to the actor that implements it. */
	AActor* YogBubbleSpeaker_GetActor(const IYogBubbleSpeaker* Self)
	{
		UObject* AsObject = const_cast<IYogBubbleSpeaker*>(Self)->_getUObject();
		return Cast<AActor>(AsObject);
	}

	UYogBubbleSubsystem* YogBubbleSpeaker_GetSubsystem(const AActor* Actor)
	{
		const UWorld* World = Actor ? Actor->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UYogBubbleSubsystem>() : nullptr;
	}
}

void IYogBubbleSpeaker::ShowBubble(const FDataTableRowHandle& RowHandle, FGameplayTag DedupTag)
{
	AActor* Speaker = YogBubbleSpeaker_GetActor(this);
	if (UYogBubbleSubsystem* Subsystem = YogBubbleSpeaker_GetSubsystem(Speaker))
	{
		Subsystem->RequestBubbleFromRow(Speaker, RowHandle, DedupTag);
	}
}

void IYogBubbleSpeaker::ShowBubbleByName(FName RowName, FGameplayTag DedupTag)
{
	AActor* Speaker = YogBubbleSpeaker_GetActor(this);
	if (UYogBubbleSubsystem* Subsystem = YogBubbleSpeaker_GetSubsystem(Speaker))
	{
		Subsystem->RequestBubbleByName(Speaker, RowName, DedupTag);
	}
}

void IYogBubbleSpeaker::ShowBubbleText(const FText& Line, float Duration)
{
	AActor* Speaker = YogBubbleSpeaker_GetActor(this);
	if (UYogBubbleSubsystem* Subsystem = YogBubbleSpeaker_GetSubsystem(Speaker))
	{
		Subsystem->RequestBubbleText(
			Speaker,
			GetBubbleSpeakerName(),
			Line,
			Duration,
			EBubbleAnchorMode::Auto,
			YogBubblePriority::Ambient,
			FGameplayTag());
	}
}

void IYogBubbleSpeaker::DismissBubble()
{
	AActor* Speaker = YogBubbleSpeaker_GetActor(this);
	if (UYogBubbleSubsystem* Subsystem = YogBubbleSpeaker_GetSubsystem(Speaker))
	{
		Subsystem->DismissBubbleFor(Speaker);
	}
}
