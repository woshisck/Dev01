#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponInfoDA.generated.h"

/**
 * 武器展示信息 DA
 * 填在 WeaponDefinition.WeaponInfo，驱动武器浮窗显示内容。
 */
UCLASS(BlueprintType)
class DEVKIT_API UWeaponInfoDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "武器信息")
	FText WeaponName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "武器信息", meta = (MultiLine = true))
	FText WeaponDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "武器信息", meta = (MultiLine = true))
	FText WeaponSubDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "武器信息")
	TObjectPtr<UTexture2D> Thumbnail;

	// 激活区图像（留空则显示点阵）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "激活区")
	TObjectPtr<UTexture2D> Zone1Image;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "激活区")
	TObjectPtr<UTexture2D> Zone2Image;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "激活区")
	TObjectPtr<UTexture2D> Zone3Image;
};
