#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HubFacilityActor.generated.h"

class UBoxComponent;
class UCommonActivatableWidget;
class APlayerCharacterBase;

UCLASS(Blueprintable)
class DEVKIT_API AHubFacilityActor : public AActor
{
	GENERATED_BODY()

public:

	AHubFacilityActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 玩家进入交互范围时由 PlayerCharacterBase 调用
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void Interact(APlayerCharacterBase* Player);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub")
	FText FacilityDisplayName;

	// 交互时推入的 Widget 类（由 BP 子类设置）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hub")
	TSubclassOf<UCommonActivatableWidget> WidgetClass;

	// 交互触发盒（玩家 Overlap 检测范围）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hub")
	TObjectPtr<UBoxComponent> InteractBox;

protected:

	virtual void BeginPlay() override;

	// BP 可覆写：Interact 被调用前的自定义逻辑（动画、音效等）
	UFUNCTION(BlueprintImplementableEvent, Category = "Hub")
	void BP_OnInteract(APlayerCharacterBase* Player);

private:

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                        bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
