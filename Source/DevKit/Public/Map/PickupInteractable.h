#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PickupInteractable.generated.h"

class APlayerCharacterBase;

UINTERFACE(MinimalAPI)
class UPickupInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPickupInteractable — 拾取物交互接口
 *
 * 所有可被玩家按 E 键拾取的 Actor 实现此接口。
 * 玩家进入碰撞范围时调用 OnPlayerEnterRange（显示提示 Widget），
 * 离开时调用 OnPlayerLeaveRange（隐藏 Widget），按 E 键时调用 TryPickup。
 */
class DEVKIT_API IPickupInteractable
{
	GENERATED_BODY()

public:
	/** 玩家进入碰撞范围：显示交互提示 Widget */
	virtual void OnPlayerEnterRange(APlayerCharacterBase* Player) = 0;

	/** 玩家离开碰撞范围：隐藏交互提示 Widget */
	virtual void OnPlayerLeaveRange(APlayerCharacterBase* Player) = 0;

	/** 玩家按 E 键：执行拾取逻辑 */
	virtual void TryPickup(APlayerCharacterBase* Player) = 0;
};
