#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PassiveAbility.h"
#include "RunePassiveAbility.generated.h"

/**
 * 专用于符文系统的被动能力基类。
 * 与 PassiveAbility 的唯一区别：被授予时自动激活，不影响其他被动能力。
 */
UCLASS()
class DEVKIT_API URunePassiveAbility : public UPassiveAbility
{
	GENERATED_BODY()

public:
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
