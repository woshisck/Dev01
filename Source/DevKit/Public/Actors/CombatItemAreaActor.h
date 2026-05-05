#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/CombatItemDataAsset.h"
#include "CombatItemAreaActor.generated.h"

class APlayerCharacterBase;
class UDecalComponent;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class ECombatItemAreaMode : uint8
{
	Thunder UMETA(DisplayName = "Thunder"),
	Smoke   UMETA(DisplayName = "Smoke")
};

UCLASS()
class DEVKIT_API ACombatItemAreaActor : public AActor
{
	GENERATED_BODY()

public:
	ACombatItemAreaActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	void InitializeThunderArea(APlayerCharacterBase* InOwnerPlayer, const FCombatItemConfig& InConfig);
	void InitializeSmokeArea(APlayerCharacterBase* InOwnerPlayer, const FCombatItemConfig& InConfig);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Item")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Item|VFX")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Item|VFX")
	TObjectPtr<UDecalComponent> DecalComponent;

	UFUNCTION(BlueprintPure, Category = "Combat Item")
	float GetCurrentRadius() const { return CurrentRadius; }

private:
	UPROPERTY(Transient)
	TObjectPtr<APlayerCharacterBase> OwnerPlayer = nullptr;

	FCombatItemConfig Config;
	ECombatItemAreaMode AreaMode = ECombatItemAreaMode::Thunder;
	float ElapsedTime = 0.0f;
	float TickAccumulator = 0.0f;
	float CurrentRadius = 0.0f;
	bool bPlayerInsideSmoke = false;

	void UpdateRadius();
	void TickThunder(float DeltaSeconds);
	void TickSmoke(float DeltaSeconds);
	void ApplyThunderTick();
	void UpdateSmokePlayerState();
	void EndSmokePlayerState();
	void UpdateVisualScale();
	TArray<AActor*> CollectEnemyTargets(float Radius) const;
	bool ShouldInterruptAsMinion(AActor* TargetActor) const;
	void SendThunderInterrupt(AActor* TargetActor, float Damage) const;
};
