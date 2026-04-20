#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LevelFlowAssetFactory.generated.h"

/**
 * 在内容浏览器右键菜单中添加 "Level Event Flow" 创建入口。
 * 使用：右键 → Flow → Level Event Flow
 */
UCLASS()
class ULevelFlowAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	ULevelFlowAssetFactory(const FObjectInitializer& ObjectInitializer);

	virtual FText GetDisplayName() const override;

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
