#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_GiveCard.generated.h"

class URuneDataAsset;

/**
 * 向玩家战斗卡组和背包各添加一张符文卡。
 * Out — 两步均成功（或背包写入成功但卡组失败由 DeckFailed 处理）
 * DeckFailed — 找不到 CombatDeckComponent
 * InventoryFailed — 找不到 APlayerCharacterBase
 */
UCLASS(meta = (DisplayName = "Give Card"))
class DEVKIT_API USNode_GiveCard : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Card")
	TObjectPtr<URuneDataAsset> CardToGive;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
