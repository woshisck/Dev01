#include "Tools/WeaponManager/WeaponDefinitionActorFactory.h"

#include "AssetRegistry/AssetData.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Item/Weapon/WeaponSpawner.h"

namespace
{
	const TCHAR* SharedWeaponSpawnerClassPath =
		TEXT("/Game/Code/Weapon/BP_WeaponSpawner.BP_WeaponSpawner_C");
}

UWeaponDefinitionActorFactory::UWeaponDefinitionActorFactory()
{
	DisplayName = NSLOCTEXT("WeaponManager", "WeaponDefinitionActorFactoryName", "武器定义");
	NewActorClass = AWeaponSpawner::StaticClass();
	bShowInEditorQuickMenu = false;
	bShouldAutoRegister = false;
}

void UWeaponDefinitionActorFactory::Configure(UWeaponDefinition* InWeaponDefinition)
{
	WeaponDefinition = InWeaponDefinition;

	if (UClass* SharedSpawnerClass = StaticLoadClass(
			AWeaponSpawner::StaticClass(),
			nullptr,
			SharedWeaponSpawnerClassPath))
	{
		NewActorClass = SharedSpawnerClass;
	}
	else
	{
		NewActorClass = AWeaponSpawner::StaticClass();
	}
}

bool UWeaponDefinitionActorFactory::CanCreateActorFrom(
	const FAssetData& AssetData,
	FText& OutErrorMsg)
{
	UWeaponDefinition* AssetWeapon = Cast<UWeaponDefinition>(AssetData.GetAsset());
	if (!AssetWeapon || AssetWeapon != WeaponDefinition)
	{
		OutErrorMsg = NSLOCTEXT(
			"WeaponManager",
			"InvalidWeaponDefinitionForPlacement",
			"所选资产不是当前配置的武器定义 DA。");
		return false;
	}

	if (!AssetWeapon->DisplayMesh)
	{
		OutErrorMsg = NSLOCTEXT(
			"WeaponManager",
			"WeaponDefinitionMissingDisplayMesh",
			"将武器拖入关卡前，请先配置“场景拾取 > 显示模型 > 场景显示网格”。");
		return false;
	}

	return true;
}

void UWeaponDefinitionActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AWeaponSpawner* WeaponSpawner = Cast<AWeaponSpawner>(NewActor);
	UWeaponDefinition* AssetWeapon = Cast<UWeaponDefinition>(Asset);
	if (!WeaponSpawner || !AssetWeapon)
	{
		return;
	}

	WeaponSpawner->Modify();
	WeaponSpawner->WeaponDefinition = AssetWeapon;
	WeaponSpawner->RerunConstructionScripts();
	WeaponSpawner->SetActorLabel(GetDefaultActorLabel(AssetWeapon));
}

UObject* UWeaponDefinitionActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
	const AWeaponSpawner* WeaponSpawner = Cast<AWeaponSpawner>(ActorInstance);
	return WeaponSpawner ? WeaponSpawner->WeaponDefinition.Get() : nullptr;
}

FString UWeaponDefinitionActorFactory::GetDefaultActorLabel(UObject* Asset) const
{
	if (const UWeaponDefinition* Weapon = Cast<UWeaponDefinition>(Asset))
	{
		if (Weapon->WeaponInfo && !Weapon->WeaponInfo->WeaponName.IsEmpty())
		{
			return FString::Printf(TEXT("Weapon_%s"), *Weapon->WeaponInfo->WeaponName.ToString());
		}

		FString Stem = Weapon->GetName();
		Stem.RemoveFromStart(TEXT("DA_WPN_"));
		Stem.RemoveFromStart(TEXT("WD_"));
		return FString::Printf(TEXT("Weapon_%s"), *Stem);
	}

	return TEXT("WeaponSpawner");
}
