// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphNodeAnimBase.h"
#include "ComboGraphNodeSequence.generated.h"

class UAnimMontage;
class UAnimSequence;
class UAnimSequenceBase;

/**
 *  Base Class for Combo Graph nodes acting based on an Anim Sequence asset.
 *
 *  Holds runtime properties for animation and effects / cues containers.
 */
UCLASS(Blueprintable)
class COMBOGRAPH_API UComboGraphNodeSequence : public UComboGraphNodeAnimBase
{
	GENERATED_BODY()

public:
	UComboGraphNodeSequence();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimSequence> Sequence;

	virtual UAnimMontage* GetMontage() const override;
	virtual UAnimSequenceBase* GetAnimationAsset() const override;
	virtual float GetAnimationLength() const override;
	virtual float GetAnimationLengthMinusStartSection() const override;

	virtual void SetAnimationAsset(UAnimationAsset* Asset) override;
	virtual bool SupportsAssetClass(UClass* AssetClass) override;
	virtual FText GetNodeTitle() const override;

#if WITH_EDITOR
	virtual FText GetAnimAssetLabel() const override;
	virtual FText GetAnimAssetLabelTooltip() const override;
	virtual FText GetAnimAssetText() const override;
	virtual FLinearColor GetBackgroundColor() const override;
#endif
};
