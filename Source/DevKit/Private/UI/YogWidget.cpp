// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/YogWidget.h"

void UYogWidget::CloseWidget()
{
	// InputMode + focus are owned by UYogUIManagerSubsystem::ApplyInputModeForLayer.
	// Closing a widget must not slam GameOnly — that stomped focus on still-active overlays.
	RemoveFromParent();
}
