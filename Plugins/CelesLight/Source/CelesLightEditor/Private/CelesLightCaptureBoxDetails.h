#pragma once

#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"

class ACelesLightCaptureBox;
class IDetailLayoutBuilder;
class UTextureRenderTarget2D;

class FCelesLightCaptureBoxDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply CreateAndAssignRenderTarget();
	FReply UpdateSelectedCaptureBoxes();
	UTextureRenderTarget2D* CreateRenderTargetAsset(const FString& ObjectPath, int32 LightInfoCount) const;
	int32 GetFirstSelectedLightCount() const;

	TArray<TWeakObjectPtr<ACelesLightCaptureBox>> SelectedCaptureBoxes;
};
