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
class UGameplayEffect;
class UAbilityData;



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
	virtual void Tick(float DeltaTime) override;
	virtual void PostActorCreated() override;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName AttachSocket;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FTransform AttachTransform;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	UPROPERTY(SaveGame)
	TObjectPtr<UAbilityData> WeaponAbilities;

	UFUNCTION(BlueprintCallable)
	void InitializeWeapon();

	UFUNCTION(BlueprintCallable)
	void EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar);

	// 热度阶段材质接口（订阅 PlayerCharacterBase::OnHeatPhaseChanged）
	// Phase: 0=无, 1=白光, 2=绿光, 3=橙黄, 4=过热红光
	UFUNCTION()
	void OnHeatPhaseChanged(int32 Phase);

	// 热度 Overlay 材质（由 WeaponSpawner 从 WeaponDefinition 自动赋入，无需 BP 手动填）
	UPROPERTY(BlueprintReadOnly, Category = "Heat")
	TObjectPtr<UMaterialInterface> HeatOverlayMaterial;

	/** 扫射时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0.1"))
	float GlowSweepDuration = 0.5f;

	/** 边缘光保持时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0.0"))
	float GlowHoldDuration = 3.0f;

	/** 边缘光淡出时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0.1"))
	float GlowFadeDuration = 0.5f;

private:
	// 运行时动态材质实例（首次调用时创建）
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HeatOverlayDynMat;

	// -1 = 未激活；>= 0 = 动画运行中
	float GlowElapsed = -1.f;
};
