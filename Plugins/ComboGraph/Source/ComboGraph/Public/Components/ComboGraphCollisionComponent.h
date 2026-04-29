// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ComboGraphCollisionComponent.generated.h"


/**
 * Actor Component providing basic collision detection mechanism for registered meshes using traces.
 *
 * It implements simple logic for hit detection not meant to replace more robust solutions such as Combat Components or AGR Pro (Combat Manager in v4).
 *
 * Rather to provide a quick and easy way to handle collision for those not having or not willing to use aforementioned plugins, or not having a game-specific custom collision system already in place.
 *
 * Registered meshes can be Static or Skeletal meshes, this component relies on Sockets attached to those primitives to draw traces for each socket.
 *
 * No sub-stepping is involved, we simply draw traces for each frame checking for collisions between last frame position and current frame position for a given socket.
 */
UCLASS(ClassGroup=ComboGraph, Blueprintable, hidecategories=(Object, LOD, Lighting, Transform, Sockets, TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class COMBOGRAPH_API UComboGraphCollisionComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComboGraphSimpleDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComboGraphOnHitRegisteredDelegate, FHitResult, HitResult, UPrimitiveComponent*, Mesh);

public:
	// Sets default values for this component's properties
	UComboGraphCollisionComponent();

	bool bCanTrace = false;
	bool bIsTraceRunning = false;

	/** If set to true, traces will be rendered on screen for a duration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Debug")
	bool bDebug = false;

	/** If set to true, registered hits will be logged to console (successful hits or ignored ones due to configuration)  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Debug", meta=(EditCondition="bDebug", EditConditionHides))
	bool bShouldLogHits = false;

	/** In debug mode, traces will be rendered on screen using this color */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Debug", meta=(EditCondition="bDebug", EditConditionHides))
	FLinearColor TraceColor = FLinearColor::Gray;

	/** In debug mode, registered hits will be rendered on screen using this color */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Debug", meta=(EditCondition="bDebug", EditConditionHides))
	FLinearColor TraceHitColor = FLinearColor::Green;

	/** In debug mode, this is the amount in seconds that visual debug traces will persist on screen */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Debug", meta=(EditCondition="bDebug", EditConditionHides))
	float DebugDrawTime = 2.f;

	/** Radius of the sphere to sweep during collision traces */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Traces")
	float TraceRadius = 3.f;

	/** True to test against complex collision, false to test against simplified collision */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Traces")
	bool bTraceComplex = true;

	/** Trace channel to use when we perform collision traces (Default set to "Camera" for ease of use, but custom collision profiles can be used for further tuning) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Traces")
	TEnumAsByte<ETraceTypeQuery> CollisionTraceChannel = TraceTypeQuery2;

	/** List of Actor classes to ignore during hit collisions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Traces")
	TArray<TSubclassOf<AActor>> ActorTypesToIgnore;

	/** List of collision profile names to ignore during hit collision. Can be useful to ignore profile for capsule component and ensure hits only register against character meshes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combo Graph|Traces")
	TArray<FName> CollisionProfilesToIgnore;

	/** Delegate invoked when a successful hit is registered, only on server */
	UPROPERTY(BlueprintAuthorityOnly, BlueprintAssignable, Category = "Combo Graph")
	FComboGraphOnHitRegisteredDelegate OnHitRegistered;

	/** Delegate invoked when we start to check for collision traces on registered meshes */
	UPROPERTY(BlueprintAssignable, Category = "Combo Graph")
	FComboGraphSimpleDelegate OnTraceStart;

	/** Delegate invoked when we end the collision tracing process on registered meshes */
	UPROPERTY(BlueprintAssignable, Category = "Combo Graph")
	FComboGraphSimpleDelegate OnTraceEnd;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void StartTrace();
	virtual void EndTrace();

	/**
	 * Use this method to push a Static or Skeletal mesh into the registered meshes to consider for collision checks.
	 *
	 * Meant to be called at least once either on Owner's Actor BeginPlay or with this component OnComponentActivated event.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	virtual void RegisterCollisionMesh(UPrimitiveComponent* InMesh);

	/** Use this method to remove a Static or Skeletal mesh from the registered meshes */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	virtual void UnregisterCollisionMesh(UPrimitiveComponent* InMesh);

	virtual void TraceTick();
	virtual void DoTraceCheckForRegisteredMeshes();
	virtual void DoTraceCheckForSocket(UPrimitiveComponent* InMesh, FName InSocketName);
	virtual void HandleSuccessfulHit(FHitResult InHitResult, AActor* HitActor, UPrimitiveComponent* HitComponent, UPrimitiveComponent* InMesh);

	virtual void UpdateSocketLocations();
	virtual FName CreateSocketUID(UPrimitiveComponent* InMesh, FName InSocketName);

	virtual void ResetHitActors();
	virtual void ResetSocketLocations();
	virtual bool IsIgnoredActorType(TSubclassOf<AActor> InActorType) const;

	/**
	 *	Check if:
	 *
	 *	1. Hit Actor was not already hit for this collision
	 *	2. Hit Actor Class is not part of the ignored classes
	 *	3. Hit Component collision profile is not part of ignored profiles
	 */
	virtual bool IsSuccessfulHit(AActor* HitActor, UPrimitiveComponent* HitComponent, FString& FailReason) const;

protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> RegisteredMeshes;

	TMap<FName, FVector> LastFrameSocketLocations;

private:
	bool HasAuthority() const;
};
