// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "WeaponInstance.generated.h"


/**
 * 
 */
struct FWeaponSpawnData;

class UYogAnimInstance;
class USceneComponent;
class UYogGameplayAbility;
class UStaticMeshComponent;
class UGameplayEffect;
class UNiagaraComponent;



USTRUCT(BlueprintType)
struct FWeaponSocketLoc
{
public:
	GENERATED_USTRUCT_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* DmgBox_Start = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* DmgBox_End = nullptr;
};

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AWeaponInstance : public AActor
{
	GENERATED_BODY()
public:
	AWeaponInstance();

	virtual void BeginPlay() override;
	virtual void PostActorCreated() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<USceneComponent> VFXAttachPoint;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName AttachSocket;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FTransform AttachTransform;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	UFUNCTION(BlueprintCallable)
	void InitializeWeapon();

	UFUNCTION(BlueprintCallable)
	void EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar);

	// Plays the Just Combo burst on VFXAttachPoint. The component is built on the first proc and
	// re-activated on every later one: Just Combo can fire several times per second, so respawning
	// a system per proc would churn the GC for no visual gain.
	UFUNCTION(BlueprintCallable, Category = "VFX", meta = (DisplayName = "播放精准连击特效"))
	void PlayJustComboVFX(UNiagaraSystem* System);

private:
	// Persistent Just Combo burst. Lives as long as the weapon actor, never auto-destroyed.
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> JustComboVFXComponent;

};
