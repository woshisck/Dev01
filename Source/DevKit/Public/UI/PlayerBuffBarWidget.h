#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerBuffBarWidget.generated.h"

class UHorizontalBox;

/**
 * Compact placeholder strip for player-visible Buff.Status gameplay tags.
 *
 * The first version intentionally renders stable square placeholders so art can
 * be swapped in later without changing the HUD root layout.
 */
UCLASS()
class DEVKIT_API UPlayerBuffBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BuffIconBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff Bar", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxDisplayedBuffs = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff Bar")
	bool bShowEmptyPlaceholderSlots = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff Bar", meta = (ClampMin = "1", ClampMax = "10", EditCondition = "bShowEmptyPlaceholderSlots"))
	int32 EmptyPlaceholderSlotCount = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff Bar", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float RefreshIntervalSeconds = 0.15f;

private:
	void EnsureRuntimeWidgetTree();
	void RefreshBuffIcons();
	void RebuildBuffIcons(const TArray<FString>& Labels);
	TArray<FString> CollectVisibleBuffLabels() const;
	TArray<FString> MakeEmptyPlaceholderLabels() const;

	static bool ShouldShowBuffTag(const FString& TagString);
	static FString MakeShortBuffLabel(const FString& TagString);

	UPROPERTY(Transient)
	TArray<FString> CachedLabels;

	float RefreshAccumulator = 0.f;
};
