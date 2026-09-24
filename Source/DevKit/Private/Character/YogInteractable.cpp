#include "Character/YogInteractable.h"

#include "Component/InteractPromptComponent.h"
#include "GameFramework/Actor.h"

UInteractPromptComponent* IYogInteractable::GetInteractPromptComponent() const
{
	// _getUObject is the only route from the interface back to the actor that owns the component.
	const UObject* Self = const_cast<IYogInteractable*>(this)->_getUObject();
	const AActor* OwnerActor = Cast<AActor>(Self);

	return OwnerActor ? OwnerActor->FindComponentByClass<UInteractPromptComponent>() : nullptr;
}

void IYogInteractable::SetInteractHoldProgress(float Normalized)
{
	if (UInteractPromptComponent* Prompt = GetInteractPromptComponent())
	{
		Prompt->SetHoldProgress(Normalized);
	}
}

void IYogInteractable::ShowInteractPrompt(bool bVisible)
{
	if (UInteractPromptComponent* Prompt = GetInteractPromptComponent())
	{
		Prompt->SetPromptVisible(bVisible);
	}
}
