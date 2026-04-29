// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png") ), __VA_ARGS__)

class FComboGraphEditorStyle : public FSlateStyleSet
{
public:
	FComboGraphEditorStyle() : FSlateStyleSet("ComboGraphEditorStyle")
	{
		const FVector2D Icon16x16(16.f, 16.f);
		const FVector2D Icon20x20(20.f, 20.f);
		const FVector2D Icon40x40(40.f, 40.f);
		const FVector2D Icon64x64(64.f, 64.f);
		const FVector2D Icon128x128(128.f, 128.f);

		SetContentRoot(IPluginManager::Get().FindPlugin("ComboGraph")->GetBaseDir() / TEXT("Resources"));


		// App Mode Icons
		Set("ComboGraph.AppMode.Default", new IMAGE_BRUSH("Icons/AssetIcons/Blueprint_40x", Icon40x40));
		Set("ComboGraph.AppMode.Default.Small", new IMAGE_BRUSH("Icons/AssetIcons/Blueprint_20x", Icon20x20));
		Set("ComboGraph.AppMode.Persona", new IMAGE_BRUSH("Icons/AssetIcons/BlendSpace_40x", Icon40x40));
		Set("ComboGraph.AppMode.Persona.Small", new IMAGE_BRUSH("Icons/AssetIcons/BlendSpace_20x", Icon20x20));

		// Toolbar Icons
		Set("ComboGraph.AutoArrange.Vertical", new IMAGE_BRUSH("Icons/auto_arrange_vertical", Icon40x40));
		Set("ComboGraph.AutoArrange.Horizontal", new IMAGE_BRUSH("Icons/auto_arrange_horizontal", Icon40x40));

		// Class Icons
		Set("ClassIcon.ComboGraph", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_x16", Icon16x16));
		Set("ClassThumbnail.ComboGraph", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_x64", Icon64x64));

		Set("ClassIcon.ComboGraphNodeMontage", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Montage_x16", Icon16x16));
		Set("ClassThumbnail.ComboGraphNodeMontage", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Montage_x64", Icon64x64));
		Set("ClassIcon.ComboGraphNodeMontageBlueprint", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Montage_x16", Icon16x16));
		Set("ClassThumbnail.ComboGraphNodeMontageBlueprint", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Montage_x64", Icon64x64));

		Set("ClassIcon.ComboGraphNodeSequence", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Sequence_x16", Icon16x16));
		Set("ClassThumbnail.ComboGraphNodeSequence", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Sequence_x64", Icon64x64));
		Set("ClassIcon.ComboGraphNodeSequenceBlueprint", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Sequence_x16", Icon16x16));
		Set("ClassThumbnail.ComboGraphNodeSequenceBlueprint", new IMAGE_BRUSH("Icons/AssetIcons/ComboGraph_Sequence_x64", Icon64x64));

		FSlateStyleRegistry::RegisterSlateStyle(*this);
	}

	static FComboGraphEditorStyle& Get()
	{
		static FComboGraphEditorStyle Inst;
		return Inst;
	}

	~FComboGraphEditorStyle()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	}
};

#undef IMAGE_BRUSH
