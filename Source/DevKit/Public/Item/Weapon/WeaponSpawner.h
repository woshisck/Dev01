// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/PlayerInteraction.h"
#include "Map/PickupInteractable.h"
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
class DEVKIT_API AWeaponSpawner : public AActor, public IPlayerInteraction, public IPickupInteractable
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

	// --- IPlayerInteraction ---
	virtual void OnPlayerBeginOverlap(APlayerCharacterBase* Player) override;
	virtual void OnPlayerEndOverlap(APlayerCharacterBase* Player) override;

	// ~ IPickupInteractable
	virtual void OnPlayerEnterRange(APlayerCharacterBase* Player) override;
	virtual void OnPlayerLeaveRange(APlayerCharacterBase* Player) override;
	virtual void TryPickup(APlayerCharacterBase* Player) override;
	// ~ End IPickupInteractable

	UFUNCTION(BlueprintImplementableEvent)
	void GrantWeapon(APlayerCharacterBase* ReceivingChar);

	// 换武器时恢复旧 Spawner：重置 bPickedUp、浮窗状态，并通知 BP 恢复网格材质
	void ResetToAvailable();

	// BP 端恢复展示网格材质（在 ResetToAvailable 末尾调用）
	UFUNCTION(BlueprintImplementableEvent)
	void OnResetToAvailable();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> PlayerInteractVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> BlockVolume;


	UPROPERTY(BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 各轴旋转速度（度/秒）。Pitch=X，Yaw=Z，Roll=Y
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "武器展示|旋转")
	FRotator RotationRate = FRotator(0.f, 40.f, 0.f);

	// 浮动偏移幅度（cm），0 = 不偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "武器展示|浮动", meta = (ClampMin = "0"))
	float BobAmplitude = 0.f;

	// 浮动频率（Hz，完整周期数/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "武器展示|浮动", meta = (ClampMin = "0.01"))
	float BobFrequency = 1.f;

	// 浮动方向（局部空间，默认 Z 轴上下浮动）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "武器展示|浮动")
	FVector BobAxis = FVector(0.f, 0.f, 1.f);

	// 玩家接近时登记 PendingWeaponSpawner，按 E 后调用 TryPickupWeapon
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 玩家按 E 后触发：换武器逻辑 + 材质变黑
	void TryPickupWeapon(APlayerCharacterBase* Player);

	UFUNCTION()
	AWeaponInstance* SpawnWeaponDeferred(UWorld* World, const FTransform& SpawnTransform, const FWeaponSpawnData& SpawnData);

	// 武器信息浮窗 WidgetComponent（Screen Space，自动跟随武器位置）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "浮窗", meta = (AllowPrivateAccess = true))
	TObjectPtr<UWidgetComponent> WeaponInfoWidgetComp;

	// 在 BP_WeaponSpawner 里指定浮窗 WBP 类
	UPROPERTY(EditDefaultsOnly, Category = "浮窗")
	TSubclassOf<UWeaponFloatWidget> WeaponFloatWidgetClass;

	// 浮窗水平偏移（cm，武器屏幕位置左/右侧偏移量）
	UPROPERTY(EditDefaultsOnly, Category = "浮窗", meta = (ClampMin = "0"))
	float WidgetSideOffset = 300.f;

	// 浮窗垂直偏移（cm，沿 Z 轴抬升）
	UPROPERTY(EditDefaultsOnly, Category = "浮窗", meta = (ClampMin = "0"))
	float WidgetZOffset = 50.f;

	// 拾取时浮窗折叠动画时长（秒），增大可预览效果
	UPROPERTY(EditDefaultsOnly, Category = "浮窗", meta = (ClampMin = "0.05"))
	float PickupCollapseDuration = 0.25f;

	// 被拾取后替换的材质（通常为纯黑 MI），所有插槽统一使用
	UPROPERTY(EditDefaultsOnly, Category = "武器展示|材质")
	TObjectPtr<UMaterialInterface> PickedUpMaterial;

private:

	// 朝向检测：玩家在范围内时每帧判断是否应显示浮窗
	bool bPlayerInRange       = false;
	bool bPickedUp            = false;  // 拾取后浮窗永久隐藏
	bool bCollapsingForPickup = false;  // 折叠动画进行中，保持 WidgetComp 可见
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	// BeginPlay 时缓存的网格原始材质，用于还原
	TArray<TObjectPtr<UMaterialInterface>> CachedMeshMaterials;

	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);

	float BobTimer = 0.f;
	FVector BaseMeshOffset = FVector::ZeroVector;
};
