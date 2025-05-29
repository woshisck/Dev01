// Copyright 2024 Eren Balatkan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "MnhComponents.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MISSNOHIT_API UMnhSphereComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMnhSphereComponent();
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MISSNOHIT_API UMnhBoxComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMnhBoxComponent();
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MISSNOHIT_API UMnhCapsuleComponent : public UCapsuleComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMnhCapsuleComponent();
};
