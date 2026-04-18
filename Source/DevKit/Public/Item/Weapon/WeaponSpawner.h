// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Item/Weapon/WeaponDefinition.h"

#include "WeaponSpawner.generated.h"


class APawn;
class UCapsuleComponent;
class AYogCharacterBase;
class APlayerCharacterBase;

class UObject;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UWeaponFloatWidget;

class UWeaponDefinition;


struct FGameplayTag;
struct FHitResult;




UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AWeaponSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponSpawner(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Definition")
	TObjectPtr<UWeaponDefinition> WeaponDefinition;

	//Delay between when the weapon is made available and when we check for a pawn standing in the spawner. Used to give the bIsWeaponAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Definition")
	float CheckExistingOverlapDelay;

	UFUNCTION(BlueprintImplementableEvent)
	void GrantWeapon(APlayerCharacterBase* ReceivingChar);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;


	UPROPERTY(BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ItemPickup")
	float WeaponMeshRotationSpeed;

	// 玩家接近时登记 PendingWeaponSpawner，按 E 后调用 TryPickupWeapon
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 玩家按 E 后触发：换武器逻辑 + 材质变黑
	void TryPickupWeapon(APlayerCharacterBase* Player);

	// 换武器时恢复本 Spawner 的展示网格材质
	void RestoreSpawnerMesh();

	UFUNCTION()
	AWeaponInstance* SpawnWeaponDeferred(UWorld* World, const FTransform& SpawnTransform, const FWeaponSpawnData& SpawnData);

	// 武器被拾取后展示网格切换到的黑色材质（在 BP 里赋值）
	UPROPERTY(EditDefaultsOnly, Category = "ItemPickup")
	TObjectPtr<UMaterialInterface> BlackedOutMaterial;

	// 武器信息浮窗 WidgetComponent（Screen Space，自动跟随武器位置）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "浮窗", meta = (AllowPrivateAccess = true))
	TObjectPtr<UWidgetComponent> WeaponInfoWidgetComp;

	// 在 BP_WeaponSpawner 里指定浮窗 WBP 类
	UPROPERTY(EditDefaultsOnly, Category = "浮窗")
	TSubclassOf<UWeaponFloatWidget> WeaponFloatWidgetClass;

	// 浮窗相对武器的侧向偏移（从玩家视角向右，单位 cm）
	UPROPERTY(EditDefaultsOnly, Category = "浮窗")
	float WidgetSideOffset = 180.f;

	// 浮窗相对武器的垂直偏移（单位 cm）
	UPROPERTY(EditDefaultsOnly, Category = "浮窗")
	float WidgetZOffset = 40.f;

private:

	// BeginPlay 时保存的原始材质，用于换武器时恢复
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMeshMaterials;

	// 朝向检测：玩家在范围内时每帧判断是否应显示浮窗
	bool bPlayerInRange = false;
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);
};
