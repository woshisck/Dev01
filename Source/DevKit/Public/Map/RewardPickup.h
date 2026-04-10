#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RewardPickup.generated.h"

class UBoxComponent;

/**
 * ARewardPickup — 关卡结算奖励拾取物
 * 在最后一个敌人被击杀处生成，玩家走近触发战利品选择界面
 */
UCLASS()
class DEVKIT_API ARewardPickup : public AActor
{
	GENERATED_BODY()

public:
	ARewardPickup();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionVolume;

private:
	bool bPickedUp = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);
};
