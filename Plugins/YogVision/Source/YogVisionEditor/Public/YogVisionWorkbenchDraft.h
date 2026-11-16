#pragma once

#include "CoreMinimal.h"
#include "YogVisionSettings.h"
#include "YogVisionWorkbenchDraft.generated.h"

/** An editor-only draft. Editing it never modifies a map or the project defaults. */
UCLASS(Transient)
class YOGVISIONEDITOR_API UYogVisionWorkbenchDraft : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="游戏默认", meta=(DisplayName="游戏中默认启用", ToolTip="保存项目默认后，新运行的游戏是否启用视野迷雾。当前关卡的临时预览由左侧按钮控制。"))
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, Category="美术参数", meta=(ShowOnlyInnerProperties, DisplayName="视野与灵视参数"))
    FYogVisionParameters Parameters;
};
