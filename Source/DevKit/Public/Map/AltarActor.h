#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/PlayerInteraction.h"
#include "GameModes/LevelFlowTypes.h"
#include "Data/AltarDataAsset.h"
#include "AltarActor.generated.h"

class APlayerCharacterBase;
class UAltarMenuWidget;
class USacrificeSelectionWidget;
class UBoxComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UNiagaraComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Sacrifice")
	TSubclassOf<USacrificeSelectionWidget> SacrificeWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Sacrifice")
	bool bOpenSacrificeDirectly = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Altar|Sacrifice")
	bool bSacrificeRewardConsumed = false;

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void SetAltarData(UAltarDataAsset* InData);

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void SetSacrificeWidgetClass(TSubclassOf<USacrificeSelectionWidget> InClass) { SacrificeWidgetClass = InClass; }

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void SetOpenSacrificeDirectly(bool bInOpenDirectly);

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void SetAltarActive(bool bInActive);

	UFUNCTION(BlueprintCallable, Category = "Altar|Sacrifice")
	void ConsumeSacrificeReward();

protected:
	// 只在整理阶段（ELevelPhase::Arrangement）为 true
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Altar")
	bool bIsActive = false;

	// 交互触发碰撞盒，可在细节面板调整 Box Extent 改变交互范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|Interaction")
	TObjectPtr<UBoxComponent> InteractBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Altar")
	TObjectPtr<UStaticMeshComponent> AltarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Altar|Prompt")
	TObjectPtr<UWidgetComponent> InteractPromptWidgetComp;

	// 未献祭时持续播放的待机特效，献祭完成后自动隐藏；可在细节面板指定 Niagara System 资产并调整位置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar|VFX")
	TObjectPtr<UNiagaraComponent> IdleVFXComponent;

	UPROPERTY()
	TObjectPtr<UAltarMenuWidget> AltarMenuWidget;

	UPROPERTY()
	TObjectPtr<USacrificeSelectionWidget> SacrificeWidget;

	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	UFUNCTION()
	void OnPhaseChanged(ELevelPhase NewPhase);

	// BP 重写：显示 / 隐藏交互提示（"按 F 交互"等）
	void ConfigureInteractPrompt();
	void SetInteractPromptVisible(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "Altar")
	void OnPlayerNearby(APlayerCharacterBase* Player, bool bNearby);
};
