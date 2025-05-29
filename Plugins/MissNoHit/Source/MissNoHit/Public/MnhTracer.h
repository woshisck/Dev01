// Copyright 2024 Eren Balatkan, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MnhHelpers.h"
#include "MnhTracerComponent.h"
#include "UObject/Object.h"
#include "Kismet/KismetSystemLibrary.h"
#include "WorldCollision.h"
#include "MnhTracer.generated.h"

struct FMnhMultiTraceResultContainer;
struct FMnhTracerData;
class UMnhTraceType;
struct FGameplayTag;
struct FMnhTraceSettings;
class UMnhHitFilter;

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, CollapseCategories, ShowCategories=(TracerBase, TracerType), meta=(FullyExpand=true))
class MISSNOHIT_API UMnhTracer : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UMnhTracer() override;

	/* Identifier for the Tracer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	FGameplayTag TracerTag;

	/* An enumeration that specifies the source of the tracer. It can be a shape component, a physics asset, or an animation notification.  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	EMnhTraceSource TraceSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="(TraceSource==EMnhTraceSource::StaticMeshSockets)", EditConditionHides))
	FName MeshSocket_1 = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="(TraceSource==EMnhTraceSource::StaticMeshSockets)", EditConditionHides))
	FName MeshSocket_2 = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="(TraceSource==EMnhTraceSource::StaticMeshSockets)", EditConditionHides))
	float MeshSocketTracerRadius = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="(TraceSource==EMnhTraceSource::StaticMeshSockets)", EditConditionHides))
	float MeshSocketTracerLengthOffset = 0;

	/* Name of the Socket or Bone Tracer will be attached to */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="(TraceSource==EMnhTraceSource::SocketOrBone)||(TraceSource==EMnhTraceSource::PhysicsAsset)", EditConditionHides))
	FName SocketOrBoneName;

	/* Shape specifications of the Tracer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="false", EditConditionHides))
	FMnhShapeData ShapeData;

	/* Collision settings for the Tracer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	FMnhTraceSettings TraceSettings;

	/* Specifies how the tracer should tick. Sub-stepping is disabled on Match Game Tick */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	EMnhTracerTickType TracerTickType;

	/* Tick Rate of the Tracer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TracerTickType==EMnhTracerTickType::FixedRateTick", EditConditionHides))
	int TargetFps = 30;

	/* Distance traveled by Tracer between each Tick */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TracerTickType==EMnhTracerTickType::DistanceTick", EditConditionHides))
	int TickDistanceTraveled = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit|Debug")
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit|Debug",
		meta=(EditCondition="DrawDebugType!=EDrawDebugTrace::None", EditConditionHides))
	FColor DebugTraceColor = FColor::Red;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit|Debug",
		meta=(EditCondition="DrawDebugType!=EDrawDebugTrace::None", EditConditionHides))
	FColor DebugTraceHitColor = FColor::Green;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit|Debug",
			meta=(EditCondition="DrawDebugType!=EDrawDebugTrace::None", EditConditionHides))
	FColor DebugTraceBlockColor = FColor::Blue; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit|Debug",
		meta=(EditCondition="DrawDebugType==EDrawDebugTrace::ForDuration", EditConditionHides))
	float DebugDrawTime = 0.5;

	UPROPERTY(BlueprintReadOnly, Category="MissNoHit")
	TObjectPtr<UPrimitiveComponent> SourceComponent;

	bool bIsTracerActive = false;
	
	TObjectPtr<UMnhTracerComponent> OwnerComponent;
	
	FCollisionQueryParams CollisionParams;
	FCollisionResponseParams ResponseParams;
	FCollisionObjectQueryParams ObjectQueryParams;

	void InitializeParameters(UPrimitiveComponent* SourceComponent);
	void InitializeFromPhysicAsset();
	void InitializeFromCollisionShape();
	void InitializeFromAnimNotify();
	void InitializeFromStaticMeshSockets();
	bool IsTracerActive() const;
	
	void ChangeTracerState(bool bIsTracerActiveArg, bool bStopImmediate=true);

	int TracerDataIdx;
	FGuid TracerDataGuid;

	void RegisterTracerData();
	void UpdateTracerData();
	void MarkTracerDataForRemoval() const;

private:
	FMnhTracerData& GetTracerData() const;
	
};

USTRUCT(BlueprintType)
struct FMnhTracerData
{
	GENERATED_BODY()

public:
	EMnhTraceSource TraceSource;
	FMnhShapeData ShapeData;
	FName SocketOrBoneName;
	FMnhTraceSettings TraceSettings;
	TArray<FTransform, TFixedAllocator<2>> TracerTransformsOverTime;
	TObjectPtr<UPrimitiveComponent> SourceComponent;
	EMnhTracerState TracerState = EMnhTracerState::Stopped;
	float DeltaTimeLastTick = 0;
	
	EMnhTracerTickType TracerTickType;
	float TargetTickInterval = 1/30;
	bool bShouldTickThisFrame = false;
	
	FCollisionQueryParams CollisionParams;
	FCollisionResponseParams ResponseParams;
	FCollisionObjectQueryParams ObjectQueryParams;
	
	UWorld* World;

	TObjectPtr<UMnhTracer> OwnerTracer;
	TObjectPtr<UMnhTracerComponent> OwnerTracerComponent;
	
	FGuid Guid;

	bool IsPendingRemoval = false;

	TArray<FMnhMultiTraceResultContainer> SubstepHits;
	
	void ChangeTracerState(bool bIsTracerActiveArg, bool bStopImmediate=true);
	
	void DoTrace(const uint32 Substeps, const uint32 TickIdx);
	FORCEINLINE FTransform GetCurrentTracerTransform() const;
	FORCEINLINE void UpdatePreviousTransform(const FTransform& Transform);
};
