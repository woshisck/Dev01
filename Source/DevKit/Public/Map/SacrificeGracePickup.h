#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/PickupInteractable.h"
#include "SacrificeGracePickup.generated.h"

class UBoxComponent;
class UWidgetComponent;
class USacrificeGraceDA;
class APlayerCharacterBase;

/**
 * ASacrificeGracePickup — 献祭恩赐拾取物
 *
 * 由 GameMode 在整理阶段随机生成，玩家走近后按 E 键获取献祭恩赐效果。
 * 实现 IPickupInteractable：进入范围显示 PickupHintWidgetComp，离开隐藏。
 * DA 由 GameMode 通过 SetSacrificeGraceDA 注入。
 */
UCLASS()
class DEVKIT_API ASacrificeGracePickup : public AActor, public IPickupInteractable
{
	GENERATED_BODY()

public:
	ASacrificeGracePickup();

	/** 由 GameMode 在 Spawn 后立即调用，注入要赋予玩家的献祭 DA */
	UFUNCTION(BlueprintCallable, Category = "SacrificeGrace")
	void SetSacrificeGraceDA(USacrificeGraceDA* DA);

	// ~ IPickupInteractable
	virtual void OnPlayerEnterRange(APlayerCharacterBase* Player) override;
	virtual void OnPlayerLeaveRange(APlayerCharacterBase* Player) override;
	virtual void TryPickup(APlayerCharacterBase* Player) override;
	// ~ End IPickupInteractable

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionVolume;

	/** 玩家进入范围时显示的交互提示 Widget（在 BP 中指定 WBP 类） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup")
	TObjectPtr<UWidgetComponent> PickupHintWidgetComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sacrifice")
	TObjectPtr<USacrificeGraceDA> SacrificeGraceDA;

private:


	bool bPickedUp = false;
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	bool IsPickupAllowed() const;
	void ClearNearbyPlayer();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
