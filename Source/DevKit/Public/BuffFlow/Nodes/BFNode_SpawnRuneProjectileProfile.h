#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_SpawnRuneProjectileProfile.generated.h"

class URuneCardEffectProfileDA;
class ACharacter;

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Rune Projectile Profile", Category = "BuffFlow|Profile"))
class DEVKIT_API UBFNode_SpawnRuneProjectileProfile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 投射物效果配置文件 — 包含投射物类、伤害和表现的 DA 资产
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "投射物配置文件"))
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void SpawnLaunchNiagara(ACharacter* SourceCharacter, const FVector& SpawnLocation, const FRotator& SpawnRotation) const;
	float ResolveDamage(ACharacter* SourceCharacter) const;
	void ConsumeSourceArmor(ACharacter* SourceCharacter) const;
};
