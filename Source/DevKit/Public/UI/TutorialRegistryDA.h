#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TutorialRegistryDA.generated.h"

class UDialogContentDA;
struct FTutorialPage;

/**
 * 教程注册表（项目级唯一）：EventID → 单弹窗内容 DA 的映射。
 *
 * 工作流：
 *   1) 每个新弹窗：新建一个 UDialogContentDA 资产（命名 DA_Tutorial_<EventID>），只填 Pages。
 *   2) 在本注册表的 Entries TMap 加一行：Key=EventID（FName），Value=对应内容 DA。
 *   3) BP_HUD 的 TutorialRegistry 字段引用本资产（仅一次配置，永不再动）。
 *
 * 代码侧通过 UTutorialManager::ShowByEventID(FName) 调用，自动经此表查询并展示。
 */
UCLASS(BlueprintType)
class DEVKIT_API UTutorialRegistryDA : public UDataAsset
{
	GENERATED_BODY()

public:
	/** EventID → 内容 DA 映射。Key 必须与代码 / LENode 中使用的 EventID 一字不差。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial",
	          meta = (DisplayName = "事件 → 弹窗内容", ForceInlineRow))
	TMap<FName, TObjectPtr<UDialogContentDA>> Entries;

	/** 按 EventID 查找内容 DA 的 Pages，找不到返回 nullptr。 */
	const TArray<FTutorialPage>* FindPages(FName EventID) const;
};
