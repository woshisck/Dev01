#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"



UCharacterDataComponent::UCharacterDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}

UCharacterData* UCharacterDataComponent::GetCharacterData() const
{
	return CharacterData;
}

void UCharacterDataComponent::SetCharacterData(UCharacterData* NewCharacterData)
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

const UCharacterData* UCharacterDataComponent::InitializeCharacterData()
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


void UCharacterDataComponent::BeginPlay()
{
	Super::BeginPlay();

	// 缓存原始 AbilityData，供 EndPlay 还原使用
	// 武器拾取会直接写 CharacterData->AbilityData（资产对象在内存中持久），
	// EndPlay 还原后下一局 PIE 从干净状态开始。
	if (CharacterData)
	{
		OriginalAbilityData = CharacterData->AbilityData;
	}
}

void UCharacterDataComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// 还原 AbilityData，避免下次 PIE 时资产对象携带武器技能数据
	if (CharacterData && OriginalAbilityData)
	{
		CharacterData->AbilityData = OriginalAbilityData;
	}
	OriginalAbilityData = nullptr;
}


