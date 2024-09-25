// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GSCLinkAnimLayersComponent.generated.h"

class UAnimInstance;

/** Modular pawn component for "pushing" linked anim layers to a Character */
UCLASS(ClassGroup="GASCompanion", meta = (BlueprintSpawnableComponent))
class GASCOMPANION_API UGSCLinkAnimLayersComponent : public UPawnComponent
{
	GENERATED_BODY()

public:

	/** List of Anim Instances Classes to link to owner skeletal mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<TSubclassOf<UAnimInstance>> LayerTypes;

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent interface

	virtual void LinkAnimLayer(TSubclassOf<UAnimInstance> AnimInstance);
	virtual void UnlinkAnimLayer(TSubclassOf<UAnimInstance> AnimInstance);

protected:

	/** Returns mesh component from owner if it is a Character */
	virtual USkeletalMeshComponent* GetOwnerSkeletalMeshComponent() const;

private:
	
	/** Goes through LayerTypes AnimInstances and sets up linked anim instance for each on owner SM component */
	virtual void LinkAnimLayers();

	/** Goes through LayerTypes AnimInstances and unlinks each anim instance that was setup previously */
	virtual void UnlinkAnimLayers();
};
