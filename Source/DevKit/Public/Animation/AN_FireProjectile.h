#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_FireProjectile.generated.h"

/**
 * Sends a GameplayEvent to the character's ASC to trigger projectile spawning inside GA_RangeAttack.
 * The notify itself spawns nothing — all actor creation happens on the game thread inside the ability.
 *
 * Workflow:
 *   1. Place on the "fire" frame of the range attack montage.
 *   2. Set EventTag to match GA_RangeAttack.FireEventTag (default: GameplayEvent.RangeAttack.Fire).
 *   3. Optionally set MuzzleSocketName; GA_RangeAttack reads it from EventData.OptionalObject.
 */
UCLASS(meta = (DisplayName = "AN Fire Projectile"))
class DEVKIT_API UAN_FireProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAN_FireProjectile();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** Sent to the character's ASC. Must match GA_RangeAttack.FireEventTag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FGameplayTag EventTag;

	/**
	 * Skeletal mesh socket used to compute the projectile spawn transform.
	 * GA_RangeAttack casts EventData.OptionalObject to this class and reads the name.
	 * Leave blank to spawn at the character's root transform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FName MuzzleSocketName;
};
