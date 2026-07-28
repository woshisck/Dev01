#include "Tools/EnemyManager/EnemyDataActorFactory.h"

#include "AssetRegistry/AssetData.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/EnemyData.h"

UEnemyDataActorFactory::UEnemyDataActorFactory()
{
	DisplayName = NSLOCTEXT("EnemyManager", "EnemyDataActorFactoryName", "敌人定义");
	NewActorClass = AEnemyCharacterBase::StaticClass();
	bShowInEditorQuickMenu = false;
	// The level viewport validates an asset against registered factories before
	// it reads the factory carried by FAssetDragDropOp. Keep one generic
	// factory registered so UEnemyData passes that first validation gate.
	bShouldAutoRegister = true;
}

void UEnemyDataActorFactory::Configure(UEnemyData* InEnemyData)
{
	EnemyData = InEnemyData;
	NewActorClass = EnemyData && EnemyData->EnemyClass
		? EnemyData->EnemyClass.Get()
		: AEnemyCharacterBase::StaticClass();
}

bool UEnemyDataActorFactory::CanCreateActorFrom(
	const FAssetData& AssetData,
	FText& OutErrorMsg)
{
	UEnemyData* AssetEnemy = Cast<UEnemyData>(AssetData.GetAsset());
	if (!AssetEnemy || (EnemyData && AssetEnemy != EnemyData))
	{
		OutErrorMsg = NSLOCTEXT(
			"EnemyManager",
			"InvalidEnemyDataForPlacement",
			"所选资产不是当前配置的敌人定义 DA。");
		return false;
	}

	if (!AssetEnemy->EnemyClass)
	{
		OutErrorMsg = NSLOCTEXT(
			"EnemyManager",
			"EnemyDataMissingEnemyClass",
			"将敌人拖入关卡前，请先在“敌人 BP”页面配置可生成的 EnemyClass。");
		return false;
	}

	return true;
}

UClass* UEnemyDataActorFactory::GetDefaultActorClass(const FAssetData& AssetData)
{
	if (const UEnemyData* AssetEnemy = Cast<UEnemyData>(AssetData.GetAsset()))
	{
		if (AssetEnemy->EnemyClass)
		{
			return AssetEnemy->EnemyClass.Get();
		}
	}
	return Super::GetDefaultActorClass(AssetData);
}

void UEnemyDataActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AEnemyCharacterBase* EnemyActor = Cast<AEnemyCharacterBase>(NewActor);
	UEnemyData* AssetEnemy = Cast<UEnemyData>(Asset);
	if (!EnemyActor || !AssetEnemy)
	{
		return;
	}

	EnemyActor->Modify();
	if (UCharacterDataComponent* CharacterData = EnemyActor->GetCharacterDataComponent())
	{
		CharacterData->Modify();
		CharacterData->SetCharacterData(AssetEnemy);
	}
	EnemyActor->SetPendingEnemyWeaponDefinition(AssetEnemy->DefaultWeaponDefinition);
	EnemyActor->SetActorLabel(GetDefaultActorLabel(AssetEnemy));
}

UObject* UEnemyDataActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
	const AEnemyCharacterBase* EnemyActor = Cast<AEnemyCharacterBase>(ActorInstance);
	const UCharacterDataComponent* CharacterData = EnemyActor
		? EnemyActor->GetCharacterDataComponent()
		: nullptr;
	return CharacterData ? Cast<UEnemyData>(CharacterData->GetCharacterData()) : nullptr;
}

FString UEnemyDataActorFactory::GetDefaultActorLabel(UObject* Asset) const
{
	if (const UEnemyData* AssetEnemy = Cast<UEnemyData>(Asset))
	{
		if (!AssetEnemy->DisplayName.IsEmpty())
		{
			return FString::Printf(TEXT("Enemy_%s"), *AssetEnemy->DisplayName.ToString());
		}

		FString Stem = AssetEnemy->GetName();
		Stem.RemoveFromStart(TEXT("DA_TEST_EN_"));
		Stem.RemoveFromStart(TEXT("DA_EN_"));
		Stem.RemoveFromStart(TEXT("DA_"));
		return FString::Printf(TEXT("Enemy_%s"), *Stem);
	}

	return TEXT("Enemy");
}
