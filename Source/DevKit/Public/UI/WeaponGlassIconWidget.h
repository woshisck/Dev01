#pragma once

#include "CoreMinimal.h"
#include "UI/GlassFrameWidget.h"
#include "WeaponGlassIconWidget.generated.h"

class UImage;
class UWeaponGlassAnimDA;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponGlassHidden);

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UWeaponGlassIconWidget : public UGlassFrameWidget
{
	GENERATED_BODY()

public:
	/** 飞行结束后调用：显示图标，进入常驻状态 */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void Show(const UWeaponGlassAnimDA* InAnimDA);

	/** 触发放大→渐隐消失动画（开背包时调用） */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void StartExpandAndHide();

	/** 消失动画播完后广播 */
	UPROPERTY(BlueprintAssignable, Category = "WeaponGlass")
	FOnWeaponGlassHidden OnHidden;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> HeatColorOverlay;

private:
	void RefreshHeatOverlay(int32 Phase);

	UFUNCTION()
	void OnHeatPhaseChanged(int32 Phase);

	UPROPERTY()
	TObjectPtr<const UWeaponGlassAnimDA> AnimDA;

	bool  bExpanding     = false;
	float ExpandTimer    = 0.f;
	bool  bWeaponShowing = false;
};
