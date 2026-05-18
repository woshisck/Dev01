#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MetaProgression/MetaTypes.h"
#include "UMetaNodeEditProxy.generated.h"

class UDataTable;

UCLASS(Transient)
class DEVKITEDITOR_API UMetaNodeEditProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Node")
	FMetaUpgradeNodeRow Data;

	FName SourceRowName;

	UPROPERTY()
	TObjectPtr<UDataTable> SourceTable;

	void LoadFromRow(UDataTable* Table, FName RowName);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
