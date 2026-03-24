#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameplayEffectTypes.h"
#include "CharacterDataComponent.generated.h"


class UCharacterData;

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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | CharacterData")
	TObjectPtr<UCharacterData> CharacterData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | CharacterData")
	TSoftClassPtr<UCharacterData> CharacterDataClass;

	// UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, Category = "Anathema | CharacterData")
	// TObjectPtr<UCharacterData> CharacterData;

};
