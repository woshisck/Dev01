#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"



UCharacterDataComponent::UCharacterDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true; // 启用 InitializeComponent 回调
}

void UCharacterDataComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// 在 Possess/RestoreRunStateFromGI 之前捕获原始资产引用和干净的 AbilityData。
	// InitializeComponent（PostInitializeComponents 阶段）早于 BeginPlay 和 Possess，
	// 因此这里拿到的一定是未被武器系统写入的干净值。
	if (CharacterData && !CharacterData->HasAnyFlags(RF_ClassDefaultObject | RF_Transient))
	{
		OriginalCharacterDataRef = CharacterData;
		OriginalAbilityData      = CharacterData->AbilityData;
		UE_LOG(LogTemp, Warning, TEXT("[CDC][InitComp] Owner=%s | Ref=%s | AbilityData=%s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			*CharacterData->GetName(),
			OriginalAbilityData ? *OriginalAbilityData->GetName() : TEXT("null"));
	}
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
			// 使用运行时副本而非 CDO，防止后续写入污染资产
			UCharacterData* CDO = pCharacterDataClass->GetDefaultObject<UCharacterData>();
			UCharacterData* RuntimeCopy = DuplicateObject<UCharacterData>(CDO, this);
			if (RuntimeCopy)
			{
				RuntimeCopy->SetFlags(RF_Transient);
				SetCharacterData(RuntimeCopy);
			}
			else
			{
				SetCharacterData(CDO); // DuplicateObject 失败时降级使用 CDO
			}
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

	if (CharacterData)
	{
		const bool bIsCDO_Before = CharacterData->HasAnyFlags(RF_ClassDefaultObject);
		UE_LOG(LogTemp, Warning, TEXT("[CDC][BeginPlay] Owner=%s | CD=%s IsCDO=%d IsTransient=%d | AbilityData=%s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
			*CharacterData->GetName(),
			(int32)bIsCDO_Before,
			(int32)CharacterData->HasAnyFlags(RF_Transient),
			CharacterData->AbilityData ? *CharacterData->AbilityData->GetName() : TEXT("null"));

		// 创建 CharacterData 的运行时副本，防止武器系统直接写入 CDO。
		UCharacterData* RuntimeCopy = DuplicateObject<UCharacterData>(CharacterData, this);
		if (RuntimeCopy)
		{
			RuntimeCopy->SetFlags(RF_Transient); // 不序列化，不写回资产
			CharacterData = RuntimeCopy;
			UE_LOG(LogTemp, Warning, TEXT("[CDC][BeginPlay] DuplicateObject 成功 → RuntimeCopy=%s IsTransient=%d IsCDO=%d | AbilityData=%s"),
				*RuntimeCopy->GetName(),
				(int32)RuntimeCopy->HasAnyFlags(RF_Transient),
				(int32)RuntimeCopy->HasAnyFlags(RF_ClassDefaultObject),
				RuntimeCopy->AbilityData ? *RuntimeCopy->AbilityData->GetName() : TEXT("null"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CDC][BeginPlay] DuplicateObject 失败！CharacterData 仍是 %s（IsCDO=%d），后续写入将污染 CDO！"),
				*CharacterData->GetName(), (int32)bIsCDO_Before);
		}

		// OriginalAbilityData 已在 InitializeComponent 中捕获（早于 Possess/RestoreRunState，值是干净的）
		// 不在此处覆盖，否则会把 RestoreRunState 写入后的"污染值"当作原始值保存
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CDC][BeginPlay] Owner=%s | CharacterData 为 null，跳过副本创建"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
	}
}

void UCharacterDataComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// CharacterData 通常已是运行时副本（RF_Transient），销毁时会被 GC，无需手动还原。
	// 若 DuplicateObject 失败导致 CharacterData 仍是 CDO，执行兜底还原。
	const bool bIsTransient = CharacterData ? CharacterData->HasAnyFlags(RF_Transient) : false;
	const bool bIsCDO       = CharacterData ? CharacterData->HasAnyFlags(RF_ClassDefaultObject) : false;
	UE_LOG(LogTemp, Warning, TEXT("[CDC][EndPlay] Owner=%s | CD=%s IsCDO=%d IsTransient=%d | CurrentAbilityData=%s | OriginalAbilityData=%s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		CharacterData ? *CharacterData->GetName() : TEXT("null"),
		(int32)bIsCDO,
		(int32)bIsTransient,
		(CharacterData && CharacterData->AbilityData) ? *CharacterData->AbilityData->GetName() : TEXT("null"),
		OriginalAbilityData ? *OriginalAbilityData->GetName() : TEXT("null"));

	// 无论 DuplicateObject 是否成功，无条件通过 OriginalCharacterDataRef 还原原始资产。
	// OriginalCharacterDataRef 在 InitializeComponent 中捕获（早于任何 Possess/RestoreRunState），
	// 因此总是指向内容浏览器中的真实 DA，不受运行时副本生命周期影响。
	if (OriginalCharacterDataRef && OriginalAbilityData)
	{
		OriginalCharacterDataRef->AbilityData = OriginalAbilityData;
		UE_LOG(LogTemp, Warning, TEXT("[CDC][EndPlay] 还原原始资产 %s → AbilityData=%s"),
			*OriginalCharacterDataRef->GetName(),
			*OriginalAbilityData->GetName());
	}
	OriginalCharacterDataRef = nullptr;
	OriginalAbilityData = nullptr;
}


