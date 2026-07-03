#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectKey.h"
#include "AnimNotifyState_AddGameplayTag.generated.h"

class UAbilitySystemComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/** Adds loose gameplay tags and optional Niagara feedback for this notify state's duration. */
UCLASS(meta = (DisplayName = "Gameplay Tag Window"))
class DEVKIT_API UAnimNotifyState_AddGameplayTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Tags added on NotifyBegin and removed on NotifyEnd. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag")
	FGameplayTagContainer Tags;

	/** Optional VFX shown at the owner actor's projected screen position while this window is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Niagara")
	TObjectPtr<UNiagaraSystem> ScreenNiagaraSystem;

	/** World offset applied before projecting the owner to screen. Use this to target the torso/head instead of feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Niagara",
		meta = (EditCondition = "ScreenNiagaraSystem != nullptr"))
	FVector ScreenProjectionWorldOffset = FVector(0.f, 0.f, 120.f);

	/** Pixel offset after projection. Positive X moves right; positive Y moves down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Niagara",
		meta = (EditCondition = "ScreenNiagaraSystem != nullptr"))
	FVector2D ScreenOffset = FVector2D::ZeroVector;

	/** Distance from the active camera where the Niagara component is placed after deprojecting the screen point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Niagara",
		meta = (EditCondition = "ScreenNiagaraSystem != nullptr", ClampMin = "1.0"))
	float ScreenNiagaraDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Niagara",
		meta = (EditCondition = "ScreenNiagaraSystem != nullptr"))
	FVector ScreenNiagaraScale = FVector::OneVector;

	/** Keep the Niagara component locked to the owner actor's projected screen point while the notify is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Niagara",
		meta = (EditCondition = "ScreenNiagaraSystem != nullptr"))
	bool bFollowOwnerScreenPosition = true;

	/** Optional VFX attached to the animated mesh while this window is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attached Niagara")
	TObjectPtr<UNiagaraSystem> AttachedNiagaraSystem;

	/** Mesh socket/bone to attach to. Leave empty to attach to the mesh root. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attached Niagara",
		meta = (EditCondition = "AttachedNiagaraSystem != nullptr"))
	FName AttachedNiagaraSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attached Niagara",
		meta = (EditCondition = "AttachedNiagaraSystem != nullptr"))
	FVector AttachedNiagaraLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attached Niagara",
		meta = (EditCondition = "AttachedNiagaraSystem != nullptr"))
	FRotator AttachedNiagaraRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attached Niagara",
		meta = (EditCondition = "AttachedNiagaraSystem != nullptr"))
	FVector AttachedNiagaraScale = FVector::OneVector;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	static UAbilitySystemComponent* GetASC(const USkeletalMeshComponent* MeshComp);
	bool ResolveScreenNiagaraTransform(const USkeletalMeshComponent* MeshComp, FVector& OutLocation, FRotator& OutRotation) const;
	void SpawnScreenNiagara(USkeletalMeshComponent* MeshComp);
	void UpdateScreenNiagara(USkeletalMeshComponent* MeshComp);
	void DestroyScreenNiagara(USkeletalMeshComponent* MeshComp);
	void SpawnAttachedNiagara(USkeletalMeshComponent* MeshComp);
	void DestroyAttachedNiagara(USkeletalMeshComponent* MeshComp);

	TMap<TObjectKey<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> ActiveScreenNiagaraComponents;
	TMap<TObjectKey<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> ActiveAttachedNiagaraComponents;
};
