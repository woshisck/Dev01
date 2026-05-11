#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Data/AltarDataAsset.h"
#include "AltarMenuWidget.generated.h"

class APlayerCharacterBase;
class URunePurificationWidget;
class USacrificeSelectionWidget;
class UButton;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class DEVKIT_API UAltarMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static TSubclassOf<UTextBlock> GetMenuTextBlockClassForTests();

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
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(BlueprintReadOnly, Category = "Altar")
	TObjectPtr<UAltarDataAsset> AltarData;

	UPROPERTY(BlueprintReadOnly, Category = "Altar")
	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;

	UPROPERTY()
	TObjectPtr<URunePurificationWidget> PurificationWidget;

	UPROPERTY()
	TObjectPtr<USacrificeSelectionWidget> SacrificeWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnPurification;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnSacrifice;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnClose;

private:
	TArray<UButton*> GetFocusableButtons() const;
	void FocusButton(int32 NewIndex);
	void MoveFocus(int32 Direction);
	void ActivateFocusedButton();

	UFUNCTION()
	void OnPurificationClicked();

	UFUNCTION()
	void OnSacrificeClicked();

	UFUNCTION()
	void OnCloseClicked();

	int32 FocusedButtonIndex = 0;
	float LastAnalogNavigationTime = 0.f;
};
