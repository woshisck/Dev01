#pragma once

#include "IDetailCustomization.h"

class ADevKitDecalCollectionActor;

class FDevKitDecalCollectionActorDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply EnterEditMode();
	TWeakObjectPtr<ADevKitDecalCollectionActor> CollectionActor;
};
