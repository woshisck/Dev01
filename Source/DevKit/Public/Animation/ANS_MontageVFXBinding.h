#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_MontageVFXBinding.generated.h"

UCLASS(meta = (DisplayName = "ANS Montage VFX Binding"))
class DEVKIT_API UANS_MontageVFXBinding : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX Binding")
	FName SlotName;
};
