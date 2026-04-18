#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/GameDialogWidget.h"
#include "DialogContentDA.generated.h"

// 一条弹窗事件：EventID 为查找键，Pages 为多页内容
USTRUCT(BlueprintType)
struct FDialogContent
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dialog")
	FName EventID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dialog")
	TArray<FTutorialPage> Pages;
};

// 全局弹窗内容 DA —— 在编辑器里填写所有弹窗事件，不改代码
UCLASS(BlueprintType)
class DEVKIT_API UDialogContentDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialog")
	TArray<FDialogContent> Contents;

	// 按 EventID 查找页面数组，找不到返回 nullptr
	const TArray<FTutorialPage>* FindPages(FName EventID) const;
};
