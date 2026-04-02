// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/YogWidget.h"

void UYogWidget::CloseWidget()
{
	RemoveFromParent();

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->bShowMouseCursor = false;

        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
    }
}
