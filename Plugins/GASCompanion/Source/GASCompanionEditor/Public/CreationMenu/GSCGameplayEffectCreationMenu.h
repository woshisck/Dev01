// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GSCGameplayEffectCreationMenu.generated.h"

class FMenuBuilder;
class UGSCTemplate_GameplayEffectDefinition;

USTRUCT()
struct FGSCGameplayEffectCreationData
{
	GENERATED_BODY()

	/** Where to show this in the menu. Use "|" for sub categories. E.g, "Status|Hard|Stun|Root". */
	UPROPERTY(EditAnywhere, Category="Gameplay Effect")
	FString MenuPath;

	/** The default BaseName of the new asset. E.g "Damage" -> GE_Damage */
	UPROPERTY(EditAnywhere, Category="Gameplay Effect")
	FString BaseName;

	UPROPERTY(EditAnywhere, Category = "Gameplay Effect", meta=(DisplayName = "Gameplay Effect Template"))
	TSubclassOf<UGSCTemplate_GameplayEffectDefinition> ParentClass;

	UPROPERTY()
	FString AssetPrefix = "GE_";

	UPROPERTY()
	FText TooltipText = FText::FromString("Tooltip");

	/** Default constructor */
	FGSCGameplayEffectCreationData() = default;

	FString ToString() const;
};

UCLASS(config=Game, defaultconfig, notplaceable)
class GASCOMPANIONEDITOR_API UGSCGameplayEffectCreationMenu : public UObject
{
	GENERATED_BODY()

public:
	UGSCGameplayEffectCreationMenu();

	void AddMenuExtensions() const;

	static void TopMenuBuild(FMenuBuilder& TopMenuBuilder, const FText InMenuLabel, const FText InMenuTooltip, const TArray<FString> SelectedPaths, TArray<FGSCGameplayEffectCreationData> Definitions);

	UPROPERTY(config, EditAnywhere, Category="Gameplay Effect")
	TArray<FGSCGameplayEffectCreationData> Definitions;

protected:

	void AddDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UObject> ParentClass, const FText TooltipText = FText());
	static FGSCGameplayEffectCreationData CreateDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UObject> ParentClass, const FText TooltipText);
};
