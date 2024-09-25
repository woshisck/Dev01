// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphEdNodeBase.h"
#include "ComboGraphEdNodeEdge.generated.h"

class UComboGraph;
class UComboGraphEdNodeConduit;
class UComboGraphEdge;
class UComboGraphEdNode;

UCLASS(MinimalAPI)
class UComboGraphEdNodeEdge : public UComboGraphEdNodeBase
{
	GENERATED_BODY()

public:
	UComboGraphEdNodeEdge();
	virtual ~UComboGraphEdNodeEdge();

	UPROPERTY(Transient)
	TObjectPtr<UComboGraph> ComboGraphOwner;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Combo Graph")
	TObjectPtr<UComboGraphEdge> RuntimeEdge;

	void SetRuntimeEdge(UComboGraphEdge* InEdge);
	void CreateConnections(const UComboGraphEdNode* Start, const UComboGraphEdNode* End);
	void CreateConnections(const UComboGraphEdNodeBase* Start, const UComboGraphEdNodeBase* End);
	void CreateBaseConnections(const UComboGraphEdNodeBase* Start, const UComboGraphEdNodeBase* End);

	UComboGraphEdNodeBase* GetStartNodeAsBase() const;
	UComboGraphEdNode* GetStartNodeAsGraphNode() const;
	UComboGraphEdNodeConduit* GetStartNodeAsConduit() const;
	UComboGraphEdNode* GetEndNode() const;

	//~ UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual void PrepareForCopying() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UEdGraphNode interface

	void UpdateCachedIcon();
	UTexture2D* GetCachedIconTexture() const;
private:
	TSoftObjectPtr<UTexture2D> CachedIcon;

	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
};
