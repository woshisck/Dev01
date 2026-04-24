#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "RuneManagerWidget.generated.h"

class URuneDataAsset;

/**
 * 符文管理器 EUW 的 C++ 基类
 * 在编辑器里：右键 Content Browser → Editor Utility Widget → 父类选此类
 * Blueprint 端用 List View 绑定 GetAllRuneDataAssets()，按需调用其他函数
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DEVKITEDITOR_API URuneManagerWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/** 扫描 /Game 路径下所有 URuneDataAsset，加载并返回 */
	UFUNCTION(BlueprintCallable, Category = "Rune Manager")
	TArray<URuneDataAsset*> GetAllRuneDataAssets();

	/**
	 * 以 Template 为蓝本复制一个新 DA，放到 DestinationPath（如 /Game/Docs/BuffDocs/Playtest_GA/MyFolder）
	 * 成功后自动在 Content Browser 选中并打开
	 */
	UFUNCTION(BlueprintCallable, Category = "Rune Manager")
	URuneDataAsset* DuplicateRuneDA(
		URuneDataAsset* Template,
		const FString& NewName,
		const FString& DestinationPath);

	/** 在 Content Browser 中高亮选中资产 */
	UFUNCTION(BlueprintCallable, Category = "Rune Manager")
	void SyncToBrowserAndSelect(UObject* Asset);

	/** 在对应编辑器中打开资产（RuneDataAsset 会用默认 DA 编辑器） */
	UFUNCTION(BlueprintCallable, Category = "Rune Manager")
	void OpenAssetEditor(UObject* Asset);
};
