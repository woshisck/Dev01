// Copyright 2024 Eren Balatkan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MnhTracer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "MnhAnimNotifyState.generated.h"

class UMnhTracerComponent;
/**
 * 
 */
UCLASS(Abstract)
class MISSNOHIT_API UMnhAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	FORCEINLINE static UMnhTracerComponent* FindMnhTracerComponent(const USkeletalMeshComponent* MeshComp);
	FORCEINLINE static UMnhTracer* FindTracer(const USkeletalMeshComponent* MeshComp, FGameplayTag TracerTag);
};


UCLASS()
class MISSNOHIT_API UMnhActivateTracer : public UMnhAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	FGameplayTagContainer MnhTracerTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	bool bResetHitCacheOnActivation = true;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

UCLASS()
class MISSNOHIT_API UMnhAnimNotifyTracer : public UMnhAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	bool bResetHitCacheOnActivation = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	FGameplayTag TracerTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	FName AttachedSocketOrBoneName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), Category="MissNoHit")
	FMnhShapeData ShapeData;

	/* WARNING: Not compatible with dynamic animation logic such as blending, retargeting, procedural animation, etc.
	 * Make sure animation skeleton is not modified or retargeted after precomputing the path.
	 * Generally not recommended to use this feature unless you are sure about the animation pipeline.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	bool bUsePrecomputedPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditConditionHides, EditCondition="bUsePrecomputedPath"))
	int PrecomputeFps = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditConditionHides, EditCondition="bUsePrecomputedPath"))
	bool bDrawPrecomputedPath = false;
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
private:
	int TickIdx = 0;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
	float CurrentTimeInNotify = 0;
#endif
};

