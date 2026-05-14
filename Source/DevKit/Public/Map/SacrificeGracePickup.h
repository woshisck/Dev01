#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/PickupInteractable.h"
#include "SacrificeGracePickup.generated.h"

class UBoxComponent;
class UWidgetComponent;
class USacrificeGraceDA;
class APlayerCharacterBase;

/**
 * Retired compatibility pickup.
 *
 * SacrificeGraceOption drops were replaced by RewardPickup/LootSelection. This
 * actor remains loadable for old assets, but it disables collision and does
 * nothing when interacted with.
 */
UCLASS()
class DEVKIT_API ASacrificeGracePickup : public AActor, public IPickupInteractable
{
	GENERATED_BODY()

public:
	ASacrificeGracePickup();

	/** Legacy setter kept for old Blueprint references. */
	UFUNCTION(BlueprintCallable, Category = "SacrificeGrace")
	void SetSacrificeGraceDA(USacrificeGraceDA* DA);

	// ~ IPickupInteractable
	virtual void OnPlayerEnterRange(APlayerCharacterBase* Player) override;
	virtual void OnPlayerLeaveRange(APlayerCharacterBase* Player) override;
	virtual void TryPickup(APlayerCharacterBase* Player) override;
	// ~ End IPickupInteractable

	/** Compatibility close path for old widget references. */
	void ConsumeAndDestroy();

	/** Compatibility reset path for old widget references. */
	void ResetForSkip(APlayerCharacterBase* Player);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup")
	TObjectPtr<UWidgetComponent> PickupHintWidgetComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sacrifice")
	TObjectPtr<USacrificeGraceDA> SacrificeGraceDA;

private:


	bool bPickedUp = false;
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	bool IsPickupAllowed() const;
	void ClearNearbyPlayer();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
