#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;

/**
 * URuneDataAsset 的自定义 Detail Panel：
 *  - 顶部加一行快速操作按钮（"复制 RuneIdTag" / "校验本条"）
 *  - 不修改字段定义，只是 UI 上加快捷操作
 *
 * 按钮回调用 Lambda + WeakObjectPtr 捕获，自身销毁后 lambda 安全 no-op。
 */
class FRuneDataAssetDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
