#include "Character/PlayerBuffLuaLibrary.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Character/PlayerCharacterBase.h"
#include "GameplayEffect.h"

static FGameplayTag PlayerBuffLuaLibrary_ResolveTag(FName TagName)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBuff][Lua] unknown gameplay tag '%s'."), *TagName.ToString());
	}
	return Tag;
}

// Searches every attribute set the ASC owns, so scripts can name attributes without knowing their set.
static FGameplayAttribute PlayerBuffLuaLibrary_ResolveAttribute(const UAbilitySystemComponent* ASC, FName AttributeName)
{
	if (!ASC)
	{
		return FGameplayAttribute();
	}

	for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}

		if (FProperty* Property = FindFProperty<FProperty>(Set->GetClass(), AttributeName))
		{
			if (FGameplayAttribute::IsGameplayAttributeDataProperty(Property))
			{
				return FGameplayAttribute(Property);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayerBuff][Lua] unknown attribute '%s'."), *AttributeName.ToString());
	return FGameplayAttribute();
}

UYogAbilitySystemComponent* UPlayerBuffLuaLibrary::GetPlayerASC(APlayerCharacterBase* Player)
{
	return IsValid(Player) ? Player->GetASC() : nullptr;
}

FActiveGameplayEffectHandle UPlayerBuffLuaLibrary::ApplyPlayerEffect(APlayerCharacterBase* Player, TSubclassOf<UGameplayEffect> Effect, float Level)
{
	UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	if (!ASC || !Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Player);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, Level, EffectContext);
	return Spec.IsValid() ? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()) : FActiveGameplayEffectHandle();
}

void UPlayerBuffLuaLibrary::RemovePlayerEffect(APlayerCharacterBase* Player, FActiveGameplayEffectHandle Handle)
{
	UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	if (ASC && Handle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
}

void UPlayerBuffLuaLibrary::AddPlayerLooseTag(APlayerCharacterBase* Player, FName TagName)
{
	UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	const FGameplayTag Tag = PlayerBuffLuaLibrary_ResolveTag(TagName);
	if (ASC && Tag.IsValid())
	{
		ASC->AddLooseGameplayTag(Tag);
	}
}

void UPlayerBuffLuaLibrary::RemovePlayerLooseTag(APlayerCharacterBase* Player, FName TagName)
{
	UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	const FGameplayTag Tag = PlayerBuffLuaLibrary_ResolveTag(TagName);
	if (ASC && Tag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(Tag);
	}
}

bool UPlayerBuffLuaLibrary::HasPlayerTag(APlayerCharacterBase* Player, FName TagName)
{
	const UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	const FGameplayTag Tag = PlayerBuffLuaLibrary_ResolveTag(TagName);
	return ASC && Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag);
}

float UPlayerBuffLuaLibrary::GetPlayerAttribute(APlayerCharacterBase* Player, FName AttributeName)
{
	const UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	const FGameplayAttribute Attribute = PlayerBuffLuaLibrary_ResolveAttribute(ASC, AttributeName);
	return Attribute.IsValid() ? ASC->GetNumericAttribute(Attribute) : 0.f;
}

void UPlayerBuffLuaLibrary::AddPlayerAttributeBase(APlayerCharacterBase* Player, FName AttributeName, float Delta)
{
	UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	const FGameplayAttribute Attribute = PlayerBuffLuaLibrary_ResolveAttribute(ASC, AttributeName);
	if (Attribute.IsValid())
	{
		ASC->SetNumericAttributeBase(Attribute, ASC->GetNumericAttributeBase(Attribute) + Delta);
	}
}

void UPlayerBuffLuaLibrary::HealPlayer(APlayerCharacterBase* Player, float Amount)
{
	UYogAbilitySystemComponent* ASC = GetPlayerASC(Player);
	if (!ASC || Amount <= 0.f || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()))
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), FMath::Min(Health + Amount, MaxHealth));
}
