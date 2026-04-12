#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameplayEffectTypes.h"
#include "CharacterDataComponent.generated.h"


class UCharacterData;
class UAbilityData;

USTRUCT(BlueprintType)
struct FAnimationUseCache
{
	GENERATED_BODY()

    TMap<FGameplayTag, UAnimMontage*> AnimationMapCache;

};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UCharacterDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCharacterDataComponent();

	// UCharacterData* GetCharacterData() const;
	// void SetCharacterData(UCharacterData* NewCharacterData);
	// // Load character definition from row configured in this component
	UCharacterData* GetCharacterData() const;
	void SetCharacterData(UCharacterData* NewCharacterData);
	const UCharacterData* InitializeCharacterData();



protected:


	// Called when the game starts
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | CharacterData")
	TObjectPtr<UCharacterData> CharacterData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | CharacterData")
	TSoftClassPtr<UCharacterData> CharacterDataClass;

	/** InitializeComponent 时捕获的原始资产引用（早于 Possess/RestoreRunState），EndPlay 无条件还原，防止 PIE 间污染 */
	UPROPERTY()
	TObjectPtr<UCharacterData> OriginalCharacterDataRef;

	/** InitializeComponent 时捕获的干净 AbilityData 值，EndPlay 时写回原始资产 */
	UPROPERTY()
	TObjectPtr<UAbilityData> OriginalAbilityData;

	// UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, Category = "Anathema | CharacterData")
	// TObjectPtr<UCharacterData> CharacterData;

};
