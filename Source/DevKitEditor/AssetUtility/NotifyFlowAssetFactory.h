#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "NotifyFlowAssetFactory.generated.h"

/**
 * 在内容浏览器右键菜单中添加 "Notify Flow Asset" 创建入口。
 * 直接创建 UNotifyFlowAsset，无需类选择器。
 *
 * 使用：右键 → Flow → Notify Flow Asset
 */
UCLASS()
class UNotifyFlowAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UNotifyFlowAssetFactory(const FObjectInitializer& ObjectInitializer);

	virtual FText GetDisplayName() const override;

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
