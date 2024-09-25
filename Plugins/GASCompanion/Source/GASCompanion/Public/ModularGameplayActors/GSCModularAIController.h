// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "GSCModularAIController.generated.h"

/** Minimal class that supports extension by game feature plugins */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularAIController : public AAIController
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface
};
