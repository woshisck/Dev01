#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StylizedCharacterShadowPolicyComponent.generated.h"

class UPrimitiveComponent;

/**
 * Applies the stylized character shadow split to an actor's skinned meshes.
 * Native projected shadows remain visible on the environment while the character's own
 * visible pixels are reserved for the stylized half-view self-shadow pass.
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent, DisplayName = "Stylized Character Shadow Policy"))
class CELESLIGHTRUNTIME_API UStylizedCharacterShadowPolicyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStylizedCharacterShadowPolicyComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Applies the current policy immediately. Safe to call again after adding modular character parts. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stylized Character|Shadow")
	void ApplyShadowPolicy();

	/** Keep native scene reception and environment casting, but remove the character's native shadow from its own pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character|Shadow")
	bool bEnvironmentOnlyNativeShadow = true;

	/** When no explicit targets are assigned, apply to every skinned mesh component owned by this actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character|Shadow")
	bool bAutoFindSkinnedMeshComponents = true;

	/** Optional explicit targets. When non-empty, only these components are changed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character|Shadow")
	TArray<TObjectPtr<UPrimitiveComponent>> ExplicitTargetComponents;

	/** Contact shadows are screen-space self-shadowing and cannot use the caster/receiver split. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character|Shadow")
	bool bDisableContactShadows = true;

	/** Capsule direct shadows are another self-shadow path and should normally stay disabled for this model. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character|Shadow")
	bool bDisableCapsuleDirectShadows = true;
};
