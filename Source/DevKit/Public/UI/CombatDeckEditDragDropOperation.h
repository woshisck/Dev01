#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "CombatDeckEditDragDropOperation.generated.h"

UCLASS()
class DEVKIT_API UCombatDeckEditDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Combat Deck|Edit")
	int32 SourceIndex = INDEX_NONE;
};
