// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PlayerUIStyleDataAsset.h"

UTexture2D* UPlayerUIStyleDataAsset::GetCursorTexture(EYogCursorState State) const
{
	UTexture2D* Texture = nullptr;
	switch (State)
	{
	case EYogCursorState::Interact:
		Texture = CursorInteractTexture;
		break;
	case EYogCursorState::Drag:
		Texture = CursorDragTexture;
		break;
	case EYogCursorState::Invalid:
		Texture = CursorInvalidTexture;
		break;
	default:
		break;
	}

	return Texture ? Texture : CursorDefaultTexture.Get();
}
