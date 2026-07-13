// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionStylizedCharacterLightingOutput.generated.h"

/**
 * Material output expression for MSM_StylizedCharacterLit controls.
 * Connect parameter expressions to these inputs to expose them in material instances.
 */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object, DisplayName = "Stylized Character Lighting Output")
class UMaterialExpressionStylizedCharacterLightingOutput : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** Painted self-shadow bias in [0, 1]. 0.5 is neutral and is remapped to [-1, 1] by the shading model. */
	UPROPERTY()
	FExpressionInput DiffuseBias;

	/** Integer character-lighting profile index, in the [0, 7] range. */
	UPROPERTY()
	FExpressionInput LightingProfile;

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual void Build(MIR::FEmitter& Emitter) override;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual int32 GetNumOutputs() const override;
	virtual FString GetFunctionName() const override;
	virtual FString GetDisplayName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};
