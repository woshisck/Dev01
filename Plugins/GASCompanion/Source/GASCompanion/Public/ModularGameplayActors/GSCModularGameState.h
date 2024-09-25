// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "GameFramework/GameState.h"
#include "GSCModularGameState.generated.h"

/** Pair this with a ModularGameModeBase */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface
};

/** Pair this with a ModularGameState */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularGameState : public AGameState
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

protected:
	//~ Begin AGameState interface
	virtual void HandleMatchHasStarted() override;
	//~ Begin AGameState interface
};
