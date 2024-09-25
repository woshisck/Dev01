// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphNodeAnimBase.h"
#include "ComboGraphNodeMontage.generated.h"

/**
 *  Base Class for Combo Graph nodes acting based on an Anim Montage asset.
 *
 *  Holds runtime properties for animation and effects / cues containers.
 */
UCLASS(Blueprintable)
class COMBOGRAPH_API UComboGraphNodeMontage : public UComboGraphNodeAnimBase
{
	GENERATED_BODY()

public:
	UComboGraphNodeMontage();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FName StartSection = NAME_None;

	/** Blueprint overridable method to return the Start Section name to use with the Combo Play Montage task. Default behavior is simply to return `StartSection` property of the combo node.  */
	UFUNCTION(BlueprintNativeEvent, Category = "Combo Graph")
	FName GetMontageStartSection() const;

	virtual UAnimMontage* GetMontage() const override;
	virtual UAnimSequenceBase* GetAnimationAsset() const override;
	virtual float GetAnimationLength() const override;
	virtual FName GetAnimationStartSection() const override;
	virtual float GetSectionLength(FName SectionName) const override;
	virtual float GetAnimationLengthMinusSection(FName SectionName) const override;
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
