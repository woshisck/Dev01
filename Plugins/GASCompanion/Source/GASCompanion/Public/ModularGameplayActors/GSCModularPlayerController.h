// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GSCModularPlayerController.generated.h"

/** Minimal class that supports extension by game feature plugins */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

	//~ Begin APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End APlayerController interface
};
