// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/YogCursorStyleDataAsset.h"

EMouseCursor::Type UYogCursorStyleDataAsset::StateToSlot(EYogCursorState State)
{
	switch (State)
	{
	case EYogCursorState::Interact:
		return EMouseCursor::Hand;
	case EYogCursorState::Drag:
		return EMouseCursor::GrabHandClosed;
	case EYogCursorState::Invalid:
		return EMouseCursor::SlashedCircle;
	case EYogCursorState::Default:
	default:
		return EMouseCursor::Default;
	}
}
