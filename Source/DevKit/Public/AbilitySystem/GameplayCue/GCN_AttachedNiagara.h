#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_AttachedNiagara.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMeshComponent;

/**
 * GameplayCue notify actor that keeps a Niagara system attached to the target actor's mesh
 * for the lifetime of an active cue. Intended for persistent status VFX such as burn or bleed.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AGCN_AttachedNiagara : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_AttachedNiagara();

	virtual bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	bool bAttachToSkeletalMesh = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FName AttachSocketName = TEXT("spine_03");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	TArray<FName> AttachSocketFallbackNames = {
		TEXT("spine_02"),
		TEXT("pelvis"),
		TEXT("root")
	};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FVector Scale = FVector(1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	bool bSpawnOnExecute = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	bool bSkipDedicatedServer = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent;

	UNiagaraComponent* SpawnNiagara(AActor* Target, bool bAutoDestroy);
	void StopNiagara();
	USceneComponent* ResolveAttachComponent(AActor* Target, FName& OutSocketName) const;
	bool HasSocketOrBone(const USkeletalMeshComponent* MeshComponent, const FName SocketOrBoneName) const;
};
