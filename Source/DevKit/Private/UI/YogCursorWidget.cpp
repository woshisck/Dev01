// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/YogCursorWidget.h"

#include "Components/Image.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "UI/YogUIManagerSubsystem.h"
#include "UI/PlayerUIStyleDataAsset.h"

void UYogCursorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OnCursorStateChanged(EYogCursorState::Default);
}

void UYogCursorWidget::OnCursorStateChanged_Implementation(EYogCursorState NewState)
{
	if (!Img_Cursor)
	{
		return;
	}

	UTexture2D* FinalTexture = DefaultCursorTexture;

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UYogUIManagerSubsystem* UI = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
		{
			UTexture2D* StyleTexture = nullptr;
			if (UPlayerUIStyleDataAsset* Style = UI->GetUIStyle())
			{
				StyleTexture = Style->GetCursorTexture(NewState);
			}

			FinalTexture = UI->ResolveStyleTexture(StyleTexture, DefaultCursorTexture, FName("Cursor"));
		}
	}

	if (FinalTexture)
	{
		Img_Cursor->SetBrushFromTexture(FinalTexture);
	}
}
