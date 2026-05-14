#include "Component/CharacterDataComponent.h"

#include "Data/CharacterData.h"

UCharacterDataComponent::UCharacterDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UCharacterDataComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!CharacterData)
	{
		return;
	}

	const bool bIsSourceAsset = !CharacterData->HasAnyFlags(RF_ClassDefaultObject | RF_Transient);
	if (bIsSourceAsset)
	{
		OriginalCharacterDataRef = CharacterData;
		OriginalAbilityData = CharacterData->AbilityData;
		UE_LOG(LogTemp, Warning, TEXT("[CDC][InitComp] Owner=%s | Ref=%s | AbilityData=%s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			*CharacterData->GetName(),
			OriginalAbilityData ? *OriginalAbilityData->GetName() : TEXT("null"));
	}

	if (!CharacterData->HasAnyFlags(RF_Transient))
	{
		UCharacterData* RuntimeCopy = DuplicateObject<UCharacterData>(CharacterData, this);
		if (RuntimeCopy)
		{
			RuntimeCopy->SetFlags(RF_Transient);
			SetCharacterData(RuntimeCopy);
			UE_LOG(LogTemp, Warning, TEXT("[CDC][InitComp] RuntimeCopy=%s IsTransient=%d | AbilityData=%s"),
				*RuntimeCopy->GetName(),
				(int32)RuntimeCopy->HasAnyFlags(RF_Transient),
				RuntimeCopy->AbilityData ? *RuntimeCopy->AbilityData->GetName() : TEXT("null"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CDC][InitComp] DuplicateObject failed for %s; runtime writes may touch source data."),
				*CharacterData->GetName());
		}
	}
}

UCharacterData* UCharacterDataComponent::GetCharacterData() const
{
	return CharacterData;
}

void UCharacterDataComponent::SetCharacterData(UCharacterData* NewCharacterData)
{
	CharacterData = NewCharacterData;
}

const UCharacterData* UCharacterDataComponent::InitializeCharacterData()
{
	if (!CharacterDataClass.IsNull())
	{
		UClass* CharacterDataClassObject = CharacterDataClass.LoadSynchronous();
		ensureAlwaysMsgf(CharacterDataClassObject, TEXT("Broken soft reference %s"), *CharacterDataClass.ToString());
		if (CharacterDataClassObject)
		{
			UCharacterData* CDO = CharacterDataClassObject->GetDefaultObject<UCharacterData>();
			UCharacterData* RuntimeCopy = DuplicateObject<UCharacterData>(CDO, this);
			if (RuntimeCopy)
			{
				RuntimeCopy->SetFlags(RF_Transient);
				SetCharacterData(RuntimeCopy);
			}
			else
			{
				SetCharacterData(CDO);
			}
		}
	}

	UCharacterData* CurrentCharacterData = GetCharacterData();
	ensureMsgf(CurrentCharacterData, TEXT("Character Data is null after calling InitializeCharacterData for actor %s"), *GetOwner()->GetName());
	return CurrentCharacterData;
}

void UCharacterDataComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!CharacterData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CDC][BeginPlay] Owner=%s | CharacterData=null; skipping runtime copy."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
		return;
	}

	const bool bIsCDO = CharacterData->HasAnyFlags(RF_ClassDefaultObject);
	const bool bIsTransient = CharacterData->HasAnyFlags(RF_Transient);
	UE_LOG(LogTemp, Warning, TEXT("[CDC][BeginPlay] Owner=%s | CD=%s IsCDO=%d IsTransient=%d | AbilityData=%s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		*CharacterData->GetName(),
		(int32)bIsCDO,
		(int32)bIsTransient,
		CharacterData->AbilityData ? *CharacterData->AbilityData->GetName() : TEXT("null"));

	if (!bIsTransient)
	{
		UCharacterData* RuntimeCopy = DuplicateObject<UCharacterData>(CharacterData, this);
		if (RuntimeCopy)
		{
			RuntimeCopy->SetFlags(RF_Transient);
			SetCharacterData(RuntimeCopy);
			UE_LOG(LogTemp, Warning, TEXT("[CDC][BeginPlay] RuntimeCopy=%s IsTransient=%d IsCDO=%d | AbilityData=%s"),
				*RuntimeCopy->GetName(),
				(int32)RuntimeCopy->HasAnyFlags(RF_Transient),
				(int32)RuntimeCopy->HasAnyFlags(RF_ClassDefaultObject),
				RuntimeCopy->AbilityData ? *RuntimeCopy->AbilityData->GetName() : TEXT("null"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CDC][BeginPlay] DuplicateObject failed for %s; runtime writes may touch source data."),
				*CharacterData->GetName());
		}
	}
}

void UCharacterDataComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	const bool bIsTransient = CharacterData ? CharacterData->HasAnyFlags(RF_Transient) : false;
	const bool bIsCDO = CharacterData ? CharacterData->HasAnyFlags(RF_ClassDefaultObject) : false;
	UE_LOG(LogTemp, Warning, TEXT("[CDC][EndPlay] Owner=%s | CD=%s IsCDO=%d IsTransient=%d | CurrentAbilityData=%s | OriginalAbilityData=%s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		CharacterData ? *CharacterData->GetName() : TEXT("null"),
		(int32)bIsCDO,
		(int32)bIsTransient,
		(CharacterData && CharacterData->AbilityData) ? *CharacterData->AbilityData->GetName() : TEXT("null"),
		OriginalAbilityData ? *OriginalAbilityData->GetName() : TEXT("null"));

	if (OriginalCharacterDataRef)
	{
		OriginalCharacterDataRef->AbilityData = OriginalAbilityData;
		UE_LOG(LogTemp, Warning, TEXT("[CDC][EndPlay] Restored source data %s -> AbilityData=%s"),
			*OriginalCharacterDataRef->GetName(),
			OriginalAbilityData ? *OriginalAbilityData->GetName() : TEXT("null"));
	}

	OriginalCharacterDataRef = nullptr;
	OriginalAbilityData = nullptr;
}
