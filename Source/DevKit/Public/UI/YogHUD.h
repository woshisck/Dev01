#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EnemyArrowWidget.h"
#include "YogHUD.generated.h"

class UTutorialPopupWidget;
class UDialogContentDA;
class UYogSaveGame;

UCLASS()
class DEVKIT_API AYogHUD : public AHUD
{
	GENERATED_BODY()

public:
	// WBP_TutorialPopup（父类 UTutorialPopupWidget）
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TSubclassOf<UTutorialPopupWidget> TutorialPopupClass;

	UPROPERTY(EditDefaultsOnly, Category = "EnemyArrow")
	TSubclassOf<UEnemyArrowWidget> EnemyArrowWidgetClass;

	// DA_DialogContent：在编辑器里填写所有弹窗事件页面内容
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TObjectPtr<UDialogContentDA> DialogContentDA;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UTutorialPopupWidget> TutorialPopupWidget;

	UPROPERTY()
	TObjectPtr<UEnemyArrowWidget> EnemyArrowWidget;

	UFUNCTION()
	void OnSaveGameLoaded(UYogSaveGame* SaveGame);
};
