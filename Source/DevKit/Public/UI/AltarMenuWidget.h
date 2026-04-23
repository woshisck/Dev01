#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Data/AltarDataAsset.h"
#include "AltarMenuWidget.generated.h"

class APlayerCharacterBase;
class URunePurificationWidget;
class USacrificeSelectionWidget;

UCLASS(Abstract, Blueprintable)
class DEVKIT_API UAltarMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// 打开菜单前调用，注入数据和玩家引用
	UFUNCTION(BlueprintCallable, Category = "Altar")
	void SetupAltar(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer);

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void OpenPurification();

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void OpenSacrifice();

	UFUNCTION(BlueprintCallable, Category = "Altar")
	void CloseMenu();

	// 升级功能存根：BP 重写实现弹"即将推出"提示
	UFUNCTION(BlueprintImplementableEvent, Category = "Altar")
	void OnUpgradeRequested();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	TSubclassOf<URunePurificationWidget> PurificationWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Altar")
	TSubclassOf<USacrificeSelectionWidget> SacrificeWidgetClass;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Altar")
	TObjectPtr<UAltarDataAsset> AltarData;

	UPROPERTY(BlueprintReadOnly, Category = "Altar")
	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;

	UPROPERTY()
	TObjectPtr<URunePurificationWidget> PurificationWidget;

	UPROPERTY()
	TObjectPtr<USacrificeSelectionWidget> SacrificeWidget;
};
