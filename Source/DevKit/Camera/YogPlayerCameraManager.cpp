// Fill out your copyright notice in the Description page of Project Settings.


#include "YogPlayerCameraManager.h"
#include "Camera/CameraComponent.h"
AYogPlayerCameraManager::AYogPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}


void AYogPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	// FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	// DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	// DisplayDebugManager.SetDrawColor(FColor::Yellow);
	// DisplayDebugManager.DrawString(FString::Printf(TEXT("AYogPlayerCameraManager: %s"), *GetNameSafe(this)));

	// Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	// const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

    // if (Pawn != nullptr){
    //    const UCameraComponent* CameraComponent = Pawn->FindComponentByClass<UCameraComponent>();
    //    CameraComponent->DrawDebug(Canvas);
    // }
}
