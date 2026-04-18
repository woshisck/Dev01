#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "TutorialManager.generated.h"

class UTutorialPopupWidget;
class UDialogContentDA;
class AYogPlayerControllerBase;
class UYogSaveGame;

UCLASS()
class DEVKIT_API UTutorialManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Init(UTutorialPopupWidget* InWidget, UDialogContentDA* InContentDA);

	void TryWeaponTutorial(AYogPlayerControllerBase* PC);
	void TryPostCombatTutorial(AYogPlayerControllerBase* PC);
	void LoadFromSave(UYogSaveGame* Save);

	ETutorialState GetState() const { return State; }

private:
	ETutorialState State = ETutorialState::NeedWeaponTutorial;

	TWeakObjectPtr<UTutorialPopupWidget> PopupWidget;

	UPROPERTY()
	TObjectPtr<UDialogContentDA> ContentDA;

	FTimerHandle DelayHandle;

	void DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);
	void DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);

	void SaveState();
};
