#include "Map/LevelIntroCameraMarker.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ALevelIntroCameraMarker::ALevelIntroCameraMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	RootComponent = CameraComp;
}

void ALevelIntroCameraMarker::TriggerIntro()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	CachedPC = PC;

	if (bDisableInputDuringIntro)
		PC->DisableInput(PC);

	// 立即切到此处视角（无过渡，如电影切镜）
	PC->SetViewTargetWithBlend(this, 0.f);

	GetWorldTimerManager().SetTimer(
		HoldTimerHandle,
		this, &ALevelIntroCameraMarker::OnHoldComplete,
		HoldDuration, false);
}

void ALevelIntroCameraMarker::OnHoldComplete()
{
	APlayerController* PC = CachedPC.Get();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	// 平滑移回玩家摄像机
	PC->SetViewTargetWithBlend(Pawn, MoveDuration, VTBlend_Cubic);

	if (bDisableInputDuringIntro)
	{
		GetWorldTimerManager().SetTimer(
			MoveTimerHandle,
			this, &ALevelIntroCameraMarker::OnMoveComplete,
			MoveDuration, false);
	}
}

void ALevelIntroCameraMarker::OnMoveComplete()
{
	if (APlayerController* PC = CachedPC.Get())
		PC->EnableInput(PC);
}
