#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponInfoDA.generated.h"

/**
 * 武器展示信息 DA。
 * 配置在 WeaponDefinition.WeaponInfo 中，用于武器名称、说明、缩略图和激活区显示。
 */
UCLASS(BlueprintType)
class DEVKIT_API UWeaponInfoDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Info", meta = (DisplayName = "武器名称"))
	FText WeaponName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Info", meta = (MultiLine = true, DisplayName = "主要描述"))
	FText WeaponDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Info", meta = (MultiLine = true, DisplayName = "补充描述"))
	FText WeaponSubDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Info", meta = (DisplayName = "缩略图"))
	TObjectPtr<UTexture2D> Thumbnail;

	// 激活区图像（留空则显示点阵）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Active Zone", meta = (DisplayName = "激活区图片 1"))
	TObjectPtr<UTexture2D> Zone1Image;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Active Zone", meta = (DisplayName = "激活区图片 2"))
	TObjectPtr<UTexture2D> Zone2Image;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Active Zone", meta = (DisplayName = "激活区图片 3"))
	TObjectPtr<UTexture2D> Zone3Image;
};
