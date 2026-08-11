#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_WeaponWhiffSound.generated.h"

/**
 * 挥击音效 AnimNotify。
 * 放在攻击蒙太奇挥砍弧线的峰值处，播放当前武器的 UWeaponDefinition::WhiffSound。
 * Fire-and-forget：衰减尾巴由音源自带，不做淡出，蒙太奇被打断也不会掐掉。
 * 与命中判定完全无关：挥空同样会响，这正是它存在的意义。
 * 目前仅玩家生效（敌人没有 UWeaponDefinition）。
 */
UCLASS(meta = (DisplayName = "AN Weapon Whiff Sound"))
class DEVKIT_API UAN_WeaponWhiffSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
