#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DevourComponent.generated.h"

class UAbilitySystemComponent;
class UDevourOfferDataAsset;
struct FDevourRunEntry;
struct FGameplayAttribute;

USTRUCT(BlueprintType)
struct DEVKIT_API FActiveDevour
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Devour")
	TObjectPtr<UDevourOfferDataAsset> Offer = nullptr;

	// Unique for the whole run; handed to Lua so it can key per-devour state.
	UPROPERTY(BlueprintReadOnly, Category = "Devour")
	int32 Id = 0;

	// Rooms entered since the devour. The devour room itself is not counted.
	UPROPERTY(BlueprintReadOnly, Category = "Devour")
	int32 RoomsEntered = 0;

	// Temp phase over, permanent phase running for the rest of the run.
	UPROPERTY(BlueprintReadOnly, Category = "Devour")
	bool bMatured = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDevourStacksChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDevourMaturedDelegate, UDevourOfferDataAsset*, Offer);

/**
 * Owns the devour rules that are not buff content: the per-room and stack caps (MaxDevoursPerRoom /
 * MaxDevourStacks), room counting, and carrying devours across rooms. What a devour actually does is
 * the offer's Lua module (see UDevourOfferDataAsset for the hooks).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UDevourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDevourComponent();

	UFUNCTION(BlueprintPure, Category = "Devour")
	bool CanDevour() const;

	UFUNCTION(BlueprintCallable, Category = "Devour")
	bool TryDevour(UDevourOfferDataAsset* Offer);

	/**
	 * A room entered by this same player instance (no respawn): resets the per-room count and ages every
	 * temp devour. Level changes respawn the player and go through RestoreFromRunState instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Devour")
	void OnRoomEntered();

	void CaptureRunState(TArray<FDevourRunEntry>& OutEntries) const;

	/** Called on the freshly spawned player of a new room; that room counts as entered. */
	void RestoreFromRunState(const TArray<FDevourRunEntry>& Entries);

	/** Ends every devour (temp and permanent) when the run is over. */
	void ResetForNewRun();

	UFUNCTION(BlueprintPure, Category = "Devour")
	int32 GetMaxDevoursPerRoom() const;

	UFUNCTION(BlueprintPure, Category = "Devour")
	int32 GetMaxDevourStacks() const;

	UFUNCTION(BlueprintPure, Category = "Devour")
	int32 GetDevoursThisRoom() const { return DevoursThisRoom; }

	/** Temp devours only; these are what MaxDevourStacks caps. */
	UFUNCTION(BlueprintPure, Category = "Devour")
	int32 GetActiveStackCount() const;

	/** Temp and matured devours. */
	UFUNCTION(BlueprintPure, Category = "Devour")
	const TArray<FActiveDevour>& GetDevours() const { return Devours; }

	UPROPERTY(BlueprintAssignable, Category = "Devour")
	FDevourStacksChangedDelegate OnDevourStacksChanged;

	UPROPERTY(BlueprintAssignable, Category = "Devour")
	FDevourMaturedDelegate OnDevourMatured;

private:
	UAbilitySystemComponent* GetOwnerASC() const;
	int32 ReadCapAttribute(const FGameplayAttribute& Attribute, int32 DefaultValue) const;
	void Mature(FActiveDevour& Entry, bool bEndTempOnLivePlayer);

	/** Calls Offer's Lua module function with (Player, Offer, Id[, RoomsEntered]). RoomsEntered < 0 is not passed. */
	void CallOfferLua(const FActiveDevour& Entry, const char* FunctionName, int32 RoomsEntered = INDEX_NONE) const;

	UPROPERTY(Transient)
	TArray<FActiveDevour> Devours;

	int32 DevoursThisRoom = 0;
	int32 NextDevourId = 1;
};
