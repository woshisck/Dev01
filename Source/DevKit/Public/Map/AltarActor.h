#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/PlayerInteraction.h"
#include "GameModes/LevelFlowTypes.h"
#include "Data/AltarDataAsset.h"
#include "AltarActor.generated.h"

class APlayerCharacterBase;
class UAltarMenuWidget;
class UBoxComponent;

UCLASS()
class DEVKIT_API AAltarActor : public AActor, public IPlayerInteraction
{
	GENERATED_BODY()

public:
	AAltarActor();
	virtual void BeginPlay() override;

	// IPlayerInteraction
	virtual void OnPlayerBeginOverlap(APlayerCharacterBase* Player) override;
	virtual void OnPlayerEndOverlap(APlayerCharacterBase* Player) override;

	// 由玩家输入或交互 UI 调用
	UFUNCTION(BlueprintCallable, Category = "Altar")
	void TryInteract(APlayerCharacterBase* Player);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar")
	TObjectPtr<UAltarDataAsset> AltarData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	TSubclassOf<UAltarMenuWidget> AltarMenuWidgetClass;

protected:
	// 只在整理阶段（ELevelPhase::Arrangement）为 true
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Altar")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> InteractBox;

	UPROPERTY()
	TObjectPtr<UAltarMenuWidget> AltarMenuWidget;

	UFUNCTION()
	void OnPhaseChanged(ELevelPhase NewPhase);

	// BP 重写：显示 / 隐藏交互提示（"按 F 交互"等）
	UFUNCTION(BlueprintImplementableEvent, Category = "Altar")
	void OnPlayerNearby(APlayerCharacterBase* Player, bool bNearby);
};
