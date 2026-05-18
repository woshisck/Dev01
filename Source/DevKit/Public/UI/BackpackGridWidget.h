#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BackpackGridWidget.generated.h"

class UUniformGridPanel;
class USizeBox;
class URuneSlotWidget;
class UBackpackGridComponent;
class UBackpackStyleDataAsset;
enum class EBackpackCellState : uint8;

/**
 * Backpack grid child widget used by WBP_BackpackGrid.
 *
 * The widget owns grid layout and cell refresh only. Heat preview controls are
 * intentionally not part of this widget; the active-zone visualization is
 * driven by the runtime backpack state passed to RefreshCells.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBackpackGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** WBP control variable name: BackpackGrid */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> BackpackGrid;

	/** WBP control variable name: GridSizeBox */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> GridSizeBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
	TSubclassOf<URuneSlotWidget> RuneSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	TObjectPtr<UBackpackStyleDataAsset> StyleDA;

	int32 CachedGridW = 5;
	int32 CachedGridH = 5;

	void BuildGrid(UBackpackGridComponent* InBackpack);

	void RefreshCells(UBackpackGridComponent* Backpack,
					  FIntPoint SelectedCell,
					  FIntPoint HoverCell,
					  FIntPoint GrabbedFromCell,
					  bool bGrabbing,
					  int32 PreviewPhase = -1);

	bool GetCellAtScreenPos(const FVector2D& AbsPos, UBackpackGridComponent* Backpack,
							int32& OutCol, int32& OutRow) const;

	void FlashAndShakeCell(int32 Col, int32 Row);

	FGeometry GetGridGeometry() const;

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args,
							  const FGeometry& AllottedGeometry,
							  const FSlateRect& MyCullingRect,
							  FSlateWindowElementList& OutDrawElements,
							  int32 LayerId,
							  const FWidgetStyle& InWidgetStyle,
							  bool bParentEnabled) const override;

private:
	UPROPERTY()
	TArray<TObjectPtr<URuneSlotWidget>> CachedSlots;

	TWeakObjectPtr<UBackpackGridComponent> CachedBackpackRef;
	FGuid CachedSelectedGuid;
	FGuid CachedHoverGuid;
	bool CachedBGrabbing = false;
	FIntPoint CachedGrabbedFromCell = FIntPoint(-1, -1);
};
