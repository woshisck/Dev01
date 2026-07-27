#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
#include "GA_WeaponSkillTypes.generated.h"

/** 双手剑多段连击战技。 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "双手剑连段 GA")
class DEVKIT_API UGA_WeaponSkill_THSwordCombo : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_THSwordCombo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool PrepareForInputActivation() override;
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

#if WITH_DEV_AUTOMATION_TESTS
	bool PrepareComboSlotForTesting(bool bAbilityIsActive, bool bComboWindowIsOpen)
	{
		return PrepareComboSlot(bAbilityIsActive, bComboWindowIsOpen, false);
	}
	int32 GetPreparedComboSlotForTesting() const { return PreparedComboSlot; }
	void SetCurrentComboSlotForTesting(int32 InComboSlot) { CurrentComboSlot = InComboSlot; }
	void ApplyPreparedComboTagForTesting() { ApplyPreparedComboTag(); }
#endif

private:
	bool PrepareComboSlot(bool bAbilityIsActive, bool bComboWindowIsOpen, bool bRequireConfiguredMontage);
	void ApplyPreparedComboTag();

	int32 CurrentComboSlot = 0;
	int32 PreparedComboSlot = 1;
};

/** 按住/松开式格挡战技；格挡独有的 C++ 效果在此扩展。 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "格挡 GA")
class DEVKIT_API UGA_WeaponSkill_Block : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Block(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

/** 单段突刺战技；突刺独有的 C++ 效果在此扩展。 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "突刺 GA")
class DEVKIT_API UGA_WeaponSkill_Thrust : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Thrust(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
