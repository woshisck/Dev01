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

	UPROPERTY(EditAnywhere, Category = "Profile")
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void SpawnLaunchNiagara(ACharacter* SourceCharacter, const FVector& SpawnLocation, const FRotator& SpawnRotation) const;
	float ResolveDamage(ACharacter* SourceCharacter) const;
	void ConsumeSourceArmor(ACharacter* SourceCharacter) const;
};
