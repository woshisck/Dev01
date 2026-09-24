#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "YogInteractable.generated.h"

class APlayerCharacterBase;
class UInteractPromptComponent;

/**
 * Tie-break order when several interact volumes overlap the player at once.
 * Higher wins. Values are spaced so new interactables can slot between them.
 */
namespace YogInteractPriority
{
	constexpr int32 WeaponSpawner = 60;
	constexpr int32 Pickup        = 50;
	constexpr int32 Altar         = 40;
	constexpr int32 Shop          = 30;
	constexpr int32 Portal        = 20;
	constexpr int32 Facility      = 10;
}

UINTERFACE(MinimalAPI)
class UYogInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Hold-to-interact contract, driven by the interact key (keyboard E / gamepad A).
 *
 * Implementers call APlayerCharacterBase::RegisterInteractable(this) on overlap begin and
 * UnregisterInteractable(this) on overlap end. The hold duration lives on IA_Interact's Hold
 * trigger; AYogPlayerControllerBase owns the hold and cancel rules.
 *
 * Add a UInteractPromptComponent to get the on-screen prompt: the two prompt functions below
 * find it on the implementing actor, so an interactable only decides *when* the prompt is up.
 */
class DEVKIT_API IYogInteractable
{
	GENERATED_BODY()

public:
	/** Hold completed — run the interaction. */
	virtual void TryInteract(APlayerCharacterBase* Player) = 0;

	/** Higher wins when several interactables overlap. See YogInteractPriority. */
	virtual int32 GetInteractPriority() const = 0;

	/** 0..1 during a hold, 0 on cancel or completion. */
	virtual void SetInteractHoldProgress(float Normalized);

	/** Show or hide the prompt. Callers apply their own gating first. */
	virtual void ShowInteractPrompt(bool bVisible);

	/** Prompt component on the implementing actor, or null when it has none. */
	UInteractPromptComponent* GetInteractPromptComponent() const;

	/** False keeps the actor registered but skips it while resolving (phase gates, already consumed). */
	virtual bool CanInteract(const APlayerCharacterBase* Player) const { return true; }
};
