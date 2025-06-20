// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "YogTimelineComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UYogTimelineComponent : public UTimelineComponent
{
	GENERATED_BODY()
	
public:
	UYogTimelineComponent();

protected:
    virtual void BeginPlay() override;

private:
    // Timeline component
    UPROPERTY()
    UTimelineComponent* MyTimeline;

    // Curve float to drive the timeline
    UPROPERTY(EditAnywhere, Category = "Timeline")
    UCurveFloat* FloatCurve;

    // Function to handle timeline updates
    UFUNCTION()
    void TimelineUpdate(float Value);

    // Function called when timeline completes
    UFUNCTION()
    void TimelineFinished();

};
