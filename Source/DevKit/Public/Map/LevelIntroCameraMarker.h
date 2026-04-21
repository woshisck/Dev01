#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelIntroCameraMarker.generated.h"

class UCameraComponent;

/**
 * 关卡开场镜头起始标记。
 * 设计师在关卡中放置此 Actor，调用 TriggerIntro() 即可：
 *   1. 立即切到此处视角
 *   2. 停留 HoldDuration 秒
 *   3. 平滑移回玩家（MoveDuration 秒）
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API ALevelIntroCameraMarker : public AActor
{
	GENERATED_BODY()

public:
	ALevelIntroCameraMarker();

	// 停留在标记处的时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelIntro")
	float HoldDuration = 2.0f;

	// 镜头从标记移回玩家所需时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelIntro")
	float MoveDuration = 1.5f;

	// 开场期间是否禁用玩家输入
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelIntro")
	bool bDisableInputDuringIntro = true;

	// 从 LevelFlow 或 BeginPlay 中调用，开始开场镜头序列
	UFUNCTION(BlueprintCallable, Category = "LevelIntro")
	void TriggerIntro();

protected:
	// 设计师通过此组件调整镜头朝向（FOV、旋转等）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelIntro")
	TObjectPtr<UCameraComponent> CameraComp;

private:
	UFUNCTION()
	void OnHoldComplete();

	UFUNCTION()
	void OnMoveComplete();

	FTimerHandle HoldTimerHandle;
	FTimerHandle MoveTimerHandle;
	TWeakObjectPtr<APlayerController> CachedPC;
};
