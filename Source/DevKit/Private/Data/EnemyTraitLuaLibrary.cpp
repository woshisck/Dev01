// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/EnemyTraitLuaLibrary.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Controller/YogAIController.h"
#include "Data/EnemyData.h"
#include "Engine/World.h"
#include "GameModes/YogGameMode.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

static FGameplayTag EnemyTraitLuaLibrary_ResolveTag(FName TagName)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyTrait][Lua] unknown gameplay tag '%s'."), *TagName.ToString());
	}
	return Tag;
}

TArray<AEnemyCharacterBase*> UEnemyTraitLuaLibrary::GetAliveEnemies(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const AYogGameMode* GM = World ? Cast<AYogGameMode>(World->GetAuthGameMode()) : nullptr;
	return GM ? GM->GetAllAliveEnemies() : TArray<AEnemyCharacterBase*>();
}

void UEnemyTraitLuaLibrary::SetEnemyLooseTag(AEnemyCharacterBase* Enemy, FName TagName, bool bPresent)
{
	UYogAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetASC() : nullptr;
	const FGameplayTag Tag = EnemyTraitLuaLibrary_ResolveTag(TagName);
	if (!ASC || !Tag.IsValid())
	{
		return;
	}

	const bool bHasTag = ASC->HasMatchingGameplayTag(Tag);
	if (bPresent && !bHasTag)
	{
		ASC->AddLooseGameplayTag(Tag);
	}
	else if (!bPresent && bHasTag)
	{
		ASC->RemoveLooseGameplayTag(Tag);
	}
}

bool UEnemyTraitLuaLibrary::HasEnemyTag(const AEnemyCharacterBase* Enemy, FName TagName)
{
	const UYogAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetASC() : nullptr;
	const FGameplayTag Tag = EnemyTraitLuaLibrary_ResolveTag(TagName);
	return ASC && Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag);
}

bool UEnemyTraitLuaLibrary::IsEnemyInCombat(const AEnemyCharacterBase* Enemy)
{
	const AYogAIController* YogAI = IsValid(Enemy) ? Cast<AYogAIController>(Enemy->GetController()) : nullptr;
	return YogAI && YogAI->GetEnemyAIState() == EEnemyAIState::Combat;
}

bool UEnemyTraitLuaLibrary::ForceEnemyCombat(AEnemyCharacterBase* Enemy)
{
	AYogAIController* YogAI = IsValid(Enemy) ? Cast<AYogAIController>(Enemy->GetController()) : nullptr;
	APawn* PlayerPawn = IsValid(Enemy) ? UGameplayStatics::GetPlayerPawn(Enemy, 0) : nullptr;
	if (!YogAI || !PlayerPawn)
	{
		return false;
	}

	YogAI->EnterCombat(PlayerPawn, false);
	return true;
}

void UEnemyTraitLuaLibrary::SendEnemyStateTreeEvent(AEnemyCharacterBase* Enemy, FName EventTagName)
{
	AYogAIController* YogAI = IsValid(Enemy) ? Cast<AYogAIController>(Enemy->GetController()) : nullptr;
	const FGameplayTag Tag = EnemyTraitLuaLibrary_ResolveTag(EventTagName);
	if (YogAI && Tag.IsValid())
	{
		YogAI->SendStateTreeEvent(Tag);
	}
}

FActiveGameplayEffectHandle UEnemyTraitLuaLibrary::ApplyEnemyEffect(AEnemyCharacterBase* Enemy, TSubclassOf<UGameplayEffect> Effect, float Level)
{
	UYogAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetASC() : nullptr;
	if (!ASC || !Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Enemy);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, Level, EffectContext);
	return Spec.IsValid() ? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()) : FActiveGameplayEffectHandle();
}

void UEnemyTraitLuaLibrary::RemoveEnemyEffect(AEnemyCharacterBase* Enemy, FActiveGameplayEffectHandle Handle)
{
	UYogAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetASC() : nullptr;
	if (ASC && Handle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
}

bool UEnemyTraitLuaLibrary::IsSameEnemyType(const AEnemyCharacterBase* A, const AEnemyCharacterBase* B)
{
	const UEnemyData* DataA = IsValid(A) ? A->GetEnemyData() : nullptr;
	const UEnemyData* DataB = IsValid(B) ? B->GetEnemyData() : nullptr;
	if (!DataA || !DataB)
	{
		return false;
	}

	if (!DataA->EnemyId.IsNone())
	{
		return DataA->EnemyId == DataB->EnemyId;
	}
	return DataA == DataB;
}
