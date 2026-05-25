#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_RecordProgress.generated.h"

/**
 * 写入剧情进度 Flag（Save 作用域）。
 * Tag 格式：Story.Encounter.Progress.{EncounterId}.{ProgressKey}
 */
UCLASS(meta = (DisplayName = "Record Progress"))
class DEVKIT_API USNode_RecordProgress : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Progress")
	FName EncounterId;

	UPROPERTY(EditAnywhere, Category = "Progress")
	FName ProgressKey;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
