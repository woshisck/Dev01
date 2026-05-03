#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/EnemyData.h"
#include "CurrentRoomBuffWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;
class URoomDataAsset;

/**
 * HUD panel for the room buffs that are already active in the current room.
 *
 * The widget can be used as a normal WBP parent with optional BindWidget fields.
 * If no WBP layout is supplied, it builds a compact default layout in C++.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UCurrentRoomBuffWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Room Buff")
	void ShowRoomBuffs(URoomDataAsset* RoomData, const TArray<FBuffEntry>& Buffs);

	UFUNCTION(BlueprintCallable, Category = "Room Buff")
	void HideRoomBuffs();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Buff")
	FText PanelTitle = NSLOCTEXT("CurrentRoomBuff", "PanelTitle", "当前关卡敌人符文");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Buff")
	FText EmptyTextValue = NSLOCTEXT("CurrentRoomBuff", "EmptyText", "本关无敌人符文");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Buff", meta = (ClampMin = "160.0"))
	float DefaultPanelWidth = 340.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Buff", meta = (ClampMin = "12.0"))
	float BuffIconSize = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Buff")
	bool bCollapseWhenEmpty = true;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> OuterBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoomNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> BuffListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

private:
	bool bDefaultLayoutBuilt = false;

	void BuildDefaultLayout();
	void RebuildBuffList(const TArray<FBuffEntry>& Buffs);
};
