#include "Component/DataCacheComponent.h"



UDataCacheComponent::UDataCacheComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}

UCharacterData* UDataCacheComponent::GetCharacterData() const
{
	return CharacterData;
}

void UDataCacheComponent::SetCharacterData(UCharacterData* NewCharacterData)
{
	CharacterData = NewCharacterData;

	// Initialize Character data initial config if valid
	// #HEXTODO Maybe move this to initialization of AIData and call it at some point
	// 
	// 
	//if (CharacterData && CharacterData->IsAICharacter())
	//{
	//	ConfigVars = CharacterData->GetAIData()->ConfigVars;
	//}
}

const UCharacterData* UDataCacheComponent::InitializeCharacterData()
{
	// Load the character data instance from the asset class
	if (!CharacterDataClass.IsNull())
	{
		UClass* pCharacterDataClass = CharacterDataClass.LoadSynchronous();
		ensureAlwaysMsgf(pCharacterDataClass, TEXT("Broken soft reference %s"), *CharacterDataClass.ToString());
		if (pCharacterDataClass)
		{
			UCharacterData* pLoadedCharacterData = pCharacterDataClass->GetDefaultObject<UCharacterData>();
			SetCharacterData(pLoadedCharacterData);
		}
	}

	// Return our current character data
	UCharacterData* pCharacterData = GetCharacterData();
	ensureMsgf(pCharacterData, TEXT("Character Data is null after calling InitializeCharacterData for actor %s"), *GetOwner()->GetName());
	return pCharacterData;
}


void UDataCacheComponent::BeginPlay()
{
	Super::BeginPlay();

	// UAbilitySystemComponent* abilitySystemComponent = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();

	// abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute()).AddUObject(this, &UDataCacheComponent::HandleHealthChange);

}

void UDataCacheComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);
}


