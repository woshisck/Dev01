#pragma once

#include "CoreMinimal.h"
#include "Character/PlayerInteraction.h"
#include "GameFramework/Actor.h"
#include "ShopActor.generated.h"

class APlayerCharacterBase;
class UBoxComponent;
class UShopDataAsset;
class UShopSelectionWidget;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class DEVKIT_API AShopActor : public AActor, public IPlayerInteraction
{
	GENERATED_BODY()

public:
	AShopActor();
	virtual void BeginPlay() override;

	virtual void OnPlayerBeginOverlap(APlayerCharacterBase* Player) override;
	virtual void OnPlayerEndOverlap(APlayerCharacterBase* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void TryInteract(APlayerCharacterBase* Player);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetShopData(UShopDataAsset* InData);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetShopWidgetClass(TSubclassOf<UShopSelectionWidget> InClass) { ShopWidgetClass = InClass; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UShopDataAsset> ShopData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TSubclassOf<UShopSelectionWidget> ShopWidgetClass;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UBoxComponent> InteractBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UStaticMeshComponent> ShopMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop|Prompt")
	TObjectPtr<UWidgetComponent> InteractPromptWidgetComp;

	UPROPERTY()
	TObjectPtr<UShopSelectionWidget> ShopWidget;

	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnPlayerNearby(APlayerCharacterBase* Player, bool bNearby);

	void ConfigureInteractPrompt();
	void SetInteractPromptVisible(bool bVisible);
};
