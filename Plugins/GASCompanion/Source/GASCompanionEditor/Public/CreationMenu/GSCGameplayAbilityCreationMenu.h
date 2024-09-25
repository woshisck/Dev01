// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GSCGameplayAbilityCreationMenu.generated.h"

class FMenuBuilder;
class UGameplayAbility;

USTRUCT()
struct FGSCGameplayAbilityCreationData
{
	GENERATED_BODY()

	/** Where to show this in the menu. Use "|" for sub categories. E.g, "Ability|Skills|Ultimate". */
	UPROPERTY(EditAnywhere, Category="Gameplay Ability")
	FString MenuPath;

	/** The default BaseName of the new asset. E.g "Damage" -> GE_Damage or GE_HeroName_AbilityName_Damage */
	UPROPERTY(EditAnywhere, Category="Gameplay Ability")
	FString BaseName;

	UPROPERTY(EditAnywhere, Category = "Gameplay Ability", meta=(DisplayName = "Parent Gameplay Ability"))
	TSubclassOf<UGameplayAbility> ParentClass;

	UPROPERTY()
	FString AssetPrefix = "GA_";

	UPROPERTY()
	FText TooltipText = FText::FromString("Tooltip");
};

UCLASS(config=Game, defaultconfig, notplaceable)
class GASCOMPANIONEDITOR_API UGSCGameplayAbilityCreationMenu : public UObject
{
	GENERATED_BODY()

public:
	UGSCGameplayAbilityCreationMenu();

	void AddMenuExtensions() const;

	static void TopMenuBuild(FMenuBuilder& TopMenuBuilder, const FText InMenuLabel, const FText InMenuTooltip, const TArray<FString> SelectedPaths, TArray<FGSCGameplayAbilityCreationData> Definitions);

	UPROPERTY(config, EditAnywhere, Category="Gameplay Ability")
	TArray<FGSCGameplayAbilityCreationData> Definitions;

protected:
	void AddDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UGameplayAbility> ParentClass, const FText TooltipText = FText());
	static FGSCGameplayAbilityCreationData CreateDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UGameplayAbility> ParentClass, const FText TooltipText);
};
