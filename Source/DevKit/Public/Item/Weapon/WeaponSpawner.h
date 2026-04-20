// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/PlayerInteraction.h"
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
class DEVKIT_API AWeaponSpawner : public AActor, public IPlayerInteraction
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

	UFUNCTION(BlueprintImplementableEvent)
	void GrantWeapon(APlayerCharacterBase* ReceivingChar);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> PlayerInteractVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> BlockVolume;


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

private:

	// 朝向检测：玩家在范围内时每帧判断是否应显示浮窗
	bool bPlayerInRange = false;
	bool bPickedUp     = false;   // 拾取后浮窗永久隐藏
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);
};
