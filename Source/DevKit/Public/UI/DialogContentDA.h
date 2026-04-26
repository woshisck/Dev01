#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/GameDialogWidget.h"
#include "DialogContentDA.generated.h"

/**
 * 单个弹窗的内容（一个资产 = 一个弹窗）。
 * 只填 Pages（标题/正文/插图/副文）。
 * EventID 在 UTutorialRegistryDA 的 TMap key 里登记，不在此 DA 重复填写。
 *
 * 命名建议：DA_Tutorial_<EventID>，例如 DA_Tutorial_WeaponPickup。
 */
UCLASS(BlueprintType)
class DEVKIT_API UDialogContentDA : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 多页内容；常规弹窗一页即可。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialog")
	TArray<FTutorialPage> Pages;
};
