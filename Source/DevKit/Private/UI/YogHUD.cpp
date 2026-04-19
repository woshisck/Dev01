#include "UI/YogHUD.h"
#include "UI/EnemyArrowWidget.h"
#include "UI/GameDialogWidget.h"
#include "UI/DialogContentDA.h"
#include "Tutorial/TutorialManager.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"

void AYogHUD::BeginPlay()
{
	Super::BeginPlay();

	if (TutorialPopupClass)
	{
		TutorialPopupWidget = CreateWidget<UTutorialPopupWidget>(GetOwningPlayerController(), TutorialPopupClass);
		// 只创建，不 AddToViewport —— ShowPopup 时才加入，关闭后自动移除
	}

	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
	{
		TM->Init(TutorialPopupWidget, DialogContentDA);
	}

	if (EnemyArrowWidgetClass)
	{
		EnemyArrowWidget = CreateWidget<UEnemyArrowWidget>(GetOwningPlayerController(), EnemyArrowWidgetClass);
		if (EnemyArrowWidget)
			EnemyArrowWidget->AddToViewport(0);
	}

	UYogSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UYogSaveSubsystem>();
	if (SaveSys)
	{
		SaveSys->OnSaveGameLoaded.AddDynamic(this, &AYogHUD::OnSaveGameLoaded);

		if (UYogSaveGame* Current = SaveSys->GetCurrentSave())
		{
			OnSaveGameLoaded(Current);
		}
	}
}

void AYogHUD::OnSaveGameLoaded(UYogSaveGame* SaveGame)
{
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
	{
		TM->LoadFromSave(SaveGame);
	}
}
