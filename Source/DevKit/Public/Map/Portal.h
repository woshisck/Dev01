#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class APlayerCharacterBase;
class UBillboardComponent;
class UBoxComponent;
class UYogSaveSubsystem;
class URoomDataAsset;

UCLASS()
class DEVKIT_API APortal : public AActor
{
	GENERATED_BODY()

public:

	APortal(const FObjectInitializer& ObjectInitializer);

	// 视觉效果由 BP 实现（雾效开关）
	UFUNCTION(BlueprintImplementableEvent)
	void EnablePortal();

	UFUNCTION(BlueprintImplementableEvent)
	void DisablePortal();

	// 关卡开始时由 GameMode 确定该门永不开启：隐藏门效果，显示静态装饰，无碰撞
	UFUNCTION(BlueprintImplementableEvent)
	void NeverOpen();

	// GameMode 在关卡结束时调用，分配目标关卡和房间配置并开启门
	UFUNCTION(BlueprintCallable)
	void Open(FName InSelectedLevel, URoomDataAsset* InSelectedRoom);

	// 直接通过名字切换关卡（保留旧接口，BP 可调）
	UFUNCTION(BlueprintCallable)
	void YogOpenLevel(FName LevelName);

protected:
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintNativeEvent)
	void EnterPortal(APlayerCharacterBase* ReceivingChar, UYogSaveSubsystem* SaveSubsystem);

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	// 场景中唯一标识（与 CampaignData.PortalDestinations[i].PortalIndex 对应）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	int32 Index = 0;

	// GameMode 写入的目标关卡名（关卡结束时由随机池选定）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FName SelectedLevel;

	// GameMode 写入的下一关房间配置（骰子决定类型后从类型池中选定）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<URoomDataAsset> SelectedRoom;

	// 是否已开启（BeginPlay 时为 false，GameMode 调 Open() 后变 true）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bIsOpen = false;

	// 关卡开始时确定永不开启（未登记在 PortalDestinations 中）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bWillNeverOpen = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> CollisionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UBillboardComponent> BillBoard;
};
