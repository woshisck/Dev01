#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YogUIRegistry.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EYogUIScreenId : uint8
{
	MainHUD,
	Backpack,
	LootSelection,
	PauseMenu,
	TutorialPopup,
	SacrificeGraceOption,
	InfoPopup,
	PortalPreview,
	PortalDirection,
	CurrentRoomBuff,
	CombatItemBar,
	FinisherQTE,
	LevelEndReveal,
	WeaponFloat,
	WeaponThumbnailFly,
	WeaponTrail,
	DamageEdgeFlash
};

USTRUCT(BlueprintType)
struct DEVKIT_API FYogUIRegistryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EYogUIScreenId ScreenId = EYogUIScreenId::MainHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<UUserWidget> WidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ZOrder = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCreateOnHUDStart = false;
};

UCLASS(BlueprintType)
class DEVKIT_API UYogUIRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "UI")
	bool FindEntry(EYogUIScreenId ScreenId, FYogUIRegistryEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	TSoftClassPtr<UUserWidget> GetWidgetClass(EYogUIScreenId ScreenId) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	int32 GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder = 0) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<FYogUIRegistryEntry> Entries;
};
