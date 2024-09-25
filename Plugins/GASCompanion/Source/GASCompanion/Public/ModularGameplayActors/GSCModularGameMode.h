// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "GSCModularGameMode.generated.h"

/** Pair this with a ModularGameStateBase */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSCModularGameModeBase();
};

/** Pair this with a ModularGameState */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AGSCModularGameMode();
};
